use crate::{
    Access, DisplayWork, GraphResource, GraphResources, Module, ModuleGraph, NodeId, ResourceId,
    Usage, WorkGraph, WorkId,
};
use ash::vk;
use bee_scene::{RenderSnapshot, Vector3};
use bee_vulkan::{Backend, Frame, Rasterizer};

pub struct RasterWork {
    color: ResourceId,
    depth: ResourceId,
    work: WorkId,
}

impl RasterWork {
    pub fn declare(
        modules: &ModuleGraph,
        node: NodeId,
        graph: &mut WorkGraph,
        color: ResourceId,
        depth: ResourceId,
    ) -> Result<Self, String> {
        if modules.module(node) != Some(Module::Rasterization)
            || color == depth
            || color.0 >= graph.resources.len()
            || depth.0 >= graph.resources.len()
        {
            return Err("raster work requires a raster module and two distinct resources".into());
        }
        let work = graph.add_work(
            "rasterization",
            vec![
                Access {
                    resource: color,
                    usage: Usage::ColorAttachment,
                },
                Access {
                    resource: depth,
                    usage: Usage::DepthAttachment,
                },
            ],
        )?;
        Ok(Self { color, depth, work })
    }

    /// # Safety
    /// 工作图须已建立附件屏障，并保证图像用途和设备有效。
    pub unsafe fn record(
        &self,
        work: WorkId,
        frame: &mut Frame,
        images: &[GraphResource],
        rasterizer: &Rasterizer,
        snapshot: &RenderSnapshot,
        width: u32,
        height: u32,
    ) -> Result<bool, String> {
        if work != self.work {
            return Ok(false);
        }
        if width == 0 || height == 0 || images.len() <= self.color.0.max(self.depth.0) {
            return Err("raster work is missing attachments or a valid extent".into());
        }
        let matrix = snapshot
            .camera
            .view_projection(width as f32 / height as f32)
            .ok_or("invalid raster camera")?
            .to_array();
        let (data, vertices) = scene_data(snapshot)?;
        frame
            .rasterize(
                rasterizer,
                images[self.color.0].image()?,
                images[self.depth.0].image()?,
                &data,
                vertices,
                matrix,
                snapshot.scene.environment,
            )
            .map_err(|error| error.to_string())?;
        Ok(true)
    }
}

fn scene_data(snapshot: &RenderSnapshot) -> Result<(Vec<[f32; 4]>, u32), String> {
    let scene = &snapshot.scene;
    let valid_color =
        |color: &[f32; 3]| color.iter().all(|value| value.is_finite() && *value >= 0.0);
    if !valid_color(&scene.environment) {
        return Err("invalid environment color".into());
    }
    let mut data = Vec::new();
    for instance in &scene.instances {
        let geometry = scene
            .geometries
            .get(instance.geometry)
            .ok_or("invalid instance geometry")?;
        let material = scene
            .materials
            .get(instance.material)
            .ok_or("invalid instance material")?;
        let transform = instance.transform;
        if !valid_color(&material.albedo)
            || !geometry.indices.len().is_multiple_of(3)
            || transform.to_array().iter().any(|value| !value.is_finite())
            || transform.m14 != 0.0
            || transform.m24 != 0.0
            || transform.m34 != 0.0
            || transform.m44 != 1.0
        {
            return Err("invalid geometry, material, or instance transform".into());
        }
        let inverse = transform
            .inverse()
            .ok_or("non-invertible instance transform")?;
        for &index in &geometry.indices {
            let vertex = geometry
                .vertices
                .get(index as usize)
                .ok_or("invalid geometry index")?;
            let position = transform
                .transform_point3d(vertex.position)
                .ok_or("invalid vertex position")?;
            let normal = Vector3::new(
                inverse.m11 * vertex.normal.x
                    + inverse.m12 * vertex.normal.y
                    + inverse.m13 * vertex.normal.z,
                inverse.m21 * vertex.normal.x
                    + inverse.m22 * vertex.normal.y
                    + inverse.m23 * vertex.normal.z,
                inverse.m31 * vertex.normal.x
                    + inverse.m32 * vertex.normal.y
                    + inverse.m33 * vertex.normal.z,
            );
            let normal_length_squared = normal.square_length();
            if ![
                position.x, position.y, position.z, normal.x, normal.y, normal.z,
            ]
            .iter()
            .all(|value| value.is_finite())
                || !normal_length_squared.is_finite()
                || normal_length_squared <= 0.0
            {
                return Err("invalid vertex position or normal".into());
            }
            let normal = normal.normalize();
            data.extend([
                [position.x, position.y, position.z, 1.0],
                [normal.x, normal.y, normal.z, 0.0],
                [
                    material.albedo[0],
                    material.albedo[1],
                    material.albedo[2],
                    0.0,
                ],
            ]);
        }
    }
    let vertices = u32::try_from(data.len() / 3).map_err(|_| "too many raster vertices")?;
    for light in &scene.lights {
        if !valid_color(&light.intensity)
            || ![light.position.x, light.position.y, light.position.z]
                .iter()
                .all(|value| value.is_finite())
        {
            return Err("invalid point light".into());
        }
        data.extend([
            [light.position.x, light.position.y, light.position.z, 1.0],
            [
                light.intensity[0],
                light.intensity[1],
                light.intensity[2],
                0.0,
            ],
        ]);
    }
    Ok((data, vertices))
}

