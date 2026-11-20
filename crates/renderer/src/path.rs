use crate::{FrameSlot, Module, ModuleGraph, PreparedFlow, ViewportRenderer};
use bee_scene::RenderSnapshot;
use bee_vulkan::Backend;

pub struct PathRenderer {
    renderer: ViewportRenderer,
    flow: Option<PreparedFlow>,
    slot: FrameSlot,
    samples: u32,
}

impl PathRenderer {
    pub fn new(backend: &Backend) -> Result<Self, String> {
        Ok(Self {
            renderer: ViewportRenderer::path(backend)?,
            flow: None,
            slot: FrameSlot::default(),
            samples: 0,
        })
    }

    pub fn sample_count(&self) -> u32 {
        self.samples
    }

    /// 同步等待 GPU 完成；界面应在后台线程调用。
    pub fn render(
        &mut self,
        backend: &Backend,
        snapshot: &RenderSnapshot,
        width: u32,
        height: u32,
        seed: u32,
    ) -> Result<Vec<u8>, String> {
        let mut modules = ModuleGraph::default();
        let path = modules.add(Module::PathTracing);
        let tone = modules.add(Module::ToneMapping {
            exposure_ev: snapshot.display.exposure_ev,
        });
        let output = modules.add(Module::ViewportOutput);
        modules.connect(path, tone)?;
        modules.connect(tone, output)?;
        self.render_graph(backend, snapshot, width, height, seed, &modules)
    }

