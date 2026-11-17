use crate::Bee;
use gpui::{
    anchored, deferred, div, prelude::*, px, rgb, App, Context, Corner, DismissEvent, Entity,
    EventEmitter, FocusHandle, Focusable, KeyDownEvent, MouseButton, SharedString, WeakEntity,
    Window,
};
use gpui_component::{
    dock::{DockArea, DockItem, Panel, PanelEvent, PanelStyle, PanelView, TabPanel},
    menu::{PopupMenu, PopupMenuItem},
    Icon, IconName, Sizable, StyledExt, Theme, ThemeMode,
};
use std::sync::Arc;

mod scene;

const BACKGROUND: u32 = 0x1b1f20;
const PANEL: u32 = 0x24292a;
const BORDER: u32 = 0x454e4c;
const TEXT: u32 = 0xe9efec;
const MUTED: u32 = 0xb1bfba;
const ACCENT: u32 = 0x79c8b0;

pub(crate) fn init_theme(cx: &mut App) {
    Theme::change(ThemeMode::Dark, None, cx);
    let theme = Theme::global_mut(cx);
    theme.background = rgb(BACKGROUND).into();
    theme.foreground = rgb(TEXT).into();
    theme.border = rgb(BORDER).into();
    theme.ring = rgb(ACCENT).into();
    theme.tab_bar = rgb(BACKGROUND).into();
    theme.tab_active = rgb(PANEL).into();
    theme.tab_active_foreground = rgb(TEXT).into();
    theme.tab_foreground = rgb(MUTED).into();
    theme.popover = rgb(PANEL).into();
    theme.popover_foreground = rgb(TEXT).into();
    theme.accent = rgb(0x344841).into();
    theme.accent_foreground = rgb(TEXT).into();
    theme.radius = px(4.0);
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum PanelKind {
    Scene,
    Viewport,
    Properties,
    Files,
    Console,
    Flow,
}

impl PanelKind {
    const ALL: [Self; 6] = [
        Self::Scene,
        Self::Viewport,
        Self::Properties,
        Self::Files,
        Self::Console,
        Self::Flow,
    ];

    fn title(self) -> &'static str {
        match self {
            Self::Scene => "Scene",
            Self::Viewport => "Viewport",
            Self::Properties => "Properties",
            Self::Files => "Files",
            Self::Console => "Console",
            Self::Flow => "Render Flow",
        }
    }

    fn id(self) -> &'static str {
        match self {
            Self::Scene => "scene",
            Self::Viewport => "viewport",
            Self::Properties => "properties",
            Self::Files => "files",
            Self::Console => "console",
            Self::Flow => "flow",
        }
    }
}

struct WorkbenchPanel {
    kind: PanelKind,
    bee: Entity<Bee>,
    selection: Entity<scene::Selection>,
    focus: FocusHandle,
    tab: Option<WeakEntity<TabPanel>>,
}

impl WorkbenchPanel {
    fn new(
        kind: PanelKind,
        bee: Entity<Bee>,
        selection: Entity<scene::Selection>,
        cx: &mut Context<Self>,
    ) -> Self {
        cx.observe(&bee, |_, _, cx| cx.notify()).detach();
        if matches!(kind, PanelKind::Scene | PanelKind::Properties) {
            cx.observe(&selection, |_, _, cx| cx.notify()).detach();
        }
        Self {
            kind,
            bee,
            selection,
            focus: cx.focus_handle(),
            tab: None,
        }
    }
}

impl EventEmitter<PanelEvent> for WorkbenchPanel {}

impl Focusable for WorkbenchPanel {
    fn focus_handle(&self, _: &App) -> FocusHandle {
        self.focus.clone()
    }
}

impl Panel for WorkbenchPanel {
    fn panel_name(&self) -> &'static str {
        self.kind.title()
    }

    fn title(&mut self, _: &mut Window, _: &mut Context<Self>) -> impl IntoElement {
        let selector = format!("tab-{}", self.kind.id());
        div()
            .id(SharedString::from(selector.clone()))
            .debug_selector(move || selector)
            .child(self.kind.title())
    }

    fn on_added_to(&mut self, tab: WeakEntity<TabPanel>, _: &mut Window, _: &mut Context<Self>) {
        self.tab = Some(tab);
    }

    fn on_removed(&mut self, _: &mut Window, _: &mut Context<Self>) {
        self.tab = None;
    }

    fn inner_padding(&self, _: &App) -> bool {
        false
    }
}

