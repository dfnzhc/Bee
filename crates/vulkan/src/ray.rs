use crate::device::{Backend, DeviceState};
use crate::test_image::memory_type;
use crate::{Frame, GraphImage};
use anyhow::{bail, Context, Result};
use ash::vk;
use std::io::Cursor;
use std::sync::Arc;

const RAYGEN: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/path.rgen.spv"));
const MISS: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/path.rmiss.spv"));
const HIT: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/path.rchit.spv"));

pub(crate) struct RayBuffer {
    pub device: Arc<DeviceState>,
    pub buffer: vk::Buffer,
    pub memory: vk::DeviceMemory,
    pub size: u64,
    pub address: vk::DeviceAddress,
    coherent: bool,
}

impl Drop for RayBuffer {
    fn drop(&mut self) {
        unsafe {
            self.device.device.destroy_buffer(self.buffer, None);
            self.device.device.free_memory(self.memory, None);
        }
    }
}

impl RayBuffer {
    pub fn new(
        device: &Arc<DeviceState>,
        size: u64,
        usage: vk::BufferUsageFlags,
        host_visible: bool,
    ) -> Result<Self> {
        if size == 0 || !device.ray_tracing_enabled {
            bail!("invalid ray tracing buffer size or unsupported device");
        }
        let mut result = Self {
            device: device.clone(),
            buffer: vk::Buffer::null(),
            memory: vk::DeviceMemory::null(),
            size,
            address: 0,
            coherent: false,
        };
        let vk_device = &device.device;
        unsafe {
            result.buffer = vk_device
                .create_buffer(
                    &vk::BufferCreateInfo::default()
                        .size(size)
                        .usage(usage | vk::BufferUsageFlags::SHADER_DEVICE_ADDRESS)
                        .sharing_mode(vk::SharingMode::EXCLUSIVE),
                    None,
                )
                .context("could not create ray tracing buffer")?;
            let requirements = vk_device.get_buffer_memory_requirements(result.buffer);
            let required = if host_visible {
                vk::MemoryPropertyFlags::HOST_VISIBLE
            } else {
                vk::MemoryPropertyFlags::empty()
            };
            let preferred = if host_visible {
                vk::MemoryPropertyFlags::HOST_COHERENT
            } else {
                vk::MemoryPropertyFlags::DEVICE_LOCAL
            };
            let (index, flags) =
                memory_type(device, requirements.memory_type_bits, required, preferred)?;
            result.coherent = flags.contains(vk::MemoryPropertyFlags::HOST_COHERENT);
            let mut address_flags = vk::MemoryAllocateFlagsInfo::default()
                .flags(vk::MemoryAllocateFlags::DEVICE_ADDRESS);
            result.memory = vk_device
                .allocate_memory(
                    &vk::MemoryAllocateInfo::default()
                        .allocation_size(requirements.size)
                        .memory_type_index(index)
                        .push_next(&mut address_flags),
                    None,
                )
                .context("could not allocate ray tracing buffer memory")?;
            vk_device
                .bind_buffer_memory(result.buffer, result.memory, 0)
                .context("could not bind ray tracing buffer memory")?;
            let info = vk::BufferDeviceAddressInfo::default().buffer(result.buffer);
            result.address = if device.api_version >= vk::API_VERSION_1_2 {
                vk_device.get_buffer_device_address(&info)
            } else {
                ash::khr::buffer_device_address::Device::new(&device.instance.instance, vk_device)
                    .get_buffer_device_address(&info)
            };
        }
        if result.address == 0 {
            bail!("ray tracing buffer has no device address");
        }
        Ok(result)
    }

    pub fn write(&self, bytes: &[u8]) -> Result<()> {
        if bytes.len() as u64 > self.size {
            bail!("ray tracing buffer upload exceeds its size");
        }
        let device = &self.device.device;
        unsafe {
            let mapped = device
                .map_memory(self.memory, 0, vk::WHOLE_SIZE, vk::MemoryMapFlags::empty())
                .context("could not map ray tracing buffer")?;
            std::ptr::copy_nonoverlapping(bytes.as_ptr(), mapped.cast::<u8>(), bytes.len());
            let flush = if self.coherent {
                Ok(())
            } else {
                device.flush_mapped_memory_ranges(&[vk::MappedMemoryRange::default()
                    .memory(self.memory)
                    .offset(0)
                    .size(vk::WHOLE_SIZE)])
            };
            device.unmap_memory(self.memory);
            flush.context("could not flush ray tracing buffer")?;
        }
        Ok(())
    }
}

#[derive(Clone)]
pub struct AccelerationScene {
    pub(crate) inner: Arc<AccelerationState>,
}

