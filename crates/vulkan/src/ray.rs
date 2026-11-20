use crate::buffer::{Buffer, BufferState};
use crate::device::{Backend, DeviceState};
use crate::{AccelerationScene, Frame, GraphImage};
use anyhow::{bail, Context, Result};
use ash::vk;
use std::io::Cursor;
use std::sync::Arc;

const RAYGEN: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/path.rgen.spv"));
const MISS: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/path.rmiss.spv"));
const HIT: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/path.rchit.spv"));

#[derive(Clone)]
pub struct SceneBuffer(pub(crate) Arc<BufferState>);

impl SceneBuffer {
    pub fn graph_buffer(&self) -> Buffer {
        Buffer {
            inner: self.0.clone(),
        }
    }

    pub(crate) fn upload(device: &Arc<DeviceState>, bytes: &[u8]) -> Result<Self> {
        let size = bytes.len().max(16);
        let limits = unsafe {
            device
                .instance
                .instance
                .get_physical_device_properties(device.physical_device)
        }
        .limits;
        if size > limits.max_storage_buffer_range as usize {
            bail!("scene data exceeds Vulkan storage buffer limits");
        }
        let buffer = BufferState::allocate(
            device,
            size as u64,
            vk::BufferUsageFlags::STORAGE_BUFFER,
            true,
            false,
        )?;
        buffer.write(bytes)?;
        Ok(Self(Arc::new(buffer)))
    }
}

impl Backend {
    pub fn upload_scene_buffer(&self, values: &[[f32; 4]]) -> Result<SceneBuffer> {
        if values.iter().flatten().any(|value| !value.is_finite()) {
            bail!("non-finite scene buffer data");
        }
        SceneBuffer::upload(&self.device, unsafe { bytes_of_slice(values) })
    }
}

/// # Safety
/// 元素的所有字节（包括填充）必须已初始化。
pub(crate) unsafe fn bytes_of_slice<T>(values: &[T]) -> &[u8] {
    std::slice::from_raw_parts(values.as_ptr().cast(), std::mem::size_of_val(values))
}

pub struct PathTracer {
    inner: Arc<RayPipeline>,
}

struct RayPipeline {
    device: Arc<DeviceState>,
    descriptor_layout: vk::DescriptorSetLayout,
    layout: vk::PipelineLayout,
    pipeline: vk::Pipeline,
    sbt: Option<Arc<BufferState>>,
    raygen: vk::StridedDeviceAddressRegionKHR,
    miss: vk::StridedDeviceAddressRegionKHR,
    hit: vk::StridedDeviceAddressRegionKHR,
    max_dispatch: u32,
}

impl Drop for RayPipeline {
    fn drop(&mut self) {
        unsafe {
            let device = &self.device.device;
            device.destroy_pipeline(self.pipeline, None);
            device.destroy_pipeline_layout(self.layout, None);
            device.destroy_descriptor_set_layout(self.descriptor_layout, None);
        }
    }
}

struct TracePass {
    device: Arc<DeviceState>,
    _pipeline: Arc<RayPipeline>,
    _scene: AccelerationScene,
    _lighting: [SceneBuffer; 2],
    _image: GraphImage,
    image_view: vk::ImageView,
    descriptor_pool: vk::DescriptorPool,
}

impl Drop for TracePass {
    fn drop(&mut self) {
        unsafe {
            self.device
                .device
                .destroy_descriptor_pool(self.descriptor_pool, None);
            self.device.device.destroy_image_view(self.image_view, None);
        }
    }
}

pub(crate) fn align_up(value: u64, alignment: u64) -> Result<u64> {
    if !alignment.is_power_of_two() {
        bail!("invalid ray tracing alignment");
    }
    Ok(value
        .checked_add(alignment - 1)
        .context("ray tracing alignment overflow")?
        & !(alignment - 1))
}

