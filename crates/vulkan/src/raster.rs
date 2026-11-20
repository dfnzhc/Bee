use crate::device::{Backend, DeviceState};
use crate::ray::{bytes_of_slice, SceneBuffer};
use crate::{Frame, GraphImage};
use anyhow::{bail, Context, Result};
use ash::vk;
use std::io::Cursor;
use std::sync::Arc;

const VERTEX: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/raster.vert.spv"));
const FRAGMENT: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/raster.frag.spv"));

pub struct Rasterizer {
    inner: Arc<Pipeline>,
}

#[derive(Clone)]
pub struct RasterGeometry {
    data: SceneBuffer,
    vertex_count: u32,
}

#[repr(C)]
pub struct RasterInstance {
    pub model: [f32; 16],
    pub normal: [[f32; 4]; 3],
    pub material: [u32; 4],
}

#[derive(Clone)]
pub struct RasterInstances {
    data: SceneBuffer,
    meshes: Arc<[usize]>,
}

struct Pipeline {
    device: Arc<DeviceState>,
    depth_format: vk::Format,
    descriptor_layout: vk::DescriptorSetLayout,
    layout: vk::PipelineLayout,
    render_pass: vk::RenderPass,
    pipeline: vk::Pipeline,
}

impl Drop for Pipeline {
    fn drop(&mut self) {
        unsafe {
            let device = &self.device.device;
            device.destroy_pipeline(self.pipeline, None);
            device.destroy_pipeline_layout(self.layout, None);
            device.destroy_render_pass(self.render_pass, None);
            device.destroy_descriptor_set_layout(self.descriptor_layout, None);
        }
    }
}

struct Pass {
    device: Arc<DeviceState>,
    _pipeline: Arc<Pipeline>,
    _color: GraphImage,
    _depth: GraphImage,
    color_view: vk::ImageView,
    depth_view: vk::ImageView,
    framebuffer: vk::Framebuffer,
    descriptor_pool: vk::DescriptorPool,
}

impl Drop for Pass {
    fn drop(&mut self) {
        unsafe {
            let device = &self.device.device;
            device.destroy_framebuffer(self.framebuffer, None);
            device.destroy_descriptor_pool(self.descriptor_pool, None);
            device.destroy_image_view(self.color_view, None);
            device.destroy_image_view(self.depth_view, None);
        }
    }
}

impl Rasterizer {
    pub fn depth_format(&self) -> vk::Format {
        self.inner.depth_format
    }
}

impl Backend {
    pub fn upload_raster_geometry(&self, data: &[[f32; 4]]) -> Result<RasterGeometry> {
        if !data.len().is_multiple_of(6) {
            bail!("invalid raster triangle data");
        }
        let vertex_count = u32::try_from(data.len() / 2).context("too many raster vertices")?;
        let data = crate::profile::span("mesh_upload_us", || self.upload_scene_buffer(data))?;
        crate::profile::record("mesh_upload", 1.0);
        Ok(RasterGeometry { data, vertex_count })
    }

    pub fn upload_raster_instances(
        &self,
        data: &[RasterInstance],
        meshes: Vec<usize>,
    ) -> Result<RasterInstances> {
        if data.len() != meshes.len()
            || u32::try_from(data.len()).is_err()
            || data.iter().any(|instance| {
                instance
                    .model
                    .iter()
                    .chain(instance.normal.iter().flatten())
                    .any(|value| !value.is_finite())
            })
        {
            bail!("invalid raster instance data");
        }
        Ok(RasterInstances {
            data: SceneBuffer::upload(&self.device, unsafe { bytes_of_slice(data) })?,
            meshes: meshes.into(),
        })
    }

