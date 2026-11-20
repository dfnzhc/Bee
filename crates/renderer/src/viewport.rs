use crate::{
    raster::scene_data, Access, DisplayWork, GraphResources, Module, ModuleGraph, NodeId,
    RasterWork, ResourceId, Usage, WorkGraph, WorkId, WorkPlan,
};
use ash::vk;
use bee_scene::{RenderSnapshot, Scene};
use bee_vulkan::{
    profile, AccelerationScene, Backend, GraphImage, PathTracer, RasterGeometry, Rasterizer,
    ReadbackBuffer, ToneMapper,
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

        let mut graph = WorkGraph::default();
        let scene = graph.add_resource(None);
        let depth = (mode == Module::Rasterization).then(|| graph.add_resource(None));
        let display = graph.add_resource(None);
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
            let mut graph = WorkGraph::default();
            let scene = graph.add_resource(Some(Usage::ShaderRead));
            let display = graph.add_resource(None);
            let trace = trace_work(&mut graph, scene, Usage::ShaderReadWrite)?;
            DisplayWork::declare(modules, &mut graph, &[(chain[0], scene)], display, display)?;
            let continued_read = read_work(&mut graph, display)?;
            debug_assert_eq!(continued_read, read);
            (Some(graph.compile()?), Some(trace))
        } else {
            (None, None)
        };
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
    raster_geometry: Option<(Arc<Scene>, u64, RasterGeometry)>,
    tracer: Option<PathTracer>,
    acceleration: Option<(Arc<Scene>, u64, AccelerationScene)>,
    history: Option<History>,
}

struct History {
    flow: Arc<FlowPlan>,
    epoch: u64,
    extent: Extent,
    image: GraphImage,
    usage: Option<Usage>,
    previous: Option<RenderSnapshot>,
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
        if tracing {
            if self
                .acceleration
                .as_ref()
                .is_none_or(|(scene, version, _)| !same_scene(scene, *version, snapshot))
            {
                let (data, vertices) = profile::span("scene_prepare_us", || scene_data(snapshot))?;
                let acceleration = profile::span("acceleration_us", || {
                    backend.build_acceleration_scene(&data, vertices)
                })
                .map_err(string_error)?;
                self.acceleration = Some((
                    snapshot.scene.clone(),
                    snapshot.versions.scene,
                    acceleration,
                ));
            }
            if self.history.as_ref().is_none_or(|history| {
                history.extent != extent
                    || history.epoch != epoch
                    || !Arc::ptr_eq(&history.flow, plan)
            }) {
                let image = make_image(
                    vk::Format::R32G32B32A32_SFLOAT,
                    vk::ImageUsageFlags::STORAGE | vk::ImageUsageFlags::SAMPLED,
                    color,
                )?;
                self.history = Some(History {
                    flow: plan.clone(),
                    epoch,
                    extent,
                    image,
                    usage: None,
                    previous: None,
                    samples: 0,
                });
            }
        } else if self
            .raster_geometry
            .as_ref()
            .is_none_or(|(scene, version, _)| !same_scene(scene, *version, snapshot))
        {
            let (data, vertices) = profile::span("scene_prepare_us", || scene_data(snapshot))?;
            let geometry = backend
                .upload_raster_geometry(&data, vertices)
                .map_err(string_error)?;
            self.raster_geometry =
                Some((snapshot.scene.clone(), snapshot.versions.scene, geometry));
        }

