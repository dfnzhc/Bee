use crate::{
    scene::{PathScene, RasterScene},
    Access, DisplayWork, GraphResources, Module, ModuleGraph, NodeId, RasterWork, ResourceId,
    Usage, WorkGraph, WorkId, WorkPlan,
};
use ash::vk;
use bee_scene::RenderSnapshot;
use bee_vulkan::{
    profile, Backend, GraphImage, PathTracer, Rasterizer, ReadbackBuffer, ToneMapper,
};
use std::sync::Arc;

#[derive(Clone)]
pub struct PreparedFlow {
    plan: Arc<FlowPlan>,
    pub exposure_ev: f32,
}

struct FlowPlan {
    chain: [NodeId; 3],
    mode: Module,
    first: WorkPlan,
    continued: Option<WorkPlan>,
    scene: ResourceId,
    depth: Option<ResourceId>,
    display: ResourceId,
    first_work: SceneWork,
    continued_trace: Option<WorkId>,
    tone: DisplayWork,
    read: WorkId,
    display_only: WorkPlan,
    display_tone: DisplayWork,
    display_read: WorkId,
}

enum SceneWork {
    Raster(RasterWork),
    Trace(WorkId),
}

impl PreparedFlow {
    pub fn prepare(modules: &ModuleGraph, previous: Option<&Self>) -> Result<Self, String> {
        let (chain, exposure_ev) = modules.display_chain()?;
        let mode = modules.module(chain[0]).ok_or("missing scene source")?;
        if let Some(previous) =
            previous.filter(|flow| flow.plan.chain == chain && flow.plan.mode == mode)
        {
            return Ok(Self {
                plan: previous.plan.clone(),
                exposure_ev,
            });
        }

        let make_graph = |scene_usage| {
            let mut graph = WorkGraph::default();
            let scene = graph.add_resource(scene_usage);
            let depth = (mode == Module::Rasterization).then(|| graph.add_resource(None));
            let display = graph.add_resource(None);
            (graph, scene, depth, display)
        };
        let (mut graph, scene, depth, display) = make_graph(None);
        let first_work = match mode {
            Module::Rasterization => SceneWork::Raster(RasterWork::declare(
                modules,
                chain[0],
                &mut graph,
                scene,
                depth.unwrap(),
            )?),
            Module::PathTracing => {
                SceneWork::Trace(trace_work(&mut graph, scene, Usage::ShaderWrite)?)
            }
            _ => return Err("viewport input is not connected to a scene renderer".into()),
        };
        let tone =
            DisplayWork::declare(modules, &mut graph, &[(chain[0], scene)], display, display)?;
        let read = read_work(&mut graph, display)?;
        let first = graph.compile()?;
        let (continued, continued_trace) = if mode == Module::PathTracing {
            // 续采仅在前一个已提交样本定义历史后使用。
            let (mut graph, scene, _, display) = make_graph(Some(Usage::ShaderRead));
            let trace = trace_work(&mut graph, scene, Usage::ShaderReadWrite)?;
            DisplayWork::declare(modules, &mut graph, &[(chain[0], scene)], display, display)?;
            let continued_read = read_work(&mut graph, display)?;
            debug_assert_eq!(continued_read, read);
            (Some(graph.compile()?), Some(trace))
        } else {
            (None, None)
        };
        let (mut graph, _, _, _) = make_graph(Some(Usage::ShaderRead));
        let display_tone =
            DisplayWork::declare(modules, &mut graph, &[(chain[0], scene)], display, display)?;
        let display_read = read_work(&mut graph, display)?;
        let display_only = graph.compile()?;
        Ok(Self {
            plan: Arc::new(FlowPlan {
                chain,
                mode,
                first,
                continued,
                scene,
                depth,
                display,
                first_work,
                continued_trace,
                tone,
                read,
                display_only,
                display_tone,
                display_read,
            }),
            exposure_ev,
        })
    }

    pub fn source(&self) -> Module {
        self.plan.mode
    }
}

