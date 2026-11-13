//! 界面和渲染路径共享的场景数据。

use euclid::default::{Point3D, Transform3D, Vector3D};
use std::sync::Arc;

pub type Point3 = Point3D<f32>;
pub type Vector3 = Vector3D<f32>;
pub type Transform3 = Transform3D<f32>;

#[derive(Clone, Debug, PartialEq)]
pub struct Vertex {
    pub position: Point3,
    pub normal: Vector3,
}

#[derive(Clone, Debug, PartialEq)]
pub struct Geometry {
    pub vertices: Vec<Vertex>,
    pub indices: Vec<u32>,
}

#[derive(Clone, Debug, PartialEq)]
pub struct Instance {
    pub geometry: usize,
    pub material: usize,
    pub transform: Transform3,
}

#[derive(Clone, Debug, PartialEq)]
pub struct DiffuseMaterial {
    pub albedo: [f32; 3],
}

#[derive(Clone, Debug, PartialEq)]
pub struct PointLight {
    pub position: Point3,
    pub intensity: [f32; 3],
}

#[derive(Clone, Debug, PartialEq)]
pub struct Scene {
    pub geometries: Vec<Geometry>,
    pub instances: Vec<Instance>,
    pub materials: Vec<DiffuseMaterial>,
    pub lights: Vec<PointLight>,
    pub environment: [f32; 3],
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct RenderParameters {
    pub max_path_bounces: u32,
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct DisplayParameters {
    pub exposure_ev: f32,
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Versions {
    pub scene: u64,
    pub camera: u64,
    pub render: u64,
    pub display: u64,
}

impl Versions {
    pub fn accumulation_key(&self) -> (u64, u64, u64) {
        (self.scene, self.camera, self.render)
    }
}

#[derive(Clone, Debug)]
pub struct RenderSnapshot {
    pub scene: Arc<Scene>,
    pub camera: Camera,
    pub render: RenderParameters,
    pub display: DisplayParameters,
    pub versions: Versions,
}

pub struct SceneState {
    scene: Arc<Scene>,
    camera: Camera,
    render: RenderParameters,
    display: DisplayParameters,
    versions: Versions,
}

impl Default for SceneState {
    fn default() -> Self {
        Self {
            scene: Arc::new(Scene::builtin()),
            camera: Camera::default(),
            render: RenderParameters {
                max_path_bounces: 2,
            },
            display: DisplayParameters { exposure_ev: 0.0 },
            versions: Versions::default(),
        }
    }
}

impl SceneState {
    pub fn scene(&self) -> &Scene {
        &self.scene
    }

    pub fn camera(&self) -> Camera {
        self.camera
    }

    pub fn versions(&self) -> Versions {
        self.versions
    }

    pub fn snapshot(&self) -> RenderSnapshot {
        RenderSnapshot {
            scene: self.scene.clone(),
            camera: self.camera,
            render: self.render,
            display: self.display,
            versions: self.versions,
        }
    }

    pub fn set_scene(&mut self, scene: Scene) -> bool {
        if self.scene.as_ref() == &scene {
            return false;
        }
        self.scene = Arc::new(scene);
        increment(&mut self.versions.scene);
        true
    }

    pub fn set_camera(&mut self, camera: Camera) -> bool {
        if self.camera == camera
            || camera.view_transform().is_none()
            || camera.projection_transform(1.0).is_none()
        {
            return false;
        }
        self.camera = camera;
        increment(&mut self.versions.camera);
        true
    }

    pub fn set_render_parameters(&mut self, render: RenderParameters) -> bool {
        if self.render == render || render.max_path_bounces < 2 {
            return false;
        }
        self.render = render;
        increment(&mut self.versions.render);
        true
    }

    pub fn set_display_parameters(&mut self, display: DisplayParameters) -> bool {
        if self.display == display || !display.exposure_ev.is_finite() {
            return false;
        }
        self.display = display;
        increment(&mut self.versions.display);
        true
    }
}

fn increment(version: &mut u64) {
    *version = version.checked_add(1).expect("scene version overflow");
}

/// 世界坐标采用右手系，+Y 向上；相机默认从 +Z 方向看向场景。
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Camera {
    pub target: Point3,
    pub yaw: f32,
    pub pitch: f32,
    pub distance: f32,
    pub vertical_fov: f32,
    pub near: f32,
    pub far: f32,
}

impl Default for Camera {
    fn default() -> Self {
        Self {
            target: Point3::new(0.0, 0.5, 0.0),
            yaw: 0.0,
            pitch: 0.2,
            distance: 6.0,
            vertical_fov: 60.0_f32.to_radians(),
            near: 0.1,
            far: 100.0,
        }
    }
}

impl Camera {
    pub fn position(&self) -> Point3 {
        let horizontal = self.distance * self.pitch.cos();
        self.target
            + Vector3::new(
                horizontal * self.yaw.sin(),
                self.distance * self.pitch.sin(),
                horizontal * self.yaw.cos(),
            )
    }

    pub fn orbit(&mut self, horizontal_pixels: f32, vertical_pixels: f32) -> bool {
        if !horizontal_pixels.is_finite()
            || !vertical_pixels.is_finite()
            || !self.yaw.is_finite()
            || !self.pitch.is_finite()
        {
            return false;
        }
        let yaw = (self.yaw - horizontal_pixels * 0.005).rem_euclid(std::f32::consts::TAU);
        let limit = std::f32::consts::FRAC_PI_2 - 0.01;
        let pitch = (self.pitch + vertical_pixels * 0.005).clamp(-limit, limit);
        if yaw == self.yaw && pitch == self.pitch {
            return false;
        }
        self.yaw = yaw;
        self.pitch = pitch;
        true
    }

    pub fn zoom(&mut self, scroll_pixels: f32) -> bool {
        if !scroll_pixels.is_finite() || !self.distance.is_finite() || self.distance <= 0.0 {
            return false;
        }
        let distance = (self.distance * (-scroll_pixels * 0.005).exp()).clamp(0.2, 100.0);
        if distance == self.distance {
            return false;
        }
        self.distance = distance;
        true
    }

    pub fn view_transform(&self) -> Option<Transform3> {
        let eye = self.position();
        let direction = self.target - eye;
        if !self.distance.is_finite()
            || self.distance <= 0.0
            || ![eye.x, eye.y, eye.z, direction.x, direction.y, direction.z]
                .iter()
                .all(|value| value.is_finite())
            || direction.square_length() <= f32::EPSILON
        {
            return None;
        }
        let forward = direction.normalize();
        let right = forward.cross(Vector3::new(0.0, 1.0, 0.0));
        if right.square_length() <= f32::EPSILON {
            return None;
        }
        let right = right.normalize();
        let up = right.cross(forward);
        let offset = eye.to_vector();

        Some(Transform3::new(
            right.x,
            up.x,
            -forward.x,
            0.0,
            right.y,
            up.y,
            -forward.y,
            0.0,
            right.z,
            up.z,
            -forward.z,
            0.0,
            -right.dot(offset),
            -up.dot(offset),
            forward.dot(offset),
            1.0,
        ))
    }

    pub fn projection_transform(&self, aspect: f32) -> Option<Transform3> {
        if !aspect.is_finite()
            || aspect <= 0.0
            || !self.vertical_fov.is_finite()
            || !(0.0..std::f32::consts::PI).contains(&self.vertical_fov)
            || self.vertical_fov == 0.0
            || !self.near.is_finite()
            || !self.far.is_finite()
            || self.near <= 0.0
            || self.far <= self.near
        {
            return None;
        }

        // Vulkan 的深度范围为 0 到 1，Y 轴翻转后屏幕上方对应世界上方。
        let scale = 1.0 / (self.vertical_fov * 0.5).tan();
        let depth = self.far / (self.near - self.far);
        if ![scale, scale / aspect, depth, self.near * depth]
            .iter()
            .all(|value| value.is_finite())
        {
            return None;
        }
        Some(Transform3::new(
            scale / aspect,
            0.0,
            0.0,
            0.0,
            0.0,
            -scale,
            0.0,
            0.0,
            0.0,
            0.0,
            depth,
            -1.0,
            0.0,
            0.0,
            self.near * depth,
            0.0,
        ))
    }

    pub fn view_projection(&self, aspect: f32) -> Option<Transform3> {
        Some(
            self.view_transform()?
                .then(&self.projection_transform(aspect)?),
        )
    }
}

impl Scene {
    pub fn builtin() -> Self {
        let mut ground = Geometry {
            vertices: Vec::new(),
            indices: Vec::new(),
        };
        add_face(
            &mut ground,
            Vector3::new(0.0, 1.0, 0.0),
            [
                [-4.0, 0.0, -4.0],
                [-4.0, 0.0, 4.0],
                [4.0, 0.0, 4.0],
                [4.0, 0.0, -4.0],
            ],
        );

        let mut cube = Geometry {
            vertices: Vec::new(),
            indices: Vec::new(),
        };
        for (normal, corners) in [
            (
                [1.0, 0.0, 0.0],
                [
                    [0.5, -0.5, -0.5],
                    [0.5, 0.5, -0.5],
                    [0.5, 0.5, 0.5],
                    [0.5, -0.5, 0.5],
                ],
            ),
            (
                [-1.0, 0.0, 0.0],
                [
                    [-0.5, -0.5, 0.5],
                    [-0.5, 0.5, 0.5],
                    [-0.5, 0.5, -0.5],
                    [-0.5, -0.5, -0.5],
                ],
            ),
            (
                [0.0, 1.0, 0.0],
                [
                    [-0.5, 0.5, -0.5],
                    [-0.5, 0.5, 0.5],
                    [0.5, 0.5, 0.5],
                    [0.5, 0.5, -0.5],
                ],
            ),
            (
                [0.0, -1.0, 0.0],
                [
                    [-0.5, -0.5, 0.5],
                    [-0.5, -0.5, -0.5],
                    [0.5, -0.5, -0.5],
                    [0.5, -0.5, 0.5],
                ],
            ),
            (
                [0.0, 0.0, 1.0],
                [
                    [0.5, -0.5, 0.5],
                    [0.5, 0.5, 0.5],
                    [-0.5, 0.5, 0.5],
                    [-0.5, -0.5, 0.5],
                ],
            ),
            (
                [0.0, 0.0, -1.0],
                [
                    [-0.5, -0.5, -0.5],
                    [-0.5, 0.5, -0.5],
                    [0.5, 0.5, -0.5],
                    [0.5, -0.5, -0.5],
                ],
            ),
        ] {
            add_face(
                &mut cube,
                Vector3::new(normal[0], normal[1], normal[2]),
                corners,
            );
        }

        Self {
            geometries: vec![ground, cube],
            instances: vec![
                Instance {
                    geometry: 0,
                    material: 0,
                    transform: Transform3::identity(),
                },
                Instance {
                    geometry: 1,
                    material: 1,
                    transform: Transform3::translation(-0.7, 0.5, 0.0),
                },
                Instance {
                    geometry: 1,
                    material: 2,
                    transform: Transform3::translation(0.8, 0.5, -0.8),
                },
            ],
            materials: vec![
                DiffuseMaterial {
                    albedo: [0.65, 0.65, 0.65],
                },
                DiffuseMaterial {
                    albedo: [0.75, 0.15, 0.1],
                },
                DiffuseMaterial {
                    albedo: [0.1, 0.3, 0.75],
                },
            ],
            lights: vec![PointLight {
                position: Point3::new(2.0, 4.0, 3.0),
                intensity: [45.0, 42.0, 38.0],
            }],
            environment: [0.03, 0.03, 0.03],
        }
    }
}

fn add_face(geometry: &mut Geometry, normal: Vector3, corners: [[f32; 3]; 4]) {
    let base = geometry.vertices.len() as u32;
    geometry.vertices.extend(corners.map(|[x, y, z]| Vertex {
        position: Point3::new(x, y, z),
        normal,
    }));
    geometry
        .indices
        .extend([base, base + 1, base + 2, base, base + 2, base + 3]);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn builtin_scene_has_consistent_geometry_and_coordinates() {
        let scene = Scene::builtin();
        assert_eq!(scene.geometries.len(), 2);
        assert_eq!(scene.instances.len(), 3);
        assert_eq!(scene.instances[1].geometry, scene.instances[2].geometry);
        assert!(!scene.lights.is_empty());
        assert!(scene.lights.iter().all(|light| light.position.y > 0.0));
        assert!(scene
            .lights
            .iter()
            .flat_map(|light| light.intensity)
            .all(|channel| channel.is_finite() && channel > 0.0));
        assert!(scene
            .materials
            .iter()
            .flat_map(|material| material.albedo)
            .all(|channel| (0.0..=1.0).contains(&channel)));

        for instance in &scene.instances {
            assert!(instance.geometry < scene.geometries.len());
            assert!(instance.material < scene.materials.len());
        }
        for instance in &scene.instances[1..] {
            let bottom = instance
                .transform
                .transform_point3d(Point3::new(0.0, -0.5, 0.0))
                .unwrap();
            assert_eq!(bottom.y, 0.0);
        }
        for geometry in &scene.geometries {
            assert_eq!(geometry.indices.len() % 3, 0);
            for triangle in geometry.indices.chunks_exact(3) {
                let a = &geometry.vertices[triangle[0] as usize];
                let b = &geometry.vertices[triangle[1] as usize];
                let c = &geometry.vertices[triangle[2] as usize];
                assert!(
                    (b.position - a.position)
                        .cross(c.position - a.position)
                        .dot(a.normal)
                        > 0.0
                );
                assert_eq!(a.normal, b.normal);
                assert_eq!(a.normal, c.normal);
            }
        }

        let camera = Camera::default();
        let eye = camera.position();
        assert!(eye.y > camera.target.y);
        assert!(eye.z > camera.target.z);
        assert!(camera.near > 0.0 && camera.far > camera.near);
        assert!(camera.vertical_fov > 0.0 && camera.vertical_fov < std::f32::consts::PI);
    }

    #[test]
    fn camera_input_changes_view_and_vulkan_projection() {
        let mut camera = Camera::default();
        let view = camera.view_transform().unwrap();
        let eye = view.transform_point3d(camera.position()).unwrap();
        let target = view.transform_point3d(camera.target).unwrap();
        assert!(eye.to_vector().length() < 1.0e-5);
        assert!(target.x.abs() < 1.0e-5 && target.y.abs() < 1.0e-5);
        assert!((target.z + camera.distance).abs() < 1.0e-5);

        let projection = camera.projection_transform(16.0 / 9.0).unwrap();
        let near = projection
            .transform_point3d(Point3::new(0.0, 0.0, -camera.near))
            .unwrap();
        let far = projection
            .transform_point3d(Point3::new(0.0, 0.0, -camera.far))
            .unwrap();
        let upper = projection
            .transform_point3d(Point3::new(0.0, 1.0, -1.0))
            .unwrap();
        assert!(near.z.abs() < 1.0e-5);
        assert!((far.z - 1.0).abs() < 1.0e-5);
        assert!(upper.y < 0.0);
        assert!(camera.projection_transform(0.0).is_none());
        assert!(camera.projection_transform(f32::NAN).is_none());

        let old_position = camera.position();
        let old_transform = camera.view_projection(16.0 / 9.0).unwrap();
        assert!(camera.orbit(120.0, 30.0));
        assert_ne!(camera.position(), old_position);
        assert_ne!(camera.view_projection(16.0 / 9.0).unwrap(), old_transform);
        let old_distance = camera.distance;
        assert!(camera.zoom(20.0));
        assert!(camera.distance < old_distance);
        assert!(!camera.orbit(f32::NAN, 0.0));
        assert!(!camera.zoom(f32::NAN));
        assert!(camera.orbit(0.0, 1.0e6));
        assert!(camera.view_transform().is_some());
    }

    #[test]
    fn snapshots_expose_independent_versions_and_preserve_previous_data() {
        let mut state = SceneState::default();
        let first = state.snapshot();
        assert_eq!(first.versions, Versions::default());
        assert!(!state.set_scene((*first.scene).clone()));
        assert!(!state.set_camera(first.camera));
        assert!(!state.set_render_parameters(first.render));
        assert!(!state.set_display_parameters(first.display));
        assert_eq!(state.versions(), first.versions);

        // 几何体、实例、材质和光源都属于同一个场景版本。
        let mut scene = (*first.scene).clone();
        scene.geometries[0].vertices[0].position.x -= 1.0;
        assert!(state.set_scene(scene.clone()));
        scene.instances[1].transform = Transform3::translation(2.0, 0.5, 0.0);
        assert!(state.set_scene(scene.clone()));
        scene.materials[0].albedo[0] = 0.4;
        assert!(state.set_scene(scene.clone()));
        scene.lights[0].intensity[0] = 50.0;
        assert!(state.set_scene(scene));
        let changed_scene = state.snapshot();
        assert_eq!(changed_scene.versions.scene, 4);
        assert_eq!(changed_scene.versions.camera, 0);
        assert_ne!(changed_scene.scene, first.scene);
        assert_eq!(first.scene.geometries[0].vertices[0].position.x, -4.0);

        let mut camera = state.camera();
        assert!(camera.orbit(40.0, 10.0));
        assert!(state.set_camera(camera));
        assert!(state.set_render_parameters(RenderParameters {
            max_path_bounces: 3,
        }));
        let before_display = state.snapshot();
        assert_eq!(before_display.versions.camera, 1);
        assert_eq!(before_display.versions.render, 1);
        assert!(state.set_display_parameters(DisplayParameters { exposure_ev: 1.0 }));
        let observed = state.snapshot();
        assert_eq!(observed.versions.display, 1);
        assert_eq!(
            observed.versions.accumulation_key(),
            before_display.versions.accumulation_key()
        );
        assert_eq!(observed.camera, camera);
        assert_eq!(observed.render.max_path_bounces, 3);
        assert_eq!(observed.display.exposure_ev, 1.0);
        assert_eq!(first.versions, Versions::default());
        assert_eq!(first.camera, Camera::default());

        let mut invalid = camera;
        invalid.distance = 0.0;
        assert!(!state.set_camera(invalid));
        assert!(!state.set_render_parameters(RenderParameters {
            max_path_bounces: 1,
        }));
        assert!(!state.set_display_parameters(DisplayParameters {
            exposure_ev: f32::NAN,
        }));
        assert_eq!(state.versions(), observed.versions);
    }
}
