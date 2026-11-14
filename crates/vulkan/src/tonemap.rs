use crate::device::{Backend, DeviceState};
use crate::{Frame, GraphImage};
use anyhow::{bail, Context, Result};
use ash::vk;
use std::io::Cursor;
use std::sync::Arc;

const VERTEX: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/tonemap.vert.spv"));
const FRAGMENT: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/tonemap.frag.spv"));

pub struct ToneMapper {
    inner: Arc<Pipeline>,
}

struct Pipeline {
    device: Arc<DeviceState>,
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
    _source: GraphImage,
    _target: GraphImage,
    source_view: vk::ImageView,
    target_view: vk::ImageView,
    framebuffer: vk::Framebuffer,
    sampler: vk::Sampler,
    descriptor_pool: vk::DescriptorPool,
}

impl Drop for Pass {
    fn drop(&mut self) {
        unsafe {
            let device = &self.device.device;
            device.destroy_framebuffer(self.framebuffer, None);
            device.destroy_descriptor_pool(self.descriptor_pool, None);
            device.destroy_sampler(self.sampler, None);
            device.destroy_image_view(self.source_view, None);
            device.destroy_image_view(self.target_view, None);
        }
    }
}

impl Backend {
    pub fn create_tone_mapper(&self) -> Result<ToneMapper> {
        let mut pipeline = Pipeline {
            device: self.device.clone(),
            descriptor_layout: vk::DescriptorSetLayout::null(),
            layout: vk::PipelineLayout::null(),
            render_pass: vk::RenderPass::null(),
            pipeline: vk::Pipeline::null(),
        };
        let device = &self.device.device;
        let bindings = [vk::DescriptorSetLayoutBinding::default()
            .binding(0)
            .descriptor_type(vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
            .descriptor_count(1)
            .stage_flags(vk::ShaderStageFlags::FRAGMENT)];
        pipeline.descriptor_layout = unsafe {
            device.create_descriptor_set_layout(
                &vk::DescriptorSetLayoutCreateInfo::default().bindings(&bindings),
                None,
            )
        }
        .context("could not create tone mapping descriptor layout")?;
        let layouts = [pipeline.descriptor_layout];
        let constants = [vk::PushConstantRange::default()
            .stage_flags(vk::ShaderStageFlags::FRAGMENT)
            .size(4)];
        pipeline.layout = unsafe {
            device.create_pipeline_layout(
                &vk::PipelineLayoutCreateInfo::default()
                    .set_layouts(&layouts)
                    .push_constant_ranges(&constants),
                None,
            )
        }
        .context("could not create tone mapping pipeline layout")?;
        let attachments = [vk::AttachmentDescription::default()
            .format(vk::Format::R8G8B8A8_UNORM)
            .samples(vk::SampleCountFlags::TYPE_1)
            .load_op(vk::AttachmentLoadOp::DONT_CARE)
            .store_op(vk::AttachmentStoreOp::STORE)
            .initial_layout(vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL)
            .final_layout(vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL)];
        let color = [vk::AttachmentReference::default()
            .attachment(0)
            .layout(vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL)];
        let subpasses = [vk::SubpassDescription::default()
            .pipeline_bind_point(vk::PipelineBindPoint::GRAPHICS)
            .color_attachments(&color)];
        pipeline.render_pass = unsafe {
            device.create_render_pass(
                &vk::RenderPassCreateInfo::default()
                    .attachments(&attachments)
                    .subpasses(&subpasses),
                None,
            )
        }
        .context("could not create tone mapping render pass")?;

        let vertex_words = ash::util::read_spv(&mut Cursor::new(VERTEX))
            .context("invalid tone mapping vertex SPIR-V")?;
        let fragment_words = ash::util::read_spv(&mut Cursor::new(FRAGMENT))
            .context("invalid tone mapping fragment SPIR-V")?;
        let vertex = unsafe {
            device.create_shader_module(
                &vk::ShaderModuleCreateInfo::default().code(&vertex_words),
                None,
            )
        }
        .context("could not create tone mapping vertex shader")?;
        let fragment = match unsafe {
            device.create_shader_module(
                &vk::ShaderModuleCreateInfo::default().code(&fragment_words),
                None,
            )
        } {
            Ok(fragment) => fragment,
            Err(error) => {
                unsafe { device.destroy_shader_module(vertex, None) };
                return Err(error).context("could not create tone mapping fragment shader");
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
            .color_blend_state(&blend)
            .dynamic_state(&dynamic)
            .layout(pipeline.layout)
            .render_pass(pipeline.render_pass)
            .subpass(0);
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
                    unsafe { device.destroy_pipeline(partial, None) }
                }
                return Err(error).context("could not create tone mapping graphics pipeline");
            }
        };
        Ok(ToneMapper {
            inner: Arc::new(pipeline),
        })
    }
}