impl Render for WorkbenchPanel {
    fn render(&mut self, _: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        let kind = self.kind;
        let content = match kind {
            PanelKind::Scene => scene::scene_list(
                &self.bee.read(cx).state,
                self.selection.read(cx),
                self.bee.clone(),
                self.selection.clone(),
            )
            .into_any_element(),
            PanelKind::Properties => {
                scene::properties(&self.bee.read(cx).state, self.selection.read(cx))
                    .into_any_element()
            }
            _ => self.bee.update(cx, |bee, cx| match kind {
                PanelKind::Viewport => bee.viewport.render(&bee.state, cx).into_any_element(),
                PanelKind::Flow => bee.viewport.flow.render(cx).into_any_element(),
                PanelKind::Files => div().child("No folder selected").into_any_element(),
                PanelKind::Console => div()
                    .flex()
                    .flex_col()
                    .gap_2()
                    .children(bee.lines.iter().cloned())
                    .children(
                        bee.viewport
                            .error()
                            .map(|error| format!("Viewport error: {error}")),
                    )
                    .into_any_element(),
                PanelKind::Scene | PanelKind::Properties => unreachable!(),
            }),
        };

        let selector = format!("panel-content-{}", kind.id());
        div()
            .id(SharedString::from(selector.clone()))
            .debug_selector(move || selector)
            .track_focus(&self.focus)
            .size_full()
            .min_w_0()
            .min_h_0()
            .bg(rgb(PANEL))
            .text_color(rgb(TEXT))
            .border_1()
            .border_color(rgb(PANEL))
            .focus(|style| style.border_color(rgb(ACCENT)))
            .p_2()
            .when(kind == PanelKind::Viewport, |this| this.flex().flex_col())
            .when(kind != PanelKind::Viewport, |this| this.overflow_y_scroll())
            .child(content)
    }
}

pub(crate) struct Workbench {
    dock: Entity<DockArea>,
    panels: Vec<Entity<WorkbenchPanel>>,
    menu_focus: FocusHandle,
    menu: Option<Entity<PopupMenu>>,
    menu_press_was_open: bool,
}

impl Workbench {
    pub(crate) fn new(bee: Entity<Bee>, window: &mut Window, cx: &mut Context<Self>) -> Self {
        let selection = cx.new(|_| scene::Selection::default());
        let observed_selection = selection.clone();
        cx.observe(&bee, move |_, bee, cx| {
            if observed_selection.read(cx).is_stale(&bee.read(cx).state) {
                observed_selection.update(cx, |selection, cx| {
                    selection.clear();
                    cx.notify();
                });
            }
        })
        .detach();
        let panels = PanelKind::ALL
            .into_iter()
            .map(|kind| cx.new(|cx| WorkbenchPanel::new(kind, bee.clone(), selection.clone(), cx)))
            .collect::<Vec<_>>();
        let dock = cx.new(|cx| {
            DockArea::new("bee-workbench", None, window, cx).panel_style(PanelStyle::TabBar)
        });
        let mut this = Self {
            dock,
            panels,
            menu_focus: cx.focus_handle().tab_index(0).tab_stop(true),
            menu: None,
            menu_press_was_open: false,
        };
        this.reset(window, cx);
        this
    }

    fn reset(&mut self, window: &mut Window, cx: &mut Context<Self>) {
        self.dock.update(cx, |dock, cx| {
            let weak = cx.entity().downgrade();
            let scene = DockItem::tab(self.panels[0].clone(), &weak, window, cx).size(px(240.0));
            let viewport = DockItem::tab(self.panels[1].clone(), &weak, window, cx).size(px(680.0));
            let properties =
                DockItem::tab(self.panels[2].clone(), &weak, window, cx).size(px(280.0));
            let top = DockItem::h_split(vec![scene, viewport, properties], &weak, window, cx)
                .size(px(600.0));
            let bottom = DockItem::tabs(
                self.panels[3..]
                    .iter()
                    .cloned()
                    .map(|panel| Arc::new(panel) as Arc<dyn PanelView>)
                    .collect(),
                &weak,
                window,
                cx,
            )
            .size(px(200.0));
            dock.set_center(
                DockItem::v_split(vec![top, bottom], &weak, window, cx),
                window,
                cx,
            );
        });
        cx.notify();
    }