fn trace_work(graph: &mut WorkGraph, scene: ResourceId, usage: Usage) -> Result<WorkId, String> {
    graph.add_work(
        "path tracing",
        vec![Access {
            resource: scene,
            usage,
        }],
    )
}

fn read_work(graph: &mut WorkGraph, display: ResourceId) -> Result<WorkId, String> {
    graph.add_work(
        "read viewport",
        vec![Access {
            resource: display,
            usage: Usage::TransferRead,
        }],
    )
}

type Extent = (u32, u32);

#[derive(Default)]
pub struct ViewportRenderer {
    display: Option<(vk::Format, ToneMapper)>,
    raster: Option<Rasterizer>,
    raster_scene: Option<RasterScene>,
    tracer: Option<PathTracer>,
    path_scene: Option<PathScene>,
    history: Option<History>,
}

struct History {
    flow: Arc<FlowPlan>,
    epoch: u64,
    extent: Extent,
    image: GraphImage,
    usage: Option<Usage>,
    previous: Option<RenderSnapshot>,
    exposure_ev: f32,
    samples: u32,
}

#[derive(Default)]
pub struct FrameSlot {
    resources: GraphResources,
    key: Option<(Extent, Module)>,
    readback: Option<ReadbackBuffer>,
}

pub struct PendingViewport {
    slot: FrameSlot,
    pub samples: u32,
    bgra: bool,
}

impl ViewportRenderer {
    pub(crate) fn path(backend: &Backend) -> Result<Self, String> {
        let mut renderer = Self::default();
        renderer.prepare_pipelines(backend, Module::PathTracing)?;
        Ok(renderer)
    }

    pub fn clear_history(&mut self) {
        self.history = None;
    }

    fn prepare_pipelines(&mut self, backend: &Backend, mode: Module) -> Result<(), String> {
        if self.display.is_none() {
            let format = backend.viewport_format();
            self.display = Some((
                format,
                backend
                    .create_tone_mapper_for(format)
                    .map_err(string_error)?,
            ));
        }
        match mode {
            Module::Rasterization if self.raster.is_none() => {
                self.raster = Some(backend.create_rasterizer().map_err(string_error)?);
            }
            Module::PathTracing if self.tracer.is_none() => {
                self.tracer = Some(backend.create_path_tracer().map_err(string_error)?);
            }
            _ => {}
        }
        Ok(())
    }