pub(crate) struct AccelerationState {
    pub device: Arc<DeviceState>,
    pub top: Structure,
    _bottom: Structure,
    pub vertices: RayBuffer,
    pub vertex_count: u32,
}

pub(crate) struct Structure {
    loader: ash::khr::acceleration_structure::Device,
    pub handle: vk::AccelerationStructureKHR,
    _storage: RayBuffer,
}

impl Drop for Structure {
    fn drop(&mut self) {
        unsafe {
            self.loader
                .destroy_acceleration_structure(self.handle, None)
        }
    }
}

impl AccelerationScene {
    pub fn triangle_count(&self) -> u32 {
        self.inner.vertex_count / 3
    }
}

impl Backend {
    pub fn build_acceleration_scene(
        &self,
        values: &[[f32; 4]],
        vertex_count: u32,
    ) -> Result<AccelerationScene> {
        if !self.device.ray_tracing_enabled {
            bail!(
                "ray tracing unavailable: {}",
                self.report.ray_tracing_missing.join(", ")
            );
        }
        let vertex_values = vertex_count
            .checked_mul(3)
            .context("too many scene vertices")? as usize;
        if !vertex_count.is_multiple_of(3)
            || values.len() < vertex_values
            || !(values.len() - vertex_values).is_multiple_of(2)
            || values.iter().flatten().any(|value| !value.is_finite())
        {
            bail!("invalid acceleration scene data");
        }
        let size = values
            .len()
            .checked_mul(16)
            .context("acceleration scene size overflow")?;
        let mut acceleration_properties =
            vk::PhysicalDeviceAccelerationStructurePropertiesKHR::default();
        let mut properties =
            vk::PhysicalDeviceProperties2::default().push_next(&mut acceleration_properties);
        unsafe {
            self.device
                .instance
                .instance
                .get_physical_device_properties2(self.device.physical_device, &mut properties)
        };
        if size.max(48) as u64 > properties.properties.limits.max_storage_buffer_range as u64
            || (vertex_count / 3) as u64 > acceleration_properties.max_primitive_count
        {
            bail!("acceleration scene exceeds Vulkan device limits");
        }
        let vertices = RayBuffer::new(
            &self.device,
            size.max(48) as u64,
            vk::BufferUsageFlags::STORAGE_BUFFER
                | vk::BufferUsageFlags::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_KHR,
            true,
        )?;
        let bytes = unsafe { std::slice::from_raw_parts(values.as_ptr().cast::<u8>(), size) };
        vertices.write(bytes)?;
        let loader = ash::khr::acceleration_structure::Device::new(
            &self.device.instance.instance,
            &self.device.device,
        );

        let triangles = vk::AccelerationStructureGeometryTrianglesDataKHR::default()
            .vertex_format(vk::Format::R32G32B32_SFLOAT)
            .vertex_data(vk::DeviceOrHostAddressConstKHR {
                device_address: vertices.address,
            })
            .vertex_stride(48)
            .max_vertex(vertex_count.saturating_sub(1))
            .index_type(vk::IndexType::NONE_KHR);
        let bottom_geometry = [vk::AccelerationStructureGeometryKHR::default()
            .geometry_type(vk::GeometryTypeKHR::TRIANGLES)
            .geometry(vk::AccelerationStructureGeometryDataKHR { triangles })
            .flags(vk::GeometryFlagsKHR::OPAQUE)];
        let bottom_info = vk::AccelerationStructureBuildGeometryInfoKHR::default()
            .ty(vk::AccelerationStructureTypeKHR::BOTTOM_LEVEL)
            .flags(vk::BuildAccelerationStructureFlagsKHR::PREFER_FAST_TRACE)
            .mode(vk::BuildAccelerationStructureModeKHR::BUILD)
            .geometries(&bottom_geometry);
        let primitives = vertex_count / 3;
        let mut bottom_sizes = vk::AccelerationStructureBuildSizesInfoKHR::default();
        unsafe {
            loader.get_acceleration_structure_build_sizes(
                vk::AccelerationStructureBuildTypeKHR::DEVICE,
                &bottom_info,
                &[primitives],
                &mut bottom_sizes,
            )
        };
        let bottom_storage = RayBuffer::new(
            &self.device,
            bottom_sizes.acceleration_structure_size,
            vk::BufferUsageFlags::ACCELERATION_STRUCTURE_STORAGE_KHR,
            false,
        )?;
        let bottom = unsafe {
            loader.create_acceleration_structure(
                &vk::AccelerationStructureCreateInfoKHR::default()
                    .buffer(bottom_storage.buffer)
                    .size(bottom_sizes.acceleration_structure_size)
                    .ty(vk::AccelerationStructureTypeKHR::BOTTOM_LEVEL),
                None,
            )
        }
        .context("could not create bottom-level acceleration structure")?;
        let bottom = Structure {
            loader: loader.clone(),
            handle: bottom,
            _storage: bottom_storage,
        };

        // 在创建过程失败时也要先销毁加速结构，再释放底层存储。
        let bottom_address = unsafe {
            loader.get_acceleration_structure_device_address(
                &vk::AccelerationStructureDeviceAddressInfoKHR::default()
                    .acceleration_structure(bottom.handle),
            )
        };
        let instance = vk::AccelerationStructureInstanceKHR {
            transform: vk::TransformMatrixKHR {
                matrix: [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0],
            },
            instance_custom_index_and_mask: vk::Packed24_8::new(0, 0xff),
            instance_shader_binding_table_record_offset_and_flags: vk::Packed24_8::new(
                0,
                vk::GeometryInstanceFlagsKHR::TRIANGLE_FACING_CULL_DISABLE.as_raw() as u8,
            ),
            acceleration_structure_reference: vk::AccelerationStructureReferenceKHR {
                device_handle: bottom_address,
            },
        };
        let result =
            (|| -> Result<_> {
                let instances = RayBuffer::new(
                    &self.device,
                    std::mem::size_of_val(&instance) as u64,
                    vk::BufferUsageFlags::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_KHR,
                    true,
                )?;
                instances.write(unsafe {
                    std::slice::from_raw_parts(
                        (&instance as *const vk::AccelerationStructureInstanceKHR).cast::<u8>(),
                        std::mem::size_of_val(&instance),
                    )
                })?;
                let instance_data = vk::AccelerationStructureGeometryInstancesDataKHR::default()
                    .array_of_pointers(false)
                    .data(vk::DeviceOrHostAddressConstKHR {
                        device_address: instances.address,
                    });
                let top_geometry = [vk::AccelerationStructureGeometryKHR::default()
                    .geometry_type(vk::GeometryTypeKHR::INSTANCES)
                    .geometry(vk::AccelerationStructureGeometryDataKHR {
                        instances: instance_data,
                    })];
                let top_info = vk::AccelerationStructureBuildGeometryInfoKHR::default()
                    .ty(vk::AccelerationStructureTypeKHR::TOP_LEVEL)
                    .flags(vk::BuildAccelerationStructureFlagsKHR::PREFER_FAST_TRACE)
                    .mode(vk::BuildAccelerationStructureModeKHR::BUILD)
                    .geometries(&top_geometry);
                let mut top_sizes = vk::AccelerationStructureBuildSizesInfoKHR::default();
                unsafe {
                    loader.get_acceleration_structure_build_sizes(
                        vk::AccelerationStructureBuildTypeKHR::DEVICE,
                        &top_info,
                        &[1],
                        &mut top_sizes,
                    )
                };
                let top_storage = RayBuffer::new(
                    &self.device,
                    top_sizes.acceleration_structure_size,
                    vk::BufferUsageFlags::ACCELERATION_STRUCTURE_STORAGE_KHR,
                    false,
                )?;
                let top = unsafe {
                    loader.create_acceleration_structure(
                        &vk::AccelerationStructureCreateInfoKHR::default()
                            .buffer(top_storage.buffer)
                            .size(top_sizes.acceleration_structure_size)
                            .ty(vk::AccelerationStructureTypeKHR::TOP_LEVEL),
                        None,
                    )
                }
                .context("could not create top-level acceleration structure")?;
                let top = Structure {
                    loader: loader.clone(),
                    handle: top,
                    _storage: top_storage,
                };
                let bottom_handle = bottom.handle;
                let top_handle = top.handle;
                let scene = Arc::new(AccelerationState {
                    device: self.device.clone(),
                    _bottom: bottom,
                    top,
                    vertices,
                    vertex_count,
                });

                let alignment = acceleration_properties
                    .min_acceleration_structure_scratch_offset_alignment
                    as u64;
                let scratch_size = bottom_sizes
                    .build_scratch_size
                    .max(top_sizes.build_scratch_size);
                let scratch = Arc::new(RayBuffer::new(
                    &self.device,
                    scratch_size
                        .checked_add(alignment)
                        .context("scratch size overflow")?,
                    vk::BufferUsageFlags::STORAGE_BUFFER,
                    false,
                )?);
                let scratch_address = align_up(scratch.address, alignment)?;
                let mut frame = self.begin_frame()?;
                let bottom_build = bottom_info
                    .dst_acceleration_structure(bottom_handle)
                    .scratch_data(vk::DeviceOrHostAddressKHR {
                        device_address: scratch_address,
                    });
                let top_build = top_info
                    .dst_acceleration_structure(top_handle)
                    .scratch_data(vk::DeviceOrHostAddressKHR {
                        device_address: scratch_address,
                    });
                unsafe {
                    loader.cmd_build_acceleration_structures(
                        frame.command_buffer,
                        &[bottom_build],
                        &[&[vk::AccelerationStructureBuildRangeInfoKHR::default()
                            .primitive_count(primitives)]],
                    );
                    let barrier = vk::MemoryBarrier::default()
                        .src_access_mask(vk::AccessFlags::ACCELERATION_STRUCTURE_WRITE_KHR)
                        .dst_access_mask(
                            vk::AccessFlags::ACCELERATION_STRUCTURE_READ_KHR
                                | vk::AccessFlags::ACCELERATION_STRUCTURE_WRITE_KHR,
                        );
                    self.device.device.cmd_pipeline_barrier(
                        frame.command_buffer,
                        vk::PipelineStageFlags::ACCELERATION_STRUCTURE_BUILD_KHR,
                        vk::PipelineStageFlags::ACCELERATION_STRUCTURE_BUILD_KHR,
                        vk::DependencyFlags::empty(),
                        &[barrier],
                        &[],
                        &[],
                    );
                    loader.cmd_build_acceleration_structures(
                        frame.command_buffer,
                        &[top_build],
                        &[&[vk::AccelerationStructureBuildRangeInfoKHR::default()
                            .primitive_count(1)]],
                    );
                    let barrier = vk::MemoryBarrier::default()
                        .src_access_mask(vk::AccessFlags::ACCELERATION_STRUCTURE_WRITE_KHR)
                        .dst_access_mask(vk::AccessFlags::ACCELERATION_STRUCTURE_READ_KHR);
                    self.device.device.cmd_pipeline_barrier(
                        frame.command_buffer,
                        vk::PipelineStageFlags::ACCELERATION_STRUCTURE_BUILD_KHR,
                        vk::PipelineStageFlags::RAY_TRACING_SHADER_KHR,
                        vk::DependencyFlags::empty(),
                        &[barrier],
                        &[],
                        &[],
                    );
                }
                frame.resources.push(scene.clone());
                frame.resources.push(scratch);
                frame.resources.push(Arc::new(instances));
                frame.submit()?.wait()?;
                Ok(AccelerationScene { inner: scene })
            })();
        result
    }
}