    fn toggle_menu(&mut self, window: &mut Window, cx: &mut Context<Self>) {
        if self.menu.take().is_some() {
            window.focus(&self.menu_focus);
            cx.notify();
            return;
        }

        let workbench = cx.weak_entity();
        let menu = PopupMenu::build(window, cx, |mut menu, _, _| {
            menu = menu.min_w(px(180.0));
            for (index, kind) in PanelKind::ALL.into_iter().enumerate() {
                let workbench = workbench.clone();
                let selector = format!("show-{}", kind.id());
                menu = menu.item(
                    PopupMenuItem::element(move |_, _| {
                        let selector = selector.clone();
                        div()
                            .id(SharedString::from(selector.clone()))
                            .debug_selector(move || selector)
                            .child(kind.title())
                    })
                    .on_click(move |_, window, cx| {
                        workbench
                            .update(cx, |this, cx| this.show_panel(index, window, cx))
                            .ok();
                    }),
                );
            }
            let workbench = workbench.clone();
            menu.separator().item(
                PopupMenuItem::element(|_, _| {
                    div()
                        .id("reset-workbench")
                        .debug_selector(|| "reset-workbench".into())
                        .child("Reset layout")
                })
                .on_click(move |_, window, cx| {
                    workbench.update(cx, |this, cx| this.reset(window, cx)).ok();
                }),
            )
        });
        cx.subscribe_in(&menu, window, |this, menu, _: &DismissEvent, window, cx| {
            if menu.read(cx).focus_handle(cx).contains_focused(window, cx) {
                window.focus(&this.menu_focus);
            }
            this.menu = None;
            cx.notify();
        })
        .detach();
        window.focus(&menu.read(cx).focus_handle(cx));
        self.menu = Some(menu);
        cx.notify();
    }

    fn show_panel(&mut self, index: usize, window: &mut Window, cx: &mut Context<Self>) {
        let panel = self.panels[index].clone();
        let tab = panel.read(cx).tab.as_ref().and_then(WeakEntity::upgrade);
        let group = if index >= 3 {
            &self.panels[3..]
        } else {
            &self.panels[..3]
        };
        if let Some(tab) = tab {
            let active = tab
                .read(cx)
                .active_panel(cx)
                .is_some_and(|active| active.view().entity_id() == panel.entity_id());
            if !active {
                tab.update(cx, |tab, cx| {
                    let panel: Arc<dyn PanelView> = Arc::new(panel.clone());
                    tab.remove_panel(panel.clone(), window, cx);
                    tab.add_panel(panel, window, cx);
                });
            }
        } else if let Some(tab) = group
            .iter()
            .chain(self.panels.iter())
            .find_map(|panel| panel.read(cx).tab.as_ref().and_then(WeakEntity::upgrade))
        {
            tab.update(cx, |tab, cx| {
                tab.add_panel(Arc::new(panel.clone()), window, cx);
            });
        } else {
            self.dock.update(cx, |dock, cx| {
                let item = DockItem::tab(panel.clone(), &cx.entity().downgrade(), window, cx);
                dock.set_center(item, window, cx);
            });
        }
        window.focus(&panel.read(cx).focus_handle(cx));
        cx.notify();
    }
}