/// 渲染快照并返回紧密排列的 BGRA 像素，调用方应在后台线程执行。
pub fn render_raster_snapshot(
    backend: &Backend,
    snapshot: &RenderSnapshot,
    width: u32,
    height: u32,
) -> Result<Vec<u8>, String> {
    let size = (width as usize)
        .checked_mul(height as usize)
        .and_then(|size| size.checked_mul(4))
        .ok_or("viewport size overflow")?;
    let rasterizer = backend
        .create_rasterizer()
        .map_err(|error| error.to_string())?;
    let mapper = backend
        .create_tone_mapper()
        .map_err(|error| error.to_string())?;
    let image = |format, usage, aspect| unsafe {
        backend
            .create_graph_image(width, height, format, usage, aspect)
            .map_err(|error| error.to_string())
    };
    let mut resources = GraphResources::default();
    let color = resources.add_temporary(image(
        vk::Format::R32G32B32A32_SFLOAT,
        vk::ImageUsageFlags::COLOR_ATTACHMENT | vk::ImageUsageFlags::SAMPLED,
        vk::ImageAspectFlags::COLOR,
    )?);
    let depth = resources.add_temporary(image(
        rasterizer.depth_format(),
        vk::ImageUsageFlags::DEPTH_STENCIL_ATTACHMENT,
        vk::ImageAspectFlags::DEPTH,
    )?);
    let mapped = resources.add_temporary(image(
        vk::Format::R8G8B8A8_UNORM,
        vk::ImageUsageFlags::COLOR_ATTACHMENT | vk::ImageUsageFlags::TRANSFER_SRC,
        vk::ImageAspectFlags::COLOR,
    )?);
    let viewport = unsafe {
        resources.add_external(
            image(
                vk::Format::R8G8B8A8_UNORM,
                vk::ImageUsageFlags::TRANSFER_DST | vk::ImageUsageFlags::TRANSFER_SRC,
                vk::ImageAspectFlags::COLOR,
            )?,
            None,
        )
    };
    let readback = backend
        .create_readback_buffer(size)
        .map_err(|error| error.to_string())?;

    let mut modules = ModuleGraph::default();
    let raster = modules.add(Module::Rasterization);
    let tone = modules.add(Module::ToneMapping {
        exposure_ev: snapshot.display.exposure_ev,
    });
    let output = modules.add(Module::ViewportOutput);
    modules.connect(raster, tone)?;
    modules.connect(tone, output)?;
    let mut graph = resources.graph()?;
    let raster_work = RasterWork::declare(&modules, raster, &mut graph, color, depth)?;
    let display = DisplayWork::declare(&modules, &mut graph, &[(raster, color)], mapped, viewport)?;
    let read = graph.add_work(
        "read viewport",
        vec![Access {
            resource: viewport,
            usage: Usage::TransferRead,
        }],
    )?;

    // 图负责附件布局、色调映射和读回之间的依赖；等待后才访问像素。
    unsafe {
        resources.submit(backend, &graph, |work, frame, images| {
            if work == read {
                frame
                    .copy_graph_image_to_readback(images[viewport.index()].image()?, &readback)
                    .map_err(|error| error.to_string())?;
            } else if !raster_work.record(
                work,
                frame,
                images,
                &rasterizer,
                snapshot,
                width,
                height,
            )? && !display.record(work, frame, images, &mapper)?
            {
                return Err("unknown raster graph work".into());
            }
            Ok(())
        })?;
        resources.wait()?;
        let mut pixels = readback.read().map_err(|error| error.to_string())?;
        for pixel in pixels.chunks_exact_mut(4) {
            pixel.swap(0, 2);
        }
        Ok(pixels)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use bee_scene::{
        Camera, DiffuseMaterial, Geometry, Instance, Point3, PointLight, Scene, SceneState,
        Transform3, Vertex,
    };
    use bee_vulkan::ValidationStatus;
    use std::sync::Arc;

    fn snapshot() -> RenderSnapshot {
        let normal = Vector3::new(0.0, 0.0, 1.0);
        let scene = Scene {
            geometries: vec![Geometry {
                vertices: vec![
                    Vertex {
                        position: Point3::new(-1.0, -1.0, 0.0),
                        normal,
                    },
                    Vertex {
                        position: Point3::new(1.0, -1.0, 0.0),
                        normal,
                    },
                    Vertex {
                        position: Point3::new(0.0, 1.0, 0.0),
                        normal,
                    },
                ],
                indices: vec![0, 1, 2],
            }],
            instances: vec![
                Instance {
                    geometry: 0,
                    material: 0,
                    transform: Transform3::translation(0.0, 0.0, 0.5),
                },
                Instance {
                    geometry: 0,
                    material: 1,
                    transform: Transform3::identity(),
                },
            ],
            materials: vec![
                DiffuseMaterial {
                    albedo: [1.0, 0.0, 0.0],
                },
                DiffuseMaterial {
                    albedo: [0.0, 0.0, 1.0],
                },
            ],
            lights: vec![],
            environment: [1.0; 3],
        };
        let mut snapshot = SceneState::default().snapshot();
        snapshot.scene = Arc::new(scene);
        snapshot.camera = Camera {
            target: Point3::origin(),
            yaw: 0.0,
            pitch: 0.0,
            distance: 3.0,
            ..Camera::default()
        };
        snapshot
    }

    #[test]
    fn validates_scene_and_transforms_normals() {
        let mut snapshot = snapshot();
        let scene = Arc::make_mut(&mut snapshot.scene);
        scene.instances.truncate(1);
        scene.instances[0].transform = Transform3::scale(2.0, 1.0, 1.0);
        for vertex in &mut scene.geometries[0].vertices {
            vertex.normal = Vector3::new(1.0, 1.0, 0.0);
        }
        let (data, vertices) = scene_data(&snapshot).unwrap();
        assert_eq!(vertices, 3);
        assert!((data[1][0] - 0.447_213_6).abs() < 0.00001);
        assert!((data[1][1] - 0.894_427_2).abs() < 0.00001);
        Arc::make_mut(&mut snapshot.scene).instances[0].transform =
            Transform3::scale(10_000.0, 10_000.0, 10_000.0);
        assert!(scene_data(&snapshot).is_ok());
        Arc::make_mut(&mut snapshot.scene).geometries[0].indices[0] = 99;
        assert!(scene_data(&snapshot).unwrap_err().contains("index"));
    }

    #[test]
    #[ignore = "requires a Vulkan graphics device and validation layer"]
    fn rasterizes_depth_and_direct_lighting() {
        if std::env::var_os("BEE_RASTER_GPU_CHILD").is_none() {
            let output = std::process::Command::new(std::env::current_exe().unwrap())
                .args([
                    "--exact",
                    "raster::tests::rasterizes_depth_and_direct_lighting",
                    "--ignored",
                    "--nocapture",
                ])
                .env("BEE_RASTER_GPU_CHILD", "1")
                .env("VK_LAYER_VALIDATE_SYNC", "1")
                .output()
                .unwrap();
            let stdout = String::from_utf8_lossy(&output.stdout);
            let stderr = String::from_utf8_lossy(&output.stderr);
            assert!(output.status.success(), "{stdout}\n{stderr}");
            assert!(
                !stderr
                    .lines()
                    .any(|line| line.starts_with("Vulkan validation ") && line.contains("ERROR")),
                "{stderr}"
            );
            return;
        }
        let backend = Backend::new(true).unwrap();
        assert_eq!(backend.report.validation, ValidationStatus::Enabled);
        let mut snapshot = snapshot();
        let pixel = |snapshot: &RenderSnapshot| {
            let image = render_raster_snapshot(&backend, snapshot, 64, 64).unwrap();
            let offset = (32 * 64 + 32) * 4;
            [
                image[offset],
                image[offset + 1],
                image[offset + 2],
                image[offset + 3],
            ]
        };
        let near = pixel(&snapshot);
        assert!(near[2] > 175 && near[0] < 5, "near surface: {near:?}");
        let expected =
            (crate::map_linear_color([1.0, 0.0, 0.0, 1.0], 0.0).unwrap()[0] * 255.0).round() as u8;
        assert!(
            near[2].abs_diff(expected) <= 2,
            "tone-mapped surface: {near:?}, expected {expected}"
        );
        let bounds = |image: &[u8], width: usize| {
            let mut xs = image
                .chunks_exact(4)
                .enumerate()
                .filter_map(|(i, p)| (p[2] > 175 && p[0] < 5).then_some(i % width));
            let first = xs.next().expect("visible red geometry");
            xs.fold((first, first), |(min, max), x| (min.min(x), max.max(x)))
        };
        let square = bounds(
            &render_raster_snapshot(&backend, &snapshot, 64, 64).unwrap(),
            64,
        );
        let wide = bounds(
            &render_raster_snapshot(&backend, &snapshot, 96, 64).unwrap(),
            96,
        );
        assert_eq!(
            (wide.0 - square.0, wide.1 - square.1),
            (16, 16),
            "resize must preserve the camera aspect ratio"
        );
        Arc::make_mut(&mut snapshot.scene).instances.reverse();
        assert_eq!(
            pixel(&snapshot),
            near,
            "depth must not depend on draw order"
        );
        Arc::make_mut(&mut snapshot.scene).instances.remove(1);
        let far = pixel(&snapshot);
        assert!(far[0] > 175 && far[2] < 5, "far surface: {far:?}");

        let scene = Arc::make_mut(&mut snapshot.scene);
        scene.materials[1].albedo = [1.0; 3];
        scene.environment = [0.0; 3];
        scene.lights.push(PointLight {
            position: Point3::new(0.0, 0.0, 2.0),
            intensity: [12.0; 3],
        });
        let lit = pixel(&snapshot);
        Arc::make_mut(&mut snapshot.scene).lights[0].position.z = -2.0;
        let unlit = pixel(&snapshot);
        assert!(
            lit[0] > unlit[0] + 100,
            "front light: {lit:?}, back light: {unlit:?}"
        );
        for vertex in &mut Arc::make_mut(&mut snapshot.scene).geometries[0].vertices {
            vertex.normal.z = -1.0;
        }
        let flipped = pixel(&snapshot);
        assert!(
            flipped[0] > unlit[0] + 100,
            "flipped normal: {flipped:?}, unlit: {unlit:?}"
        );
        Arc::make_mut(&mut snapshot.scene).lights.push(PointLight {
            position: Point3::new(0.0, 0.0, -2.0),
            intensity: [12.0; 3],
        });
        assert!(
            pixel(&snapshot)[0] > flipped[0],
            "all point lights must contribute"
        );

        // 验证共享相机、尺寸与色调映射确实改变完整渲染结果。
        let mut snapshot = SceneState::default().snapshot();
        let original = render_raster_snapshot(&backend, &snapshot, 128, 96).unwrap();
        assert_eq!(original.len(), 128 * 96 * 4);
        assert!(
            original
                .chunks_exact(4)
                .any(|p| p[2] > p[0].saturating_add(20)),
            "red geometry is missing"
        );
        assert!(
            original
                .chunks_exact(4)
                .any(|p| p[0] > p[2].saturating_add(20)),
            "blue geometry is missing"
        );
        snapshot.camera.orbit(120.0, 20.0);
        assert_ne!(
            render_raster_snapshot(&backend, &snapshot, 128, 96).unwrap(),
            original
        );
        snapshot.display.exposure_ev = 1.0;
        let brighter = render_raster_snapshot(&backend, &snapshot, 128, 96).unwrap();
        assert!(
            brighter[0] > original[0],
            "tone mapping must apply exposure"
        );
        let resized = render_raster_snapshot(&backend, &snapshot, 96, 64).unwrap();
        assert_eq!(resized.len(), 96 * 64 * 4);
    }
}