    /// 使用已验证的算法图追踪并累积当前场景。
    pub fn render_graph(
        &mut self,
        backend: &Backend,
        snapshot: &RenderSnapshot,
        width: u32,
        height: u32,
        seed: u32,
        modules: &ModuleGraph,
    ) -> Result<Vec<u8>, String> {
        let flow = PreparedFlow::prepare(modules, self.flow.as_ref())?;
        if flow.source() != Module::PathTracing {
            return Err("viewport source is not a path tracing module".into());
        }
        let pending = self.renderer.submit(
            backend,
            &flow,
            snapshot,
            (width, height),
            0,
            seed,
            &mut self.slot,
        )?;
        self.samples = pending.samples;
        let (slot, pixels) = pending.finish();
        self.slot = slot;
        self.flow = Some(flow);
        pixels
    }
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
    fn traces_builtin_scene() {
        if std::env::var_os("BEE_PATH_GPU_CHILD").is_none() {
            let output = std::process::Command::new(std::env::current_exe().unwrap())
                .args([
                    "--exact",
                    "path::tests::traces_builtin_scene",
                    "--ignored",
                    "--nocapture",
                ])
                .env("BEE_PATH_GPU_CHILD", "1")
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
        let mut renderer = PathRenderer::new(&backend).unwrap();
        let image = renderer
            .render(&backend, &SceneState::default().snapshot(), 64, 64, 1)
            .unwrap();
        assert_eq!(renderer.sample_count(), 1);
        assert!(
            image
                .chunks_exact(4)
                .any(|p| p[2] > p[0].saturating_add(20)),
            "red geometry missing"
        );
        assert!(
            image
                .chunks_exact(4)
                .any(|p| p[0] > p[2].saturating_add(20)),
            "blue geometry missing"
        );

        let (mut snapshot, wall) = test_scene();
        snapshot.scene = Arc::new(Scene {
            lights: vec![PointLight {
                position: Point3::new(2.0, 0.0, 2.0),
                intensity: [16.0; 3],
            }],
            environment: [0.0; 3],
            ..(*snapshot.scene).clone()
        });
        let lit = renderer.render(&backend, &snapshot, 1, 1, 1).unwrap();
        let linear = 16.0 * (2.0_f32 / 8.0_f32.sqrt()) / (std::f32::consts::PI * 8.0);
        let expected = (crate::map_linear_color([linear, linear, linear, 1.0], 0.0).unwrap()[0]
            * 255.0)
            .round() as u8;
        assert!(
            lit[0].abs_diff(expected) <= 2,
            "direct light {lit:?}, expected {expected}"
        );
        Arc::make_mut(&mut snapshot.scene).geometries[0].vertices[2]
            .normal
            .z = -1.0;
        assert_eq!(
            renderer.render(&backend, &snapshot, 1, 1, 1).unwrap(),
            lit,
            "opposing interpolated normals must fall back to the geometric normal"
        );
        Arc::make_mut(&mut snapshot.scene).geometries[0].vertices[2]
            .normal
            .z = 1.0;
        snapshot.render.max_path_bounces = 4;
        assert_eq!(
            renderer.render(&backend, &snapshot, 1, 1, 1).unwrap(),
            lit,
            "extra diffuse bounces must not count the same point light twice"
        );
        let scene = Arc::make_mut(&mut snapshot.scene);
        scene.geometries.push(wall.clone());
        scene.instances.push(Instance {
            geometry: 1,
            material: 0,
            transform: Transform3::translation(2.0, 0.0, 0.0),
        });
        let blocked = renderer.render(&backend, &snapshot, 1, 1, 1).unwrap();
        assert!(
            blocked[0] < 5 && lit[0] > 100,
            "unblocked {lit:?}, blocked {blocked:?}"
        );

        let (mut indirect, wall) = test_scene();
        let scene = Arc::make_mut(&mut indirect.scene);
        scene.geometries.push(wall);
        scene.instances.push(Instance {
            geometry: 1,
            material: 0,
            transform: Transform3::identity(),
        });
        indirect.render.max_path_bounces = 2;
        let two = renderer.render(&backend, &indirect, 1, 1, 1).unwrap();
        indirect.render.max_path_bounces = 3;
        let three = renderer.render(&backend, &indirect, 1, 1, 1).unwrap();
        assert!(
            two[0] < 5 && three[0] > 150,
            "two interactions {two:?}, indirect contribution {three:?}"
        );

        indirect.render.max_path_bounces = 2;
        let first = renderer.render(&backend, &indirect, 1, 1, 1).unwrap();
        let second = renderer.render(&backend, &indirect, 1, 1, 1).unwrap();
        let expected =
            (crate::map_linear_color([0.5, 0.5, 0.5, 1.0], 0.0).unwrap()[0] * 255.0).round() as u8;
        assert!(
            first[0] < 5 && second[0].abs_diff(expected) <= 2,
            "samples {first:?}, {second:?}, expected {expected}"
        );
        assert_eq!(renderer.sample_count(), 2);
        indirect.display.exposure_ev = 1.0;
        let display_change = renderer.render(&backend, &indirect, 1, 1, 1).unwrap();
        let expected =
            (crate::map_linear_color([1.0 / 3.0; 4], 1.0).unwrap()[0] * 255.0).round() as u8;
        assert_eq!(renderer.sample_count(), 3);
        assert!(
            display_change[0].abs_diff(expected) <= 2,
            "display change lost history: {display_change:?}, expected {expected}"
        );

        indirect.camera.orbit(15.0, 0.0);
        let moved = renderer.render(&backend, &indirect, 1, 1, 1).unwrap();
        assert_eq!(renderer.sample_count(), 1);
        let mut fresh = PathRenderer::new(&backend).unwrap();
        assert_eq!(moved, fresh.render(&backend, &indirect, 1, 1, 1).unwrap());
        renderer.render(&backend, &indirect, 1, 1, 1).unwrap();
        Arc::make_mut(&mut indirect.scene).materials[0].albedo = [0.5; 3];
        let changed_scene = renderer.render(&backend, &indirect, 1, 1, 1).unwrap();
        assert_eq!(renderer.sample_count(), 1);
        assert_eq!(
            changed_scene,
            fresh.render(&backend, &indirect, 1, 1, 1).unwrap()
        );
        renderer.render(&backend, &indirect, 1, 1, 1).unwrap();
        indirect.render.max_path_bounces = 3;
        let changed_parameters = renderer.render(&backend, &indirect, 1, 1, 1).unwrap();
        assert_eq!(renderer.sample_count(), 1);
        assert_eq!(
            changed_parameters,
            fresh.render(&backend, &indirect, 1, 1, 1).unwrap()
        );
        renderer.render(&backend, &indirect, 1, 1, 1).unwrap();
        let resized = renderer.render(&backend, &indirect, 3, 2, 1).unwrap();
        assert_eq!(renderer.sample_count(), 1);
        assert_eq!(resized, fresh.render(&backend, &indirect, 3, 2, 1).unwrap());
        Arc::make_mut(&mut indirect.scene).instances.clear();
        let empty = renderer.render(&backend, &indirect, 1, 1, 1).unwrap();
        let expected = (crate::map_linear_color([1.0; 4], 1.0).unwrap()[0] * 255.0).round() as u8;
        assert!(
            empty[0].abs_diff(expected) <= 2,
            "empty scene should show the environment: {empty:?}"
        );
    }

    fn test_scene() -> (RenderSnapshot, Geometry) {
        let triangle = Geometry {
            vertices: vec![
                Vertex {
                    position: Point3::new(-2.0, -2.0, 0.0),
                    normal: Vector3::new(0.0, 0.0, 1.0),
                },
                Vertex {
                    position: Point3::new(2.0, -2.0, 0.0),
                    normal: Vector3::new(0.0, 0.0, 1.0),
                },
                Vertex {
                    position: Point3::new(0.0, 2.0, 0.0),
                    normal: Vector3::new(0.0, 0.0, 1.0),
                },
            ],
            indices: vec![0, 1, 2],
        };
        let wall = Geometry {
            vertices: vec![
                Vertex {
                    position: Point3::new(-1.0, -10.0, 0.0),
                    normal: Vector3::new(1.0, 0.0, 0.0),
                },
                Vertex {
                    position: Point3::new(-1.0, 10.0, 0.0),
                    normal: Vector3::new(1.0, 0.0, 0.0),
                },
                Vertex {
                    position: Point3::new(-1.0, 10.0, 10.0),
                    normal: Vector3::new(1.0, 0.0, 0.0),
                },
                Vertex {
                    position: Point3::new(-1.0, -10.0, 10.0),
                    normal: Vector3::new(1.0, 0.0, 0.0),
                },
            ],
            indices: vec![0, 1, 2, 0, 2, 3],
        };
        let scene = Scene {
            geometries: vec![triangle],
            instances: vec![Instance {
                geometry: 0,
                material: 0,
                transform: Transform3::identity(),
            }],
            materials: vec![DiffuseMaterial { albedo: [1.0; 3] }],
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
        (snapshot, wall)
    }
}
