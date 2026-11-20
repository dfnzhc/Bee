use crate::GraphResource;
use bee_scene::{Geometry, Instance, Scene, Transform3};
use bee_vulkan::{
    profile, AccelerationMesh, AccelerationScene, Backend, RasterGeometry, RasterInstance,
    RasterInstances, RayInstance, SceneBuffer,
};
use std::sync::Arc;

pub(crate) type RasterScene = CachedScene<RasterGeometry, RasterInstances>;
pub(crate) type PathScene = CachedScene<AccelerationMesh, AccelerationScene>;

pub(crate) struct CachedScene<M, I> {
    pub source: Arc<Scene>,
    pub meshes: Vec<M>,
    pub instances: I,
    pub materials: SceneBuffer,
    pub lights: SceneBuffer,
    pub reads: [GraphResource; 2],
}

impl<M: Clone, I> CachedScene<M, I> {
    fn prepare(
        backend: &Backend,
        source: &Arc<Scene>,
        previous: Option<&Self>,
        upload_mesh: impl Fn(&Backend, &[[f32; 4]]) -> Result<M, String>,
        prepare_instances: impl FnOnce(&[M], bool) -> Result<I, String>,
        graph_reads: impl FnOnce(&[M], &I, &SceneBuffer, &SceneBuffer) -> [GraphResource; 2],
    ) -> Result<Self, String> {
        if !valid_color(&source.environment) {
            return Err("invalid environment color".into());
        }
        for instance in &source.instances {
            if instance.geometry >= source.geometries.len() {
                return Err("invalid instance geometry".into());
            }
            if instance.material >= source.materials.len()
                || u32::try_from(instance.material).is_err()
            {
                return Err("invalid instance material".into());
            }
        }
        let materials =
            if let Some(old) = previous.filter(|old| old.source.materials == source.materials) {
                old.materials.clone()
            } else {
                let mut values = Vec::with_capacity(source.materials.len());
                for material in &source.materials {
                    let [r, g, b] = material.albedo;
                    if !valid_color(&material.albedo) {
                        return Err("invalid material albedo".into());
                    }
                    values.push([r, g, b, 0.0]);
                }
                profile::record("material_upload", 1.0);
                backend
                    .upload_scene_buffer(&values)
                    .map_err(|error| error.to_string())?
            };
        let lights = if let Some(old) = previous.filter(|old| old.source.lights == source.lights) {
            old.lights.clone()
        } else {
            let mut values = Vec::with_capacity(source.lights.len() * 2);
            for light in &source.lights {
                let [r, g, b] = light.intensity;
                if !valid_color(&light.intensity) {
                    return Err("invalid point light".into());
                }
                values.extend([
                    [light.position.x, light.position.y, light.position.z, 1.0],
                    [r, g, b, 0.0],
                ]);
            }
            profile::record("light_upload", 1.0);
            backend
                .upload_scene_buffer(&values)
                .map_err(|error| error.to_string())?
        };
        // 比较实际使用过的快照，输入合并后也不能漏掉任何网格更新。
        let mut meshes_changed =
            previous.is_none_or(|old| old.meshes.len() != source.geometries.len());
        let mut meshes = Vec::with_capacity(source.geometries.len());
        for (index, geometry) in source.geometries.iter().enumerate() {
            if let Some(old) =
                previous.filter(|old| old.source.geometries.get(index) == Some(geometry))
            {
                meshes.push(old.meshes[index].clone());
            } else {
                meshes_changed = true;
                meshes.push(upload_mesh(backend, &mesh_vertices(geometry)?)?);
            }
        }
        let instances = prepare_instances(&meshes, meshes_changed)?;
        let reads = graph_reads(&meshes, &instances, &materials, &lights);
        Ok(Self {
            source: source.clone(),
            meshes,
            instances,
            materials,
            lights,
            reads,
        })
    }
}

impl RasterScene {
    pub fn raster(
        backend: &Backend,
        source: &Arc<Scene>,
        previous: Option<&Self>,
    ) -> Result<Self, String> {
        Self::prepare(
            backend,
            source,
            previous,
            |backend, values| {
                backend
                    .upload_raster_geometry(values)
                    .map_err(|error| error.to_string())
            },
            |_, _| {
                if let Some(old) = previous.filter(|old| old.source.instances == source.instances) {
                    return Ok(old.instances.clone());
                }
                let data = source
                    .instances
                    .iter()
                    .map(raster_instance)
                    .collect::<Result<Vec<_>, _>>()?;
                let meshes = source
                    .instances
                    .iter()
                    .map(|instance| instance.geometry)
                    .collect();
                profile::record("instance_upload", 1.0);
                backend
                    .upload_raster_instances(&data, meshes)
                    .map_err(|error| error.to_string())
            },
            |meshes, instances, materials, lights| {
                let vertex: Vec<_> = meshes
                    .iter()
                    .map(RasterGeometry::graph_buffer)
                    .chain([instances.graph_buffer(), materials.graph_buffer()])
                    .collect();
                [vertex.into(), lights.graph_buffer().into()]
            },
        )
    }
}

