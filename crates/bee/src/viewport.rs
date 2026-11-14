use crate::Bee;
use bee_scene::{SceneState, Versions};
use bee_vulkan::{Backend, TestImage};
use gpui::{
    canvas, div, prelude::*, px, rgb, Bounds, Context, Corners, MouseButton, Pixels, Point,
    RenderImage, Window,
};
use std::sync::Arc;

type Extent = (u32, u32);

#[cfg(test)]
static PAINTED_IMAGES: std::sync::atomic::AtomicUsize = std::sync::atomic::AtomicUsize::new(0);

pub struct Viewport {
    backend: Option<Arc<Backend>>,
    extent: Option<Extent>,
    versions: Option<Versions>,
    generation: u64,
    running: bool,
    image: Option<Arc<RenderImage>>,
    image_versions: Option<Versions>,
    error: Option<String>,
    drag_position: Option<Point<Pixels>>,
}

impl Viewport {
    pub fn new(backend: Option<Arc<Backend>>) -> Self {
        Self {
            backend,
            extent: None,
            versions: None,
            generation: 0,
            running: false,
            image: None,
            image_versions: None,
            error: None,
            drag_position: None,
        }
    }

    pub fn render(&self, state: &SceneState, cx: &mut Context<Bee>) -> impl IntoElement {
        let extent = self.extent;
        let versions = state.versions();
        let requested_versions = self.versions;
        let image = (self.image_versions == Some(versions))
            .then(|| self.image.clone())
            .flatten();
        let status = if self.error.is_some() {
            "Viewport error; see device info"
        } else if self.backend.is_none() {
            "Viewport unavailable"
        } else if image.is_some() {
            "Image path: async Vulkan readback"
        } else {
            "Preparing Vulkan image..."
        };
        let resize_view = cx.weak_entity();
        let error_view = resize_view.clone();
        div()
            .flex()
            .flex_col()
            .flex_1()
            .min_w_0()
            .min_h_0()
            .gap_2()
            .child(
                div()
                    .h(px(28.0))
                    .flex_shrink_0()
                    .overflow_hidden()
                    .whitespace_nowrap()
                    .text_ellipsis()
                    .text_color(if self.error.is_some() {
                        rgb(0xffa8a8)
                    } else {
                        rgb(0xd1d7d4)
                    })
                    .child(status),
            )
            .child(
                div()
                    .id("scene-canvas")
                    .debug_selector(|| "scene-canvas".into())
                    .w_full()
                    .flex_1()
                    .min_h_0()
                    .bg(rgb(0x111417))
                    .on_mouse_down(
                        MouseButton::Left,
                        cx.listener(|view, event: &gpui::MouseDownEvent, _, _| {
                            view.viewport.drag_position = Some(event.position);
                        }),
                    )
                    .on_mouse_move(cx.listener(|view, event: &gpui::MouseMoveEvent, _, cx| {
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
                            cx.notify();
                        }
                    }))
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
                    .on_scroll_wheel(cx.listener(|view, event: &gpui::ScrollWheelEvent, _, cx| {
                        let scroll = f32::from(event.delta.pixel_delta(px(20.0)).y);
                        let mut camera = view.state.camera();
                        if camera.zoom(scroll) && view.state.set_camera(camera) {
                            cx.notify();
                        }
                    }))
                    .child(
                        canvas(
                            move |bounds, window, cx| {
                                // 延后同步尺寸和状态，并隐藏已过期的图像。
                                let next = physical_extent(bounds, window.scale_factor());
                                if next != extent || requested_versions != Some(versions) {
                                    window.defer(cx, move |window, cx| {
                                        resize_view
                                            .update(cx, |view, cx| {
                                                view.viewport.synchronize(
                                                    next,
                                                    &view.state,
                                                    window,
                                                    cx,
                                                )
                                            })
                                            .ok();
                                    });
                                }
                                (next == extent).then_some(image).flatten()
                            },
                            move |bounds, image, window, cx| {
                                if let Some(image) = image {
                                    let id = image.id;
                                    let result = window.paint_image(
                                        bounds,
                                        Corners::default(),
                                        image,
                                        0,
                                        false,
                                    );
                                    #[cfg(test)]
                                    if result.is_ok() {
                                        PAINTED_IMAGES
                                            .fetch_add(1, std::sync::atomic::Ordering::Relaxed);
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
                            },
                        )
                        .size_full(),
                    ),
            )
    }

    pub fn error(&self) -> Option<&str> {
        self.error.as_deref()
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

        // 使进行中的读回结果失效，并释放旧的图集图像。
        self.extent = extent;
        self.versions = Some(versions);
        self.generation = self.generation.wrapping_add(1);
        self.error = None;
        self.drop_image(window);
        self.start(state, window, cx);
        cx.notify();
    }

    fn start(&mut self, state: &SceneState, window: &mut Window, cx: &mut Context<Bee>) {
        let (Some(backend), Some((width, height))) = (&self.backend, self.extent) else {
            return;
        };

        // 同时只执行一次读回，后续尺寸变化将在完成后处理。
        if self.running {
            return;
        }
        self.running = true;
        let backend = backend.clone();
        let generation = self.generation;
        let snapshot = state.snapshot();

        // 后台任务持有快照，直到 Vulkan 提交和读回完成。
        let task = cx.background_spawn(async move {
            let result = bee_renderer::render_raster_snapshot(&backend, &snapshot, width, height)
                .map_err(|error| format!("Vulkan image readback failed: {error}"))
                .and_then(|bgra| {
                    let row_stride = (width as usize)
                        .checked_mul(4)
                        .ok_or("Image row size overflow")?;
                    render_image(TestImage {
                        width,
                        height,
                        row_stride,
                        bgra,
                    })
                });
            (snapshot, result)
        });
        cx.spawn_in(window, async move |view, cx| {
            let (snapshot, result) = task.await;
            view.update_in(cx, |view, window, cx| {
                view.viewport.running = false;
                // 丢弃过期结果，并开始处理最新的尺寸和状态。
                if view.viewport.generation == generation
                    && snapshot.versions == view.state.versions()
                {
                    match result {
                        Ok(image) => {
                            view.viewport.image_versions = Some(snapshot.versions);
                            view.viewport.image = Some(image);
                        }
                        Err(reason) => view.viewport.fail(reason, window),
                    }
                } else {
                    let extent = view.viewport.extent;
                    view.viewport.synchronize(extent, &view.state, window, cx);
                    view.viewport.start(&view.state, window, cx);
                }
                cx.notify();
            })
            .ok();
        })
        .detach();
    }

    fn fail(&mut self, reason: String, window: &mut Window) {
        eprintln!("{reason}");
        self.drop_image(window);
        self.error = Some(reason);
    }

    fn drop_image(&mut self, window: &mut Window) {
        self.image_versions = None;
        if let Some(image) = self.image.take() {
            if let Err(error) = window.drop_image(image) {
                eprintln!("Could not release GPUI image: {error:#}");
            }
        }
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

    // 去除 Vulkan 的行填充，再将连续像素交给 GPUI。
    let mut packed = Vec::with_capacity(bytes);
    for row in image
        .bgra
        .chunks_exact(image.row_stride)
        .take(image.height as usize)
    {
        packed.extend_from_slice(&row[..row_bytes]);
    }
    // 尽管 image 的缓冲区类型为 RGBA，GPUI 仍将原始字节解释为 BGRA。
    let buffer = image::RgbaImage::from_raw(image.width, image.height, packed)
        .ok_or("Could not create GPUI image")?;
    Ok(Arc::new(RenderImage::new(vec![image::Frame::new(buffer)])))
}

#[cfg(test)]
mod tests {
    use super::*;
    use gpui::{point, px, size, Modifiers, ScrollDelta, ScrollWheelEvent};

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
        let (camera_before, image_before) = cx.update(|_, cx| {
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
            )
        });
        let start = point(px(100.0), px(150.0));
        let end = point(px(170.0), px(180.0));
        cx.simulate_mouse_move(start, None, Modifiers::none());
        cx.simulate_mouse_down(start, MouseButton::Left, Modifiers::none());
        cx.simulate_mouse_move(end, MouseButton::Left, Modifiers::none());
        cx.simulate_mouse_up(end, MouseButton::Left, Modifiers::none());
        cx.run_until_parked();
        cx.update(|_, cx| {
            let view = view.read(cx);
            assert_ne!(view.state.camera(), camera_before);
            assert!(view.viewport.error.is_none(), "{:?}", view.viewport.error);
            assert_eq!(view.viewport.image_versions, Some(view.state.versions()));
            assert!(
                view.viewport.image.as_ref().unwrap().as_bytes(0).unwrap() != image_before,
                "camera movement must change the rendered image"
            );
        });
    }