impl Render for Workbench {
    fn render(&mut self, _: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        let menu_open = self.menu.is_some();
        div()
            .size_full()
            .flex()
            .flex_col()
            .bg(rgb(BACKGROUND))
            .text_color(rgb(TEXT))
            .text_size(px(13.0))
            .on_action(cx.listener(|_, _: &crate::FocusNext, window, _| window.focus_next()))
            .on_action(cx.listener(|_, _: &crate::FocusPrevious, window, _| window.focus_prev()))
            .child(
                div()
                    .flex()
                    .items_center()
                    .h(px(36.0))
                    .flex_shrink_0()
                    .gap_3()
                    .px_3()
                    .border_b_1()
                    .border_color(rgb(BORDER))
                    .tab_group()
                    .child(div().font_semibold().text_color(rgb(ACCENT)).child("Bee"))
                    .child(
                        div()
                            .relative()
                            .child(
                                div()
                                    .id("panel-menu")
                                    .debug_selector(|| "panel-menu".into())
                                    .track_focus(&self.menu_focus)
                                    .tab_index(0)
                                    .tab_stop(true)
                                    .flex()
                                    .items_center()
                                    .gap_1()
                                    .h(px(28.0))
                                    .px_2()
                                    .rounded_sm()
                                    .border_1()
                                    .border_color(rgb(BACKGROUND))
                                    .cursor_pointer()
                                    .hover(|style| style.bg(rgb(PANEL)))
                                    .focus(|style| style.border_color(rgb(ACCENT)))
                                    .when(menu_open, |this| this.bg(rgb(PANEL)))
                                    .child("Panels")
                                    .child(Icon::new(IconName::ChevronDown).xsmall())
                                    .on_mouse_down(
                                        MouseButton::Left,
                                        cx.listener(move |this, _, _, _| {
                                            this.menu_press_was_open = menu_open;
                                        }),
                                    )
                                    .on_click(cx.listener(|this, _, window, cx| {
                                        if this.menu_press_was_open {
                                            this.menu = None;
                                            window.focus(&this.menu_focus);
                                            cx.notify();
                                        } else {
                                            this.toggle_menu(window, cx);
                                        }
                                    }))
                                    .on_key_down(cx.listener(
                                        |this, event: &KeyDownEvent, window, cx| {
                                            if matches!(
                                                event.keystroke.key.as_str(),
                                                "enter" | "space" | "down"
                                            ) {
                                                this.toggle_menu(window, cx);
                                            }
                                        },
                                    )),
                            )
                            .when_some(self.menu.clone(), |this, menu| {
                                this.child(deferred(
                                    anchored()
                                        .anchor(Corner::TopLeft)
                                        .snap_to_window_with_margin(px(8.0))
                                        .child(div().top_1().occlude().child(menu)),
                                ))
                            }),
                    ),
            )
            .child(
                div()
                    .flex_1()
                    .min_h_0()
                    .min_w_0()
                    .tab_index(1)
                    .tab_stop(false)
                    .tab_group()
                    .child(self.dock.clone()),
            )
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use gpui::{
        point, size, KeyBinding, Modifiers, MouseButton, ScrollDelta, ScrollWheelEvent,
        VisualTestContext,
    };
    use gpui_component::dock::ClosePanel;

    fn redraw(cx: &mut VisualTestContext, workbench: &Entity<Workbench>) {
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(960.0), px(640.0)),
            |_, _| workbench.clone(),
        );
        cx.run_until_parked();
    }

    fn click(cx: &mut VisualTestContext, workbench: &Entity<Workbench>, selector: &'static str) {
        if selector.starts_with("show-") || selector == "reset-workbench" {
            let trigger = cx.debug_bounds("panel-menu").unwrap().center();
            cx.simulate_click(trigger, Modifiers::none());
            redraw(cx, workbench);
        }
        let bounds = cx.debug_bounds(selector).expect(selector);
        cx.simulate_click(bounds.center(), Modifiers::none());
        redraw(cx, workbench);
    }

    fn names(state: &gpui_component::dock::PanelState, result: &mut Vec<String>) {
        if PanelKind::ALL
            .iter()
            .any(|kind| kind.title() == state.panel_name)
        {
            result.push(state.panel_name.clone());
        }
        for child in &state.children {
            names(child, result);
        }
    }

    #[test]
    fn scene_selection_shows_real_objects_and_clears_on_replacement() {
        let mut app = gpui::TestAppContext::single();
        app.update(|cx| {
            gpui_component::init(cx);
            init_theme(cx);
            cx.bind_keys([
                KeyBinding::new("tab", crate::FocusNext, None),
                KeyBinding::new("shift-tab", crate::FocusPrevious, None),
            ]);
        });
        let (workbench, cx) = app.add_window_view(|window, cx| {
            let bee = cx.new(|_| Bee::failure("test device".into()));
            Workbench::new(bee, window, cx)
        });
        cx.simulate_resize(size(px(960.0), px(640.0)));
        redraw(cx, &workbench);
        let bee = cx.update(|_, cx| workbench.read(cx).panels[0].read(cx).bee.clone());
        let selection = cx.update(|_, cx| workbench.read(cx).panels[0].read(cx).selection.clone());
        let versions = cx.update(|_, cx| bee.read(cx).state.versions());
        let (instances, lights) = cx.update(|_, cx| {
            let scene = bee.read(cx).state.scene();
            (scene.instances.len(), scene.lights.len())
        });
        assert_eq!((instances, lights), (3, 1));
        for selector in [
            "scene-instance-0",
            "scene-instance-1",
            "scene-instance-2",
            "scene-light-0",
            "scene-camera",
            "properties-empty-0",
        ] {
            assert!(cx.debug_bounds(selector).is_some(), "{selector}");
        }
        assert!(cx.debug_bounds("scene-instance-3").is_none());
        assert!(cx.debug_bounds("scene-light-1").is_none());

        cx.update(|window, cx| window.focus(&workbench.read(cx).menu_focus));
        redraw(cx, &workbench);
        cx.simulate_keystrokes("tab enter");
        redraw(cx, &workbench);
        cx.update(|_, cx| {
            assert_eq!(
                selection.read(cx).current(&bee.read(cx).state),
                Some(scene::Item::Instance(0))
            );
        });

        for (selector, expected, field) in [
            (
                "scene-instance-1",
                scene::Item::Instance(1),
                "property-albedo",
            ),
            ("scene-light-0", scene::Item::Light(0), "property-intensity"),
            ("scene-camera", scene::Item::Camera, "property-fov"),
        ] {
            click(cx, &workbench, selector);
            cx.update(|_, cx| {
                assert_eq!(
                    selection.read(cx).current(&bee.read(cx).state),
                    Some(expected)
                );
                assert_eq!(bee.read(cx).state.versions(), versions);
            });
            assert!(cx.debug_bounds(field).is_some(), "{field}");
        }

        click(cx, &workbench, "scene-clear");
        assert!(cx.debug_bounds("properties-empty-0").is_some());
        cx.update(|_, cx| {
            assert_eq!(selection.read(cx).current(&bee.read(cx).state), None);
            assert_eq!(bee.read(cx).state.versions(), versions);
        });

        click(cx, &workbench, "scene-instance-0");
        cx.update(|_, cx| {
            bee.update(cx, |bee, cx| {
                let mut replacement = bee.state.scene().clone();
                replacement.environment = [0.3, 0.2, 0.1];
                assert!(bee.state.set_scene(replacement));
                cx.notify();
            });
        });
        cx.run_until_parked();
        redraw(cx, &workbench);
        assert!(cx.debug_bounds("properties-empty-1").is_some());
        cx.update(|_, cx| {
            let selection = selection.read(cx);
            assert_eq!(selection.current(&bee.read(cx).state), None);
            assert!(!selection.is_stale(&bee.read(cx).state));
        });

        click(cx, &workbench, "scene-instance-2");
        cx.update(|_, cx| {
            bee.update(cx, |bee, cx| {
                let mut replacement = bee.state.scene().clone();
                replacement.instances.clear();
                replacement.lights.clear();
                assert!(bee.state.set_scene(replacement));
                cx.notify();
            });
        });
        cx.run_until_parked();
        redraw(cx, &workbench);
        assert!(cx.debug_bounds("scene-instances-empty").is_some());
        assert!(cx.debug_bounds("scene-lights-empty").is_some());
        assert!(cx.debug_bounds("scene-camera").is_some());
        assert!(cx.debug_bounds("properties-empty-2").is_some());
    }

    #[test]
    fn panel_entries_and_layout_preserve_scene_camera_and_flow_draft() {
        let mut app = gpui::TestAppContext::single();
        app.update(|cx| {
            gpui_component::init(cx);
            init_theme(cx);
            cx.bind_keys([
                KeyBinding::new("tab", crate::FocusNext, None),
                KeyBinding::new("shift-tab", crate::FocusPrevious, None),
                KeyBinding::new("ctrl-w", ClosePanel, None),
            ]);
        });
        let (workbench, cx) = app.add_window_view(|window, cx| {
            let bee = cx.new(|_| Bee::failure("test device".into()));
            Workbench::new(bee, window, cx)
        });
        cx.simulate_resize(size(px(960.0), px(640.0)));
        redraw(cx, &workbench);

        cx.update(|_, cx| {
            let workbench = workbench.read(cx);
            let mut actual = Vec::new();
            names(&workbench.dock.read(cx).dump(cx).center, &mut actual);
            assert_eq!(
                actual,
                [
                    "Scene",
                    "Viewport",
                    "Properties",
                    "Files",
                    "Console",
                    "Render Flow"
                ]
            );
        });

        for (entry, content) in [
            ("show-scene", "panel-content-scene"),
            ("show-viewport", "panel-content-viewport"),
            ("show-properties", "panel-content-properties"),
            ("show-files", "panel-content-files"),
            ("show-console", "panel-content-console"),
            ("show-flow", "panel-content-flow"),
        ] {
            click(cx, &workbench, entry);
            let actual = cx.update(|_, cx| {
                let workbench = workbench.read(cx);
                workbench
                    .panels
                    .iter()
                    .filter_map(|panel| {
                        let panel = panel.read(cx);
                        let tab = panel.tab.as_ref()?.upgrade()?;
                        Some((
                            panel.kind.title(),
                            tab.read(cx)
                                .active_panel(cx)
                                .map(|active| active.panel_name(cx)),
                        ))
                    })
                    .collect::<Vec<_>>()
            });
            assert!(cx.debug_bounds(content).is_some(), "{content}: {actual:?}");
        }
        let flow_bounds = cx.debug_bounds("panel-content-flow").unwrap();
        cx.simulate_event(ScrollWheelEvent {
            position: flow_bounds.center(),
            delta: ScrollDelta::Lines(point(0.0, -1.0)),
            ..Default::default()
        });
        redraw(cx, &workbench);
        let add_button = cx.debug_bounds("flow-add-raster").unwrap();
        assert!(
            add_button.center().y > flow_bounds.top()
                && add_button.center().y < flow_bounds.bottom(),
            "flow: {flow_bounds:?}, add: {add_button:?}"
        );
        click(cx, &workbench, "flow-add-raster");

        let bee = cx.update(|_, cx| workbench.read(cx).panels[0].read(cx).bee.clone());
        cx.update(|_, cx| {
            bee.update(cx, |bee, cx| {
                let mut scene = bee.state.scene().clone();
                scene.environment = [0.3, 0.2, 0.1];
                assert!(bee.state.set_scene(scene));
                let mut camera = bee.state.camera();
                assert!(camera.orbit(20.0, 10.0));
                assert!(bee.state.set_camera(camera));
                cx.notify();
            });
        });
        let (versions, camera, scene, draft) = cx.update(|_, cx| {
            let bee = bee.read(cx);
            (
                bee.state.versions(),
                bee.state.camera(),
                bee.state.snapshot().scene,
                bee.viewport.flow.draft().clone(),
            )
        });
        assert_eq!(draft.nodes().count(), 5);

        let before = cx.debug_bounds("panel-content-scene").unwrap();
        let start = point(before.right(), before.center().y);
        cx.simulate_mouse_move(start, None, Modifiers::none());
        cx.simulate_mouse_down(start, MouseButton::Left, Modifiers::none());
        cx.simulate_mouse_move(
            point(start.x + px(8.0), start.y),
            MouseButton::Left,
            Modifiers::none(),
        );
        redraw(cx, &workbench);
        let end = point(start.x + px(60.0), start.y);
        cx.simulate_mouse_move(end, MouseButton::Left, Modifiers::none());
        redraw(cx, &workbench);
        cx.simulate_mouse_up(end, MouseButton::Left, Modifiers::none());
        redraw(cx, &workbench);
        let after = cx.debug_bounds("panel-content-scene").unwrap();
        assert!(after.size.width > before.size.width + px(10.0));

        click(cx, &workbench, "show-flow");
        cx.simulate_keystrokes("ctrl-w");
        redraw(cx, &workbench);
        assert!(cx.update(|_, cx| workbench.read(cx).panels[5].read(cx).tab.is_none()));
        click(cx, &workbench, "show-flow");
        assert!(cx.debug_bounds("panel-content-flow").is_some());
        click(cx, &workbench, "reset-workbench");
        click(cx, &workbench, "show-flow");

        cx.update(|_, cx| {
            let bee = bee.read(cx);
            assert_eq!(bee.state.versions(), versions);
            assert_eq!(bee.state.camera(), camera);
            assert!(Arc::ptr_eq(&bee.state.snapshot().scene, &scene));
            assert_eq!(bee.viewport.flow.draft(), &draft);
        });

        let start = cx.debug_bounds("tab-flow").expect("flow tab").center();
        let scene_center = cx
            .debug_bounds("panel-content-scene")
            .expect("scene panel")
            .center();
        cx.simulate_mouse_move(start, None, Modifiers::none());
        cx.simulate_mouse_down(start, MouseButton::Left, Modifiers::none());
        cx.simulate_mouse_move(
            point(start.x + px(20.0), start.y),
            MouseButton::Left,
            Modifiers::none(),
        );
        cx.simulate_mouse_move(scene_center, MouseButton::Left, Modifiers::none());
        redraw(cx, &workbench);
        cx.simulate_mouse_up(scene_center, MouseButton::Left, Modifiers::none());
        redraw(cx, &workbench);
        cx.update(|_, cx| {
            let workbench = workbench.read(cx);
            let scene_tab = workbench.panels[0].read(cx).tab.as_ref().unwrap().upgrade();
            let flow_tab = workbench.panels[5].read(cx).tab.as_ref().unwrap().upgrade();
            assert_eq!(scene_tab, flow_tab);
            let bee = bee.read(cx);
            assert_eq!(bee.state.versions(), versions);
            assert_eq!(bee.state.camera(), camera);
            assert!(Arc::ptr_eq(&bee.state.snapshot().scene, &scene));
            assert_eq!(bee.viewport.flow.draft(), &draft);
        });
    }

    #[test]
    fn panel_entries_fit_and_work_with_keyboard_at_small_sizes() {
        let mut app = gpui::TestAppContext::single();
        app.update(|cx| {
            gpui_component::init(cx);
            init_theme(cx);
            cx.bind_keys([
                KeyBinding::new("tab", crate::FocusNext, None),
                KeyBinding::new("shift-tab", crate::FocusPrevious, None),
            ]);
        });
        let mut workbench = None;
        let (root, cx) = app.add_window_view(|window, cx| {
            let bee = cx.new(|_| Bee::failure("test device".into()));
            let view = cx.new(|cx| Workbench::new(bee, window, cx));
            workbench = Some(view.clone());
            gpui_component::Root::new(view, window, cx)
        });
        let workbench = workbench.unwrap();

        for dimensions in [
            size(px(1200.0), px(800.0)),
            size(px(960.0), px(640.0)),
            size(px(640.0), px(426.0)),
        ] {
            cx.simulate_resize(dimensions);
            cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
            cx.run_until_parked();
            assert_eq!(cx.update(|window, _| window.scale_factor()), 2.0);
            let menu = cx.debug_bounds("panel-menu").unwrap();
            assert!(menu.right() <= dimensions.width && menu.bottom() <= dimensions.height);
            let viewport = cx.debug_bounds("panel-content-viewport").unwrap();
            let canvas = cx.debug_bounds("scene-canvas").unwrap();
            assert!(
                canvas.size.width > px(0.0) && canvas.size.height > px(0.0),
                "canvas has no drawable area: {canvas:?} in {viewport:?}"
            );
            if dimensions.width >= px(960.0) {
                let scene = cx.debug_bounds("panel-content-scene").unwrap();
                let properties = cx.debug_bounds("panel-content-properties").unwrap();
                assert!(
                    viewport.size.width > scene.size.width
                        && viewport.size.width > properties.size.width,
                    "viewport: {viewport:?}, scene: {scene:?}, properties: {properties:?}"
                );
            }
            for selector in ["raster-mode", "path-mode"] {
                let bounds = cx.debug_bounds(selector).expect(selector);
                assert!(
                    bounds.left() >= viewport.left()
                        && bounds.right() <= viewport.right()
                        && bounds.bottom() <= viewport.bottom(),
                    "{selector}: {bounds:?} in {viewport:?}"
                );
            }
            cx.simulate_click(menu.center(), Modifiers::none());
            cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
            cx.run_until_parked();
            for selector in [
                "show-scene",
                "show-viewport",
                "show-properties",
                "show-files",
                "show-console",
                "show-flow",
                "reset-workbench",
            ] {
                let item = cx.debug_bounds(selector).expect(selector);
                assert!(
                    item.left() >= px(0.0)
                        && item.top() >= menu.bottom()
                        && item.right() <= dimensions.width
                        && item.bottom() <= dimensions.height,
                    "{selector}: {item:?} in {dimensions:?}"
                );
            }
            cx.simulate_click(menu.center(), Modifiers::none());
            cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
            cx.run_until_parked();
            assert!(cx.update(|_, cx| workbench.read(cx).menu.is_none()));
        }

        cx.update(|window, cx| window.focus(&workbench.read(cx).menu_focus));
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(640.0), px(426.0)),
            |_, _| root.clone(),
        );
        cx.simulate_keystrokes("tab");
        assert!(cx.update(|window, cx| workbench.read(cx).panels[0]
            .read(cx)
            .focus
            .contains_focused(window, cx)));
        cx.simulate_keystrokes("shift-tab");
        assert!(cx.update(|window, cx| workbench.read(cx).menu_focus.is_focused(window)));
        cx.simulate_keystrokes("enter");
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(640.0), px(426.0)),
            |_, _| root.clone(),
        );
        cx.simulate_keystrokes("escape");
        assert!(cx.update(|window, cx| {
            let workbench = workbench.read(cx);
            workbench.menu.is_none() && workbench.menu_focus.is_focused(window)
        }));

        for (index, content) in [
            "panel-content-scene",
            "panel-content-viewport",
            "panel-content-properties",
            "panel-content-files",
            "panel-content-console",
            "panel-content-flow",
        ]
        .into_iter()
        .enumerate()
        {
            cx.update(|window, cx| window.focus(&workbench.read(cx).menu_focus));
            cx.simulate_keystrokes(if index % 2 == 0 { "enter" } else { "space" });
            cx.draw(
                point(px(0.0), px(0.0)),
                size(px(640.0), px(426.0)),
                |_, _| root.clone(),
            );
            cx.run_until_parked();
            for _ in 0..=index {
                cx.simulate_keystrokes("down");
            }
            cx.simulate_keystrokes("enter");
            cx.draw(
                point(px(0.0), px(0.0)),
                size(px(640.0), px(426.0)),
                |_, _| root.clone(),
            );
            cx.run_until_parked();
            assert!(cx.debug_bounds(content).is_some(), "{content}");
        }

        let flow_bounds = cx.debug_bounds("panel-content-flow").unwrap();
        for _ in 0..8 {
            let add_button = cx.debug_bounds("flow-add-raster").unwrap();
            if add_button.center().y < flow_bounds.bottom() {
                break;
            }
            cx.simulate_event(ScrollWheelEvent {
                position: flow_bounds.center(),
                delta: ScrollDelta::Lines(point(0.0, -1.0)),
                ..Default::default()
            });
            cx.draw(
                point(px(0.0), px(0.0)),
                size(px(640.0), px(426.0)),
                |_, _| root.clone(),
            );
            cx.run_until_parked();
        }
        let add_button = cx.debug_bounds("flow-add-raster").unwrap();
        assert!(
            add_button.center().x >= flow_bounds.left()
                && add_button.center().x < flow_bounds.right()
                && add_button.center().y >= flow_bounds.top()
                && add_button.center().y < flow_bounds.bottom(),
            "flow: {flow_bounds:?}, add: {add_button:?}"
        );
        cx.simulate_click(add_button.center(), Modifiers::none());
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(640.0), px(426.0)),
            |_, _| root.clone(),
        );
        cx.run_until_parked();
        cx.update(|window, cx| window.focus(&workbench.read(cx).panels[5].read(cx).focus));
        cx.simulate_keystrokes("tab");
        assert!(cx.update(|window, cx| workbench.read(cx).panels[5]
            .read(cx)
            .focus
            .contains_focused(window, cx)));
        cx.update(|_, cx| {
            let bee = workbench.read(cx).panels[5].read(cx).bee.read(cx);
            assert_eq!(bee.viewport.flow.draft().nodes().count(), 5);
        });

        cx.update(|window, cx| window.focus(&workbench.read(cx).menu_focus));
        cx.simulate_keystrokes("enter");
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(640.0), px(426.0)),
            |_, _| root.clone(),
        );
        for _ in 0..=PanelKind::ALL.len() {
            cx.simulate_keystrokes("down");
        }
        cx.simulate_keystrokes("enter");
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(640.0), px(426.0)),
            |_, _| root.clone(),
        );
        cx.run_until_parked();
        assert!(cx.debug_bounds("panel-content-files").is_some());
        cx.update(|_, cx| {
            let bee = workbench.read(cx).panels[5].read(cx).bee.read(cx);
            assert_eq!(bee.viewport.flow.draft().nodes().count(), 5);
        });
    }
}