    pub fn submit(
        &mut self,
        backend: &Backend,
        flow: &PreparedFlow,
        snapshot: &RenderSnapshot,
        extent: Extent,
        epoch: u64,
        seed: u32,
        slot: &mut FrameSlot,
    ) -> Result<PendingViewport, String> {
        let plan = &flow.plan;
        let (width, height) = extent;
        if width == 0 || height == 0 || width.max(height) > backend.report.max_image_dimension_2d {
            return Err("invalid viewport extent".into());
        }
        let matrix = snapshot
            .camera
            .view_projection(width as f32 / height as f32)
            .ok_or("invalid viewport camera")?;
        let tracing = plan.mode == Module::PathTracing;
        let matrix = if tracing {
            matrix.inverse().ok_or("invalid path tracing camera")?
        } else {
            matrix
        }
        .to_array();
        profile::span("pipeline_us", || self.prepare_pipelines(backend, plan.mode))?;
        let (format, mapper) = self.display.as_ref().unwrap();

        let make_image = |format, usage, aspect| unsafe {
            backend
                .create_graph_image(width, height, format, usage, aspect)
                .map_err(string_error)
        };
        let color = vk::ImageAspectFlags::COLOR;
        if self.history.as_ref().is_none_or(|history| {
            history.extent != extent || history.epoch != epoch || !Arc::ptr_eq(&history.flow, plan)
        }) {
            let image = profile::span("targets_us", || {
                make_image(
                    vk::Format::R32G32B32A32_SFLOAT,
                    (if tracing {
                        vk::ImageUsageFlags::STORAGE
                    } else {
                        vk::ImageUsageFlags::COLOR_ATTACHMENT
                    }) | vk::ImageUsageFlags::SAMPLED,
                    color,
                )
            })?;
            self.history = Some(History {
                flow: plan.clone(),
                epoch,
                extent,
                image,
                usage: None,
                previous: None,
                exposure_ev: flow.exposure_ev,
                samples: 0,
            });
        }

        let history = self.history.as_mut().unwrap();
        let same = history.previous.as_ref().is_some_and(|previous| {
            Arc::ptr_eq(&previous.scene, &snapshot.scene)
                && previous.versions.accumulation_key() == snapshot.versions.accumulation_key()
                && previous.camera == snapshot.camera
                && previous.render == snapshot.render
        });
        let display_only = same
            && (history.exposure_ev != flow.exposure_ev
                || history.previous.as_ref().is_some_and(|previous| {
                    previous.versions.display != snapshot.versions.display
                }));
        let samples = if tracing && same && history.samples < u32::MAX {
            history.samples
        } else {
            0
        };
        if !display_only {
            if tracing
                && self
                    .path_scene
                    .as_ref()
                    .is_none_or(|scene| !Arc::ptr_eq(&scene.source, &snapshot.scene))
            {
                self.path_scene = Some(profile::span("scene_prepare_us", || {
                    PathScene::path(backend, &snapshot.scene, self.path_scene.as_ref())
                })?);
            }
            if !tracing
                && self
                    .raster_scene
                    .as_ref()
                    .is_none_or(|scene| !Arc::ptr_eq(&scene.source, &snapshot.scene))
            {
                self.raster_scene = Some(profile::span("scene_prepare_us", || {
                    RasterScene::raster(backend, &snapshot.scene, self.raster_scene.as_ref())
                })?);
            }
        }

        let key = (extent, plan.mode);
        if slot.key != Some(key) {
            // 分配成功后才替换槽中的绑定。命令池与尺寸无关，可继续复用。
            let (depth, display, readback) = profile::span("targets_us", || {
                let depth = self
                    .raster
                    .as_ref()
                    .filter(|_| !tracing)
                    .map(|raster| {
                        make_image(
                            raster.depth_format(),
                            vk::ImageUsageFlags::DEPTH_STENCIL_ATTACHMENT,
                            vk::ImageAspectFlags::DEPTH,
                        )
                    })
                    .transpose()?;
                let display = make_image(
                    *format,
                    vk::ImageUsageFlags::COLOR_ATTACHMENT | vk::ImageUsageFlags::TRANSFER_SRC,
                    color,
                )?;
                let bytes = (width as usize)
                    .checked_mul(height as usize)
                    .and_then(|n| n.checked_mul(4))
                    .ok_or("viewport size overflow")?;
                let readback = backend
                    .create_readback_buffer(bytes)
                    .map_err(string_error)?;
                Ok::<_, String>((depth, display, readback))
            })?;
            slot.resources.clear_bindings();
            let scene_id = slot.resources.add_history(history.image.clone());
            let depth_id = depth.map(|image| slot.resources.add_history(image));
            let display_id = slot.resources.add_history(display);
            assert_eq!(
                (scene_id, depth_id, display_id),
                (plan.scene, plan.depth, plan.display)
            );
            slot.readback = Some(readback);
            slot.key = Some(key);
        }

        let (work_plan, tone, read) = if display_only {
            (&plan.display_only, &plan.display_tone, plan.display_read)
        } else if samples == 0 {
            (&plan.first, &plan.tone, plan.read)
        } else {
            (plan.continued.as_ref().unwrap(), &plan.tone, plan.read)
        };
        let readback = slot.readback.as_ref().unwrap();
        unsafe {
            // 两个槽在同一队列交接 HDR，按已提交的顺序继续访问与转换布局。
            slot.resources
                .bind_ordered(plan.scene, history.image.clone(), history.usage);
            slot.resources
                .submit_plan(backend, work_plan, |work, frame, images| {
                    if work == read {
                        frame
                            .copy_graph_image_to_readback(
                                images[plan.display.index()].image()?,
                                readback,
                            )
                            .map_err(string_error)?;
                        return Ok(());
                    }
                    if tone.record_exposure(work, frame, images, mapper, flow.exposure_ev)? {
                        return Ok(());
                    }
                    if !display_only {
                        match &plan.first_work {
                            SceneWork::Raster(raster) => {
                                if raster.record(
                                    work,
                                    frame,
                                    images,
                                    self.raster.as_ref().unwrap(),
                                    self.raster_scene.as_ref().unwrap(),
                                    matrix,
                                )? {
                                    return Ok(());
                                }
                            }
                            SceneWork::Trace(first)
                                if Some(work)
                                    == if samples == 0 {
                                        Some(*first)
                                    } else {
                                        plan.continued_trace
                                    } =>
                            {
                                let scene = self.path_scene.as_ref().unwrap();
                                frame
                                    .trace_paths(
                                        self.tracer.as_ref().unwrap(),
                                        &scene.instances,
                                        &scene.materials,
                                        &scene.lights,
                                        images[plan.scene.index()].image()?,
                                        matrix,
                                        snapshot.scene.environment,
                                        samples,
                                        snapshot.render.max_path_bounces,
                                        seed,
                                    )
                                    .map_err(string_error)?;
                                return Ok(());
                            }
                            _ => {}
                        }
                    }
                    Err("unknown viewport graph work".into())
                })?;
        }
        history.usage = slot.resources.usage(plan.scene);
        history.exposure_ev = flow.exposure_ev;
        history.previous = Some(snapshot.clone());
        if !display_only {
            history.samples = if tracing { samples + 1 } else { 0 };
        }
        Ok(PendingViewport {
            slot: std::mem::take(slot),
            samples: history.samples,
            bgra: *format == vk::Format::B8G8R8A8_UNORM,
        })
    }
}