impl Backend {
    pub fn create_path_tracer(&self) -> Result<PathTracer> {
        if !self.device.ray_tracing_enabled {
            bail!(
                "ray tracing unavailable: {}",
                self.report.ray_tracing_missing.join(", ")
            );
        }
        let device = &self.device.device;
        let loader =
            ash::khr::ray_tracing_pipeline::Device::new(&self.device.instance.instance, device);
        let mut pipeline = RayPipeline {
            device: self.device.clone(),
            descriptor_layout: vk::DescriptorSetLayout::null(),
            layout: vk::PipelineLayout::null(),
            pipeline: vk::Pipeline::null(),
            sbt: None,
            raygen: vk::StridedDeviceAddressRegionKHR::default(),
            miss: vk::StridedDeviceAddressRegionKHR::default(),
            hit: vk::StridedDeviceAddressRegionKHR::default(),
            max_dispatch: 0,
        };
        let stages = vk::ShaderStageFlags::RAYGEN_KHR
            | vk::ShaderStageFlags::CLOSEST_HIT_KHR
            | vk::ShaderStageFlags::MISS_KHR;
        let bindings = [
            vk::DescriptorSetLayoutBinding::default()
                .binding(0)
                .descriptor_type(vk::DescriptorType::ACCELERATION_STRUCTURE_KHR)
                .descriptor_count(1)
                .stage_flags(stages),
            vk::DescriptorSetLayoutBinding::default()
                .binding(1)
                .descriptor_type(vk::DescriptorType::STORAGE_IMAGE)
                .descriptor_count(1)
                .stage_flags(stages),
            vk::DescriptorSetLayoutBinding::default()
                .binding(2)
                .descriptor_type(vk::DescriptorType::STORAGE_BUFFER)
                .descriptor_count(1)
                .stage_flags(vk::ShaderStageFlags::CLOSEST_HIT_KHR),
            vk::DescriptorSetLayoutBinding::default()
                .binding(3)
                .descriptor_type(vk::DescriptorType::STORAGE_BUFFER)
                .descriptor_count(1)
                .stage_flags(vk::ShaderStageFlags::CLOSEST_HIT_KHR),
            vk::DescriptorSetLayoutBinding::default()
                .binding(4)
                .descriptor_type(vk::DescriptorType::STORAGE_BUFFER)
                .descriptor_count(1)
                .stage_flags(vk::ShaderStageFlags::RAYGEN_KHR),
        ];
        pipeline.descriptor_layout = unsafe {
            device.create_descriptor_set_layout(
                &vk::DescriptorSetLayoutCreateInfo::default().bindings(&bindings),
                None,
            )
        }
        .context("could not create path tracing descriptor layout")?;
        let layouts = [pipeline.descriptor_layout];
        let constants = [vk::PushConstantRange::default()
            .stage_flags(stages)
            .size(std::mem::size_of::<TraceConstants>() as u32)];
        pipeline.layout = unsafe {
            device.create_pipeline_layout(
                &vk::PipelineLayoutCreateInfo::default()
                    .set_layouts(&layouts)
                    .push_constant_ranges(&constants),
                None,
            )
        }
        .context("could not create path tracing pipeline layout")?;

        let mut shaders = Vec::new();
        for code in [RAYGEN, MISS, HIT] {
            let words = match ash::util::read_spv(&mut Cursor::new(code)) {
                Ok(words) => words,
                Err(error) => {
                    for shader in shaders {
                        unsafe { device.destroy_shader_module(shader, None) };
                    }
                    return Err(error).context("invalid path tracing SPIR-V");
                }
            };
            match unsafe {
                device
                    .create_shader_module(&vk::ShaderModuleCreateInfo::default().code(&words), None)
            } {
                Ok(shader) => shaders.push(shader),
                Err(error) => {
                    for shader in shaders {
                        unsafe { device.destroy_shader_module(shader, None) };
                    }
                    return Err(error).context("could not create path tracing shader");
                }
            }
        }
        let shader_stages = [
            vk::PipelineShaderStageCreateInfo::default()
                .stage(vk::ShaderStageFlags::RAYGEN_KHR)
                .module(shaders[0])
                .name(c"main"),
            vk::PipelineShaderStageCreateInfo::default()
                .stage(vk::ShaderStageFlags::MISS_KHR)
                .module(shaders[1])
                .name(c"main"),
            vk::PipelineShaderStageCreateInfo::default()
                .stage(vk::ShaderStageFlags::CLOSEST_HIT_KHR)
                .module(shaders[2])
                .name(c"main"),
        ];
        let groups = [
            vk::RayTracingShaderGroupCreateInfoKHR::default()
                .ty(vk::RayTracingShaderGroupTypeKHR::GENERAL)
                .general_shader(0)
                .closest_hit_shader(vk::SHADER_UNUSED_KHR)
                .any_hit_shader(vk::SHADER_UNUSED_KHR)
                .intersection_shader(vk::SHADER_UNUSED_KHR),
            vk::RayTracingShaderGroupCreateInfoKHR::default()
                .ty(vk::RayTracingShaderGroupTypeKHR::GENERAL)
                .general_shader(1)
                .closest_hit_shader(vk::SHADER_UNUSED_KHR)
                .any_hit_shader(vk::SHADER_UNUSED_KHR)
                .intersection_shader(vk::SHADER_UNUSED_KHR),
            vk::RayTracingShaderGroupCreateInfoKHR::default()
                .ty(vk::RayTracingShaderGroupTypeKHR::TRIANGLES_HIT_GROUP)
                .general_shader(vk::SHADER_UNUSED_KHR)
                .closest_hit_shader(2)
                .any_hit_shader(vk::SHADER_UNUSED_KHR)
                .intersection_shader(vk::SHADER_UNUSED_KHR),
        ];
        let info = vk::RayTracingPipelineCreateInfoKHR::default()
            .stages(&shader_stages)
            .groups(&groups)
            .max_pipeline_ray_recursion_depth(1)
            .layout(pipeline.layout);
        crate::profile::record("pipeline_create", 1.0);
        let result = unsafe {
            loader.create_ray_tracing_pipelines(
                vk::DeferredOperationKHR::null(),
                vk::PipelineCache::null(),
                &[info],
                None,
            )
        };
        for shader in shaders {
            unsafe { device.destroy_shader_module(shader, None) };
        }
        pipeline.pipeline = match result {
            Ok(pipelines) => pipelines[0],
            Err((pipelines, error)) => {
                for partial in pipelines {
                    unsafe { device.destroy_pipeline(partial, None) };
                }
                return Err(error).context("could not create ray tracing pipeline");
            }
        };

        let mut properties = vk::PhysicalDeviceRayTracingPipelinePropertiesKHR::default();
        let mut properties2 = vk::PhysicalDeviceProperties2::default().push_next(&mut properties);
        unsafe {
            self.device
                .instance
                .instance
                .get_physical_device_properties2(self.device.physical_device, &mut properties2)
        };
        let handle_size = properties.shader_group_handle_size as usize;
        let stride = align_up(
            handle_size as u64,
            properties.shader_group_handle_alignment as u64,
        )?;
        let base_alignment = properties.shader_group_base_alignment as u64;
        let region_stride = align_up(stride, base_alignment)?;
        let total = region_stride
            .checked_mul(3)
            .and_then(|n| n.checked_add(base_alignment))
            .context("shader binding table size overflow")?;
        let sbt = BufferState::allocate(
            &self.device,
            total,
            vk::BufferUsageFlags::SHADER_BINDING_TABLE_KHR,
            true,
            true,
        )?;
        let first = align_up(sbt.address, base_alignment)? - sbt.address;
        let handles = unsafe {
            loader.get_ray_tracing_shader_group_handles(pipeline.pipeline, 0, 3, handle_size * 3)
        }
        .context("could not obtain ray tracing shader group handles")?;
        let mut bytes = vec![0u8; total as usize];
        for index in 0..3 {
            let offset = (first + region_stride * index as u64) as usize;
            bytes[offset..offset + handle_size]
                .copy_from_slice(&handles[index * handle_size..(index + 1) * handle_size]);
        }
        sbt.write(&bytes)?;
        let region = |index| vk::StridedDeviceAddressRegionKHR {
            device_address: sbt.address + first + region_stride * index,
            stride,
            size: stride,
        };
        if stride > properties.max_shader_group_stride as u64 {
            bail!("shader binding table stride exceeds device limit");
        }
        pipeline.raygen = region(0);
        pipeline.miss = region(1);
        pipeline.hit = region(2);
        pipeline.max_dispatch = properties.max_ray_dispatch_invocation_count;
        pipeline.sbt = Some(Arc::new(sbt));
        Ok(PathTracer {
            inner: Arc::new(pipeline),
        })
    }
}

