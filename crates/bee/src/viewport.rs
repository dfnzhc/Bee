mod queue;

use crate::flow_editor::FlowEditor;
use crate::Bee;
use bee_renderer::Module;
use bee_scene::{DisplayParameters, SceneState, Versions};
use bee_vulkan::{Backend, DiagnosticLevel, DiagnosticSender, TestImage};
use gpui::{
    canvas, div, prelude::*, px, rgb, Bounds, Context, Corners, EventEmitter, MouseButton, Pixels,
    Point, RenderImage, Window,
};
use gpui_component::tooltip::Tooltip;
use queue::{Completed, RenderQueue, Request};
use std::cell::RefCell;
use std::rc::Rc;
use std::sync::Arc;
#[cfg(test)]
use std::time::Duration;

type Extent = (u32, u32);

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum RenderMode {
    Raster,
    Path,
}

pub(crate) struct ViewportChanged;
impl EventEmitter<ViewportChanged> for Bee {}

#[cfg(test)]
thread_local! {
    static PAINTED_IMAGES: std::cell::Cell<(usize, Option<(Bounds<Pixels>, gpui::ImageId)>)> =
        const { std::cell::Cell::new((0, None)) };
}

pub struct Viewport {
    backend: Option<Arc<Backend>>,
    diagnostics: DiagnosticSender,
    extent: Option<Extent>,
    versions: Option<Versions>,
    generation: u64,
    history_epoch: u64,
    displayed_sequence: u64,
    queue: Option<RenderQueue>,
    docked: bool,
    closing: bool,
    active: bool,
    zoom_hidden: bool,
    image: Option<Arc<RenderImage>>,
    painted: Rc<RefCell<Option<Arc<RenderImage>>>>,
    image_versions: Option<Versions>,
    error: Option<String>,
    drag_position: Option<Point<Pixels>>,
    mode: RenderMode,
    samples: u32,
    pub(crate) flow: FlowEditor,
    #[cfg(test)]
    render_delay: Option<Duration>,
}

impl Viewport {
    pub fn new(backend: Option<Arc<Backend>>, diagnostics: DiagnosticSender) -> Self {
        if backend.is_some() {
            diagnostics.send(
                DiagnosticLevel::Info,
                "Viewport",
                "Image presentation: async Vulkan readback",
            );
        }
        Self {
            backend,
            diagnostics,
            extent: None,
            versions: None,
            generation: 0,
            history_epoch: 0,
            displayed_sequence: 0,
            queue: None,
            docked: false,
            closing: false,
            active: true,
            zoom_hidden: false,
            image: None,
            painted: Rc::default(),
            image_versions: None,
            error: None,
            drag_position: None,
            mode: RenderMode::Raster,
            samples: 0,
            flow: FlowEditor::default(),
            #[cfg(test)]
            render_delay: None,
        }
    }

    fn notice(&self) -> Option<String> {
        if let Some(error) = &self.error {
            Some(format!("Viewport error: {error} | See Console"))
        } else if let Some(error) = self.flow.error() {
            Some(format!("{error} | See Render Flow"))
        } else if self.backend.is_none() {
            Some("Viewport unavailable | See Console".into())
        } else {
            None
        }
    }

    fn path_unavailable(&self) -> Option<String> {
        match &self.backend {
            None => Some("Vulkan device unavailable".into()),
            Some(backend) if !backend.report.ray_tracing_missing.is_empty() => {
                Some(backend.report.ray_tracing_missing.join(", "))
            }
            _ => None,
        }
    }

    fn mode_button(
        &self,
        mode: RenderMode,
        unavailable: Option<String>,
        cx: &Context<Bee>,
    ) -> impl IntoElement {
        let active = self.mode == mode;
        let (id, index, width) = match mode {
            RenderMode::Raster => ("raster-mode", 0, 104.0),
            RenderMode::Path => ("path-mode", 1, 156.0),
        };
        let label = match (mode, active, unavailable.is_some()) {
            (RenderMode::Raster, true, _) => "Raster (active)",
            (RenderMode::Raster, false, _) => "Raster",
            (RenderMode::Path, _, true) => "Path tracing unavailable",
            (RenderMode::Path, true, false) => "Path tracing (active)",
            (RenderMode::Path, false, false) => "Path tracing",
        };
        div()
            .id(id)
            .debug_selector(move || id.into())
            .tab_index(index)
            .cursor_pointer()
            .flex()
            .items_center()
            .justify_center()
            .w(px(width))
            .h(px(24.0))
            .flex_shrink_0()
            .rounded_sm()
            .border_1()
            .border_color(rgb(if active { 0x76c6b2 } else { 0x454e4c }))
            .bg(rgb(if active { 0x344841 } else { 0x292f2e }))
            .text_color(rgb(if unavailable.is_some() {
                0xb1bfba
            } else {
                0xe9efec
            }))
            .hover(|style| style.bg(rgb(0x3b4c47)))
            .focus(|style| style.border_color(rgb(0xffffff)))
            .when_some(unavailable, |button, reason| {
                let text = format!("Path tracing unavailable: {reason}");
                button.tooltip(move |window, cx| {
                    Tooltip::new(text.clone())
                        .max_w(px(400.0))
                        .build(window, cx)
                })
            })
            .child(label)
            .on_click(cx.listener(move |view, _, window, cx| {
                view.viewport.select_mode(mode, &mut view.state, window, cx);
            }))
            .on_key_down(
                cx.listener(move |view, event: &gpui::KeyDownEvent, window, cx| {
                    if matches!(event.keystroke.key.as_str(), "enter" | "space") {
                        view.viewport.select_mode(mode, &mut view.state, window, cx);
                    }
                }),
            )
    }

    pub fn render(&self, state: &SceneState, cx: &mut Context<Bee>) -> impl IntoElement {
        let extent = self.extent;
        let versions = state.versions();
        let requested_versions = self.versions;
        let generation = self.generation;
        let image = self.image.clone();
        let painted = self.painted.clone();
        let resize_view = cx.weak_entity();
        let error_view = resize_view.clone();
        div()
            .flex()
            .flex_col()
            .flex_1()
            .min_w_0()
            .min_h_0()
            .child(
                div()
                    .id("viewport-toolbar")
                    .debug_selector(|| "viewport-toolbar".into())
                    .flex()
                    .items_center()
                    .gap_1()
                    .px_1p5()
                    .h(px(30.0))
                    .flex_shrink_0()
                    .overflow_x_scroll()
                    .whitespace_nowrap()
                    .border_b_1()
                    .border_color(rgb(0x454e4c))
                    .text_size(px(12.0))
                    .tab_group()
                    .child(self.mode_button(RenderMode::Raster, None, cx))
                    .child(self.mode_button(RenderMode::Path, self.path_unavailable(), cx))
                    .when(self.mode == RenderMode::Path, |toolbar| {
                        toolbar.child(
                            div()
                                .id("viewport-samples")
                                .debug_selector(|| "viewport-samples".into())
                                .w(px(120.0))
                                .flex_shrink_0()
                                .pl_1()
                                .text_color(rgb(0xb1bfba))
                                .child(format!("{} samples", self.samples)),
                        )
                    }),
            )
            .child(
                div()
                    .id("scene-canvas")
                    .debug_selector(|| "scene-canvas".into())
                    .w_full()
                    .flex_1()
                    .min_h_0()
                    .relative()
                    .overflow_hidden()
                    .bg(rgb(0x111417))
                    .on_mouse_down(
                        MouseButton::Left,
                        cx.listener(|view, event: &gpui::MouseDownEvent, _, _| {
                            view.viewport.drag_position = Some(event.position);
                        }),
                    )
                    .on_mouse_move(
                        cx.listener(|view, event: &gpui::MouseMoveEvent, window, cx| {
                            let Some(previous) = view.viewport.drag_position else {
                                return;
                            };
                            if event.pressed_button != Some(MouseButton::Left) {
                                view.viewport.drag_position = None;
                                return;
                            }
                            view.viewport.drag_position = Some(event.position);
                            let horizontal = f32::from(event.position.x - previous.x);
                            let vertical = f32::from(event.position.y - previous.y);
                            let mut camera = view.state.camera();
                            if camera.orbit(horizontal, vertical) && view.state.set_camera(camera) {
                                view.viewport.synchronize(
                                    view.viewport.extent,
                                    &view.state,
                                    window,
                                    cx,
                                );
                            }
                        }),
                    )
                    .on_mouse_up(
                        MouseButton::Left,
                        cx.listener(|view, _: &gpui::MouseUpEvent, _, _| {
                            view.viewport.drag_position = None;
                        }),
                    )
                    .on_mouse_up_out(
                        MouseButton::Left,
                        cx.listener(|view, _: &gpui::MouseUpEvent, _, _| {
                            view.viewport.drag_position = None;
                        }),
                    )
                    .on_scroll_wheel(cx.listener(
                        |view, event: &gpui::ScrollWheelEvent, window, cx| {
                            let scroll = f32::from(event.delta.pixel_delta(px(20.0)).y);
                            let mut camera = view.state.camera();
                            if camera.zoom(scroll) && view.state.set_camera(camera) {
                                view.viewport.synchronize(
                                    view.viewport.extent,
                                    &view.state,
                                    window,
                                    cx,
                                );
                            }
                        },
                    ))
                    .child(
                        canvas(
                            move |bounds, window, cx| {
                                // 布局提供物理尺寸，也补齐尚未同步的场景状态。
                                let next = physical_extent(bounds, window.scale_factor());
                                if next != extent || requested_versions != Some(versions) {
                                    window.defer(cx, move |window, cx| {
                                        resize_view
                                            .update(cx, |view, cx| {
                                                if view.viewport.generation == generation
                                                    && view.viewport.active
                                                    && !view.viewport.zoom_hidden
                                                {
                                                    view.viewport.synchronize(
                                                        next,
                                                        &view.state,
                                                        window,
                                                        cx,
                                                    )
                                                }
                                            })
                                            .ok();
                                    });
                                }
                                (next == extent).then_some(image).flatten()
                            },
                            move |bounds, image, window, cx| {
                                // 退役的是已绘入 GPUI 场景的纹理，不是尚未绘制的 CPU 帧。
                                let mut next_painted = None;
                                if let Some(image) = image {
                                    let id = image.id;
                                    let result = window.paint_image(
                                        bounds,
                                        Corners::default(),
                                        image.clone(),
                                        0,
                                        false,
                                    );
                                    if result.is_ok() {
                                        next_painted = Some(image);
                                    }
                                    #[cfg(test)]
                                    if result.is_ok() {
                                        let visible =
                                            bounds.intersect(&window.content_mask().bounds);
                                        PAINTED_IMAGES
                                            .set((PAINTED_IMAGES.get().0 + 1, Some((visible, id))));
                                    }
                                    if let Err(error) = result {
                                        let reason = format!("GPUI image upload failed: {error:#}");
                                        window.defer(cx, move |window, cx| {
                                            error_view
                                                .update(cx, |view, cx| {
                                                    // 旧图像上传失败时不能清除已替换的新图像。
                                                    if view
                                                        .viewport
                                                        .image
                                                        .as_ref()
                                                        .map(|image| image.id)
                                                        == Some(id)
                                                    {
                                                        view.viewport.fail(reason, window);
                                                        cx.notify();
                                                    }
                                                })
                                                .ok();
                                        });
                                    }
                                }
                                let mut painted = painted.borrow_mut();
                                if painted.as_ref().map(|old| old.id) != next_painted.as_ref().map(|new| new.id) {
                                    if let Some(old) = std::mem::replace(&mut *painted, next_painted) {
                                        window.defer(cx, move |window, _| {
                                            if let Err(error) = window.drop_image(old) {
                                                eprintln!("Could not release GPUI image: {error:#}");
                                            }
                                        });
                                    }
                                }
                            },
                        )
                        .size_full(),
                    )
                    .when_some(self.notice(), |canvas, notice| {
                        let tooltip = notice.clone();
                        canvas.child(
                            div().absolute().left_2().right_2().top_2().flex().child(
                                div()
                                    .id("viewport-notice")
                                    .debug_selector(|| "viewport-notice".into())
                                    .min_w_0()
                                    .rounded_sm()
                                    .bg(rgb(0x34272a))
                                    .px_2()
                                    .py_1()
                                    .text_ellipsis()
                                    .text_size(px(12.0))
                                    .text_color(rgb(0xffa8a8))
                                    .tooltip(move |window, cx| {
                                        Tooltip::new(tooltip.clone())
                                            .max_w(px(400.0))
                                            .build(window, cx)
                                    })
                                    .child(notice),
                            ),
                        )
                    }),
            )
    }

