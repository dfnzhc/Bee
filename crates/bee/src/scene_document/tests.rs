use super::*;
use crate::project::{Project, ProjectLocation};
use bee_renderer::{map_linear_color, FrameSlot, ViewportRenderer};
use bee_scene::{Transform3, Versions};
use bee_vulkan::{diagnostic_channel, Backend, DiagnosticLevel, ValidationStatus};
use serde_json::{json, Value};
use std::fs;
use std::path::Path;
use uuid::Uuid;

fn project(path: &Path, document: &SceneDocument) -> Project<SceneDocument> {
    Project::create(
        ProjectLocation::ExistingEmptyDirectory(path),
        "Test project",
        "First scene",
        document,
    )
    .unwrap()
}

fn use_path(graph: &mut ModuleGraph, exposure_ev: f32) {
    let (path, _) = graph
        .nodes()
        .find(|(_, m)| *m == Module::PathTracing)
        .unwrap();
    let (tone, _) = graph
        .nodes()
        .find(|(_, m)| matches!(m, Module::ToneMapping { .. }))
        .unwrap();
    graph.connect(path, tone).unwrap();
    *graph.module_mut(tone).unwrap() = Module::ToneMapping { exposure_ev };
}

#[test]
fn scene_document_round_trips_and_loads_as_runtime_state() {
    let root = tempfile::tempdir().unwrap();
    let mut document = SceneDocument::builtin();
    assert_eq!(document.scene, Scene::builtin());
    document.scene.environment = [0.01, 0.2, 0.3];
    document.scene.instances[1].transform = Transform3::translation(0.4, 0.8, -0.9);
    assert!(document.start_camera.orbit(25.0, -26.0));
    assert!(document.start_camera.zoom(35.0));
    document.render.max_path_bounces = 5;
    let mut project = project(root.path(), &document);
    let id = project.manifest().startup_scene;
    let blank = SceneDocument::blank();
    let blank_id = project.new_scene("Blank", &blank).unwrap();
    drop(project);

    let reopened = Project::<SceneDocument>::open(root.path()).unwrap();
    let loaded = reopened.read_scene(id).unwrap();
    assert_eq!(loaded.id(), id);
    assert_eq!(loaded.content, document);
    assert_eq!(reopened.read_scene(blank_id).unwrap().content, blank);
    let mut prepared = loaded.content.prepare(None).unwrap();
    let snapshot = prepared.state.snapshot();
    assert_eq!(*snapshot.scene, document.scene);
    assert_eq!(snapshot.camera, document.start_camera);
    assert_eq!(snapshot.render, document.render);
    assert_eq!(snapshot.versions, Versions::default());
    let mut camera = snapshot.camera;
    assert!(camera.zoom(15.0));
    assert!(prepared.state.set_camera(camera));
    assert_eq!(loaded.content.start_camera, snapshot.camera);
}

#[test]
fn malformed_scene_files_are_rejected_before_rendering() {
    let root = tempfile::tempdir().unwrap();
    let project = project(root.path(), &SceneDocument::builtin());
    let id = project.manifest().startup_scene;
    let path = project.scene_path(id).unwrap();
    let saved: Value = serde_json::from_slice(&fs::read(&path).unwrap()).unwrap();
    let reject = |change: Value, expected: &str| {
        fs::write(&path, change.to_string()).unwrap();
        let error = project.read_scene(id).unwrap_err().to_string();
        assert!(error.contains(expected), "{error}");
    };

    let mut wrong_id = saved.clone();
    wrong_id["scene_id"] = json!(Uuid::nil());
    reject(wrong_id, "scene id");
    let mut invalid = saved.clone();
    invalid["content"]["scene"]["geometries"][0]["indices"][0] = json!(u32::MAX);
    reject(invalid, "geometry index");
    let mut invalid = saved.clone();
    invalid["content"]["scene"]["instances"][0]["geometry"] = json!(99);
    reject(invalid, "instance geometry");
    let mut invalid = saved.clone();
    invalid["content"]["scene"]["instances"][0]["material"] = json!(99);
    reject(invalid, "instance material");
    let mut invalid = saved.clone();
    invalid["content"]["scene"]["instances"][0]["transform"]["m11"] = json!(0);
    reject(invalid, "non-invertible instance transform");
    let mut invalid = saved.clone();
    invalid["content"]["scene"]["environment"][0] = json!(-0.1);
    reject(invalid, "environment color");
    let mut invalid = saved.clone();
    invalid["content"]["scene"]["lights"][0]["intensity"][0] = json!(1e39);
    reject(invalid, "invalid point light");
    let mut invalid = saved.clone();
    invalid["content"]["start_camera"]["near"] = json!(0.0);
    reject(invalid, "start camera");
    let mut invalid = saved.clone();
    invalid["content"]["start_camera"]["far"] = json!(1e20);
    reject(invalid, "start camera");
    let mut invalid = saved;
    invalid["content"]["render"]["max_path_bounces"] = json!(1);
    reject(invalid, "render parameters");
}

