use crate::{Access, DisplayWork, GraphResources, Module, ModuleGraph, ResourceId, Usage};
use ash::vk;
use bee_scene::{RenderSnapshot, Scene};
use bee_vulkan::{AccelerationScene, Backend, GraphImage, PathTracer, ReadbackBuffer};
use std::sync::Arc;

pub struct PathRenderer {
    tracer: PathTracer,
    mapper: bee_vulkan::ToneMapper,
    scene: Option<(Arc<Scene>, u64, AccelerationScene)>,
    history: Option<History>,
    previous: Option<RenderSnapshot>,
    samples: u32,
}

struct History {
    resources: GraphResources,
    color: ResourceId,
    mapped: ResourceId,
    viewport: ResourceId,
    mapped_image: GraphImage,
    readback: ReadbackBuffer,
    width: u32,
    height: u32,
}

impl PathRenderer {
    pub fn new(backend: &Backend) -> Result<Self, String> {
        Ok(Self {
            tracer: backend
                .create_path_tracer()
                .map_err(|error| error.to_string())?,
            mapper: backend
                .create_tone_mapper()
                .map_err(|error| error.to_string())?,
            scene: None,
            history: None,
            previous: None,
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
        if width == 0 || height == 0 {
            return Err("invalid path tracing extent".into());
        }
        let scene_changed = self.scene.as_ref().is_none_or(|(scene, version, _)| {
            !Arc::ptr_eq(scene, &snapshot.scene) || *version != snapshot.versions.scene
        });
        if scene_changed {
            let (data, vertices) = crate::raster::scene_data(snapshot)?;
            let scene = backend
                .build_acceleration_scene(&data, vertices)
                .map_err(|error| error.to_string())?;
            self.scene = Some((snapshot.scene.clone(), snapshot.versions.scene, scene));
        }
        let changed = scene_changed
            || self.previous.as_ref().is_none_or(|previous| {
                previous.versions.accumulation_key() != snapshot.versions.accumulation_key()
                    || previous.camera != snapshot.camera
                    || previous.render != snapshot.render
            })
            || self
                .history
                .as_ref()
                .is_none_or(|history| history.width != width || history.height != height);
        if self
            .history
            .as_ref()
            .is_none_or(|history| history.width != width || history.height != height)
        {
            self.history = Some(History::new(backend, width, height)?);
        }
        if changed || self.samples == u32::MAX {
            self.samples = 0;
        }
        let matrix = snapshot
            .camera
            .view_projection(width as f32 / height as f32)
            .and_then(|matrix| matrix.inverse())
            .ok_or("invalid path tracing camera")?
            .to_array();
        let history = self.history.as_mut().unwrap();
        history
            .resources
            .set_temporary(history.mapped, history.mapped_image.clone())?;
        let mut modules = ModuleGraph::default();
        let path = modules.add(Module::PathTracing);
        let tone = modules.add(Module::ToneMapping {
            exposure_ev: snapshot.display.exposure_ev,
        });
        let output = modules.add(Module::ViewportOutput);
        modules.connect(path, tone)?;
        modules.connect(tone, output)?;
        let mut graph = history.resources.graph()?;
        let trace = graph.add_work(
            "path tracing",
            vec![Access {
                resource: history.color,
                usage: if self.samples == 0 {
                    Usage::ShaderWrite
                } else {
                    Usage::ShaderReadWrite
                },
            }],
        )?;
        let display = DisplayWork::declare(
            &modules,
            &mut graph,
            &[(path, history.color)],
            history.mapped,
            history.viewport,
        )?;
        let read = graph.add_work(
            "read viewport",
            vec![Access {
                resource: history.viewport,
                usage: Usage::TransferRead,
            }],
        )?;
        let scene = &self.scene.as_ref().unwrap().2;
        unsafe {
            history
                .resources
                .submit(backend, &graph, |work, frame, images| {
                    if work == trace {
                        frame
                            .trace_paths(
                                &self.tracer,
                                scene,
                                images[history.color.index()].image()?,
                                matrix,
                                snapshot.scene.environment,
                                self.samples,
                                snapshot.render.max_path_bounces,
                                seed,
                            )
                            .map_err(|error| error.to_string())?;
                    } else if work == read {
                        frame
                            .copy_graph_image_to_readback(
                                images[history.viewport.index()].image()?,
                                &history.readback,
                            )
                            .map_err(|error| error.to_string())?;
                    } else if !display.record(work, frame, images, &self.mapper)? {
                        return Err("unknown path tracing work".into());
                    }
                    Ok(())
                })?;
            history.resources.wait()?;
            self.samples += 1;
            self.previous = Some(snapshot.clone());
            let mut pixels = history.readback.read().map_err(|error| error.to_string())?;
            for pixel in pixels.chunks_exact_mut(4) {
                pixel.swap(0, 2);
            }
            Ok(pixels)
        }
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

impl History {
    fn new(backend: &Backend, width: u32, height: u32) -> Result<Self, String> {
        let size = (width as usize)
            .checked_mul(height as usize)
            .and_then(|size| size.checked_mul(4))
            .ok_or("viewport size overflow")?;
        let image = |format, usage| unsafe {
            backend
                .create_graph_image(width, height, format, usage, vk::ImageAspectFlags::COLOR)
                .map_err(|error| error.to_string())
        };
        let mut resources = GraphResources::default();
        let color = resources.add_history(image(
            vk::Format::R32G32B32A32_SFLOAT,
            vk::ImageUsageFlags::STORAGE | vk::ImageUsageFlags::SAMPLED,
        )?);
        let mapped_image = image(
            vk::Format::R8G8B8A8_UNORM,
            vk::ImageUsageFlags::COLOR_ATTACHMENT | vk::ImageUsageFlags::TRANSFER_SRC,
        )?;
        let mapped = resources.add_temporary(mapped_image.clone());
        let viewport = unsafe {
            resources.add_external(
                image(
                    vk::Format::R8G8B8A8_UNORM,
                    vk::ImageUsageFlags::TRANSFER_DST | vk::ImageUsageFlags::TRANSFER_SRC,
                )?,
                None,
            )
        };
        let readback = backend
            .create_readback_buffer(size)
            .map_err(|error| error.to_string())?;
        Ok(Self {
            resources,
            color,
            mapped,
            viewport,
            mapped_image,
            readback,
            width,
            height,
        })
    }
}