        let history = self.history.as_mut().filter(|_| tracing);
        let samples = history.as_ref().map_or(0, |history| {
            let same = history.previous.as_ref().is_some_and(|previous| {
                same_scene(&previous.scene, previous.versions.scene, snapshot)
                    && previous.versions.accumulation_key() == snapshot.versions.accumulation_key()
                    && previous.camera == snapshot.camera
                    && previous.render == snapshot.render
            });
            if same && history.samples < u32::MAX {
                history.samples
            } else {
                0
            }
        });
        let key = (extent, plan.mode);
        if slot.key != Some(key) {
            // 分配成功后才替换槽中的绑定。命令池与尺寸无关，可继续复用。
            let (scene, depth, display, readback) = profile::span("targets_us", || {
                let scene = if let Some(history) = &history {
                    history.image.clone()
                } else {
                    make_image(
                        vk::Format::R32G32B32A32_SFLOAT,
                        vk::ImageUsageFlags::COLOR_ATTACHMENT | vk::ImageUsageFlags::SAMPLED,
                        color,
                    )?
                };
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
                let readback = backend.create_readback_buffer(bytes).map_err(string_error)?;
                Ok::<_, String>((scene, depth, display, readback))
            })?;
            slot.resources.clear_bindings();
            let scene_id = slot.resources.add_history(scene);
            let depth_id = depth.map(|image| slot.resources.add_history(image));
            let display_id = slot.resources.add_history(display);
            assert_eq!(
                (scene_id, depth_id, display_id),
                (plan.scene, plan.depth, plan.display)
            );
            slot.readback = Some(readback);
            slot.key = Some(key);
        }

        let work_plan = if samples == 0 {
            &plan.first
        } else {
            plan.continued.as_ref().unwrap()
        };
        let readback = slot.readback.as_ref().unwrap();
        unsafe {
            if let Some(history) = &history {
                // 共享历史只有此协调器提交，后续命令按队列顺序接续其布局和访问。
                slot.resources
                    .bind_ordered(plan.scene, history.image.clone(), history.usage);
            }
            slot.resources
                .submit_plan(backend, work_plan, |work, frame, images| {
                    if work == plan.read {
                        frame
                            .copy_graph_image_to_readback(
                                images[plan.display.index()].image()?,
                                readback,
                            )
                            .map_err(string_error)?;
                    } else if let SceneWork::Raster(raster) = &plan.first_work {
                        if raster.record(
                            work,
                            frame,
                            images,
                            self.raster.as_ref().unwrap(),
                            &self.raster_geometry.as_ref().unwrap().2,
                            snapshot,
                            width,
                            height,
                        )? {
                            return Ok(());
                        }
                        if !plan.tone.record_exposure(
                            work,
                            frame,
                            images,
                            mapper,
                            flow.exposure_ev,
                        )? {
                            return Err("unknown raster graph work".into());
                        }
                    } else if Some(work)
                        == if samples == 0 {
                            match plan.first_work {
                                SceneWork::Trace(id) => Some(id),
                                _ => None,
                            }
                        } else {
                            plan.continued_trace
                        }
                    {
                        frame
                            .trace_paths(
                                self.tracer.as_ref().unwrap(),
                                &self.acceleration.as_ref().unwrap().2,
                                images[plan.scene.index()].image()?,
                                matrix,
                                snapshot.scene.environment,
                                samples,
                                snapshot.render.max_path_bounces,
                                seed,
                            )
                            .map_err(string_error)?;
                    } else if !plan.tone.record_exposure(
                        work,
                        frame,
                        images,
                        mapper,
                        flow.exposure_ev,
                    )? {
                        return Err("unknown path graph work".into());
                    }
                    Ok(())
                })?;
        }
        if let Some(history) = history {
            history.usage = slot.resources.usage(plan.scene);
            history.previous = Some(snapshot.clone());
            history.samples = samples + 1;
        }
        Ok(PendingViewport {
            slot: std::mem::take(slot),
            samples: if tracing { samples + 1 } else { 0 },
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

fn same_scene(scene: &Arc<Scene>, version: u64, snapshot: &RenderSnapshot) -> bool {
    version == snapshot.versions.scene && Arc::ptr_eq(scene, &snapshot.scene)
}

fn string_error(error: impl std::fmt::Display) -> String {
    error.to_string()
}

#[cfg(test)]
mod tests {
    use super::*;
    use bee_scene::SceneState;

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
        let (updated, events) =
            profile::capture(|| PreparedFlow::prepare(&graph, Some(&first)));
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
            1
        );
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
