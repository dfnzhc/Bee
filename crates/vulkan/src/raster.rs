use crate::device::{Backend, DeviceState};
use crate::test_image::memory_type;
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
    buffer: vk::Buffer,
    memory: vk::DeviceMemory,
}

impl Drop for Pass {
    fn drop(&mut self) {
        unsafe {
            let device = &self.device.device;
            device.destroy_framebuffer(self.framebuffer, None);
            device.destroy_descriptor_pool(self.descriptor_pool, None);
            device.destroy_buffer(self.buffer, None);
            device.free_memory(self.memory, None);
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
        let bindings = [vk::DescriptorSetLayoutBinding::default()
            .binding(0)
            .descriptor_type(vk::DescriptorType::STORAGE_BUFFER)
            .descriptor_count(1)
            .stage_flags(stages)];
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
    light_offset: u32,
}

impl Frame {
    /// # Safety
    /// 图像须处于图形附件布局，且场景数据由调用方在提交前准备完成。
    pub unsafe fn rasterize(
        &mut self,
        rasterizer: &Rasterizer,
        color: &GraphImage,
        depth: &GraphImage,
        data: &[[f32; 4]],
        vertex_count: u32,
        view_projection: [f32; 16],
        environment: [f32; 3],
    ) -> Result<()> {
        let light_offset = vertex_count
            .checked_mul(3)
            .context("too many raster vertices")?;
        let size = data
            .len()
            .checked_mul(16)
            .context("raster scene size overflow")?;
        let limits = self
            .device
            .instance
            .instance
            .get_physical_device_properties(self.device.physical_device)
            .limits;
        if !Arc::ptr_eq(&self.device, &rasterizer.inner.device)
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
            || !vertex_count.is_multiple_of(3)
            || data.len() < light_offset as usize
            || !(data.len() - light_offset as usize).is_multiple_of(2)
            || size > limits.max_storage_buffer_range as usize
            || view_projection
                .iter()
                .chain(&environment)
                .chain(data.iter().flatten())
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
            buffer: vk::Buffer::null(),
            memory: vk::DeviceMemory::null(),
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

        // 场景数据在提交前上传；本帧持有缓冲区直到 GPU 完成。
        let buffer_size = size.max(16) as u64;
        pass.buffer = device
            .create_buffer(
                &vk::BufferCreateInfo::default()
                    .size(buffer_size)
                    .usage(vk::BufferUsageFlags::STORAGE_BUFFER)
                    .sharing_mode(vk::SharingMode::EXCLUSIVE),
                None,
            )
            .context("could not create raster scene buffer")?;
        let requirements = device.get_buffer_memory_requirements(pass.buffer);
        let (index, flags) = memory_type(
            &self.device,
            requirements.memory_type_bits,
            vk::MemoryPropertyFlags::HOST_VISIBLE,
            vk::MemoryPropertyFlags::HOST_COHERENT,
        )?;
        pass.memory = device
            .allocate_memory(
                &vk::MemoryAllocateInfo::default()
                    .allocation_size(requirements.size)
                    .memory_type_index(index),
                None,
            )
            .context("could not allocate raster scene memory")?;
        device
            .bind_buffer_memory(pass.buffer, pass.memory, 0)
            .context("could not bind raster scene memory")?;
        let mapped = device
            .map_memory(pass.memory, 0, vk::WHOLE_SIZE, vk::MemoryMapFlags::empty())
            .context("could not map raster scene memory")?;
        if size > 0 {
            std::ptr::copy_nonoverlapping(data.as_ptr().cast::<u8>(), mapped.cast::<u8>(), size);
        }
        let flush = if flags.contains(vk::MemoryPropertyFlags::HOST_COHERENT) {
            Ok(())
        } else {
            device.flush_mapped_memory_ranges(&[vk::MappedMemoryRange::default()
                .memory(pass.memory)
                .offset(0)
                .size(vk::WHOLE_SIZE)])
        };
        device.unmap_memory(pass.memory);
        flush.context("could not flush raster scene memory")?;

        let sizes = [vk::DescriptorPoolSize::default()
            .ty(vk::DescriptorType::STORAGE_BUFFER)
            .descriptor_count(1)];
        pass.descriptor_pool = device
            .create_descriptor_pool(
                &vk::DescriptorPoolCreateInfo::default()
                    .max_sets(1)
                    .pool_sizes(&sizes),
                None,
            )
            .context("could not create raster descriptor pool")?;
        let layouts = [rasterizer.inner.descriptor_layout];
        let descriptor = device
            .allocate_descriptor_sets(
                &vk::DescriptorSetAllocateInfo::default()
                    .descriptor_pool(pass.descriptor_pool)
                    .set_layouts(&layouts),
            )
            .context("could not allocate raster descriptor")?[0];
        let buffers = [vk::DescriptorBufferInfo::default()
            .buffer(pass.buffer)
            .offset(0)
            .range(buffer_size)];
        let writes = [vk::WriteDescriptorSet::default()
            .dst_set(descriptor)
            .dst_binding(0)
            .descriptor_type(vk::DescriptorType::STORAGE_BUFFER)
            .buffer_info(&buffers)];
        device.update_descriptor_sets(&writes, &[]);

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
            light_offset,
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
        device.cmd_bind_descriptor_sets(
            self.command_buffer,
            vk::PipelineBindPoint::GRAPHICS,
            rasterizer.inner.layout,
            0,
            &[descriptor],
            &[],
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
        device.cmd_draw(self.command_buffer, vertex_count, 1, 0, 0);
        device.cmd_end_render_pass(self.command_buffer);
        self.resources.push(Arc::new(pass));
        Ok(())
    }
}