    pub fn error(&self) -> Option<&str> {
        self.error.as_deref()
    }

    #[cfg(test)]
    pub(crate) fn extent(&self) -> Option<Extent> {
        self.extent
    }

    fn select_mode(
        &mut self,
        mode: RenderMode,
        state: &mut SceneState,
        window: &mut Window,
        cx: &mut Context<Bee>,
    ) {
        if self.mode == mode {
            return;
        }
        let source = match mode {
            RenderMode::Raster => Module::Rasterization,
            RenderMode::Path => Module::PathTracing,
        };
        match self.flow.select_source(source) {
            Ok(()) => self.apply_flow(state, window, cx),
            Err(error) => {
                eprintln!("Render Flow: {error}");
                self.diagnostics
                    .send(DiagnosticLevel::Error, "Render Flow", error);
                cx.notify();
            }
        }
    }

    pub(crate) fn apply_flow(
        &mut self,
        state: &mut SceneState,
        window: &mut Window,
        cx: &mut Context<Bee>,
    ) {
        let unavailable = self.path_unavailable();
        let previous = self.flow.active().clone();
        let graph = match self.flow.apply(unavailable) {
            Ok(graph) => graph,
            Err(error) => {
                eprintln!("Render Flow: {error}");
                self.diagnostics
                    .send(DiagnosticLevel::Error, "Render Flow", error);
                cx.notify();
                return;
            }
        };
        if graph == previous {
            cx.notify();
            return;
        }
        let exposure_ev = self.flow.prepared().exposure_ev;
        let mode = match self.flow.prepared().source() {
            Module::Rasterization => RenderMode::Raster,
            Module::PathTracing => RenderMode::Path,
            _ => unreachable!("validated scene source"),
        };
        let mode_changed = self.mode != mode;
        self.mode = mode;
        state.set_display_parameters(DisplayParameters { exposure_ev });
        self.versions = Some(state.versions());
        self.generation = self.generation.wrapping_add(1);
        if mode_changed {
            self.history_epoch += 1;
            self.drop_image();
        }
        self.error = None;
        self.start(state, window, cx);
        cx.notify();
    }

    pub(crate) fn set_active(
        &mut self,
        active: bool,
        state: &SceneState,
        window: &mut Window,
        cx: &mut Context<Bee>,
    ) {
        if self.active != active {
            self.active = active;
            self.visibility_changed(state, window, cx);
        }
    }

    pub(crate) fn request_close(&mut self, _window: &mut Window) -> bool {
        self.closing = true;
        self.drop_image();
        // 窗口可能仍呈现缓存的场景；纹理随窗口一起释放。
        self.painted.borrow_mut().take();
        self.backend = None;
        if let Some(queue) = &self.queue {
            queue.close();
            false
        } else {
            true
        }
    }

    pub(crate) fn set_zoom_hidden(
        &mut self,
        hidden: bool,
        state: &SceneState,
        window: &mut Window,
        cx: &mut Context<Bee>,
    ) {
        if self.zoom_hidden != hidden {
            self.zoom_hidden = hidden;
            self.visibility_changed(state, window, cx);
        }
    }

    fn visibility_changed(
        &mut self,
        state: &SceneState,
        window: &mut Window,
        cx: &mut Context<Bee>,
    ) {
        self.generation = self.generation.wrapping_add(1);
        if !self.active || self.zoom_hidden {
            self.drag_position = None;
            self.synchronize(None, state, window, cx);
        }
        self.notify_frame(cx);
    }

    fn synchronize(
        &mut self,
        extent: Option<Extent>,
        state: &SceneState,
        window: &mut Window,
        cx: &mut Context<Bee>,
    ) {
        let versions = state.versions();
        if self.extent == extent && self.versions == Some(versions) {
            return;
        }

        // 尺寸变化时释放旧图像；状态变化期间保留上一帧。
        if self.extent != extent {
            self.drop_image();
            self.extent = extent;
            self.generation = self.generation.wrapping_add(1);
            self.history_epoch += 1;
        }
        self.versions = Some(versions);
        self.error = None;
        self.start(state, window, cx);
        self.notify_frame(cx);
    }

    fn start(&mut self, state: &SceneState, window: &mut Window, cx: &mut Context<Bee>) {
        if self.closing {
            return;
        }
        let (Some(backend), Some(extent)) = (&self.backend, self.extent) else {
            if let Some(queue) = &self.queue {
                queue.replace(None);
            }
            return;
        };
        if !self.active || self.zoom_hidden {
            if let Some(queue) = &self.queue {
                queue.replace(None);
            }
            return;
        }
        let queue = self.queue.get_or_insert_with(|| {
            let (queue, ready) = RenderQueue::new(backend.clone(), cx.background_executor());
            cx.spawn_in(window, async move |view, cx| {
                while ready.recv().await.is_ok() {
                    let alive = view
                        .update_in(cx, |view, window, cx| {
                            let viewport = &mut view.viewport;
                            if let Some(completed) =
                                viewport.queue.as_ref().and_then(RenderQueue::take)
                            {
                                // 状态变更可能先于布局回调，丢弃旧帧前补交最新输入。
                                viewport.synchronize(viewport.extent, &view.state, window, cx);
                                viewport.accept(completed, view.state.versions(), window, cx);
                            }
                        })
                        .is_ok();
                    if !alive {
                        return;
                    }
                }
                let _ = view.update_in(cx, |view, window, _| {
                    view.viewport.queue = None;
                    if view.viewport.closing {
                        window.remove_window();
                    }
                });
            })
            .detach();
            queue
        });
        queue.replace(Some(Request {
            snapshot: state.snapshot(),
            flow: self.flow.prepared().clone(),
            extent,
            generation: self.generation,
            history_epoch: self.history_epoch,
            accumulate: self.mode == RenderMode::Path,
            #[cfg(test)]
            delay: self.render_delay,
        }));
    }

    fn accept(
        &mut self,
        completed: Completed,
        current: Versions,
        window: &mut Window,
        cx: &mut Context<Bee>,
    ) {
        let versions = completed.request.snapshot.versions;
        // 允许相机逐步追上输入，禁止旧代际或乱序完成回退画面。
        if self.closing
            || !self.active
            || self.zoom_hidden
            || completed.request.generation != self.generation
            || completed.sequence <= self.displayed_sequence
            || (versions.scene, versions.render, versions.display)
                != (current.scene, current.render, current.display)
            || self
                .image_versions
                .is_some_and(|shown| versions.camera < shown.camera)
        {
            return;
        }
        match completed.image {
            Ok(image) => {
                self.drop_image();
                self.displayed_sequence = completed.sequence;
                self.samples = completed.samples;
                self.image_versions = Some(versions);
                self.image = Some(image);
                self.error = None;
                self.notify_frame(cx);
            }
            Err(reason) if versions == current => {
                self.fail(format!("Vulkan image readback failed: {reason}"), window);
                self.notify_frame(cx);
            }
            _ => {}
        }
    }

    pub(crate) fn set_docked(&mut self) {
        self.docked = true;
    }

    fn notify_frame(&self, cx: &mut Context<Bee>) {
        if self.docked {
            cx.emit(ViewportChanged);
        } else {
            cx.notify();
        }
    }

    #[cfg(test)]
    fn running(&self) -> bool {
        self.queue.as_ref().is_some_and(RenderQueue::running)
    }

    fn fail(&mut self, reason: String, _window: &mut Window) {
        eprintln!("{reason}");
        self.diagnostics
            .send(DiagnosticLevel::Error, "Viewport", reason.clone());
        self.drop_image();
        self.error = Some(reason);
    }

    fn drop_image(&mut self) {
        self.image_versions = None;
        self.samples = 0;
        self.image = None;
    }
}

fn physical_extent(bounds: Bounds<Pixels>, scale: f32) -> Option<Extent> {
    let width = (f32::from(bounds.size.width) * scale).ceil() as u32;
    let height = (f32::from(bounds.size.height) * scale).ceil() as u32;
    (width > 0 && height > 0).then_some((width, height))
}