impl Frame {
    /// # Safety
    /// 源图像须处于 SHADER_READ_ONLY_OPTIMAL，目标图像须处于 COLOR_ATTACHMENT_OPTIMAL。
    pub unsafe fn tone_map(
        &mut self,
        mapper: &ToneMapper,
        source: &GraphImage,
        target: &GraphImage,
        exposure_ev: f32,
    ) -> Result<()> {
        if !exposure_ev.is_finite()
            || !Arc::ptr_eq(&self.device, &mapper.inner.device)
            || !Arc::ptr_eq(&self.device, &source.inner.device)
            || !Arc::ptr_eq(&self.device, &target.inner.device)
            || source.raw() == target.raw()
            || source.inner.width != target.inner.width
            || source.inner.height != target.inner.height
            || source.inner.format != vk::Format::R32G32B32A32_SFLOAT
            || target.inner.format != vk::Format::R8G8B8A8_UNORM
            || source.inner.aspect != vk::ImageAspectFlags::COLOR
            || target.inner.aspect != vk::ImageAspectFlags::COLOR
            || !source.inner.usage.contains(vk::ImageUsageFlags::SAMPLED)
            || !target
                .inner
                .usage
                .contains(vk::ImageUsageFlags::COLOR_ATTACHMENT)
        {
            bail!("incompatible tone mapping images or parameters");
        }
        let mut pass = Pass {
            device: self.device.clone(),
            _pipeline: mapper.inner.clone(),
            _source: source.clone(),
            _target: target.clone(),
            source_view: vk::ImageView::null(),
            target_view: vk::ImageView::null(),
            framebuffer: vk::Framebuffer::null(),
            sampler: vk::Sampler::null(),
            descriptor_pool: vk::DescriptorPool::null(),
        };
        let device = &self.device.device;
        let range = vk::ImageSubresourceRange::default()
            .aspect_mask(vk::ImageAspectFlags::COLOR)
            .level_count(1)
            .layer_count(1);
        pass.source_view = device
            .create_image_view(
                &vk::ImageViewCreateInfo::default()
                    .image(source.raw())
                    .view_type(vk::ImageViewType::TYPE_2D)
                    .format(source.inner.format)
                    .subresource_range(range),
                None,
            )
            .context("could not create scene color view")?;
        pass.target_view = device
            .create_image_view(
                &vk::ImageViewCreateInfo::default()
                    .image(target.raw())
                    .view_type(vk::ImageViewType::TYPE_2D)
                    .format(target.inner.format)
                    .subresource_range(range),
                None,
            )
            .context("could not create display color view")?;
        let views = [pass.target_view];
        pass.framebuffer = device
            .create_framebuffer(
                &vk::FramebufferCreateInfo::default()
                    .render_pass(mapper.inner.render_pass)
                    .attachments(&views)
                    .width(target.inner.width)
                    .height(target.inner.height)
                    .layers(1),
                None,
            )
            .context("could not create tone mapping framebuffer")?;
        pass.sampler = device
            .create_sampler(
                &vk::SamplerCreateInfo::default()
                    .mag_filter(vk::Filter::NEAREST)
                    .min_filter(vk::Filter::NEAREST)
                    .mipmap_mode(vk::SamplerMipmapMode::NEAREST)
                    .address_mode_u(vk::SamplerAddressMode::CLAMP_TO_EDGE)
                    .address_mode_v(vk::SamplerAddressMode::CLAMP_TO_EDGE)
                    .address_mode_w(vk::SamplerAddressMode::CLAMP_TO_EDGE),
                None,
            )
            .context("could not create tone mapping sampler")?;
        let sizes = [vk::DescriptorPoolSize::default()
            .ty(vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
            .descriptor_count(1)];
        pass.descriptor_pool = device
            .create_descriptor_pool(
                &vk::DescriptorPoolCreateInfo::default()
                    .max_sets(1)
                    .pool_sizes(&sizes),
                None,
            )
            .context("could not create tone mapping descriptor pool")?;
        let layouts = [mapper.inner.descriptor_layout];
        let descriptor = device
            .allocate_descriptor_sets(
                &vk::DescriptorSetAllocateInfo::default()
                    .descriptor_pool(pass.descriptor_pool)
                    .set_layouts(&layouts),
            )
            .context("could not allocate tone mapping descriptor")?[0];
        let image_info = [vk::DescriptorImageInfo::default()
            .sampler(pass.sampler)
            .image_view(pass.source_view)
            .image_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL)];
        let writes = [vk::WriteDescriptorSet::default()
            .dst_set(descriptor)
            .dst_binding(0)
            .descriptor_type(vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
            .image_info(&image_info)];
        device.update_descriptor_sets(&writes, &[]);

        let extent = vk::Extent2D {
            width: target.inner.width,
            height: target.inner.height,
        };
        let area = vk::Rect2D {
            offset: vk::Offset2D { x: 0, y: 0 },
            extent,
        };
        let begin = vk::RenderPassBeginInfo::default()
            .render_pass(mapper.inner.render_pass)
            .framebuffer(pass.framebuffer)
            .render_area(area);
        let viewport = vk::Viewport {
            x: 0.0,
            y: 0.0,
            width: extent.width as f32,
            height: extent.height as f32,
            min_depth: 0.0,
            max_depth: 1.0,
        };
        device.cmd_begin_render_pass(self.command_buffer, &begin, vk::SubpassContents::INLINE);
        device.cmd_bind_pipeline(
            self.command_buffer,
            vk::PipelineBindPoint::GRAPHICS,
            mapper.inner.pipeline,
        );
        device.cmd_bind_descriptor_sets(
            self.command_buffer,
            vk::PipelineBindPoint::GRAPHICS,
            mapper.inner.layout,
            0,
            &[descriptor],
            &[],
        );
        device.cmd_push_constants(
            self.command_buffer,
            mapper.inner.layout,
            vk::ShaderStageFlags::FRAGMENT,
            0,
            &exposure_ev.to_ne_bytes(),
        );
        device.cmd_set_viewport(self.command_buffer, 0, &[viewport]);
        device.cmd_set_scissor(self.command_buffer, 0, &[area]);
        device.cmd_draw(self.command_buffer, 3, 1, 0, 0);
        device.cmd_end_render_pass(self.command_buffer);
        // 描述符、管线、图像视图和帧缓冲均需存活到提交完成。
        self.resources.push(Arc::new(pass));
        Ok(())
    }
}
