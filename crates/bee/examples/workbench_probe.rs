#[allow(dead_code)]
#[path = "../src/main.rs"]
mod bee_app;

use bee_app::{flow_editor, Bee};
#[cfg(test)]
use bee_app::{FocusNext, FocusPrevious};
use gpui::{
    div, prelude::*, px, size, App, Application, Bounds, Context, Entity, EventEmitter,
    FocusHandle, Focusable, PathPromptOptions, SharedString, Window, WindowBounds, WindowOptions,
};
use gpui_component::{
    dock::{DockArea, DockItem, Panel, PanelEvent, PanelStyle},
    Root,
};

struct ProbePanel {
    title: &'static str,
    focus: FocusHandle,
    bee: Option<Entity<Bee>>,
    folder: String,
}

struct ProbeWorkbench {
    dock: Entity<DockArea>,
    viewport: Entity<ProbePanel>,
    files: Entity<ProbePanel>,
}

fn reset_layout(
    dock: &mut DockArea,
    viewport: Entity<ProbePanel>,
    files: Entity<ProbePanel>,
    window: &mut Window,
    cx: &mut Context<DockArea>,
) {
    let weak = cx.entity().downgrade();
    let files = DockItem::tab(files, &weak, window, cx).size(px(240.0));
    let viewport = DockItem::tab(viewport, &weak, window, cx);
    let layout = DockItem::h_split(vec![files, viewport], &weak, window, cx);
    dock.set_center(layout, window, cx);
}

impl Render for ProbeWorkbench {
    fn render(&mut self, _: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        div()
            .size_full()
            .flex()
            .flex_col()
            .child(
                div()
                    .id("reset-layout")
                    .cursor_pointer()
                    .px_3()
                    .child("Reset probe layout")
                    .on_click(cx.listener(|this, _, window, cx| {
                        this.dock.update(cx, |dock, cx| {
                            reset_layout(
                                dock,
                                this.viewport.clone(),
                                this.files.clone(),
                                window,
                                cx,
                            );
                        });
                    })),
            )
            .child(div().flex_1().min_h_0().child(self.dock.clone()))
    }
}

impl EventEmitter<PanelEvent> for ProbePanel {}

impl Focusable for ProbePanel {
    fn focus_handle(&self, _: &App) -> FocusHandle {
        self.focus.clone()
    }
}

impl Panel for ProbePanel {
    fn panel_name(&self) -> &'static str {
        self.title
    }

    fn title(&mut self, _: &mut Window, _: &mut Context<Self>) -> impl IntoElement {
        self.title
    }

    fn tab_name(&self, _: &App) -> Option<SharedString> {
        Some(self.title.into())
    }
}

impl Render for ProbePanel {
    fn render(&mut self, _: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        div()
            .size_full()
            .when_some(self.bee.clone(), |this, bee| this.child(bee))
            .when(self.bee.is_none(), |this| {
                this.flex()
                    .flex_col()
                    .gap_2()
                    .p_4()
                    .child(
                        div()
                            .id("pick-folder")
                            .cursor_pointer()
                            .child("Choose folder")
                            .on_click(cx.listener(|_, _, _, cx| {
                                let receiver = cx.prompt_for_paths(PathPromptOptions {
                                    files: false,
                                    directories: true,
                                    multiple: false,
                                    prompt: Some("Choose a folder".into()),
                                });
                                cx.spawn(async move |panel, cx| match receiver.await {
                                    Ok(Ok(Some(paths))) => {
                                        if let Some(path) = paths.first() {
                                            let folder = path.display().to_string();
                                            let _ = panel.update(cx, |panel, cx| {
                                                panel.folder = folder;
                                                cx.notify();
                                            });
                                        }
                                    }
                                    Ok(Ok(None)) => {}
                                    error => eprintln!("Folder selection failed: {error:?}"),
                                })
                                .detach();
                            })),
                    )
                    .child(self.folder.clone())
            })
    }
}

fn main() {
    Application::new()
        .with_assets(gpui_component_assets::Assets)
        .run(|cx: &mut App| {
            gpui_component::init(cx);
            let bounds = Bounds::centered(None, size(px(1200.0), px(800.0)), cx);
            cx.open_window(
                WindowOptions {
                    window_bounds: Some(WindowBounds::Windowed(bounds)),
                    ..Default::default()
                },
                |window, cx| {
                    let bee = cx.new(|_| Bee::new());
                    let viewport = cx.new(|cx| ProbePanel {
                        title: "Viewport",
                        focus: cx.focus_handle(),
                        bee: Some(bee),
                        folder: String::new(),
                    });
                    let files = cx.new(|cx| ProbePanel {
                        title: "Files",
                        focus: cx.focus_handle(),
                        bee: None,
                        folder: "No folder selected".into(),
                    });
                    let dock = cx.new(|cx| {
                        DockArea::new("workbench-probe", None, window, cx)
                            .panel_style(PanelStyle::TabBar)
                    });
                    dock.update(cx, |dock, cx| {
                        reset_layout(dock, viewport.clone(), files.clone(), window, cx);
                    });
                    let workbench = cx.new(|_| ProbeWorkbench {
                        dock,
                        viewport,
                        files,
                    });
                    cx.new(|cx| Root::new(workbench, window, cx))
                },
            )
            .expect("Failed to open the workbench probe");
            cx.activate(true);
        });
}

#[cfg(test)]
mod tests {
    use super::*;
    use gpui_component::dock::DockPlacement;
    use std::sync::Arc;

    #[test]
    fn reset_restores_closed_panel_without_losing_its_state() {
        let mut app = gpui::TestAppContext::single();
        app.update(gpui_component::init);
        let (workbench, cx) = app.add_window_view(|window, cx| {
            let files = cx.new(|cx| ProbePanel {
                title: "Files",
                focus: cx.focus_handle(),
                bee: None,
                folder: "selected folder".into(),
            });
            let viewport = cx.new(|cx| ProbePanel {
                title: "Viewport",
                focus: cx.focus_handle(),
                bee: None,
                folder: String::new(),
            });
            let dock = cx.new(|cx| DockArea::new("test-probe", None, window, cx));
            dock.update(cx, |dock, cx| {
                reset_layout(dock, viewport.clone(), files.clone(), window, cx);
            });
            ProbeWorkbench {
                dock,
                viewport,
                files,
            }
        });
        cx.run_until_parked();
        cx.update(|window, cx| {
            let workbench = workbench.read(cx);
            let (dock, viewport, files) = (
                workbench.dock.clone(),
                workbench.viewport.clone(),
                workbench.files.clone(),
            );
            dock.update(cx, |dock, cx| {
                dock.remove_panel(Arc::new(files.clone()), DockPlacement::Center, window, cx);
                reset_layout(dock, viewport, files.clone(), window, cx);
            });
            assert_eq!(files.read(cx).folder, "selected folder");
            assert_eq!(dock.read(cx).dump(cx).center.children.len(), 2);
        });
    }
}