fn render_image(image: TestImage) -> Result<Arc<RenderImage>, String> {
    let row_bytes = (image.width as usize)
        .checked_mul(4)
        .ok_or("Image row size overflow")?;
    let bytes = row_bytes
        .checked_mul(image.height as usize)
        .ok_or("Image size overflow")?;
    let source_bytes = image
        .row_stride
        .checked_mul(image.height as usize)
        .ok_or("Image stride overflow")?;
    if image.width == 0
        || image.height == 0
        || image.row_stride < row_bytes
        || image.bgra.len() < source_bytes
    {
        return Err("Invalid Vulkan image dimensions or stride".into());
    }

    let mut packed = image.bgra;
    if image.row_stride != row_bytes {
        bee_vulkan::profile::record("pixel_repack", 1.0);
        for row in 1..image.height as usize {
            let offset = row * image.row_stride;
            packed.copy_within(offset..offset + row_bytes, row * row_bytes);
        }
    }
    packed.truncate(bytes);
    // 尽管 image 的缓冲区类型为 RGBA，GPUI 仍将原始字节解释为 BGRA。
    let buffer = image::RgbaImage::from_raw(image.width, image.height, packed)
        .ok_or("Could not create GPUI image")?;
    Ok(Arc::new(RenderImage::new(vec![image::Frame::new(buffer)])))
}

#[cfg(test)]
mod tests {
    use super::*;
    use gpui::{point, px, size, Modifiers, ScrollDelta, ScrollWheelEvent};

    const FRAME_DELAY: Duration = Duration::from_millis(5);

    fn finish_frame(cx: &mut gpui::VisualTestContext) {
        cx.executor().advance_clock(FRAME_DELAY);
        cx.run_until_parked();
    }

    #[test]
    fn readback_removes_padding_without_changing_bgra() {
        let image = TestImage {
            width: 2,
            height: 2,
            row_stride: 12,
            bgra: vec![
                0, 0, 255, 255, 0, 255, 0, 255, 9, 9, 9, 9, 255, 0, 0, 255, 255, 255, 255, 255, 8,
                8, 8, 8,
            ],
        };
        let rendered = render_image(image).unwrap();
        assert_eq!(
            rendered.as_bytes(0).unwrap(),
            &[0, 0, 255, 255, 0, 255, 0, 255, 255, 0, 0, 255, 255, 255, 255, 255]
        );
        assert_eq!(rendered.size(0).width.0, 2);
        assert_eq!(rendered.size(0).height.0, 2);
        assert!(render_image(TestImage {
            width: 2,
            height: 2,
            row_stride: 7,
            bgra: vec![0; 14],
        })
        .is_err());
    }

    #[test]
    fn runtime_errors_reach_console_without_removing_local_errors_or_active_flow() {
        let mut app = gpui::TestAppContext::single();
        let (view, cx) = app.add_window_view(|_, _| Bee::failure("test device".into()));
        view.update_in(cx, |view, window, cx| {
            let active = view.viewport.flow.active().clone();
            view.viewport
                .fail("Vulkan image readback failed: simulated".into(), window);
            assert!(view.viewport.notice().unwrap().contains("See Console"));
            view.viewport
                .flow
                .select_source(Module::PathTracing)
                .unwrap();
            view.viewport.apply_flow(&mut view.state, window, cx);
            assert_eq!(view.viewport.flow.active(), &active);
            assert!(view
                .viewport
                .flow
                .error()
                .unwrap()
                .contains("Vulkan device unavailable"));
            assert!(view.viewport.error().unwrap().contains("readback failed"));
            view.console.drain();
            for (source, text) in [
                ("Viewport", "readback failed"),
                ("Render Flow", "Vulkan device unavailable"),
            ] {
                assert!(view
                    .console
                    .visible()
                    .any(|message| message.level == DiagnosticLevel::Error
                        && message.source == source
                        && message.message.contains(text)));
            }
        });
    }

    #[test]
    fn invalid_viewport_extent_is_not_rendered() {
        assert_eq!(
            physical_extent(
                Bounds::new(point(px(0.0), px(0.0)), size(px(17.0), px(9.0))),
                2.0
            ),
            Some((34, 18))
        );
        assert_eq!(
            physical_extent(
                Bounds::new(point(px(0.0), px(0.0)), size(px(0.0), px(9.0))),
                1.0
            ),
            None
        );
    }

    #[test]
    #[ignore = "requires a Vulkan graphics device"]
    fn workbench_viewport_renders_after_layout_and_resize() {
        use gpui::VisualTestContext;
        use gpui_component::dock::ToggleZoom;

        let mut app = gpui::TestAppContext::single();
        app.update(|cx| {
            gpui_component::init(cx);
            crate::workbench::init_theme(cx);
        });
        let mut bee = None;
        let (root, cx) = app.add_window_view(|window, cx| {
            let view = cx.new(|_| Bee::new());
            bee = Some(view.clone());
            let workbench = cx.new(|cx| crate::workbench::Workbench::new(view, window, cx));
            gpui_component::Root::new(workbench, window, cx)
        });
        let bee = bee.unwrap();
        let redraw = |cx: &mut VisualTestContext, dimensions| {
            cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
        };
        let assert_frame = |cx: &mut VisualTestContext, dimensions| {
            redraw(cx, dimensions);
            cx.run_until_parked();
            let previous_paints = PAINTED_IMAGES.get().0;
            redraw(cx, dimensions);
            let canvas = cx.debug_bounds("scene-canvas").unwrap();
            let scale = cx.update(|window, _| window.scale_factor());
            let expected = physical_extent(canvas, scale).expect("drawable canvas");
            cx.update(|_, cx| {
                let bee = bee.read(cx);
                let viewport = &bee.viewport;
                assert!(viewport.error.is_none(), "{:?}", viewport.error);
                assert_eq!(viewport.extent, Some(expected));
                assert_eq!(viewport.image_versions, Some(bee.state.versions()));
                let image = viewport.image.as_ref().expect("Vulkan readback image");
                assert_eq!(image.size(0).width.0 as u32, expected.0);
                assert_eq!(image.size(0).height.0 as u32, expected.1);
                let pixels = image.as_bytes(0).unwrap();
                assert!(pixels
                    .chunks_exact(4)
                    .any(|pixel| pixel[2] > pixel[0].saturating_add(20)));
                assert!(pixels
                    .chunks_exact(4)
                    .any(|pixel| pixel[0] > pixel[2].saturating_add(20)));
            });
            let (paints, painted_image) = PAINTED_IMAGES.get();
            assert!(paints > previous_paints);
            assert_eq!(painted_image.unwrap().0, canvas);
            expected
        };
        let click = |cx: &mut VisualTestContext, selector| {
            let target = cx.debug_bounds(selector).expect(selector).center();
            cx.simulate_click(target, Modifiers::none());
        };

        let large = size(px(1200.0), px(800.0));
        cx.simulate_resize(large);
        let default_extent = assert_frame(cx, large);
        let painted = PAINTED_IMAGES.get().1.unwrap().0.size;
        let width = f32::from(painted.width);
        let height = f32::from(painted.height);
        let coverage = (width * height) / (f32::from(large.width) * f32::from(large.height));
        println!(
            "Default viewport image: {width:.1} x {height:.1} logical pixels, {:.2}% of the client area",
            coverage * 100.0
        );
        assert!(coverage >= 0.5);

        let small = size(px(960.0), px(640.0));
        cx.simulate_resize(small);
        let closed = assert_frame(cx, small);
        click(cx, "drawer-console");
        let open = assert_frame(cx, small);
        assert_eq!(open.0, closed.0);
        assert!(open.1 < closed.1);
        click(cx, "drawer-console");
        assert_eq!(assert_frame(cx, small), closed);

        click(cx, "tab-scene");
        cx.dispatch_action(ToggleZoom);
        redraw(cx, small);
        cx.run_until_parked();
        cx.update(|_, cx| {
            let viewport = &bee.read(cx).viewport;
            assert!(viewport.extent.is_none());
            assert!(viewport.image.is_none());
            assert!(!viewport.running());
        });
        cx.dispatch_action(ToggleZoom);
        assert_eq!(assert_frame(cx, small), closed);

        cx.simulate_resize(large);
        assert_eq!(assert_frame(cx, large), default_extent);
    }