    #[test]
    #[ignore = "requires a Vulkan graphics device"]
    fn gpui_displays_readback_and_recovers_from_resize_and_failure() {
        use std::sync::atomic::Ordering;

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
            assert!(!viewport.running);
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
        // 替换图集条目前，先验证 GPUI 的 paint_image 路径。
        let previous_paints = PAINTED_IMAGES.load(Ordering::Relaxed);
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(700.0), px(500.0)),
            |_, _| view.clone(),
        );
        assert_eq!(PAINTED_IMAGES.load(Ordering::Relaxed), previous_paints + 1);
        cx.simulate_resize(size(px(850.0), px(600.0)));
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(850.0), px(600.0)),
            |_, _| view.clone(),
        );
        cx.run_until_parked();
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
        let previous_paints = PAINTED_IMAGES.load(Ordering::Relaxed);
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(850.0), px(600.0)),
            |_, _| view.clone(),
        );
        cx.run_until_parked();
        assert_eq!(PAINTED_IMAGES.load(Ordering::Relaxed), previous_paints + 1);
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert_eq!(viewport.generation, generation);
            assert_eq!(viewport.image.as_ref().unwrap().id, image_id);
            assert!(!viewport.running);
        });

        // 在途任务结束后，只接受最新场景、相机和参数对应的图像。
        view.update_in(cx, |view, window, cx| {
            view.viewport
                .synchronize(Some((23, 11)), &view.state, window, cx);
            assert!(view.viewport.running);

            let mut scene = view.state.scene().clone();
            scene.materials[0].albedo[0] = 0.4;
            assert!(view.state.set_scene(scene));
            let mut camera = view.state.camera();
            assert!(camera.orbit(20.0, 10.0));
            assert!(view.state.set_camera(camera));
            assert!(view
                .state
                .set_render_parameters(bee_scene::RenderParameters {
                    max_path_bounces: 3,
                }));
            assert!(view
                .state
                .set_display_parameters(bee_scene::DisplayParameters { exposure_ev: 1.0 }));
            assert_ne!(view.viewport.versions, Some(view.state.versions()));
        });
        cx.run_until_parked();
        cx.update(|_, cx| {
            let view = view.read(cx);
            assert!(!view.viewport.running);
            assert!(view.viewport.image.is_some());
            assert_eq!(view.viewport.versions, Some(view.state.versions()));
            assert_eq!(view.viewport.image_versions, Some(view.state.versions()));
        });

        // 只显示最新尺寸的图像；尺寸为空时清除图像。
        view.update_in(cx, |view, window, cx| {
            view.viewport
                .synchronize(Some((17, 9)), &view.state, window, cx);
            view.viewport
                .synchronize(Some((7, 13)), &view.state, window, cx);
            assert!(view.viewport.running);
            assert!(view.viewport.image.is_none());
        });
        cx.run_until_parked();
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
            view.viewport.synchronize(None, &view.state, window, cx);
            assert!(view.viewport.image.is_none());
            assert!(view.viewport.running);
        });
        cx.run_until_parked();
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert!(!viewport.running);
            assert!(viewport.image.is_none());
            assert!(viewport.extent.is_none());
        });
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(850.0), px(600.0)),
            |_, _| view.clone(),
        );
        cx.run_until_parked();
        cx.update(|_, cx| assert!(view.read(cx).viewport.image.is_some()));

        // 读回和上传失败时应清除图像，并在尺寸变化后恢复。
        view.update_in(cx, |view, window, cx| {
            view.viewport
                .synchronize(Some((u32::MAX, 1)), &view.state, window, cx);
        });
        cx.run_until_parked();
        cx.update(|_, cx| {
            let viewport = &view.read(cx).viewport;
            assert!(viewport.image.is_none());
            assert!(viewport.error.as_ref().unwrap().contains("readback failed"));
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
    }
}