pub struct PathTracer {
    inner: Arc<RayPipeline>,
}

struct RayPipeline {
    device: Arc<DeviceState>,
    descriptor_layout: vk::DescriptorSetLayout,
    layout: vk::PipelineLayout,
    pipeline: vk::Pipeline,
    sbt: Option<RayBuffer>,
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

fn align_up(value: u64, alignment: u64) -> Result<u64> {
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
                .stage_flags(stages),
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
        let sbt = RayBuffer::new(
            &self.device,
            total,
            vk::BufferUsageFlags::SHADER_BINDING_TABLE_KHR,
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
        pipeline.sbt = Some(sbt);
        Ok(PathTracer {
            inner: Arc::new(pipeline),
        })
    }
}

#[repr(C)]
struct TraceConstants {
    inverse_view_projection: [f32; 16],
    environment: [f32; 4],
    light_offset: u32,
    sample_index: u32,
    max_bounces: u32,
    seed: u32,
}

impl Frame {
    /// # Safety
    /// 历史图像须处于 GENERAL 布局，且前序读写已同步；首次采样须使用编号零。
    pub unsafe fn trace_paths(
        &mut self,
        tracer: &PathTracer,
        scene: &AccelerationScene,
        history: &GraphImage,
        inverse_view_projection: [f32; 16],
        environment: [f32; 3],
        sample_index: u32,
        max_bounces: u32,
        seed: u32,
    ) -> Result<()> {
        let size = (history.inner.width as u64) * (history.inner.height as u64);
        if !Arc::ptr_eq(&self.device, &tracer.inner.device)
            || !Arc::ptr_eq(&self.device, &scene.inner.device)
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
                .descriptor_count(1),
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
        let structures = [scene.inner.top.handle];
        let mut acceleration = vk::WriteDescriptorSetAccelerationStructureKHR::default()
            .acceleration_structures(&structures);
        let image = [vk::DescriptorImageInfo::default()
            .image_view(pass.image_view)
            .image_layout(vk::ImageLayout::GENERAL)];
        let buffer = [vk::DescriptorBufferInfo::default()
            .buffer(scene.inner.vertices.buffer)
            .offset(0)
            .range(scene.inner.vertices.size)];
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
                .buffer_info(&buffer),
        ];
        device.update_descriptor_sets(&writes, &[]);
        let constants = TraceConstants {
            inverse_view_projection,
            environment: [environment[0], environment[1], environment[2], 0.0],
            light_offset: scene
                .inner
                .vertex_count
                .checked_mul(3)
                .context("too many scene vertices")?,
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
        Ok(())
    }
}