    #[test]
    #[ignore = "requires a ray tracing device and Vulkan validation layer"]
    fn docked_viewport_paints_camera_progress_during_drag() {
        for mode in [RenderMode::Raster, RenderMode::Path] {
            let mut app = gpui::TestAppContext::single();
            app.update(|cx| {
                gpui_component::init(cx);
                crate::workbench::init_theme(cx);
            });
            let mut bee = None;
            let (root, cx) = app.add_window_view(|window, cx| {
                let view = cx.new(|_| Bee::new());
                bee = Some(view.clone());
                let workbench = cx.new(|cx| crate::workbench::Workbench::new(view, window, cx));
                gpui_component::Root::new(workbench, window, cx)
            });
            let bee = bee.unwrap();
            let dimensions = size(px(960.0), px(640.0));
            let redraw = |cx: &mut gpui::VisualTestContext| {
                cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
            };
            cx.simulate_resize(dimensions);
            redraw(cx);
            cx.run_until_parked();
            if mode == RenderMode::Path {
                let button = cx.debug_bounds("path-mode").unwrap().center();
                cx.simulate_click(button, Modifiers::none());
            }
            bee.update_in(cx, |bee, _, _| {
                let viewport = &mut bee.viewport;
                assert_eq!(viewport.mode, mode);
                assert_eq!(
                    viewport.backend.as_ref().unwrap().report.validation,
                    bee_vulkan::ValidationStatus::Enabled
                );
                assert!(viewport.image.is_some());
                viewport.render_delay = Some(FRAME_DELAY * 4);
            });

            let start = cx.debug_bounds("scene-canvas").unwrap().center();
            cx.simulate_mouse_move(start, None, Modifiers::none());
            cx.simulate_mouse_down(start, MouseButton::Left, Modifiers::none());
            let drag = |cx: &mut gpui::VisualTestContext, offset| {
                let position = point(start.x + px(offset), start.y + px(offset / 2.0));
                cx.simulate_mouse_move(position, MouseButton::Left, Modifiers::none());
                cx.update(|_, cx| {
                    let bee = bee.read(cx);
                    assert!(bee.viewport.running());
                    (bee.state.versions(), position)
                })
            };
            let paint = |cx: &mut gpui::VisualTestContext, versions, position| {
                let image = cx.update(|_, cx| {
                    let viewport = &bee.read(cx).viewport;
                    assert_eq!(viewport.drag_position, Some(position));
                    assert_eq!(viewport.image_versions, Some(versions));
                    assert!(viewport.error.is_none(), "{:?}", viewport.error);
                    viewport.image.as_ref().unwrap().clone()
                });
                let previous_paints = PAINTED_IMAGES.get().0;
                redraw(cx);
                let (paints, painted_image) = PAINTED_IMAGES.get();
                assert!(paints > previous_paints);
                assert_eq!(
                    painted_image,
                    Some((cx.debug_bounds("scene-canvas").unwrap(), image.id))
                );
                image
            };

            let in_flight = |cx: &mut gpui::VisualTestContext| {
                cx.update(|_, cx| bee.read(cx).viewport.queue.as_ref().unwrap().in_flight())
            };

            // 留住第一个完成回调，其余请求只能用第二个槽推进。
            drag(cx, 18.0);
            assert_eq!(in_flight(cx), 1);
            bee.update_in(cx, |bee, _, _| {
                bee.viewport.render_delay = Some(FRAME_DELAY)
            });
            let (second, position) = drag(cx, 36.0);
            assert_eq!(in_flight(cx), 2);
            finish_frame(cx);
            assert_eq!(in_flight(cx), 1);
            let second_image = paint(cx, second, position);

            let (third, _) = drag(cx, 54.0);
            drag(cx, 72.0);
            let (latest, position) = drag(cx, 90.0);
            assert_eq!(in_flight(cx), 2);
            finish_frame(cx);
            assert_eq!(in_flight(cx), 2);
            cx.update(|_, cx| assert_eq!(bee.read(cx).viewport.image_versions, Some(third)));
            finish_frame(cx);
            let latest_image = paint(cx, latest, position);
            assert!(second_image.as_bytes(0) != latest_image.as_bytes(0));
            assert_eq!(in_flight(cx), 1);
            finish_frame(cx);
            cx.update(|_, cx| assert!(!bee.read(cx).viewport.running()));
            assert_eq!(paint(cx, latest, position).id, latest_image.id);
            cx.simulate_mouse_up(position, MouseButton::Left, Modifiers::none());
            bee.update_in(cx, |bee, window, _| bee.viewport.request_close(window));
            cx.run_until_parked();
            cx.quit();
        }
    }

    #[test]
    fn workbench_close_without_vulkan_is_immediate() {
        let mut app = gpui::TestAppContext::single();
        app.update(|cx| {
            gpui_component::init(cx);
            crate::workbench::init_theme(cx);
        });
        let (_workbench, cx) = app.add_window_view(|window, cx| {
            let bee = cx.new(|_| Bee::failure("test device".into()));
            crate::workbench::Workbench::new(bee, window, cx)
        });
        assert!(cx.simulate_close());
        assert!(cx.simulate_close());
    }

