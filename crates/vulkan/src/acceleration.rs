use crate::device::DeviceState;
use crate::ray::{align_up, bytes_of_slice, RayBuffer, SceneBuffer};
use crate::{profile, Backend, Frame};
use anyhow::{bail, Context, Result};
use ash::vk;
use std::sync::Arc;

const BUILD_INPUT: vk::BufferUsageFlags =
    vk::BufferUsageFlags::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_KHR;

#[derive(Clone)]
pub struct AccelerationMesh(Option<Arc<Mesh>>);

struct Mesh {
    _bottom: Arc<Structure>,
    vertices: Arc<RayBuffer>,
    address: vk::DeviceAddress,
    triangles: u32,
}

pub struct RayInstance {
    pub mesh: usize,
    pub transform: [f32; 12],
    pub material: u32,
}

fn valid_transform(matrix: [f32; 12]) -> bool {
    if !matrix.into_iter().all(f32::is_finite) {
        return false;
    }
    let [a, b, c, _, d, e, f, _, g, h, i, _] = matrix.map(f64::from);
    a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g) != 0.0
}

#[derive(Clone)]
pub struct AccelerationScene {
    top: Arc<TopLevel>,
    pub(crate) hits: SceneBuffer,
    hit_inputs: Arc<[HitInput]>,
    triangles: u32,
}

struct TopLevel {
    structure: Arc<Structure>,
    inputs: Vec<TopInput>,
    _meshes: Vec<AccelerationMesh>,
}

#[derive(PartialEq)]
struct TopInput {
    address: vk::DeviceAddress,
    transform: [f32; 12],
}

#[derive(PartialEq)]
#[repr(C)]
struct HitInput {
    vertices: vk::DeviceAddress,
    material: u32,
    padding: u32,
}

struct Structure {
    loader: ash::khr::acceleration_structure::Device,
    handle: vk::AccelerationStructureKHR,
    storage: RayBuffer,
}

impl Drop for Structure {
    fn drop(&mut self) {
        unsafe {
            self.loader
                .destroy_acceleration_structure(self.handle, None)
        };
    }
}

impl AccelerationScene {
    pub fn triangle_count(&self) -> u32 {
        self.triangles
    }

    pub(crate) fn handle(&self) -> vk::AccelerationStructureKHR {
        self.top.structure.handle
    }

    pub(crate) fn device(&self) -> &Arc<DeviceState> {
        &self.top.structure.storage.device
    }
}