impl PendingViewport {
    pub fn finish(mut self) -> (FrameSlot, Result<Vec<u8>, String>) {
        let pixels = self.read_pixels();
        (self.slot, pixels)
    }

    fn read_pixels(&mut self) -> Result<Vec<u8>, String> {
        self.slot.resources.wait()?;
        let readback = self.slot.readback.as_ref().unwrap();
        let mut pixels =
            profile::span("readback_us", || unsafe { readback.read() }).map_err(string_error)?;
        if !self.bgra {
            profile::span("convert_us", || {
                for pixel in pixels.chunks_exact_mut(4) {
                    pixel.swap(0, 2);
                }
            });
        }
        Ok(pixels)
    }
}

fn string_error(error: impl std::fmt::Display) -> String {
    error.to_string()
}

#[cfg(test)]
mod tests {
    use super::*;
    use bee_scene::{Camera, Point3, SceneState, Transform3};

    fn modules(mode: Module) -> (ModuleGraph, NodeId, NodeId) {
        let mut graph = ModuleGraph::default();
        let scene = graph.add(mode);
        let tone = graph.add(Module::ToneMapping { exposure_ev: 0.0 });
        let output = graph.add(Module::ViewportOutput);
        graph.connect(scene, tone).unwrap();
        graph.connect(tone, output).unwrap();
        (graph, tone, output)
    }

    #[test]
    fn plan_identity_follows_reachable_topology_and_not_exposure() {
        let (mut graph, tone, output) = modules(Module::Rasterization);
        let first = PreparedFlow::prepare(&graph, None).unwrap();
        graph.add(Module::PathTracing);
        *graph.module_mut(tone).unwrap() = Module::ToneMapping { exposure_ev: 2.0 };
        let (updated, events) = profile::capture(|| PreparedFlow::prepare(&graph, Some(&first)));
        let updated = updated.unwrap();
        assert!(Arc::ptr_eq(&first.plan, &updated.plan));
        assert_eq!(updated.exposure_ev, 2.0);
        assert!(!events.iter().any(|event| event.name == "graph_compile"));
        graph.disconnect(output);
        assert!(PreparedFlow::prepare(&graph, Some(&updated)).is_err());
        assert_eq!(updated.source(), Module::Rasterization);
        graph.connect(tone, output).unwrap();
        let replacement = graph.add(Module::Rasterization);
        graph.connect(replacement, tone).unwrap();
        let (replaced, events) = profile::capture(|| PreparedFlow::prepare(&graph, Some(&updated)));
        assert!(!Arc::ptr_eq(&updated.plan, &replaced.unwrap().plan));
        assert_eq!(
            events
                .iter()
                .filter(|event| event.name == "graph_compile")
                .count(),
            2
        );
    }

