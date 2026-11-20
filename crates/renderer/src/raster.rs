use crate::scene::RasterScene;
use crate::{
    Access, GraphResource, Module, ModuleGraph, NodeId, ResourceId, Usage, WorkGraph, WorkId,
};
use bee_scene::RenderSnapshot;
use bee_vulkan::{Backend, Frame, RasterPass, Rasterizer};

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
        Self::declare_with_inputs(modules, node, graph, color, depth, [])
    }

    pub(crate) fn declare_with_inputs(
        modules: &ModuleGraph,
        node: NodeId,
        graph: &mut WorkGraph,
        color: ResourceId,
        depth: ResourceId,
        inputs: impl IntoIterator<Item = Access>,
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
            [
                Access {
                    resource: color,
                    usage: Usage::ColorAttachment,
                },
                Access {
                    resource: depth,
                    usage: Usage::DepthAttachment,
                },
            ]
            .into_iter()
            .chain(inputs)
            .collect(),
        )?;
        Ok(Self { color, depth, work })
    }

    pub(crate) fn prepare_draws(
        &self,
        backend: &Backend,
        resources: &[GraphResource],
        rasterizer: &Rasterizer,
        scene: &RasterScene,
        view_projection: [f32; 16],
    ) -> Result<(WorkId, RasterPass), String> {
        let pass = backend
            .prepare_raster(
                rasterizer,
                resources[self.color.0].image()?,
                resources[self.depth.0].image()?,
                &scene.meshes,
                &scene.instances,
                &scene.materials,
                &scene.lights,
                view_projection,
                scene.source.environment,
            )
            .map_err(|error| error.to_string())?;
        Ok((self.work, pass))
    }

    /// # Safety
    /// 工作图须已建立附件屏障，并保证图像用途和设备有效。
    pub(crate) unsafe fn record(
        &self,
        work: WorkId,
        frame: &mut Frame,
        images: &[GraphResource],
        rasterizer: &Rasterizer,
        scene: &RasterScene,
        view_projection: [f32; 16],
    ) -> Result<bool, String> {
        if work != self.work {
            return Ok(false);
        }
        if images.len() <= self.color.0.max(self.depth.0) {
            return Err("raster work is missing attachments".into());
        }
        frame
            .rasterize(
                rasterizer,
                images[self.color.0].image()?,
                images[self.depth.0].image()?,
                &scene.meshes,
                &scene.instances,
                &scene.materials,
                &scene.lights,
                view_projection,
                scene.source.environment,
            )
            .map_err(|error| error.to_string())?;
        Ok(true)
    }
}

/// 渲染快照并返回紧密排列的 BGRA 像素，调用方应在后台线程执行。
pub fn render_raster_snapshot(
    backend: &Backend,
    snapshot: &RenderSnapshot,
    width: u32,
    height: u32,
) -> Result<Vec<u8>, String> {
    let mut modules = ModuleGraph::default();
    let raster = modules.add(Module::Rasterization);
    let tone = modules.add(Module::ToneMapping {
        exposure_ev: snapshot.display.exposure_ev,
    });
    let output = modules.add(Module::ViewportOutput);
    modules.connect(raster, tone)?;
    modules.connect(tone, output)?;
    render_raster_graph(backend, snapshot, width, height, &modules)
}

/// 使用已验证的算法图渲染光栅化流程。
pub fn render_raster_graph(
    backend: &Backend,
    snapshot: &RenderSnapshot,
    width: u32,
    height: u32,
    modules: &ModuleGraph,
) -> Result<Vec<u8>, String> {
    let flow = crate::PreparedFlow::prepare(modules, None)?;
    if flow.source() != Module::Rasterization {
        return Err("viewport source is not a rasterization module".into());
    }
    crate::ViewportRenderer::default()
        .submit(
            backend,
            &flow,
            snapshot,
            (width, height),
            0,
            1,
            &mut crate::FrameSlot::default(),
        )?
        .finish()
        .1
}

#[cfg(test)]
mod tests {
    use super::*;
    use bee_scene::{
        Camera, DiffuseMaterial, Geometry, Instance, Point3, PointLight, Scene, SceneState,
        Transform3, Vector3, Vertex,
    };
    use bee_vulkan::ValidationStatus;
    use std::sync::Arc;

    #[test]
    #[ignore = "requires a ray tracing device and Vulkan validation layer"]
    fn builds_builtin_acceleration_scene() {
        if std::env::var_os("BEE_ACCEL_GPU_CHILD").is_none() {
            let output = std::process::Command::new(std::env::current_exe().unwrap())
                .args([
                    "--exact",
                    "raster::tests::builds_builtin_acceleration_scene",
                    "--ignored",
                    "--nocapture",
                ])
                .env("BEE_ACCEL_GPU_CHILD", "1")
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
        assert!(
            backend.report.ray_tracing_missing.is_empty(),
            "{:?}",
            backend.report
        );
        let snapshot = SceneState::default().snapshot();
        let scene = crate::scene::PathScene::path(&backend, &snapshot.scene, None).unwrap();
        assert_eq!(scene.instances.triangle_count(), 26);
    }

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

        let mut shared = self::snapshot();
        let scene = Arc::make_mut(&mut shared.scene);
        scene.instances[0].transform =
            Transform3::scale(0.55, 1.0, 1.0).then_translate(Vector3::new(-0.8, 0.0, 0.0));
        scene.instances[1].transform =
            Transform3::scale(0.4, 1.0, 1.0).then_translate(Vector3::new(0.8, 0.0, 0.0));
        scene.instances[1].transform.m21 = 0.2;
        scene.environment = [0.0; 3];
        scene.lights.push(PointLight {
            position: Point3::new(1.5, 0.0, 2.0),
            intensity: [12.0; 3],
        });
        for vertex in &mut scene.geometries[0].vertices {
            vertex.normal = Vector3::new(1.0, 0.0, 1.0);
        }
        let transformed = render_raster_snapshot(&backend, &shared, 64, 64).unwrap();
        assert!(transformed.chunks_exact(4).any(|p| p[2] > 100));
        assert!(transformed.chunks_exact(4).any(|p| p[0] > 100));
        // 预变换的参考网格同时覆盖实例位置和非均匀缩放的逆转置法线。
        let scene = Arc::make_mut(&mut shared.scene);
        let object_mesh = scene.geometries.pop().unwrap();
        for (index, instance) in scene.instances.iter_mut().enumerate() {
            let mut mesh = object_mesh.clone();
            for vertex in &mut mesh.vertices {
                vertex.position = instance
                    .transform
                    .transform_point3d(vertex.position)
                    .unwrap();
                vertex.normal.x /= instance.transform.m11;
                vertex.normal.y -= instance.transform.m21 * vertex.normal.x;
            }
            scene.geometries.push(mesh);
            instance.geometry = index;
            instance.transform = Transform3::identity();
        }
        let reference = render_raster_snapshot(&backend, &shared, 64, 64).unwrap();
        assert!(transformed
            .iter()
            .zip(reference)
            .all(|(&a, b)| a.abs_diff(b) <= 1));

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