impl Backend {
    fn acceleration_properties(
        &self,
    ) -> Result<vk::PhysicalDeviceAccelerationStructurePropertiesKHR<'static>> {
        if !self.device.ray_tracing_enabled {
            bail!(
                "ray tracing unavailable: {}",
                self.report.ray_tracing_missing.join(", ")
            );
        }
        let mut properties = vk::PhysicalDeviceAccelerationStructurePropertiesKHR::default();
        unsafe {
            self.device
                .instance
                .instance
                .get_physical_device_properties2(
                    self.device.physical_device,
                    &mut vk::PhysicalDeviceProperties2::default().push_next(&mut properties),
                );
        }
        Ok(properties)
    }

    pub fn build_acceleration_mesh(&self, values: &[[f32; 4]]) -> Result<AccelerationMesh> {
        let properties = self.acceleration_properties()?;
        if !values.len().is_multiple_of(6)
            || u32::try_from(values.len()).is_err()
            || values.iter().flatten().any(|v| !v.is_finite())
        {
            bail!("invalid acceleration mesh data");
        }
        if values.is_empty() {
            return Ok(AccelerationMesh(None));
        }
        let vertex_count = u32::try_from(values.len() / 2).context("too many mesh vertices")?;
        let primitives = vertex_count / 3;
        if u64::from(primitives) > properties.max_primitive_count {
            bail!("acceleration mesh exceeds Vulkan primitive limit");
        }
        let bytes = unsafe { bytes_of_slice(values) };
        let vertices = Arc::new(profile::span("mesh_upload_us", || {
            let vertices = RayBuffer::new(
                &self.device,
                bytes.len() as u64,
                vk::BufferUsageFlags::STORAGE_BUFFER | BUILD_INPUT,
                true,
            )?;
            vertices.write(bytes)?;
            Ok::<_, anyhow::Error>(vertices)
        })?);
        profile::record("mesh_upload", 1.0);
        let triangles = vk::AccelerationStructureGeometryTrianglesDataKHR::default()
            .vertex_format(vk::Format::R32G32B32_SFLOAT)
            .vertex_data(vk::DeviceOrHostAddressConstKHR {
                device_address: vertices.address,
            })
            .vertex_stride(32)
            .max_vertex(vertex_count - 1)
            .index_type(vk::IndexType::NONE_KHR);
        let geometry = [vk::AccelerationStructureGeometryKHR::default()
            .geometry_type(vk::GeometryTypeKHR::TRIANGLES)
            .geometry(vk::AccelerationStructureGeometryDataKHR { triangles })
            .flags(vk::GeometryFlagsKHR::OPAQUE)];
        let mut frame = self.begin_frame()?;
        let bottom = self.record_acceleration_build(
            &mut frame,
            vk::AccelerationStructureTypeKHR::BOTTOM_LEVEL,
            &geometry,
            primitives,
            &properties,
            None,
        )?;
        frame.resources.push(vertices.clone());
        frame.submit()?.wait()?;
        let address = unsafe {
            bottom.loader.get_acceleration_structure_device_address(
                &vk::AccelerationStructureDeviceAddressInfoKHR::default()
                    .acceleration_structure(bottom.handle),
            )
        };
        if address == 0 {
            bail!("bottom-level acceleration structure has no device address");
        }
        Ok(AccelerationMesh(Some(Arc::new(Mesh {
            _bottom: bottom,
            vertices,
            address,
            triangles: primitives,
        }))))
    }

    pub fn build_acceleration_scene(
        &self,
        meshes: &[AccelerationMesh],
        instances: &[RayInstance],
        previous: Option<&AccelerationScene>,
    ) -> Result<AccelerationScene> {
        let properties = self.acceleration_properties()?;
        let count = instances.len();
        if count > (1 << 24)
            || count as u64 > properties.max_instance_count
            || previous.is_some_and(|scene| !Arc::ptr_eq(scene.device(), &self.device))
            || meshes
                .iter()
                .filter_map(|m| m.0.as_ref())
                .any(|mesh| !Arc::ptr_eq(&mesh.vertices.device, &self.device))
        {
            bail!("incompatible acceleration scene instances or device");
        }
        let mut top_inputs = Vec::with_capacity(count);
        let mut hit_inputs = Vec::with_capacity(count);
        let mut triangles = 0u32;
        for instance in instances {
            let mesh = meshes
                .get(instance.mesh)
                .context("invalid ray instance mesh")?
                .0
                .as_ref();
            if !valid_transform(instance.transform) {
                bail!("invalid ray instance transform");
            }
            triangles = triangles
                .checked_add(mesh.map_or(0, |m| m.triangles))
                .context("too many instanced triangles")?;
            top_inputs.push(TopInput {
                address: mesh.map_or(0, |m| m.address),
                transform: instance.transform,
            });
            hit_inputs.push(HitInput {
                vertices: mesh.map_or(0, |m| m.vertices.address),
                material: instance.material,
                padding: 0,
            });
        }
        let (hits, hit_inputs) = match previous.filter(|old| *old.hit_inputs == hit_inputs) {
            Some(old) => (old.hits.clone(), old.hit_inputs.clone()),
            None => (
                SceneBuffer::upload(&self.device, unsafe { bytes_of_slice(&hit_inputs) })?,
                hit_inputs.into(),
            ),
        };
        let previous_top = previous.map(|old| &old.top);
        let top = if let Some(old) = previous_top.filter(|old| old.inputs == top_inputs) {
            old.clone()
        } else {
            // UPDATE 要求实例数与每个实例的活跃状态保持一致。
            let update = previous_top.filter(|old| {
                old.inputs.len() == count
                    && old
                        .inputs
                        .iter()
                        .zip(&top_inputs)
                        .all(|(a, b)| (a.address == 0) == (b.address == 0))
            });
            let records: Vec<_> = top_inputs
                .iter()
                .enumerate()
                .map(|(i, instance)| vk::AccelerationStructureInstanceKHR {
                    transform: vk::TransformMatrixKHR {
                        matrix: instance.transform,
                    },
                    instance_custom_index_and_mask: vk::Packed24_8::new(i as u32, 0xff),
                    instance_shader_binding_table_record_offset_and_flags: vk::Packed24_8::new(
                        0,
                        vk::GeometryInstanceFlagsKHR::TRIANGLE_FACING_CULL_DISABLE.as_raw() as u8,
                    ),
                    acceleration_structure_reference: vk::AccelerationStructureReferenceKHR {
                        device_handle: instance.address,
                    },
                })
                .collect();
            let bytes = unsafe { bytes_of_slice(&records) };
            let input = Arc::new(RayBuffer::new(
                &self.device,
                bytes.len().max(64) as u64,
                BUILD_INPUT,
                true,
            )?);
            input.write(bytes)?;
            let data = vk::AccelerationStructureGeometryInstancesDataKHR::default().data(
                vk::DeviceOrHostAddressConstKHR {
                    device_address: input.address,
                },
            );
            let geometry = [vk::AccelerationStructureGeometryKHR::default()
                .geometry_type(vk::GeometryTypeKHR::INSTANCES)
                .geometry(vk::AccelerationStructureGeometryDataKHR { instances: data })];
            let mut frame = self.begin_frame()?;
            let structure = self.record_acceleration_build(
                &mut frame,
                vk::AccelerationStructureTypeKHR::TOP_LEVEL,
                &geometry,
                count as u32,
                &properties,
                update.map(|old| &old.structure),
            )?;
            let top = Arc::new(TopLevel {
                structure,
                inputs: top_inputs,
                _meshes: instances
                    .iter()
                    .map(|instance| meshes[instance.mesh].clone())
                    .collect(),
            });
            // 旧 TLAS 可被在途帧读取，更新写入独立目标，源结构及 BLAS 保留到完成。
            if let Some(old) = update {
                frame.resources.push(old.clone());
            }
            frame.resources.push(input);
            frame.resources.push(top.clone());
            frame.submit()?.wait()?;
            top
        };
        Ok(AccelerationScene {
            top,
            hits,
            hit_inputs,
            triangles,
        })
    }

    fn record_acceleration_build(
        &self,
        frame: &mut Frame,
        ty: vk::AccelerationStructureTypeKHR,
        geometry: &[vk::AccelerationStructureGeometryKHR],
        primitives: u32,
        properties: &vk::PhysicalDeviceAccelerationStructurePropertiesKHR,
        source: Option<&Arc<Structure>>,
    ) -> Result<Arc<Structure>> {
        let device = &self.device.device;
        let loader =
            ash::khr::acceleration_structure::Device::new(&self.device.instance.instance, device);
        let top = ty == vk::AccelerationStructureTypeKHR::TOP_LEVEL;
        let flags = vk::BuildAccelerationStructureFlagsKHR::PREFER_FAST_TRACE
            | if top {
                vk::BuildAccelerationStructureFlagsKHR::ALLOW_UPDATE
            } else {
                vk::BuildAccelerationStructureFlagsKHR::empty()
            };
        let info = vk::AccelerationStructureBuildGeometryInfoKHR::default()
            .ty(ty)
            .flags(flags)
            .geometries(geometry);
        let mut sizes = vk::AccelerationStructureBuildSizesInfoKHR::default();
        unsafe {
            loader.get_acceleration_structure_build_sizes(
                vk::AccelerationStructureBuildTypeKHR::DEVICE,
                &info,
                &[primitives],
                &mut sizes,
            );
        }
        let storage = RayBuffer::new(
            &self.device,
            sizes.acceleration_structure_size,
            vk::BufferUsageFlags::ACCELERATION_STRUCTURE_STORAGE_KHR,
            false,
        )?;
        let handle = unsafe {
            loader.create_acceleration_structure(
                &vk::AccelerationStructureCreateInfoKHR::default()
                    .buffer(storage.buffer)
                    .size(storage.size)
                    .ty(ty),
                None,
            )
        }
        .context("could not create acceleration structure")?;
        let structure = Arc::new(Structure {
            loader,
            handle,
            storage,
        });
        let (scratch_size, info, counter) = match source {
            Some(source) => (
                sizes.update_scratch_size,
                info.mode(vk::BuildAccelerationStructureModeKHR::UPDATE)
                    .src_acceleration_structure(source.handle),
                "tlas_update",
            ),
            None => (
                sizes.build_scratch_size,
                info,
                if top { "tlas_build" } else { "blas_build" },
            ),
        };
        let alignment = u64::from(properties.min_acceleration_structure_scratch_offset_alignment);
        let scratch = Arc::new(RayBuffer::new(
            &self.device,
            scratch_size
                .checked_add(alignment)
                .context("scratch size overflow")?,
            vk::BufferUsageFlags::STORAGE_BUFFER,
            false,
        )?);
        let build =
            info.dst_acceleration_structure(handle)
                .scratch_data(vk::DeviceOrHostAddressKHR {
                    device_address: align_up(scratch.address, alignment)?,
                });
        let timestamp = frame.profile_begin(if top { "gpu_tlas_us" } else { "gpu_blas_us" });
        profile::record(counter, 1.0);
        unsafe {
            structure.loader.cmd_build_acceleration_structures(
                frame.command_buffer,
                &[build],
                &[&[vk::AccelerationStructureBuildRangeInfoKHR::default()
                    .primitive_count(primitives)]],
            );
            frame.profile_end(timestamp);
            let build_stage = vk::PipelineStageFlags::ACCELERATION_STRUCTURE_BUILD_KHR;
            let barrier = vk::MemoryBarrier::default()
                .src_access_mask(vk::AccessFlags::ACCELERATION_STRUCTURE_WRITE_KHR)
                .dst_access_mask(vk::AccessFlags::ACCELERATION_STRUCTURE_READ_KHR);
            device.cmd_pipeline_barrier(
                frame.command_buffer,
                build_stage,
                build_stage | vk::PipelineStageFlags::RAY_TRACING_SHADER_KHR,
                vk::DependencyFlags::empty(),
                &[barrier],
                &[],
                &[],
            );
        }
        frame.resources.push(structure.clone());
        frame.resources.push(scratch);
        Ok(structure)
    }
}

#[cfg(test)]
mod tests {
    use super::valid_transform;

    #[test]
    fn acceleration_instances_require_invertible_affine_transforms() {
        let mut matrix = [2.0, 1.0, 0.0, 4.0, 0.0, 2.0, 0.0, 5.0, 0.0, 0.0, -1.0, 6.0];
        assert!(valid_transform(matrix));
        matrix[10] = 0.0;
        assert!(!valid_transform(matrix));
        matrix[10] = 1.0;
        matrix[3] = f32::INFINITY;
        assert!(!valid_transform(matrix));
    }
}