    #[test]
    #[ignore = "requires a ray tracing device and Vulkan validation layer"]
    fn updates_scene_parts_and_remaps_in_flight_hdr() {
        let (diagnostics, messages) = bee_vulkan::diagnostic_channel();
        let backend = Backend::new_with_diagnostics(true, diagnostics).unwrap();
        assert_eq!(
            backend.report.validation,
            bee_vulkan::ValidationStatus::Enabled
        );
        let extent = (64, 64);
        let pixel = |pixels: &[u8], x: usize, y: usize| {
            let offset = (y * 64 + x) * 4;
            <[u8; 4]>::try_from(&pixels[offset..offset + 4]).unwrap()
        };
        let counts = |events: &[profile::Event], expected: &[&str]| {
            for name in [
                "graph_compile",
                "mesh_upload",
                "instance_upload",
                "material_upload",
                "light_upload",
                "blas_build",
                "tlas_build",
                "tlas_update",
                "raster_pass",
                "trace_sample",
            ] {
                assert_eq!(
                    events.iter().filter(|event| event.name == name).count(),
                    expected.iter().filter(|&&event| event == name).count(),
                    "{name}",
                );
            }
        };

        for mode in [Module::Rasterization, Module::PathTracing] {
            let tracing = mode == Module::PathTracing;
            let scene_pass = if tracing {
                "trace_sample"
            } else {
                "raster_pass"
            };
            let (mut graph, tone, _) = modules(mode);
            let mut flow = PreparedFlow::prepare(&graph, None).unwrap();
            let mut renderer = ViewportRenderer::default();
            let mut slots: [FrameSlot; 2] = Default::default();
            let mut state = SceneState::default();
            state.set_camera(Camera {
                target: Point3::new(0.0, 0.5, 0.0),
                pitch: 0.0,
                distance: 4.0,
                ..Camera::default()
            });
            let mut scene = state.scene().clone();
            scene.instances[1].transform = Transform3::translation(-0.85, 0.5, 0.0);
            scene.instances[2].transform = Transform3::translation(0.85, 0.5, 0.0);
            assert!(state.set_scene(scene));

            let draw = |renderer: &mut ViewportRenderer,
                        flow: &PreparedFlow,
                        state: &SceneState,
                        slot: &mut FrameSlot| {
                let pending = renderer
                    .submit(&backend, flow, &state.snapshot(), extent, 0, 1, slot)
                    .unwrap();
                let samples = pending.samples;
                let pixels;
                (*slot, pixels) = pending.finish();
                (samples, pixels.unwrap())
            };
            let (first_samples, pixels) = draw(&mut renderer, &flow, &state, &mut slots[0]);
            assert_eq!(first_samples, u32::from(tracing));
            let red = |[b, _, r, _]: [u8; 4]| r > b.saturating_add(25);
            let blue = |[b, _, r, _]: [u8; 4]| b > r.saturating_add(25);
            assert!(red(pixel(&pixels, 19, 32)));
            assert!(blue(pixel(&pixels, 45, 32)));

            let original = renderer
                .submit(
                    &backend,
                    &flow,
                    &state.snapshot(),
                    extent,
                    0,
                    1,
                    &mut slots[0],
                )
                .unwrap();
            *graph.module_mut(tone).unwrap() = Module::ToneMapping { exposure_ev: 2.0 };
            let ((samples, brighter), events) = profile::capture(|| {
                flow = PreparedFlow::prepare(&graph, Some(&flow)).unwrap();
                draw(&mut renderer, &flow, &state, &mut slots[1])
            });
            counts(&events, &[]);
            assert_eq!(samples, original.samples);
            assert_eq!(samples, if tracing { 2 } else { 0 });
            let old_pixels;
            (slots[0], old_pixels) = original.finish();
            let mut channels = brighter.iter().zip(old_pixels.as_ref().unwrap());
            assert!(channels.clone().all(|(after, before)| after >= before));
            assert!(channels.any(|(after, before)| after > before));

            let mut previous = brighter;
            for edit in 0..5 {
                let in_flight = renderer
                    .submit(
                        &backend,
                        &flow,
                        &state.snapshot(),
                        extent,
                        0,
                        1,
                        &mut slots[1],
                    )
                    .unwrap();
                let mut scene = state.scene().clone();
                let expected = match edit {
                    0 => {
                        scene.instances[1].transform = Transform3::translation(-0.85, 1.55, 0.0);
                        if tracing {
                            "tlas_update"
                        } else {
                            "instance_upload"
                        }
                    }
                    1 => {
                        for vertex in &mut scene.geometries[1].vertices {
                            vertex.position.x += 0.65;
                        }
                        "mesh_upload"
                    }
                    2 => {
                        scene.materials[2].albedo = [0.1, 0.75, 0.15];
                        "material_upload"
                    }
                    3 => {
                        scene.lights[0].intensity = [2.0; 3];
                        "light_upload"
                    }
                    _ => {
                        scene.instances.pop();
                        if tracing {
                            "tlas_build"
                        } else {
                            "instance_upload"
                        }
                    }
                };
                assert!(state.set_scene(scene));
                let ((samples, changed), events) = profile::capture(|| {
                    flow = PreparedFlow::prepare(&graph, Some(&flow)).unwrap();
                    draw(&mut renderer, &flow, &state, &mut slots[0])
                });
                if tracing && edit == 1 {
                    counts(
                        &events,
                        &[scene_pass, expected, "blas_build", "tlas_update"],
                    );
                } else {
                    counts(&events, &[scene_pass, expected]);
                }
                assert_eq!(samples, u32::from(tracing), "scene edit {edit}");
                if edit == 0 {
                    assert!(!red(pixel(&changed, 19, 32)));
                    assert!(red(pixel(&changed, 19, 16)));
                    assert!(blue(pixel(&changed, 45, 32)));
                }
                slots[1] = in_flight.finish().0;
                // 以新渲染器的首帧核对每次更新的数值和累积清零。
                let (_, fresh) = draw(
                    &mut ViewportRenderer::default(),
                    &flow,
                    &state,
                    &mut slots[1],
                );
                assert!(
                    changed.iter().zip(fresh).all(|(&a, b)| a.abs_diff(b) <= 1),
                    "scene edit {edit}"
                );
                assert_ne!(changed, previous, "scene edit {edit}");
                previous = changed;
                let (samples, _) = draw(&mut renderer, &flow, &state, &mut slots[0]);
                assert_eq!(samples, if tracing { 2 } else { 0 });
            }
            let mut camera = state.camera();
            assert!(camera.orbit(10.0, 0.0));
            assert!(state.set_camera(camera));
            *graph.module_mut(tone).unwrap() = Module::ToneMapping { exposure_ev: -1.0 };
            flow = PreparedFlow::prepare(&graph, Some(&flow)).unwrap();
            let ((samples, _), events) =
                profile::capture(|| draw(&mut renderer, &flow, &state, &mut slots[1]));
            counts(&events, &[scene_pass]);
            assert_eq!(samples, u32::from(tracing));
        }
        drop(backend);
        for message in messages.drain() {
            assert_ne!(
                message.level,
                bee_vulkan::DiagnosticLevel::Error,
                "{}",
                message.message
            );
        }
    }