    pub fn create_rasterizer(&self) -> Result<Rasterizer> {
        let device = &self.device.device;
        let instance = &self.device.instance.instance;
        let depth_format = [vk::Format::D32_SFLOAT, vk::Format::D16_UNORM]
            .into_iter()
            .find(|&format| unsafe {
                instance
                    .get_physical_device_format_properties(self.device.physical_device, format)
                    .optimal_tiling_features
                    .contains(vk::FormatFeatureFlags::DEPTH_STENCIL_ATTACHMENT)
            })
            .context("no supported Vulkan depth attachment format")?;
        let mut pipeline = Pipeline {
            device: self.device.clone(),
            depth_format,
            descriptor_layout: vk::DescriptorSetLayout::null(),
            layout: vk::PipelineLayout::null(),
            render_pass: vk::RenderPass::null(),
            pipeline: vk::Pipeline::null(),
        };
        let stages = vk::ShaderStageFlags::VERTEX | vk::ShaderStageFlags::FRAGMENT;
        let bindings = [0, 1, 2, 3].map(|binding| {
            vk::DescriptorSetLayoutBinding::default()
                .binding(binding)
                .descriptor_type(vk::DescriptorType::STORAGE_BUFFER)
                .descriptor_count(1)
                .stage_flags(if binding == 3 {
                    vk::ShaderStageFlags::FRAGMENT
                } else {
                    vk::ShaderStageFlags::VERTEX
                })
        });
        pipeline.descriptor_layout = unsafe {
            device.create_descriptor_set_layout(
                &vk::DescriptorSetLayoutCreateInfo::default().bindings(&bindings),
                None,
            )
        }
        .context("could not create raster descriptor layout")?;
        let layouts = [pipeline.descriptor_layout];
        let constants = [vk::PushConstantRange::default()
            .stage_flags(stages)
            .size(80)];
        pipeline.layout = unsafe {
            device.create_pipeline_layout(
                &vk::PipelineLayoutCreateInfo::default()
                    .set_layouts(&layouts)
                    .push_constant_ranges(&constants),
                None,
            )
        }
        .context("could not create raster pipeline layout")?;
        let attachments = [
            vk::AttachmentDescription::default()
                .format(vk::Format::R32G32B32A32_SFLOAT)
                .samples(vk::SampleCountFlags::TYPE_1)
                .load_op(vk::AttachmentLoadOp::CLEAR)
                .store_op(vk::AttachmentStoreOp::STORE)
                .initial_layout(vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL)
                .final_layout(vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL),
            vk::AttachmentDescription::default()
                .format(depth_format)
                .samples(vk::SampleCountFlags::TYPE_1)
                .load_op(vk::AttachmentLoadOp::CLEAR)
                .store_op(vk::AttachmentStoreOp::DONT_CARE)
                .initial_layout(vk::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
                .final_layout(vk::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
        ];
        let color = [vk::AttachmentReference::default()
            .attachment(0)
            .layout(vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL)];
        let depth = vk::AttachmentReference::default()
            .attachment(1)
            .layout(vk::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        let subpasses = [vk::SubpassDescription::default()
            .pipeline_bind_point(vk::PipelineBindPoint::GRAPHICS)
            .color_attachments(&color)
            .depth_stencil_attachment(&depth)];
        pipeline.render_pass = unsafe {
            device.create_render_pass(
                &vk::RenderPassCreateInfo::default()
                    .attachments(&attachments)
                    .subpasses(&subpasses),
                None,
            )
        }
        .context("could not create raster render pass")?;

        let vertex_words = ash::util::read_spv(&mut Cursor::new(VERTEX))?;
        let fragment_words = ash::util::read_spv(&mut Cursor::new(FRAGMENT))?;
        let vertex = unsafe {
            device.create_shader_module(
                &vk::ShaderModuleCreateInfo::default().code(&vertex_words),
                None,
            )
        }
        .context("could not create raster vertex shader")?;
        let fragment = match unsafe {
            device.create_shader_module(
                &vk::ShaderModuleCreateInfo::default().code(&fragment_words),
                None,
            )
        } {
            Ok(shader) => shader,
            Err(error) => {
                unsafe { device.destroy_shader_module(vertex, None) };
                return Err(error).context("could not create raster fragment shader");
            }
        };
        let shaders = [
            vk::PipelineShaderStageCreateInfo::default()
                .stage(vk::ShaderStageFlags::VERTEX)
                .module(vertex)
                .name(c"main"),
            vk::PipelineShaderStageCreateInfo::default()
                .stage(vk::ShaderStageFlags::FRAGMENT)
                .module(fragment)
                .name(c"main"),
        ];
        let vertex_input = vk::PipelineVertexInputStateCreateInfo::default();
        let assembly = vk::PipelineInputAssemblyStateCreateInfo::default()
            .topology(vk::PrimitiveTopology::TRIANGLE_LIST);
        let viewport = vk::PipelineViewportStateCreateInfo::default()
            .viewport_count(1)
            .scissor_count(1);
        let raster = vk::PipelineRasterizationStateCreateInfo::default()
            .polygon_mode(vk::PolygonMode::FILL)
            .cull_mode(vk::CullModeFlags::NONE)
            .front_face(vk::FrontFace::COUNTER_CLOCKWISE)
            .line_width(1.0);
        let multisample = vk::PipelineMultisampleStateCreateInfo::default()
            .rasterization_samples(vk::SampleCountFlags::TYPE_1);
        let depth_test = vk::PipelineDepthStencilStateCreateInfo::default()
            .depth_test_enable(true)
            .depth_write_enable(true)
            .depth_compare_op(vk::CompareOp::LESS);
        let color_blend = [vk::PipelineColorBlendAttachmentState::default()
            .color_write_mask(vk::ColorComponentFlags::RGBA)];
        let blend = vk::PipelineColorBlendStateCreateInfo::default().attachments(&color_blend);
        let dynamic_states = [vk::DynamicState::VIEWPORT, vk::DynamicState::SCISSOR];
        let dynamic = vk::PipelineDynamicStateCreateInfo::default().dynamic_states(&dynamic_states);
        let info = vk::GraphicsPipelineCreateInfo::default()
            .stages(&shaders)
            .vertex_input_state(&vertex_input)
            .input_assembly_state(&assembly)
            .viewport_state(&viewport)
            .rasterization_state(&raster)
            .multisample_state(&multisample)
            .depth_stencil_state(&depth_test)
            .color_blend_state(&blend)
            .dynamic_state(&dynamic)
            .layout(pipeline.layout)
            .render_pass(pipeline.render_pass);
        let result =
            unsafe { device.create_graphics_pipelines(vk::PipelineCache::null(), &[info], None) };
        crate::profile::record("pipeline_create", 1.0);
        unsafe {
            device.destroy_shader_module(fragment, None);
            device.destroy_shader_module(vertex, None);
        }
        pipeline.pipeline = match result {
            Ok(pipelines) => pipelines[0],
            Err((pipelines, error)) => {
                for partial in pipelines {
                    unsafe { device.destroy_pipeline(partial, None) };
                }
                return Err(error).context("could not create raster graphics pipeline");
            }
        };
        Ok(Rasterizer {
            inner: Arc::new(pipeline),
        })
    }
}

#[repr(C)]
struct Constants {
    view_projection: [f32; 16],
    environment: [f32; 3],
    _padding: u32,
}

impl Frame {
    /// # Safety
    /// 图像须处于图形附件布局；实例中的材质索引须指向有效条目。
    pub unsafe fn rasterize(
        &mut self,
        rasterizer: &Rasterizer,
        color: &GraphImage,
        depth: &GraphImage,
        meshes: &[RasterGeometry],
        instances: &RasterInstances,
        materials: &SceneBuffer,
        lights: &SceneBuffer,
        view_projection: [f32; 16],
        environment: [f32; 3],
    ) -> Result<()> {
        if !Arc::ptr_eq(&self.device, &rasterizer.inner.device)
            || [&instances.data, materials, lights]
                .into_iter()
                .chain(meshes.iter().map(|mesh| &mesh.data))
                .any(|buffer| !Arc::ptr_eq(&self.device, &buffer.0.device))
            || instances.meshes.iter().any(|&index| index >= meshes.len())
            || !Arc::ptr_eq(&self.device, &color.inner.device)
            || !Arc::ptr_eq(&self.device, &depth.inner.device)
            || color.inner.width != depth.inner.width
            || color.inner.height != depth.inner.height
            || color.inner.format != vk::Format::R32G32B32A32_SFLOAT
            || depth.inner.format != rasterizer.inner.depth_format
            || color.inner.aspect != vk::ImageAspectFlags::COLOR
            || depth.inner.aspect != vk::ImageAspectFlags::DEPTH
            || !color
                .inner
                .usage
                .contains(vk::ImageUsageFlags::COLOR_ATTACHMENT)
            || !depth
                .inner
                .usage
                .contains(vk::ImageUsageFlags::DEPTH_STENCIL_ATTACHMENT)
            || view_projection
                .iter()
                .chain(&environment)
                .any(|v| !v.is_finite())
        {
            bail!("incompatible raster attachments or scene data");
        }
        let mut pass = Pass {
            device: self.device.clone(),
            _pipeline: rasterizer.inner.clone(),
            _color: color.clone(),
            _depth: depth.clone(),
            color_view: vk::ImageView::null(),
            depth_view: vk::ImageView::null(),
            framebuffer: vk::Framebuffer::null(),
            descriptor_pool: vk::DescriptorPool::null(),
        };
        let device = &self.device.device;
        for (image, view) in [(color, &mut pass.color_view), (depth, &mut pass.depth_view)] {
            let range = vk::ImageSubresourceRange::default()
                .aspect_mask(image.inner.aspect)
                .level_count(1)
                .layer_count(1);
            *view = device
                .create_image_view(
                    &vk::ImageViewCreateInfo::default()
                        .image(image.raw())
                        .view_type(vk::ImageViewType::TYPE_2D)
                        .format(image.inner.format)
                        .subresource_range(range),
                    None,
                )
                .context("could not create raster attachment view")?;
        }
        let views = [pass.color_view, pass.depth_view];
        pass.framebuffer = device
            .create_framebuffer(
                &vk::FramebufferCreateInfo::default()
                    .render_pass(rasterizer.inner.render_pass)
                    .attachments(&views)
                    .width(color.inner.width)
                    .height(color.inner.height)
                    .layers(1),
                None,
            )
            .context("could not create raster framebuffer")?;

        let set_count = u32::try_from(meshes.len().max(1)).context("too many raster meshes")?;
        let sizes = [vk::DescriptorPoolSize::default()
            .ty(vk::DescriptorType::STORAGE_BUFFER)
            .descriptor_count(
                set_count
                    .checked_mul(4)
                    .context("too many raster bindings")?,
            )];
        pass.descriptor_pool = device
            .create_descriptor_pool(
                &vk::DescriptorPoolCreateInfo::default()
                    .max_sets(set_count)
                    .pool_sizes(&sizes),
                None,
            )
            .context("could not create raster descriptor pool")?;
        let layouts = vec![rasterizer.inner.descriptor_layout; set_count as usize];
        let descriptors = device
            .allocate_descriptor_sets(
                &vk::DescriptorSetAllocateInfo::default()
                    .descriptor_pool(pass.descriptor_pool)
                    .set_layouts(&layouts),
            )
            .context("could not allocate raster descriptors")?;
        for (mesh, &descriptor) in meshes.iter().zip(&descriptors) {
            let buffers = [&mesh.data, &instances.data, materials, lights]
                .map(|buffer| [buffer.0.descriptor()]);
            let writes = [0, 1, 2, 3].map(|binding| {
                vk::WriteDescriptorSet::default()
                    .dst_set(descriptor)
                    .dst_binding(binding)
                    .descriptor_type(vk::DescriptorType::STORAGE_BUFFER)
                    .buffer_info(&buffers[binding as usize])
            });
            device.update_descriptor_sets(&writes, &[]);
        }

        let extent = vk::Extent2D {
            width: color.inner.width,
            height: color.inner.height,
        };
        let area = vk::Rect2D {
            offset: vk::Offset2D { x: 0, y: 0 },
            extent,
        };
        let clears = [
            vk::ClearValue {
                color: vk::ClearColorValue {
                    float32: [environment[0], environment[1], environment[2], 1.0],
                },
            },
            vk::ClearValue {
                depth_stencil: vk::ClearDepthStencilValue {
                    depth: 1.0,
                    stencil: 0,
                },
            },
        ];
        let begin = vk::RenderPassBeginInfo::default()
            .render_pass(rasterizer.inner.render_pass)
            .framebuffer(pass.framebuffer)
            .render_area(area)
            .clear_values(&clears);
        let viewport = vk::Viewport {
            x: 0.0,
            y: 0.0,
            width: extent.width as f32,
            height: extent.height as f32,
            min_depth: 0.0,
            max_depth: 1.0,
        };
        let constants = Constants {
            view_projection,
            environment,
            _padding: 0,
        };
        let bytes = std::slice::from_raw_parts(
            (&constants as *const Constants).cast::<u8>(),
            std::mem::size_of::<Constants>(),
        );
        device.cmd_begin_render_pass(self.command_buffer, &begin, vk::SubpassContents::INLINE);
        device.cmd_bind_pipeline(
            self.command_buffer,
            vk::PipelineBindPoint::GRAPHICS,
            rasterizer.inner.pipeline,
        );
        device.cmd_push_constants(
            self.command_buffer,
            rasterizer.inner.layout,
            vk::ShaderStageFlags::VERTEX | vk::ShaderStageFlags::FRAGMENT,
            0,
            bytes,
        );
        device.cmd_set_viewport(self.command_buffer, 0, &[viewport]);
        device.cmd_set_scissor(self.command_buffer, 0, &[area]);
        let mut first_instance = 0;
        for batch in instances.meshes.chunk_by(|a, b| a == b) {
            let index = batch[0];
            let vertex_count = meshes[index].vertex_count;
            if vertex_count > 0 {
                device.cmd_bind_descriptor_sets(
                    self.command_buffer,
                    vk::PipelineBindPoint::GRAPHICS,
                    rasterizer.inner.layout,
                    0,
                    &[descriptors[index]],
                    &[],
                );
                device.cmd_draw(
                    self.command_buffer,
                    vertex_count,
                    batch.len() as u32,
                    0,
                    first_instance,
                );
            }
            first_instance += batch.len() as u32;
        }
        device.cmd_end_render_pass(self.command_buffer);
        for buffer in [&instances.data, materials, lights]
            .into_iter()
            .chain(meshes.iter().map(|mesh| &mesh.data))
        {
            self.resources.push(buffer.0.clone());
        }
        self.resources.push(Arc::new(pass));
        crate::profile::record("raster_pass", 1.0);
        Ok(())
    }
}