impl PathTracer {
    pub fn shader_table(&self) -> Buffer {
        Buffer {
            inner: self.inner.sbt.as_ref().unwrap().clone(),
        }
    }
}

#[repr(C)]
struct TraceConstants {
    inverse_view_projection: [f32; 16],
    environment: [f32; 4],
    sample_index: u32,
    max_bounces: u32,
    seed: u32,
}

impl Frame {
    /// # Safety
    /// 历史图像须处于 GENERAL 布局，且前序读写已同步；首次采样须使用编号零。
    /// 追踪输入须对光追阶段可见，实例的材质索引须指向有效条目。
    pub unsafe fn trace_paths(
        &mut self,
        tracer: &PathTracer,
        scene: &AccelerationScene,
        materials: &SceneBuffer,
        lights: &SceneBuffer,
        history: &GraphImage,
        inverse_view_projection: [f32; 16],
        environment: [f32; 3],
        sample_index: u32,
        max_bounces: u32,
        seed: u32,
    ) -> Result<()> {
        let size = (history.inner.width as u64) * (history.inner.height as u64);
        if !Arc::ptr_eq(&self.device, &tracer.inner.device)
            || !Arc::ptr_eq(&self.device, scene.device())
            || !Arc::ptr_eq(&self.device, &materials.0.device)
            || !Arc::ptr_eq(&self.device, &lights.0.device)
            || !Arc::ptr_eq(&self.device, &history.inner.device)
            || history.inner.format != vk::Format::R32G32B32A32_SFLOAT
            || history.inner.aspect != vk::ImageAspectFlags::COLOR
            || !history.inner.usage.contains(vk::ImageUsageFlags::STORAGE)
            || size > tracer.inner.max_dispatch as u64
            || sample_index == u32::MAX
            || max_bounces == 0
            || inverse_view_projection
                .iter()
                .chain(&environment)
                .any(|v| !v.is_finite())
        {
            bail!("incompatible path tracing resources or parameters");
        }

        let mut pass = TracePass {
            device: self.device.clone(),
            _pipeline: tracer.inner.clone(),
            _scene: scene.clone(),
            _lighting: [materials.clone(), lights.clone()],
            _image: history.clone(),
            image_view: vk::ImageView::null(),
            descriptor_pool: vk::DescriptorPool::null(),
        };
        let device = &self.device.device;
        let range = vk::ImageSubresourceRange::default()
            .aspect_mask(vk::ImageAspectFlags::COLOR)
            .level_count(1)
            .layer_count(1);
        pass.image_view = device
            .create_image_view(
                &vk::ImageViewCreateInfo::default()
                    .image(history.raw())
                    .view_type(vk::ImageViewType::TYPE_2D)
                    .format(history.inner.format)
                    .subresource_range(range),
                None,
            )
            .context("could not create path history image view")?;
        let pool_sizes = [
            vk::DescriptorPoolSize::default()
                .ty(vk::DescriptorType::ACCELERATION_STRUCTURE_KHR)
                .descriptor_count(1),
            vk::DescriptorPoolSize::default()
                .ty(vk::DescriptorType::STORAGE_IMAGE)
                .descriptor_count(1),
            vk::DescriptorPoolSize::default()
                .ty(vk::DescriptorType::STORAGE_BUFFER)
                .descriptor_count(3),
        ];
        pass.descriptor_pool = device
            .create_descriptor_pool(
                &vk::DescriptorPoolCreateInfo::default()
                    .max_sets(1)
                    .pool_sizes(&pool_sizes),
                None,
            )
            .context("could not create path tracing descriptor pool")?;
        let layouts = [tracer.inner.descriptor_layout];
        let descriptor = device
            .allocate_descriptor_sets(
                &vk::DescriptorSetAllocateInfo::default()
                    .descriptor_pool(pass.descriptor_pool)
                    .set_layouts(&layouts),
            )
            .context("could not allocate path tracing descriptor")?[0];
        let structures = [scene.handle()];
        let mut acceleration = vk::WriteDescriptorSetAccelerationStructureKHR::default()
            .acceleration_structures(&structures);
        let image = [vk::DescriptorImageInfo::default()
            .image_view(pass.image_view)
            .image_layout(vk::ImageLayout::GENERAL)];
        let buffers = [&scene.hits, materials, lights].map(|buffer| [buffer.0.descriptor()]);
        let writes = [
            vk::WriteDescriptorSet::default()
                .dst_set(descriptor)
                .dst_binding(0)
                .descriptor_type(vk::DescriptorType::ACCELERATION_STRUCTURE_KHR)
                .descriptor_count(1)
                .push_next(&mut acceleration),
            vk::WriteDescriptorSet::default()
                .dst_set(descriptor)
                .dst_binding(1)
                .descriptor_type(vk::DescriptorType::STORAGE_IMAGE)
                .image_info(&image),
            vk::WriteDescriptorSet::default()
                .dst_set(descriptor)
                .dst_binding(2)
                .descriptor_type(vk::DescriptorType::STORAGE_BUFFER)
                .buffer_info(&buffers[0]),
            vk::WriteDescriptorSet::default()
                .dst_set(descriptor)
                .dst_binding(3)
                .descriptor_type(vk::DescriptorType::STORAGE_BUFFER)
                .buffer_info(&buffers[1]),
            vk::WriteDescriptorSet::default()
                .dst_set(descriptor)
                .dst_binding(4)
                .descriptor_type(vk::DescriptorType::STORAGE_BUFFER)
                .buffer_info(&buffers[2]),
        ];
        device.update_descriptor_sets(&writes, &[]);
        let constants = TraceConstants {
            inverse_view_projection,
            environment: [environment[0], environment[1], environment[2], 0.0],
            sample_index,
            max_bounces,
            seed,
        };
        let bytes = std::slice::from_raw_parts(
            (&constants as *const TraceConstants).cast::<u8>(),
            std::mem::size_of::<TraceConstants>(),
        );
        device.cmd_bind_pipeline(
            self.command_buffer,
            vk::PipelineBindPoint::RAY_TRACING_KHR,
            tracer.inner.pipeline,
        );
        device.cmd_bind_descriptor_sets(
            self.command_buffer,
            vk::PipelineBindPoint::RAY_TRACING_KHR,
            tracer.inner.layout,
            0,
            &[descriptor],
            &[],
        );
        device.cmd_push_constants(
            self.command_buffer,
            tracer.inner.layout,
            vk::ShaderStageFlags::RAYGEN_KHR
                | vk::ShaderStageFlags::MISS_KHR
                | vk::ShaderStageFlags::CLOSEST_HIT_KHR,
            0,
            bytes,
        );
        let loader =
            ash::khr::ray_tracing_pipeline::Device::new(&self.device.instance.instance, device);
        loader.cmd_trace_rays(
            self.command_buffer,
            &tracer.inner.raygen,
            &tracer.inner.miss,
            &tracer.inner.hit,
            &vk::StridedDeviceAddressRegionKHR::default(),
            history.inner.width,
            history.inner.height,
            1,
        );
        self.resources.push(Arc::new(pass));
        crate::profile::record("trace_sample", 1.0);
        Ok(())
    }
}