#[test]
fn flow_draft_and_applied_graph_survive_a_raster_preview_save() {
    let root = tempfile::tempdir().unwrap();
    let mut document = SceneDocument::blank();
    use_path(&mut document.applied_flow, 2.5);
    let (tone, _) = document
        .draft_flow
        .nodes()
        .find(|(_, m)| matches!(m, Module::ToneMapping { .. }))
        .unwrap();
    document.draft_flow.remove(tone).unwrap();
    document
        .draft_flow
        .add(Module::ToneMapping { exposure_ev: 1.0 });
    assert!(document.draft_flow.validate().is_err());
    let project = project(root.path(), &document);
    let id = project.manifest().startup_scene;
    let mut loaded = project.read_scene(id).unwrap();
    let reason = "VK_KHR_ray_tracing_pipeline is unavailable";
    let prepared = loaded.content.prepare(Some(reason.into())).unwrap();
    assert_eq!(prepared.preview.source(), Module::Rasterization);
    assert_eq!(prepared.preview_reason.as_deref(), Some(reason));
    assert_eq!(prepared.preview.exposure_ev, 2.5);
    assert_eq!(prepared.state.snapshot().display.exposure_ev, 2.5);
    assert!(!prepared.flow.is_applied());
    assert_eq!(prepared.flow.active(), Some(&document.applied_flow));
    assert_eq!(prepared.flow.draft(), &document.draft_flow);

    loaded.content.scene.environment = [0.4; 3];
    project.save_scene(&mut loaded).unwrap();
    let saved = project.read_scene(id).unwrap().content;
    assert_eq!(saved.applied_flow, document.applied_flow);
    assert_eq!(saved.draft_flow, document.draft_flow);
    assert_eq!(saved.scene.environment, [0.4; 3]);
    let native = saved.prepare(None).unwrap();
    assert_eq!(native.preview.source(), Module::PathTracing);
    assert_eq!(native.preview_reason, None);
}

#[test]
fn invalid_flow_references_and_unexecutable_applied_flow_are_rejected() {
    let root = tempfile::tempdir().unwrap();
    let project = project(root.path(), &SceneDocument::blank());
    let id = project.manifest().startup_scene;
    let path = project.scene_path(id).unwrap();
    let saved: Value = serde_json::from_slice(&fs::read(&path).unwrap()).unwrap();
    for (field, connections, reason) in [
        ("draft_flow", json!([[0, 99]]), "refers to a missing module"),
        ("draft_flow", json!([[0, 2], [1, 2]]), "multiple input"),
        (
            "applied_flow",
            json!([[0, 99]]),
            "refers to a missing module",
        ),
        (
            "applied_flow",
            json!([[2, 3]]),
            "missing its required input",
        ),
    ] {
        let mut invalid = saved.clone();
        invalid["content"][field]["connections"] = connections;
        fs::write(&path, invalid.to_string()).unwrap();
        let error = project.read_scene(id).unwrap_err().to_string();
        assert!(error.contains(reason), "{error}");
    }
}