    #[test]
    fn file_exit_without_vulkan_closes_the_window() {
        let mut app = gpui::TestAppContext::single();
        app.update(|cx| {
            gpui_component::init(cx);
            crate::workbench::init_theme(cx);
        });
        let (workbench, cx) = app.add_window_view(|window, cx| {
            let bee = cx.new(|_| Bee::failure("test device".into()));
            crate::workbench::Workbench::new(bee, window, cx)
        });
        let dimensions = size(px(960.0), px(640.0));
        cx.simulate_resize(dimensions);
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| {
            workbench.clone()
        });
        let file = cx.debug_bounds("file-menu").unwrap().center();
        cx.simulate_click(file, Modifiers::none());
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| {
            workbench.clone()
        });
        assert!(cx.debug_bounds("exit-bee").is_some());
        cx.simulate_keystrokes("down enter");
        cx.cx.update(|cx| assert!(cx.windows().is_empty()));
    }

    #[test]
    #[ignore = "requires a ray tracing device and Vulkan validation layer"]
    fn workbench_close_releases_resources_before_process_exit() {
        if std::env::var_os("BEE_TEST_CLOSE_CHILD").is_none() {
            let output = std::process::Command::new(std::env::current_exe().unwrap())
                .args([
                    "--exact",
                    "viewport::tests::workbench_close_releases_resources_before_process_exit",
                    "--ignored",
                    "--nocapture",
                ])
                .env("BEE_TEST_CLOSE_CHILD", "1")
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

        for (mode, from_menu) in [
            (RenderMode::Raster, false),
            (RenderMode::Path, false),
            (RenderMode::Raster, true),
            (RenderMode::Path, true),
        ] {
            for in_flight in [false, true] {
                let mut app = gpui::TestAppContext::single();
                app.update(|cx| {
                    gpui_component::init(cx);
                    crate::workbench::init_theme(cx);
                });
                let mut bee = None;
                let (root, cx) = app.add_window_view(|window, cx| {
                    let view = cx.new(|_| Bee::new());
                    bee = Some(view.clone());
                    let workbench = cx.new(|cx| crate::workbench::Workbench::new(view, window, cx));
                    gpui_component::Root::new(workbench, window, cx)
                });
                let bee = bee.unwrap();
                let dimensions = size(px(960.0), px(640.0));
                cx.simulate_resize(dimensions);
                cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
                cx.run_until_parked();
                let backend = cx.update(|_, cx| {
                    let backend = bee.read(cx).viewport.backend.as_ref().unwrap();
                    assert_eq!(
                        backend.report.validation,
                        bee_vulkan::ValidationStatus::Enabled
                    );
                    assert!(backend.report.ray_tracing_missing.is_empty());
                    Arc::downgrade(backend)
                });
                if mode == RenderMode::Path {
                    let button = cx.debug_bounds("path-mode").unwrap().center();
                    cx.simulate_click(button, Modifiers::none());
                    cx.run_until_parked();
                }
                let file_menu_window = from_menu.then(|| {
                    let file = cx.debug_bounds("file-menu").unwrap().center();
                    cx.simulate_click(file, Modifiers::none());
                    cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
                    cx.simulate_keystrokes("down");
                    cx.update(|window, _| window.window_handle())
                });
                bee.update_in(cx, |bee, window, cx| {
                    assert_eq!(bee.viewport.mode, mode);
                    assert!(bee.viewport.error.is_none(), "{:?}", bee.viewport.error);
                    assert!(!bee.viewport.running());
                    assert!(bee.viewport.image.is_some());
                    if mode == RenderMode::Path {
                        assert!(bee.viewport.queue.is_some());
                    }
                    if in_flight {
                        bee.viewport.render_delay = Some(FRAME_DELAY);
                        bee.viewport.start(&bee.state, window, cx);
                        assert!(bee.viewport.running());
                    }
                });
                cx.run_until_parked();

                if let Some(window) = file_menu_window {
                    // 直接发送确认键，检查窗口状态后再让后台任务运行。
                    cx.cx
                        .dispatch_keystroke(window, gpui::Keystroke::parse("enter").unwrap());
                    cx.cx.update(|cx| {
                        let viewport = &bee.read(cx).viewport;
                        assert!(viewport.closing);
                        assert_eq!(viewport.running(), in_flight);
                        assert_eq!(cx.windows().len(), 1);
                    });
                } else {
                    assert!(!cx.simulate_close());
                }
                cx.run_until_parked();
                if in_flight {
                    cx.cx.update(|cx| {
                        assert!(bee.read(cx).viewport.running());
                        assert_eq!(cx.windows().len(), 1);
                    });
                    assert!(backend.upgrade().is_some());
                }
                cx.executor().advance_clock(Duration::from_millis(120));
                cx.run_until_parked();
                cx.cx.update(|cx| {
                    // 界面实体仍被测试持有时，也必须完成 GPU 清理。
                    let viewport = &bee.read(cx).viewport;
                    assert!(!viewport.running());
                    assert!(viewport.backend.is_none());
                    assert!(viewport.queue.is_none());
                    assert!(viewport.image.is_none());
                    assert!(cx.windows().is_empty());
                });
                assert!(backend.upgrade().is_none());
                drop(root);
                drop(bee);
                cx.quit();
            }
        }
    }

    #[test]
    #[ignore = "requires a ray tracing device and Vulkan validation layer"]
    fn workbench_selection_preserves_path_samples() {
        let mut app = gpui::TestAppContext::single();
        app.update(|cx| {
            gpui_component::init(cx);
            crate::workbench::init_theme(cx);
        });
        let mut bee = None;
        let (root, cx) = app.add_window_view(|window, cx| {
            let view = cx.new(|_| Bee::new());
            bee = Some(view.clone());
            let workbench = cx.new(|cx| crate::workbench::Workbench::new(view, window, cx));
            gpui_component::Root::new(workbench, window, cx)
        });
        let bee = bee.unwrap();
        let dimensions = size(px(960.0), px(640.0));
        cx.simulate_resize(dimensions);
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
        cx.run_until_parked();
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
        let path = cx.debug_bounds("path-mode").unwrap().center();
        cx.simulate_click(path, Modifiers::none());
        cx.run_until_parked();
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
        cx.run_until_parked();
        let (extent, generation, versions, mut samples) = cx.update(|_, cx| {
            let bee = bee.read(cx);
            assert_eq!(bee.viewport.mode, RenderMode::Path);
            let graph = bee.viewport.flow.active();
            let (source, _) = graph.display_source().unwrap();
            assert_eq!(graph.module(source), Some(Module::PathTracing));
            assert!(bee.viewport.error.is_none(), "{:?}", bee.viewport.error);
            assert!(bee.viewport.samples > 0);
            (
                bee.viewport.extent,
                bee.viewport.generation,
                bee.state.versions(),
                bee.viewport.samples,
            )
        });
        let toolbar = cx.debug_bounds("viewport-toolbar").unwrap();
        let path = cx.debug_bounds("path-mode").unwrap();
        let sample_count = cx.debug_bounds("viewport-samples").unwrap();
        assert!(sample_count.left() >= path.right() && sample_count.right() <= toolbar.right());
        assert!(sample_count.top() >= toolbar.top() && sample_count.bottom() <= toolbar.bottom());

        bee.update_in(cx, |bee, _, cx| {
            assert!(bee
                .viewport
                .flow
                .apply(Some("simulated missing ray tracing".into()))
                .is_err());
            let notice = bee.viewport.notice().unwrap();
            assert!(notice.contains("simulated missing ray tracing"));
            cx.notify();
        });
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
        cx.run_until_parked();
        assert_eq!(cx.debug_bounds("viewport-samples"), Some(sample_count));
        assert!(cx.debug_bounds("viewport-notice").is_some());
        cx.update(|_, cx| {
            let bee = bee.read(cx);
            assert_eq!(bee.viewport.extent, extent);
            assert_eq!(bee.viewport.generation, generation);
            assert!(bee.viewport.samples >= samples);
        });
        bee.update_in(cx, |bee, _, cx| {
            bee.viewport.flow.apply(None).unwrap();
            cx.notify();
        });

        for (selector, property) in [
            ("scene-instance-0", "property-albedo"),
            ("scene-light-0", "property-intensity"),
            ("scene-camera", "property-fov"),
            ("scene-clear", "properties-empty-0"),
        ] {
            let bounds = cx.debug_bounds(selector).expect(selector);
            cx.simulate_click(bounds.center(), Modifiers::none());
            cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
            cx.run_until_parked();
            assert!(cx.debug_bounds(property).is_some(), "{property}");
            cx.update(|_, cx| {
                let bee = bee.read(cx);
                assert_eq!(bee.viewport.extent, extent);
                assert_eq!(bee.viewport.generation, generation);
                assert_eq!(bee.state.versions(), versions);
                assert!(bee.viewport.samples >= samples);
            });
            cx.executor().advance_clock(Duration::from_millis(17));
            cx.run_until_parked();
            let next = cx.update(|_, cx| bee.read(cx).viewport.samples);
            assert!(next > samples, "{selector}: {samples} -> {next}");
            samples = next;
        }

        // 在途样本完成后保持暂停，恢复时继续使用当前场景和流程。
        bee.update_in(cx, |bee, window, cx| {
            bee.viewport.start(&bee.state, window, cx);
            assert!(bee.viewport.running());
            bee.viewport.set_active(false, &bee.state, window, cx);
        });
        cx.run_until_parked();
        cx.executor().advance_clock(Duration::from_millis(50));
        cx.run_until_parked();
        cx.update(|_, cx| {
            let viewport = &bee.read(cx).viewport;
            assert!(!viewport.running());
            assert!(viewport.extent.is_none());
            assert!(viewport.image.is_none());
            assert_eq!(viewport.samples, 0);
        });
        bee.update_in(cx, |bee, window, cx| {
            bee.viewport.set_active(true, &bee.state, window, cx);
        });
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
        cx.run_until_parked();
        cx.update(|_, cx| {
            let viewport = &bee.read(cx).viewport;
            assert_eq!(viewport.mode, RenderMode::Path);
            assert!(viewport.image.is_some());
            assert!(viewport.samples > 0);
        });

        // 按文档通过流程面板切换渲染源，再用快捷按钮切回。
        let flow_entry = cx.debug_bounds("drawer-flow").unwrap().center();
        cx.simulate_click(flow_entry, Modifiers::none());
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
        cx.run_until_parked();
        let click_flow = |cx: &mut gpui::VisualTestContext, selector: &'static str| {
            let panel = cx.debug_bounds("panel-content-flow").unwrap();
            for _ in 0..40 {
                let target = cx.debug_bounds(selector).expect(selector).center();
                if target.y > panel.top() && target.y < panel.bottom() {
                    cx.simulate_click(target, Modifiers::none());
                    cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
                    cx.run_until_parked();
                    return;
                }
                let direction = if target.y < panel.top() { 1.0 } else { -1.0 };
                cx.simulate_event(ScrollWheelEvent {
                    position: panel.center(),
                    delta: ScrollDelta::Lines(point(0.0, direction)),
                    ..Default::default()
                });
                cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
                cx.run_until_parked();
            }
            panic!("could not scroll to {selector}");
        };
        click_flow(cx, "flow-output-0");
        click_flow(cx, "flow-input-2");
        click_flow(cx, "flow-apply");
        cx.update(|_, cx| {
            let viewport = &bee.read(cx).viewport;
            let graph = viewport.flow.active();
            let (source, _) = graph.display_source().unwrap();
            assert_eq!(graph.module(source), Some(Module::Rasterization));
            assert_eq!(viewport.mode, RenderMode::Raster);
            assert!(viewport.image.is_some());
        });

        let path = cx.debug_bounds("path-mode").unwrap().center();
        cx.simulate_click(path, Modifiers::none());
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
        cx.run_until_parked();
        cx.update(|_, cx| {
            let viewport = &bee.read(cx).viewport;
            let graph = viewport.flow.active();
            let (source, _) = graph.display_source().unwrap();
            assert_eq!(graph.module(source), Some(Module::PathTracing));
            assert_eq!(viewport.mode, RenderMode::Path);
            assert!(viewport.image.is_some());
        });

        // 停止采样并消化已安排的定时器后再释放测试窗口。
        let raster = cx.debug_bounds("raster-mode").unwrap().center();
        cx.simulate_click(raster, Modifiers::none());
        cx.run_until_parked();
        cx.executor().advance_clock(Duration::from_millis(17));
        cx.run_until_parked();
        cx.update(|_, cx| {
            let viewport = &bee.read(cx).viewport;
            assert_eq!(viewport.mode, RenderMode::Raster);
            let graph = viewport.flow.active();
            let (source, _) = graph.display_source().unwrap();
            assert_eq!(graph.module(source), Some(Module::Rasterization));
            assert!(!viewport.running());
        });
        drop(root);
        drop(bee);
        cx.quit();
    }

    #[test]
    fn viewport_mouse_input_changes_camera() {
        let mut app = gpui::TestAppContext::single();
        let (view, cx) = app.add_window_view(|_, _| Bee::failure("test device".into()));
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(700.0), px(500.0)),
            |_, _| view.clone(),
        );

        let start = point(px(100.0), px(150.0));
        let end = point(px(150.0), px(180.0));
        let bounds = cx.debug_bounds("scene-canvas").unwrap();
        assert!(bounds.contains(&end), "viewport bounds: {bounds:?}");
        let initial = cx.update(|_, cx| view.read(cx).state.camera());
        cx.simulate_mouse_move(start, None, Modifiers::none());
        cx.simulate_mouse_down(start, MouseButton::Left, Modifiers::none());
        assert_eq!(
            cx.update(|_, cx| view.read(cx).viewport.drag_position),
            Some(start),
            "viewport bounds: {:?}",
            cx.debug_bounds("scene-canvas")
        );
        cx.simulate_mouse_move(end, MouseButton::Left, Modifiers::none());
        assert_eq!(
            cx.update(|_, cx| view.read(cx).viewport.drag_position),
            Some(end)
        );
        let rotated = cx.update(|_, cx| view.read(cx).state.camera());
        assert_ne!(rotated.position(), initial.position());
        assert_ne!(
            rotated.view_transform().unwrap(),
            initial.view_transform().unwrap()
        );

        cx.simulate_mouse_up(
            point(px(690.0), px(480.0)),
            MouseButton::Left,
            Modifiers::none(),
        );
        cx.simulate_mouse_move(start, None, Modifiers::none());
        assert_eq!(cx.update(|_, cx| view.read(cx).state.camera()), rotated);
        cx.simulate_event(ScrollWheelEvent {
            position: start,
            delta: ScrollDelta::Lines(point(0.0, 1.0)),
            ..Default::default()
        });
        let zoomed = cx.update(|_, cx| view.read(cx).state.camera());
        assert!(zoomed.distance < rotated.distance);
        assert_ne!(
            zoomed.view_transform().unwrap(),
            rotated.view_transform().unwrap()
        );
        assert_eq!(cx.update(|_, cx| view.read(cx).state.versions().camera), 2);
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(700.0), px(500.0)),
            |_, _| view.clone(),
        );
        cx.run_until_parked();
        cx.update(|_, cx| {
            let view = view.read(cx);
            assert_eq!(view.viewport.versions, Some(view.state.versions()));
        });
    }

    #[test]
    #[ignore = "requires a ray tracing device and Vulkan validation layer"]
    fn gpui_flow_connections_and_exposure_drive_rendering() {
        if std::env::var_os("BEE_TEST_FLOW_CHILD").is_none() {
            let output = std::process::Command::new(std::env::current_exe().unwrap())
                .args([
                    "--exact",
                    "viewport::tests::gpui_flow_connections_and_exposure_drive_rendering",
                    "--ignored",
                    "--nocapture",
                ])
                .env("BEE_TEST_FLOW_CHILD", "1")
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

        let mut app = gpui::TestAppContext::single();
        let (view, cx) = app.add_window_view(|_, _| Bee::new());
        cx.simulate_resize(size(px(1100.0), px(1400.0)));
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(1100.0), px(1400.0)),
            |_, _| view.clone(),
        );
        cx.run_until_parked();
        let click = |cx: &mut gpui::VisualTestContext, selector: &'static str| {
            let bounds = cx.debug_bounds(selector).expect(selector);
            cx.simulate_click(bounds.center(), Modifiers::none());
            cx.run_until_parked();
        };
        let original = cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert!(viewport.error.is_none(), "{:?}", viewport.error);
            viewport
                .image
                .as_ref()
                .unwrap()
                .as_bytes(0)
                .unwrap()
                .to_vec()
        });

        // 节点参数作用于当前的光栅化输出。
        click(cx, "flow-exposure-up-2");
        click(cx, "flow-exposure-up-2");
        click(cx, "flow-apply");
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            let brighter = viewport.image.as_ref().unwrap().as_bytes(0).unwrap();
            assert!(brighter.iter().zip(&original).any(|(new, old)| *new > *old));
        });

        // 重连后应执行实际路径追踪，而不是只改变界面标记。
        click(cx, "flow-output-1");
        click(cx, "flow-input-2");
        click(cx, "flow-apply");
        let (backend, snapshot, extent, graph, path_pixels) = cx.update(|_, cx| {
            let view = view.read(cx);
            assert_eq!(view.viewport.mode, RenderMode::Path);
            assert_eq!(view.viewport.samples, 1);
            assert!(view.viewport.error.is_none(), "{:?}", view.viewport.error);
            (
                view.viewport.backend.as_ref().unwrap().clone(),
                view.state.snapshot(),
                view.viewport.extent.unwrap(),
                view.viewport.flow.active().clone(),
                view.viewport
                    .image
                    .as_ref()
                    .unwrap()
                    .as_bytes(0)
                    .unwrap()
                    .to_vec(),
            )
        });
        let mut fresh = bee_renderer::PathRenderer::new(&backend).unwrap();
        let expected = fresh
            .render_graph(&backend, &snapshot, extent.0, extent.1, 1, &graph)
            .unwrap();
        assert_eq!(path_pixels.len(), expected.len());
        let different = path_pixels
            .iter()
            .zip(&expected)
            .filter(|(a, b)| a.abs_diff(**b) > 2)
            .count();
        assert_eq!(
            different, 0,
            "{different} path image channels differ by more than 2"
        );

        // 重新映射曝光时仍呈现已有的首个样本。
        click(cx, "flow-exposure-up-2");
        click(cx, "flow-apply");
        let graph = cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert_eq!(viewport.samples, 1);
            assert!(viewport.image.as_ref().unwrap().as_bytes(0).unwrap() != path_pixels);
            viewport.flow.active().clone()
        });

        click(cx, "flow-output-0");
        click(cx, "flow-input-3");
        click(cx, "flow-apply");
        view.update_in(cx, |view, _, _| {
            let viewport = &view.viewport;
            assert_eq!(viewport.mode, RenderMode::Path);
            assert_eq!(viewport.flow.active(), &graph);
            let error = viewport.flow.error().expect("invalid flow error");
            assert!(viewport.notice().unwrap().contains(error));
            view.console.drain();
            assert!(view.console.visible().any(|message| {
                message.level == DiagnosticLevel::Error
                    && message.source == "Render Flow"
                    && message.message == error
            }));
        });
        click(cx, "flow-output-2");
        click(cx, "flow-input-3");
        click(cx, "flow-output-0");
        click(cx, "flow-input-2");
        click(cx, "flow-apply");
        cx.update(|_, cx| {
            let view = view.read(cx);
            assert_eq!(view.viewport.mode, RenderMode::Raster);
            assert!(view.viewport.error.is_none(), "{:?}", view.viewport.error);
            let expected = bee_renderer::render_raster_graph(
                &backend,
                &view.state.snapshot(),
                extent.0,
                extent.1,
                view.viewport.flow.active(),
            )
            .unwrap();
            assert!(view.viewport.image.as_ref().unwrap().as_bytes(0).unwrap() == expected);
        });
        cx.executor().advance_clock(Duration::from_millis(17));
        cx.run_until_parked();
        drop(fresh);
        drop(backend);
        assert!(!view.update_in(cx, |view, window, _| view.viewport.request_close(window)));
        cx.run_until_parked();
        assert!(cx.cx.update(|cx| cx.windows().is_empty()));
        drop(view);
        cx.quit();
    }

    #[test]
    #[ignore = "requires a ray tracing device and Vulkan validation layer"]
    fn gpui_path_tracing_accumulates_and_discards_stale_frames() {
        if std::env::var_os("BEE_TEST_PATH_CHILD").is_none() {
            let output = std::process::Command::new(std::env::current_exe().unwrap())
                .args([
                    "--exact",
                    "viewport::tests::gpui_path_tracing_accumulates_and_discards_stale_frames",
                    "--ignored",
                    "--nocapture",
                ])
                .env("BEE_TEST_PATH_CHILD", "1")
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
        let mut app = gpui::TestAppContext::single();
        let (view, cx) = app.add_window_view(|_, _| Bee::new());
        let dimensions = size(px(800.0), px(300.0));
        cx.simulate_resize(dimensions);
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| view.clone());
        cx.run_until_parked();
        let path_button = cx.debug_bounds("path-mode").unwrap().center();
        cx.simulate_click(path_button, Modifiers::none());
        cx.run_until_parked();
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert_eq!(viewport.mode, RenderMode::Path);
            assert!(viewport.error.is_none(), "{:?}", viewport.error);
            assert_eq!(viewport.samples, 1);
            assert!(viewport.image.is_some());
        });
        cx.executor().advance_clock(Duration::from_millis(17));
        cx.run_until_parked();
        cx.update(|_, cx| assert_eq!(view.read(cx).viewport.samples, 2));
        let start = cx.debug_bounds("scene-canvas").unwrap().center();
        let first = point(start.x + px(8.0), start.y + px(4.0));
        let second = point(start.x + px(16.0), start.y + px(8.0));
        let (initial_camera, initial_image) = view.update_in(cx, |view, _, _| {
            view.viewport.render_delay = Some(FRAME_DELAY);
            (
                view.state.camera(),
                view.viewport.image.as_ref().unwrap().id,
            )
        });
        cx.simulate_mouse_move(start, None, Modifiers::none());
        cx.simulate_mouse_down(start, MouseButton::Left, Modifiers::none());
        cx.simulate_mouse_move(first, MouseButton::Left, Modifiers::none());
        let first_version = cx.update(|_, cx| view.read(cx).state.versions());
        cx.simulate_mouse_move(second, MouseButton::Left, Modifiers::none());
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert!(viewport.running());
            assert_eq!(viewport.image.as_ref().unwrap().id, initial_image);
            assert_eq!(viewport.samples, 2);
        });
        finish_frame(cx);
        let latest_image = cx.update(|_, cx| {
            let view = view.read(cx);
            assert_ne!(view.state.versions(), first_version);
            assert_ne!(view.state.camera(), initial_camera);
            assert_eq!(view.viewport.image_versions, Some(view.state.versions()));
            assert_eq!(view.viewport.samples, 1);
            assert!(!view.viewport.running());
            view.viewport.image.as_ref().unwrap().id
        });
        cx.simulate_mouse_up(second, MouseButton::Left, Modifiers::none());

        // 旧计时器先到期，此时最新视角已显示且处于空闲。
        cx.executor().advance_clock(Duration::from_millis(8));
        cx.run_until_parked();
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert!(!viewport.running());
            assert_eq!(viewport.image.as_ref().unwrap().id, latest_image);
            assert_eq!(viewport.samples, 1);
        });
        cx.executor().advance_clock(Duration::from_millis(15));
        finish_frame(cx);
        cx.update(|_, cx| {
            let view = view.read(cx);
            assert_eq!(view.viewport.image_versions, Some(view.state.versions()));
            assert_ne!(view.viewport.image.as_ref().unwrap().id, latest_image);
            assert_eq!(view.viewport.samples, 2);
        });

        // 旧快照仍在处理时连续更改状态；新历史必须从首个样本开始。
        view.update_in(cx, |view, window, cx| {
            view.viewport.render_delay = None;
            let original_id = view.viewport.image.as_ref().unwrap().id;
            view.viewport.start(&view.state, window, cx);
            assert!(view.viewport.running());
            let mut camera = view.state.camera();
            camera.orbit(30.0, 8.0);
            assert!(view.state.set_camera(camera));
            view.viewport
                .synchronize(view.viewport.extent, &view.state, window, cx);
            assert_eq!(view.viewport.image.as_ref().unwrap().id, original_id);
            let mut camera = view.state.camera();
            camera.orbit(25.0, -4.0);
            assert!(view.state.set_camera(camera));
            view.viewport
                .synchronize(view.viewport.extent, &view.state, window, cx);
            assert_eq!(view.viewport.image.as_ref().unwrap().id, original_id);
            let mut scene = view.state.scene().clone();
            scene.materials[1].albedo = [0.2, 0.7, 0.3];
            assert!(view.state.set_scene(scene));
            view.viewport
                .synchronize(view.viewport.extent, &view.state, window, cx);
        });
        cx.run_until_parked();
        let (backend, snapshot, extent, actual) = cx.update(|_, cx| {
            let view = view.read(cx);
            assert!(!view.viewport.running());
            assert!(view.viewport.error.is_none(), "{:?}", view.viewport.error);
            assert_eq!(view.viewport.samples, 1);
            assert_eq!(view.viewport.image_versions, Some(view.state.versions()));
            (
                view.viewport.backend.as_ref().unwrap().clone(),
                view.state.snapshot(),
                view.viewport.extent.unwrap(),
                view.viewport
                    .image
                    .as_ref()
                    .unwrap()
                    .as_bytes(0)
                    .unwrap()
                    .to_vec(),
            )
        });
        let mut fresh = bee_renderer::PathRenderer::new(&backend).unwrap();
        let expected = fresh
            .render(&backend, &snapshot, extent.0, extent.1, 1)
            .unwrap();
        let differences = actual
            .iter()
            .zip(&expected)
            .filter(|(a, b)| a.abs_diff(**b) > 2)
            .count();
        assert!(
            differences == 0,
            "old frames contaminated the new sampling history: {differences} channels differ"
        );
        cx.executor().advance_clock(Duration::from_millis(17));
        cx.run_until_parked();
        cx.update(|_, cx| assert_eq!(view.read(cx).viewport.samples, 2));
        let raster_button = cx.debug_bounds("raster-mode").unwrap().center();
        cx.simulate_click(raster_button, Modifiers::none());
        cx.run_until_parked();
        cx.executor().advance_clock(Duration::from_millis(17));
        cx.run_until_parked();
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert_eq!(viewport.mode, RenderMode::Raster);
            assert_eq!(viewport.samples, 0);
            assert!(!viewport.running());
            assert!(viewport.error.is_none(), "{:?}", viewport.error);
            assert!(viewport.image.is_some());
        });
        let path_button = cx.debug_bounds("path-mode").unwrap().center();
        cx.simulate_click(path_button, Modifiers::none());
        cx.run_until_parked();
        cx.update(|_, cx| assert_eq!(view.read(cx).viewport.mode, RenderMode::Path));
        cx.update(|_, cx| {
            cx.bind_keys([
                gpui::KeyBinding::new("tab", crate::FocusNext, None),
                gpui::KeyBinding::new("shift-tab", crate::FocusPrevious, None),
            ])
        });
        cx.simulate_keystrokes("shift-tab enter");
        cx.run_until_parked();
        cx.update(|_, cx| assert_eq!(view.read(cx).viewport.mode, RenderMode::Raster));
        cx.executor().advance_clock(Duration::from_millis(17));
        cx.run_until_parked();
        drop(fresh);
        drop(backend);
        assert!(!view.update_in(cx, |view, window, _| view.viewport.request_close(window)));
        cx.run_until_parked();
        assert!(cx.cx.update(|cx| cx.windows().is_empty()));
        drop(view);
        cx.quit();
    }

    #[test]
    #[ignore = "requires a Vulkan graphics device"]
    fn gpui_unavailable_path_keeps_raster_view() {
        let mut backend = Backend::new(true).unwrap();
        backend
            .report
            .ray_tracing_missing
            .push("simulated missing ray tracing".into());
        let backend = Arc::new(backend);
        let mut app = gpui::TestAppContext::single();
        app.update(|cx| {
            gpui_component::init(cx);
            crate::workbench::init_theme(cx);
        });
        let mut bee = None;
        let (root, cx) = app.add_window_view(|window, cx| {
            let view = cx.new(|_| {
                let console = crate::workbench::console::Console::default();
                Bee {
                    viewport: Viewport::new(Some(backend.clone()), console.sender.clone()),
                    state: SceneState::default(),
                    lines: vec![],
                    failed: false,
                    console,
                }
            });
            bee = Some(view.clone());
            let workbench = cx.new(|cx| crate::workbench::Workbench::new(view, window, cx));
            gpui_component::Root::new(workbench, window, cx)
        });
        let bee = bee.unwrap();
        let dimensions = size(px(960.0), px(640.0));
        cx.simulate_resize(dimensions);
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
        cx.run_until_parked();
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
        cx.run_until_parked();
        let (active, image_id, extent) = cx.update(|_, cx| {
            let bee = bee.read(cx);
            let viewport = &bee.viewport;
            assert_eq!(viewport.mode, RenderMode::Raster);
            assert!(viewport.notice().is_none());
            (
                viewport.flow.active().clone(),
                viewport.image.as_ref().expect("raster image").id,
                viewport.extent,
            )
        });
        let canvas = cx.debug_bounds("scene-canvas").unwrap();
        let path_button = cx.debug_bounds("path-mode").unwrap().center();
        cx.simulate_click(path_button, Modifiers::none());
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
        cx.run_until_parked();
        cx.update(|_, cx| {
            let bee = bee.read(cx);
            let viewport = &bee.viewport;
            assert_eq!(viewport.mode, RenderMode::Raster);
            assert!(viewport.error.is_none(), "{:?}", viewport.error);
            assert_eq!(viewport.flow.active(), &active);
            assert_eq!(viewport.image.as_ref().unwrap().id, image_id);
            assert_eq!(viewport.extent, extent);
            assert!(viewport
                .notice()
                .unwrap()
                .contains("Path tracing unavailable: simulated missing ray tracing"));
        });
        assert_eq!(cx.debug_bounds("scene-canvas").unwrap(), canvas);
        let notice = cx.debug_bounds("viewport-notice").unwrap();
        assert!(notice.left() >= canvas.left() && notice.right() <= canvas.right());
        assert!(notice.top() >= canvas.top() && notice.bottom() <= canvas.bottom());

        let start = canvas.center();
        let end = point(start.x + px(20.0), start.y + px(10.0));
        cx.simulate_mouse_move(start, None, Modifiers::none());
        cx.simulate_mouse_down(start, MouseButton::Left, Modifiers::none());
        cx.simulate_mouse_move(end, MouseButton::Left, Modifiers::none());
        cx.simulate_mouse_up(end, MouseButton::Left, Modifiers::none());
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
        cx.run_until_parked();
        bee.update_in(cx, |bee, _, _| {
            let viewport = &bee.viewport;
            assert_eq!(viewport.mode, RenderMode::Raster);
            assert_eq!(viewport.flow.active(), &active);
            assert_eq!(viewport.image_versions, Some(bee.state.versions()));
            assert_ne!(viewport.image.as_ref().unwrap().id, image_id);
            assert!(viewport.error.is_none(), "{:?}", viewport.error);
            bee.console.drain();
            assert_eq!(
                bee.console
                    .visible()
                    .filter(|message| {
                        message.level == DiagnosticLevel::Info
                            && message.source == "Viewport"
                            && message.message == "Image presentation: async Vulkan readback"
                    })
                    .count(),
                1
            );
            assert!(bee.console.visible().any(|message| {
                message.level == DiagnosticLevel::Error
                    && message.source == "Render Flow"
                    && message.message.contains("simulated missing ray tracing")
            }));
        });
    }

    #[test]
    #[ignore = "requires a Vulkan graphics device and validation layer"]
    fn gpui_camera_input_changes_raster_image() {
        if std::env::var_os("BEE_TEST_CAMERA_CHILD").is_none() {
            let output = std::process::Command::new(std::env::current_exe().unwrap())
                .args([
                    "--exact",
                    "viewport::tests::gpui_camera_input_changes_raster_image",
                    "--ignored",
                    "--nocapture",
                ])
                .env("BEE_TEST_CAMERA_CHILD", "1")
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
        let mut app = gpui::TestAppContext::single();
        let (view, cx) = app.add_window_view(|_, _| Bee::new());
        cx.simulate_resize(size(px(700.0), px(500.0)));
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(700.0), px(500.0)),
            |_, _| view.clone(),
        );
        cx.run_until_parked();
        let (camera_before, image_before, idle_backend_owners) = cx.update(|_, cx| {
            let view = view.read(cx);
            assert!(view.viewport.error.is_none(), "{:?}", view.viewport.error);
            (
                view.state.camera(),
                view.viewport
                    .image
                    .as_ref()
                    .unwrap()
                    .as_bytes(0)
                    .unwrap()
                    .to_vec(),
                Arc::strong_count(view.viewport.backend.as_ref().unwrap()),
            )
        });
        view.update_in(cx, |view, _, _| {
            view.viewport.render_delay = Some(FRAME_DELAY);
        });
        let start = point(px(100.0), px(150.0));
        cx.simulate_mouse_move(start, None, Modifiers::none());
        cx.simulate_mouse_down(start, MouseButton::Left, Modifiers::none());
        for (index, position) in [
            point(px(120.0), px(160.0)),
            point(px(140.0), px(170.0)),
            point(px(170.0), px(180.0)),
        ]
        .into_iter()
        .enumerate()
        {
            cx.simulate_mouse_move(position, MouseButton::Left, Modifiers::none());
            cx.update(|_, cx| {
                let view = view.read(cx);
                assert_eq!(view.state.versions().camera, index as u64 + 1);
                assert!(view.viewport.running());
                assert_eq!(
                    Arc::strong_count(view.viewport.backend.as_ref().unwrap()),
                    idle_backend_owners
                );
            });
        }
        finish_frame(cx);
        finish_frame(cx);
        cx.update(|_, cx| {
            let view = view.read(cx);
            assert_ne!(view.state.camera(), camera_before);
            assert!(view.viewport.error.is_none(), "{:?}", view.viewport.error);
            assert!(!view.viewport.running());
            assert_eq!(view.viewport.image_versions, Some(view.state.versions()));
            assert!(
                view.viewport.image.as_ref().unwrap().as_bytes(0).unwrap() != image_before,
                "camera movement must change the rendered image"
            );
        });
        let end = point(px(170.0), px(180.0));
        cx.simulate_mouse_up(end, MouseButton::Left, Modifiers::none());
        cx.simulate_event(ScrollWheelEvent {
            position: end,
            delta: ScrollDelta::Lines(point(0.0, 1.0)),
            ..Default::default()
        });
        cx.update(|_, cx| {
            let view = view.read(cx);
            assert_eq!(view.state.versions().camera, 4);
            assert!(view.viewport.running());
        });
        finish_frame(cx);
        cx.update(|_, cx| {
            let view = view.read(cx);
            assert_eq!(view.viewport.image_versions, Some(view.state.versions()));
            assert!(!view.viewport.running());
        });
    }

    #[test]
    #[ignore = "requires a Vulkan graphics device"]
    fn gpui_displays_readback_and_recovers_from_resize_and_failure() {
        if std::env::var_os("BEE_TEST_VIEWPORT_CHILD").is_none() {
            let output = std::process::Command::new(std::env::current_exe().unwrap())
                .args([
                    "--exact",
                    "viewport::tests::gpui_displays_readback_and_recovers_from_resize_and_failure",
                    "--ignored",
                    "--nocapture",
                ])
                .env("BEE_TEST_VIEWPORT_CHILD", "1")
                .env("VK_LAYER_VALIDATE_SYNC", "1")
                .output()
                .unwrap();
            let stdout = String::from_utf8_lossy(&output.stdout);
            let stderr = String::from_utf8_lossy(&output.stderr);
            assert!(output.status.success(), "{stdout}\n{stderr}");
            assert!(
                !stderr.lines().any(|line| {
                    line.starts_with("Vulkan validation ") && line.contains("ERROR")
                }),
                "{stderr}"
            );
            return;
        }

        // 渲染初始图像，验证尺寸和场景内容。
        let mut app = gpui::TestAppContext::single();
        let cx = app.add_empty_window();
        let view = cx.update(|_, cx| cx.new(|_| Bee::new()));
        cx.update(|_, cx| {
            assert_eq!(
                view.read(cx)
                    .viewport
                    .backend
                    .as_ref()
                    .unwrap()
                    .report
                    .validation,
                bee_vulkan::ValidationStatus::Enabled
            );
        });

        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(700.0), px(500.0)),
            |_, _| view.clone(),
        );
        cx.run_until_parked();
        let (old_image, original_extent) = cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert!(!viewport.running());
            assert!(viewport.error.is_none(), "{:?}", viewport.error);
            let image = viewport.image.as_ref().expect("Vulkan image");
            let extent = viewport.extent.unwrap();
            assert_eq!(image.size(0).width.0 as u32, extent.0);
            assert_eq!(image.size(0).height.0 as u32, extent.1);
            let bytes = image.as_bytes(0).unwrap();
            assert!(bytes
                .chunks_exact(4)
                .any(|p| p[2] > p[0].saturating_add(20)));
            assert!(bytes
                .chunks_exact(4)
                .any(|p| p[0] > p[2].saturating_add(20)));
            (Arc::downgrade(image), extent)
        });
        // 新输入等待期间保留上一张图，完成的兼容相机帧要通知界面。
        let notified = std::rc::Rc::new(std::cell::Cell::new(None));
        let _observer = view.update_in(cx, |view, _, cx| {
            view.viewport.render_delay = Some(FRAME_DELAY);
            let notified = notified.clone();
            cx.observe_self(move |view, _| notified.set(view.viewport.image_versions))
        });
        let latest_version = view.update_in(cx, |view, window, cx| {
            let original_id = view.viewport.image.as_ref().unwrap().id;
            let mut camera = view.state.camera();
            assert!(camera.orbit(20.0, 5.0));
            assert!(view.state.set_camera(camera));
            view.viewport
                .synchronize(view.viewport.extent, &view.state, window, cx);
            assert!(view.viewport.running());
            assert_eq!(view.viewport.image.as_ref().unwrap().id, original_id);
            assert!(camera.orbit(25.0, -3.0));
            assert!(view.state.set_camera(camera));
            view.viewport
                .synchronize(view.viewport.extent, &view.state, window, cx);
            assert_eq!(view.viewport.image.as_ref().unwrap().id, original_id);
            view.state.versions()
        });
        let previous_paints = PAINTED_IMAGES.get().0;
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(700.0), px(500.0)),
            |_, _| view.clone(),
        );
        assert_eq!(PAINTED_IMAGES.get().0, previous_paints + 1);
        finish_frame(cx);
        assert_eq!(notified.get(), Some(latest_version));
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert!(!viewport.running());
            assert_eq!(viewport.image_versions, Some(latest_version));
            assert!(viewport.error.is_none(), "{:?}", viewport.error);
        });
        // 替换图集条目前，先验证 GPUI 的 paint_image 路径。
        let previous_paints = PAINTED_IMAGES.get().0;
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(700.0), px(500.0)),
            |_, _| view.clone(),
        );
        assert_eq!(PAINTED_IMAGES.get().0, previous_paints + 1);
        cx.simulate_resize(size(px(850.0), px(600.0)));
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(850.0), px(600.0)),
            |_, _| view.clone(),
        );
        finish_frame(cx);
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert_ne!(viewport.extent, Some(original_extent));
            let extent = viewport.extent.unwrap();
            let image = viewport.image.as_ref().unwrap();
            assert_eq!(image.size(0).width.0 as u32, extent.0);
            assert_eq!(image.size(0).height.0 as u32, extent.1);
        });
        assert!(old_image.upgrade().is_none());

        // GPUI 在窗口最小化时暂停帧回调，恢复时发送尺寸变化事件。
        let (generation, image_id) = cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            (viewport.generation, viewport.image.as_ref().unwrap().id)
        });
        cx.run_until_parked();
        cx.simulate_resize(size(px(850.0), px(600.0)));
        let previous_paints = PAINTED_IMAGES.get().0;
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(850.0), px(600.0)),
            |_, _| view.clone(),
        );
        cx.run_until_parked();
        assert_eq!(PAINTED_IMAGES.get().0, previous_paints + 1);
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert_eq!(viewport.generation, generation);
            assert_eq!(viewport.image.as_ref().unwrap().id, image_id);
            assert!(!viewport.running());
        });

        let expect_replacement = |cx: &mut gpui::VisualTestContext, previous| {
            cx.run_until_parked();
            cx.update(|_, cx| {
                let viewport = &view.read(cx).viewport;
                assert!(
                    viewport.running(),
                    "replaced request must start immediately"
                );
                assert_eq!(viewport.image.as_ref().map(|image| image.id), previous);
                assert!(viewport.error.is_none(), "{:?}", viewport.error);
            });
            finish_frame(cx);
            cx.update(|_, cx| {
                let view = view.read(cx);
                let viewport = &view.viewport;
                assert!(!viewport.running());
                assert!(viewport.error.is_none(), "{:?}", viewport.error);
                assert_eq!(viewport.versions, Some(view.state.versions()));
                assert_eq!(viewport.image_versions, Some(view.state.versions()));
                let image = viewport.image.as_ref().unwrap();
                assert_ne!(Some(image.id), previous);
                assert_eq!(
                    Some((image.size(0).width.0 as u32, image.size(0).height.0 as u32)),
                    viewport.extent
                );
            });
        };

        // 分别更改尚未同步的场景、渲染和显示参数，避免一处检查掩盖另一处。
        let changes: [fn(&mut SceneState); 3] = [
            |state| {
                let mut scene = state.scene().clone();
                scene.materials[0].albedo[0] = 0.4;
                assert!(state.set_scene(scene));
            },
            |state| {
                assert!(state.set_render_parameters(bee_scene::RenderParameters {
                    max_path_bounces: 3,
                }));
            },
            |state| {
                assert!(state.set_display_parameters(DisplayParameters { exposure_ev: 1.0 }));
            },
        ];
        for change in changes {
            let previous = view.update_in(cx, |view, window, cx| {
                view.viewport.start(&view.state, window, cx);
                change(&mut view.state);
                assert_ne!(view.viewport.versions, Some(view.state.versions()));
                view.viewport.image.as_ref().unwrap().id
            });
            cx.run_until_parked();
            finish_frame(cx);
            expect_replacement(cx, Some(previous));
        }

        // 同模式应用不同流程也会淘汰在途帧。
        let previous = view.update_in(cx, |view, window, cx| {
            view.viewport.start(&view.state, window, cx);
            let versions = view.state.versions();
            let viewport = &mut view.viewport;
            let original_graph = viewport.flow.active().clone();
            let tone = original_graph
                .nodes()
                .find_map(|(id, module)| matches!(module, Module::ToneMapping { .. }).then_some(id))
                .unwrap();
            *viewport.flow.draft_mut().module_mut(tone).unwrap() = Module::ToneMapping {
                exposure_ev: view.state.snapshot().display.exposure_ev,
            };
            viewport.apply_flow(&mut view.state, window, cx);
            assert_ne!(viewport.flow.active(), &original_graph);
            assert_eq!(viewport.mode, RenderMode::Raster);
            assert_eq!(view.state.versions(), versions);
            viewport.image.as_ref().unwrap().id
        });
        expect_replacement(cx, Some(previous));

        // 只显示最新尺寸的图像；尺寸为空时清除图像。
        view.update_in(cx, |view, window, cx| {
            view.viewport
                .synchronize(Some((17, 9)), &view.state, window, cx);
            view.viewport
                .synchronize(Some((7, 13)), &view.state, window, cx);
            assert!(view.viewport.running());
            assert!(view.viewport.image.is_none());
        });
        expect_replacement(cx, None);
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert_eq!(viewport.extent, Some((7, 13)));
            let image = viewport.image.as_ref().unwrap();
            assert_eq!(image.size(0).width.0, 7);
            assert_eq!(image.size(0).height.0, 13);
        });

        view.update_in(cx, |view, window, cx| {
            view.viewport
                .synchronize(Some((19, 11)), &view.state, window, cx);
        });
        cx.run_until_parked();
        view.update_in(cx, |view, window, cx| {
            view.viewport.synchronize(None, &view.state, window, cx);
            assert!(view.viewport.image.is_none());
            assert!(view.viewport.running());
        });
        finish_frame(cx);
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert!(!viewport.running());
            assert!(viewport.image.is_none());
            assert!(viewport.extent.is_none());
        });
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(850.0), px(600.0)),
            |_, _| view.clone(),
        );
        finish_frame(cx);
        cx.update(|_, cx| assert!(view.read(cx).viewport.image.is_some()));

        // 隐藏后以相同尺寸和相机恢复，旧帧的版本仍相同。
        view.update_in(cx, |view, window, cx| {
            let extent = view.viewport.extent;
            view.viewport.start(&view.state, window, cx);
            assert!(view.viewport.running());
            view.viewport.set_active(false, &view.state, window, cx);
            view.viewport.set_active(true, &view.state, window, cx);
            view.viewport.synchronize(extent, &view.state, window, cx);
            assert!(view.viewport.image.is_none());
        });
        expect_replacement(cx, None);

        // 已被取代的失败不能清除上一张图或送出诊断。
        let previous = view.update_in(cx, |view, window, cx| {
            let scene = view.state.scene().clone();
            let mut invalid = scene.clone();
            invalid.environment[0] = -1.0;
            assert!(view.state.set_scene(invalid));
            view.viewport
                .synchronize(view.viewport.extent, &view.state, window, cx);
            assert!(view.viewport.running());
            assert!(view.state.set_scene(scene));
            view.viewport
                .synchronize(view.viewport.extent, &view.state, window, cx);
            view.viewport.image.as_ref().unwrap().id
        });
        expect_replacement(cx, Some(previous));
        view.update_in(cx, |view, _, _| {
            view.console.drain();
            assert!(!view
                .console
                .visible()
                .any(|message| message.level == DiagnosticLevel::Error
                    && message.source == "Viewport"));
        });

        // 读回和上传失败时应清除图像，并在尺寸变化后恢复。
        view.update_in(cx, |view, window, cx| {
            view.viewport.render_delay = None;
            view.viewport
                .synchronize(Some((u32::MAX, 1)), &view.state, window, cx);
        });
        cx.run_until_parked();
        view.update_in(cx, |view, _, _| {
            let viewport = &view.viewport;
            assert!(viewport.image.is_none());
            let error = viewport.error.as_ref().unwrap();
            assert!(error.contains("readback failed"));
            let notice = viewport.notice().unwrap();
            assert!(notice.contains(error) && notice.contains("See Console"));
            view.console.drain();
            assert!(view.console.visible().any(|message| {
                message.level == DiagnosticLevel::Error
                    && message.source == "Viewport"
                    && message.message == *error
            }));
        });
        view.update_in(cx, |view, window, cx| {
            view.viewport
                .synchronize(Some((17, 9)), &view.state, window, cx);
        });
        cx.run_until_parked();
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert!(viewport.error.is_none());
            assert_eq!(viewport.image.as_ref().unwrap().size(0).width.0, 17);
        });
        view.update_in(cx, |view, window, _| {
            view.viewport
                .fail("GPUI image upload failed: simulated".into(), window);
            assert!(view.viewport.image.is_none());
            assert!(view.viewport.error().unwrap().contains("upload failed"));
        });
        cx.simulate_resize(size(px(250.0), px(320.0)));
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(250.0), px(320.0)),
            |_, _| view.clone(),
        );
        cx.run_until_parked();
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert!(viewport.error.is_none());
            assert!(viewport.extent.is_some());
            assert!(viewport.image.is_some());
        });
        assert!(!view.update_in(cx, |view, window, _| view.viewport.request_close(window)));
        cx.run_until_parked();
    }
}