impl PathScene {
    pub fn path(
        backend: &Backend,
        source: &Arc<Scene>,
        previous: Option<&Self>,
    ) -> Result<Self, String> {
        Self::prepare(
            backend,
            source,
            previous,
            |backend, values| {
                profile::span("acceleration_us", || {
                    backend.build_acceleration_mesh(values)
                })
                .map_err(|error| error.to_string())
            },
            |meshes, meshes_changed| {
                if let Some(old) = previous
                    .filter(|old| !meshes_changed && old.source.instances == source.instances)
                {
                    return Ok(old.instances.clone());
                }
                let instances = source
                    .instances
                    .iter()
                    .map(|instance| {
                        inverse_transform(instance.transform)?;
                        let t = instance.transform;
                        // euclid 为列主序，Vulkan 实例读取行主序的 3x4 仿射矩阵。
                        Ok::<_, String>(RayInstance {
                            mesh: instance.geometry,
                            transform: [
                                t.m11, t.m21, t.m31, t.m41, t.m12, t.m22, t.m32, t.m42, t.m13,
                                t.m23, t.m33, t.m43,
                            ],
                            material: instance.material as u32,
                        })
                    })
                    .collect::<Result<Vec<_>, _>>()?;
                profile::span("acceleration_us", || {
                    backend.build_acceleration_scene(
                        meshes,
                        &instances,
                        previous.map(|old| &old.instances),
                    )
                })
                .map_err(|error| error.to_string())
            },
            |_, instances, materials, lights| {
                let mut shader = instances.shader_buffers();
                shader.extend([materials.graph_buffer(), lights.graph_buffer()]);
                [shader.into(), instances.acceleration_buffers().into()]
            },
        )
    }
}

fn valid_color(color: &[f32; 3]) -> bool {
    color.iter().all(|value| value.is_finite() && *value >= 0.0)
}

fn mesh_vertices(geometry: &Geometry) -> Result<Vec<[f32; 4]>, String> {
    if !geometry.indices.len().is_multiple_of(3) || geometry.indices.len() > u32::MAX as usize / 2 {
        return Err("invalid triangle count".into());
    }
    let mut values = Vec::with_capacity(geometry.indices.len() * 2);
    for &index in &geometry.indices {
        let vertex = geometry
            .vertices
            .get(index as usize)
            .ok_or("invalid geometry index")?;
        let p = vertex.position;
        let n = vertex.normal;
        if ![p.x, p.y, p.z, n.x, n.y, n.z]
            .into_iter()
            .all(f32::is_finite)
        {
            return Err("invalid vertex position or normal".into());
        }
        let scale = n.x.abs().max(n.y.abs()).max(n.z.abs());
        if scale == 0.0 {
            return Err("invalid vertex normal".into());
        }
        values.extend([
            [p.x, p.y, p.z, 1.0],
            [n.x / scale, n.y / scale, n.z / scale, 0.0],
        ]);
    }
    Ok(values)
}

fn inverse_transform(transform: Transform3) -> Result<Transform3, String> {
    if transform.to_array().iter().any(|value| !value.is_finite())
        || transform.m14 != 0.0
        || transform.m24 != 0.0
        || transform.m34 != 0.0
        || transform.m44 != 1.0
        || !transform.determinant().is_finite()
    {
        return Err("invalid instance transform".into());
    }
    transform
        .inverse()
        .filter(|inverse| inverse.to_array().iter().all(|v| v.is_finite()))
        .ok_or_else(|| "non-invertible instance transform".into())
}

fn raster_instance(instance: &Instance) -> Result<RasterInstance, String> {
    let inverse = inverse_transform(instance.transform)?;
    Ok(RasterInstance {
        model: instance.transform.to_array(),
        normal: [
            [inverse.m11, inverse.m21, inverse.m31, 0.0],
            [inverse.m12, inverse.m22, inverse.m32, 0.0],
            [inverse.m13, inverse.m23, inverse.m33, 0.0],
        ],
        material: [instance.material as u32, 0, 0, 0],
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use bee_scene::{Point3, Vector3, Vertex};

    #[test]
    fn prepares_object_space_mesh_and_inverse_transpose_instances() {
        let mut mesh = Geometry {
            vertices: vec![Vertex {
                position: Point3::new(2.0, 3.0, 4.0),
                normal: Vector3::new(1.0, 1.0, 0.0),
            }],
            indices: vec![0, 0, 0],
        };
        let vertices = mesh_vertices(&mesh).unwrap();
        assert_eq!(
            &vertices[..2],
            &[[2.0, 3.0, 4.0, 1.0], [1.0, 1.0, 0.0, 0.0]]
        );
        let mut instance = Instance {
            geometry: 0,
            material: 1,
            transform: Transform3::scale(2.0, 1.0, 1.0).then_translate(Vector3::new(5.0, 0.0, 0.0)),
        };
        let data = raster_instance(&instance).unwrap();
        assert_eq!(data.model[12], 5.0);
        assert_eq!(data.normal[0], [0.5, 0.0, 0.0, 0.0]);
        assert_eq!(data.material[0], 1);
        instance.transform = Transform3::scale(10_000.0, 10_000.0, 10_000.0);
        assert!(raster_instance(&instance).is_ok());
        instance.transform = Transform3::scale(0.0, 1.0, 1.0);
        assert!(raster_instance(&instance).is_err());
        mesh.indices[0] = 99;
        assert!(mesh_vertices(&mesh).unwrap_err().contains("index"));
    }
}