#[test]
#[ignore = "requires a Vulkan graphics device and validation layer"]
fn blank_document_raster_viewport() {
    blank_document_viewport(Module::Rasterization);
}

#[test]
#[ignore = "requires a Vulkan ray tracing device and validation layer"]
fn blank_document_path_viewport() {
    blank_document_viewport(Module::PathTracing);
}

fn blank_document_viewport(source: Module) {
    let root = tempfile::tempdir().unwrap();
    let mut document = SceneDocument::blank();
    let environment = [0.17, 0.04, 0.61];
    document.scene.environment = environment;
    let tracing = source == Module::PathTracing;
    if tracing {
        use_path(&mut document.applied_flow, 0.0);
        document.draft_flow = document.applied_flow.clone();
    }
    let project = project(root.path(), &document);
    let loaded = project
        .read_scene(project.manifest().startup_scene)
        .unwrap();
    let (diagnostics, messages) = diagnostic_channel();
    {
        let backend = Backend::new_with_diagnostics(true, diagnostics).unwrap();
        println!("{}: {source:?}", backend.report.name);
        assert_eq!(backend.report.validation, ValidationStatus::Enabled);
        assert!(
            !tracing || backend.report.ray_tracing_missing.is_empty(),
            "{:?}",
            backend.report.ray_tracing_missing
        );
        let PreparedScene {
            mut state, preview, ..
        } = loaded.content.prepare(None).unwrap();
        assert_eq!(preview.source(), source);
        assert!(state.scene().geometries.is_empty());
        assert!(state.scene().instances.is_empty());
        assert!(state.scene().materials.is_empty());
        assert!(state.scene().lights.is_empty());

        let mut renderer = ViewportRenderer::default();
        let mut slot = FrameSlot::default();
        let (width, height) = (20, 12);
        let mut render = |flow: &PreparedFlow, state: &SceneState, samples| {
            let pending = renderer
                .submit(
                    &backend,
                    flow,
                    &state.snapshot(),
                    (width, height),
                    0,
                    1,
                    &mut slot,
                )
                .unwrap();
            assert_eq!(pending.samples, samples);
            let (next_slot, pixels) = pending.finish();
            slot = next_slot;
            let pixels = pixels.unwrap();
            let [r, g, b] = environment;
            let [r, g, b, a] = map_linear_color([r, g, b, 1.0], flow.exposure_ev).unwrap();
            let bgra = [b, g, r, a].map(|v| (v * 255.0).round() as u8);
            assert_eq!(pixels.len(), (width * height * 4) as usize);
            for pixel in pixels.chunks_exact(4) {
                assert!(
                    pixel.iter().zip(bgra).all(|(a, b)| a.abs_diff(b) <= 2),
                    "expected {bgra:?}, got {pixel:?}"
                );
            }
        };
        render(&preview, &state, u32::from(tracing));
        if tracing {
            render(&preview, &state, 2);
        }
        let mut camera = state.camera();
        assert!(camera.orbit(15.0, 20.0));
        assert!(camera.zoom(25.0));
        assert!(state.set_camera(camera));
        render(&preview, &state, u32::from(tracing));

        let mut graph = loaded.content.applied_flow;
        let (tone, _) = graph
            .nodes()
            .find(|(_, m)| matches!(m, Module::ToneMapping { .. }))
            .unwrap();
        *graph.module_mut(tone).unwrap() = Module::ToneMapping { exposure_ev: 1.5 };
        let exposure = PreparedFlow::prepare(&graph, Some(&preview)).unwrap();
        assert!(state.set_display_parameters(DisplayParameters {
            exposure_ev: exposure.exposure_ev,
        }));
        render(&exposure, &state, u32::from(tracing));
    }
    assert_eq!(messages.take_dropped(), 0);
    let errors: Vec<_> = messages
        .drain()
        .filter(|message| message.level == DiagnosticLevel::Error)
        .collect();
    assert!(errors.is_empty(), "{errors:#?}");
}