    #[test]
    #[ignore = "requires a ray tracing device and Vulkan validation layer"]
    fn reuses_prepared_work_and_preserves_bgra_for_both_output_formats() {
        let (diagnostics, messages) = bee_vulkan::diagnostic_channel();
        let backend = Backend::new_with_diagnostics(true, diagnostics).unwrap();
        assert_eq!(
            backend.report.validation,
            bee_vulkan::ValidationStatus::Enabled
        );
        assert_eq!(backend.viewport_format(), vk::Format::B8G8R8A8_UNORM);
        let mut renderer = ViewportRenderer::default();
        let mut slots: [FrameSlot; 2] = Default::default();
        for mode in [Module::Rasterization, Module::PathTracing] {
            let (graph, tone, _) = modules(mode);
            let flow = PreparedFlow::prepare(&graph, None).unwrap();
            let mut state = SceneState::default();
            let snapshot = state.snapshot();
            let extent = (97, 51);
            let first = renderer
                .submit(&backend, &flow, &snapshot, extent, 0, 1, &mut slots[0])
                .unwrap();
            let second = renderer
                .submit(&backend, &flow, &snapshot, extent, 0, 1, &mut slots[1])
                .unwrap();
            if mode == Module::PathTracing {
                assert_eq!((first.samples, second.samples), (1, 2));
            }
            let first_pixels;
            (slots[0], first_pixels) = first.finish();
            slots[1] = second.finish().0;
            let first_pixels = first_pixels.unwrap();
            assert_eq!(first_pixels.len(), 97 * 51 * 4);
            assert!(first_pixels.chunks_exact(4).any(|p| p[0] != p[2]));

            let mut fallback = ViewportRenderer {
                display: Some((
                    vk::Format::R8G8B8A8_UNORM,
                    backend.create_tone_mapper().unwrap(),
                )),
                ..Default::default()
            };
            let rgba = fallback
                .submit(
                    &backend,
                    &flow,
                    &snapshot,
                    extent,
                    0,
                    1,
                    &mut FrameSlot::default(),
                )
                .unwrap()
                .finish()
                .1
                .unwrap();
            assert!(first_pixels
                .iter()
                .zip(rgba)
                .all(|(&a, b)| a.abs_diff(b) <= 1));

            let ((), events) = profile::capture(|| {
                for (index, extent) in [extent, extent, (101, 53), (101, 53)]
                    .into_iter()
                    .enumerate()
                {
                    let mut camera = state.camera();
                    assert!(camera.orbit(4.0, 1.0));
                    assert!(state.set_camera(camera));
                    let slot = &mut slots[index % 2];
                    let pending = renderer
                        .submit(&backend, &flow, &state.snapshot(), extent, 0, 1, slot)
                        .unwrap();
                    if mode == Module::PathTracing {
                        assert_eq!(pending.samples, 1);
                    }
                    let pixels;
                    (*slot, pixels) = pending.finish();
                    assert_eq!(pixels.unwrap().len(), (extent.0 * extent.1 * 4) as usize);
                }
            });
            for name in [
                "graph_compile",
                "pipeline_create",
                "mesh_upload",
                "blas_build",
                "tlas_build",
                "image_copy",
                "convert_us",
            ] {
                assert!(
                    !events.iter().any(|event| event.name == name),
                    "{mode:?} rebuilt {name}"
                );
            }

            let ((), events) = profile::capture(|| {
                let mut graph = graph.clone();
                *graph.module_mut(tone).unwrap() = Module::ToneMapping { exposure_ev: 1.0 };
                let exposure = PreparedFlow::prepare(&graph, Some(&flow)).unwrap();
                assert!(Arc::ptr_eq(&exposure.plan, &flow.plan));
                let mut scene = state.scene().clone();
                scene.instances[0].transform = bee_scene::Transform3::translation(1.0, 0.0, 0.0);
                assert!(state.set_scene(scene));
                let pending = renderer
                    .submit(
                        &backend,
                        &exposure,
                        &state.snapshot(),
                        extent,
                        0,
                        1,
                        &mut slots[0],
                    )
                    .unwrap();
                if mode == Module::PathTracing {
                    assert_eq!(pending.samples, 1);
                }
                slots[0] = pending.finish().0;
            });
            assert!(!events.iter().any(|event| event.name == "graph_compile"));
        }
        drop(slots);
        drop(renderer);
        drop(backend);
        assert!(!messages.drain().any(|message| {
            assert_ne!(
                message.level,
                bee_vulkan::DiagnosticLevel::Error,
                "{}",
                message.message
            );
            false
        }));
    }
}
