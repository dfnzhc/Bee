use crate::Bee;
use gpui::{
    anchored, deferred, div, prelude::*, px, rgb, App, Context, Corner, DismissEvent, Entity,
    EventEmitter, FocusHandle, Focusable, KeyDownEvent, MouseButton, Pixels, SharedString,
    Subscription, WeakEntity, Window,
};
use gpui_component::{
    button::{Button, ButtonVariants},
    dock::{
        DockArea, DockEvent, DockItem, Panel, PanelEvent, PanelState, PanelStyle, PanelView,
        StackPanel, TabPanel, ToggleZoom,
    },
    menu::{PopupMenu, PopupMenuItem},
    Icon, IconName, Sizable, StyledExt, Theme, ThemeMode,
};
use std::sync::Arc;

pub(crate) mod console;
mod files;
mod scene;

const BACKGROUND: u32 = 0x1b1f20;
const PANEL: u32 = 0x24292a;
const BORDER: u32 = 0x454e4c;
const TEXT: u32 = 0xe9efec;
const MUTED: u32 = 0xb1bfba;
const ACCENT: u32 = 0x79c8b0;
const SELECTED: u32 = 0x344841;
const HOVER: u32 = 0x333b39;
const DRAWER_HEIGHT: Pixels = px(200.0);

pub(crate) fn init_theme(cx: &mut App) {
    Theme::change(ThemeMode::Dark, None, cx);
    let theme = Theme::global_mut(cx);
    theme.background = rgb(BACKGROUND).into();
    theme.foreground = rgb(TEXT).into();
    theme.border = rgb(BORDER).into();
    theme.ring = rgb(ACCENT).into();
    theme.tab_bar = rgb(BACKGROUND).into();
    theme.tab_active = rgb(SELECTED).into();
    theme.tab_active_foreground = rgb(TEXT).into();
    theme.tab_foreground = rgb(MUTED).into();
    theme.popover = rgb(PANEL).into();
    theme.popover_foreground = rgb(TEXT).into();
    theme.accent = rgb(SELECTED).into();
    theme.accent_foreground = rgb(TEXT).into();
    theme.secondary = rgb(PANEL).into();
    theme.secondary_hover = rgb(HOVER).into();
    theme.secondary_active = rgb(SELECTED).into();
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
    const DRAWER_START: usize = Self::Files as usize;

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
    zoom_subscription: Option<Subscription>,
    tab_zoomed: bool,
    files: files::Files,
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
        if kind == PanelKind::Console {
            // 消息统一在界面线程收取；面板关闭后仍保留有界历史。
            cx.spawn(async move |panel, cx| loop {
                cx.background_executor()
                    .timer(std::time::Duration::from_millis(100))
                    .await;
                if panel
                    .update(cx, |panel, cx| {
                        if panel.bee.update(cx, |bee, _| bee.console.drain()) {
                            cx.notify();
                        }
                    })
                    .is_err()
                {
                    break;
                }
            })
            .detach();
        }
        Self {
            kind,
            bee,
            selection,
            focus: cx.focus_handle(),
            tab: None,
            zoom_subscription: None,
            tab_zoomed: false,
            files: files::Files::default(),
        }
    }

    fn set_viewport_active(&self, active: bool, window: &mut Window, cx: &mut Context<Self>) {
        self.bee.update(cx, |bee, cx| {
            bee.viewport.set_active(active, &bee.state, window, cx);
        });
    }

    fn tab(&self) -> Option<Entity<TabPanel>> {
        self.tab.as_ref()?.upgrade()
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
            .text_size(px(12.0))
            .line_height(px(16.0))
            .text_ellipsis()
            .child(self.kind.title())
    }

    fn set_active(&mut self, active: bool, window: &mut Window, cx: &mut Context<Self>) {
        if self.kind == PanelKind::Viewport {
            self.set_viewport_active(active, window, cx);
        }
    }

    fn on_added_to(
        &mut self,
        tab: WeakEntity<TabPanel>,
        window: &mut Window,
        cx: &mut Context<Self>,
    ) {
        self.tab_zoomed = false;
        self.zoom_subscription = tab.upgrade().map(|tab| {
            cx.subscribe_in(&tab, window, |this, tab, event, window, cx| {
                let zoomed = match event {
                    PanelEvent::ZoomIn => {
                        this.tab_zoomed = true;
                        true
                    }
                    PanelEvent::ZoomOut => {
                        this.tab_zoomed = false;
                        false
                    }
                    PanelEvent::LayoutChanged => {
                        if this.kind == PanelKind::Viewport || this.tab_zoomed {
                            let tab = tab.downgrade();
                            let panel = cx.weak_entity();
                            window.defer(cx, move |window, cx| {
                                let Some(tab) = tab.upgrade() else {
                                    return;
                                };
                                let active = tab.read(cx).active_panel(cx).is_some_and(|panel| {
                                    panel.panel_name(cx) == PanelKind::Viewport.title()
                                });
                                panel
                                    .update(cx, |this, cx| {
                                        if this.tab.as_ref().and_then(WeakEntity::upgrade)
                                            == Some(tab)
                                        {
                                            if this.kind == PanelKind::Viewport {
                                                this.set_viewport_active(active, window, cx);
                                            }
                                            if this.tab_zoomed {
                                                this.bee.update(cx, |bee, cx| {
                                                    bee.viewport.set_zoom_hidden(
                                                        !active, &bee.state, window, cx,
                                                    );
                                                });
                                            }
                                        }
                                    })
                                    .ok();
                            });
                        }
                        return;
                    }
                };
                let tab = tab.downgrade();
                let bee = this.bee.downgrade();
                window.defer(cx, move |window, cx| {
                    let hidden = zoomed
                        && tab.upgrade().is_some_and(|tab| {
                            tab.read(cx).active_panel(cx).is_some_and(|panel| {
                                panel.panel_name(cx) != PanelKind::Viewport.title()
                            })
                        });
                    bee.update(cx, |bee, cx| {
                        bee.viewport.set_zoom_hidden(hidden, &bee.state, window, cx);
                    })
                    .ok();
                });
            })
        });
        self.tab = Some(tab);
    }

    fn on_removed(&mut self, window: &mut Window, cx: &mut Context<Self>) {
        // 停靠库在移除面板后才发送缩放恢复事件，订阅保留至下次停靠。
        self.tab = None;
        if self.kind == PanelKind::Viewport {
            self.set_viewport_active(false, window, cx);
        }
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
            PanelKind::Files => self.files.render(cx).into_any_element(),
            PanelKind::Console => self
                .bee
                .read(cx)
                .console
                .render(self.bee.clone())
                .into_any_element(),
            _ => self.bee.update(cx, |bee, cx| match kind {
                PanelKind::Viewport => bee.viewport.render(&bee.state, cx).into_any_element(),
                PanelKind::Flow => bee.viewport.flow.render(cx).into_any_element(),
                PanelKind::Scene
                | PanelKind::Properties
                | PanelKind::Files
                | PanelKind::Console => unreachable!(),
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
            .border_color(rgb(BORDER))
            .focus(|style| style.border_color(rgb(ACCENT)))
            .when(kind == PanelKind::Viewport, |this| {
                this.p_2().flex().flex_col()
            })
            .when(kind != PanelKind::Viewport, |this| {
                this.p_1p5().overflow_y_scroll()
            })
            .child(content)
    }
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum AppMenu {
    File,
    View,
}

struct Drawer {
    stack: Entity<StackPanel>,
    tabs: Entity<TabPanel>,
    open: bool,
    height: Pixels,
}

fn contains_panel(state: &PanelState, kind: PanelKind) -> bool {
    state.panel_name == kind.title()
        || state
            .children
            .iter()
            .any(|child| contains_panel(child, kind))
}

pub(crate) struct Workbench {
    bee: Entity<Bee>,
    dock: Entity<DockArea>,
    panels: Vec<Entity<WorkbenchPanel>>,
    drawer: Drawer,
    drawer_focus: [FocusHandle; 3],
    menu_focus: [FocusHandle; 2],
    menu: Option<(AppMenu, Entity<PopupMenu>)>,
    menu_press_was_open: bool,
}

impl Workbench {
    pub(crate) fn new(bee: Entity<Bee>, window: &mut Window, cx: &mut Context<Self>) -> Self {
        let closing_bee = bee.downgrade();
        window.on_window_should_close(cx, move |window, cx| {
            Self::request_close(&closing_bee, window, cx)
        });
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
        let drawer = Self::build_layout(&dock, &panels, window, cx);
        window.blur();
        cx.subscribe(&dock, |this, _, event, cx| {
            if matches!(event, DockEvent::LayoutChanged) {
                // 停靠组件会在最后一个面板移走或关闭后移除空分区。
                if this.drawer.open && this.drawer.stack.read(cx).dump(cx).children.is_empty() {
                    this.drawer.open = false;
                }
                cx.notify();
            }
        })
        .detach();
        Self {
            bee,
            dock,
            panels,
            drawer,
            drawer_focus: std::array::from_fn(|_| cx.focus_handle().tab_index(0).tab_stop(true)),
            menu_focus: std::array::from_fn(|_| cx.focus_handle().tab_index(0).tab_stop(true)),
            menu: None,
            menu_press_was_open: false,
        }
    }

    fn request_close(bee: &WeakEntity<Bee>, window: &mut Window, cx: &mut App) -> bool {
        bee.update(cx, |bee, _| bee.viewport.request_close(window))
            .unwrap_or(true)
    }

    fn reset(&mut self, window: &mut Window, cx: &mut Context<Self>) {
        self.bee.update(cx, |bee, cx| {
            bee.viewport.set_zoom_hidden(false, &bee.state, window, cx);
        });
        self.drawer = Self::build_layout(&self.dock, &self.panels, window, cx);
        window.focus(self.menu_focus(AppMenu::View));
        cx.notify();
    }

    fn build_layout(
        dock: &Entity<DockArea>,
        panels: &[Entity<WorkbenchPanel>],
        window: &mut Window,
        cx: &mut App,
    ) -> Drawer {
        dock.update(cx, |dock, cx| {
            dock.set_zoomed_out(window, cx);
            let weak = cx.entity().downgrade();
            let top = DockItem::h_split(
                [200.0, 780.0, 220.0]
                    .into_iter()
                    .zip(panels)
                    .map(|(size, panel)| {
                        DockItem::tab(panel.clone(), &weak, window, cx).size(px(size))
                    })
                    .collect(),
                &weak,
                window,
                cx,
            );
            let bottom = DockItem::tabs(
                panels[PanelKind::DRAWER_START..]
                    .iter()
                    .cloned()
                    .map(|panel| Arc::new(panel) as Arc<dyn PanelView>)
                    .collect(),
                &weak,
                window,
                cx,
            );
            let DockItem::Tabs { view: tabs, .. } = &bottom else {
                unreachable!();
            };
            let tabs = tabs.clone();
            let DockItem::Split { view: stack, .. } =
                DockItem::v_split(vec![bottom], &weak, window, cx)
            else {
                unreachable!();
            };
            dock.set_center(DockItem::v_split(vec![top], &weak, window, cx), window, cx);
            Drawer {
                stack,
                tabs,
                open: false,
                height: DRAWER_HEIGHT,
            }
        })
    }

    fn menu_focus(&self, kind: AppMenu) -> &FocusHandle {
        &self.menu_focus[kind as usize]
    }

    fn menu_item(label: &'static str, selector: impl Into<SharedString>) -> PopupMenuItem {
        let selector = selector.into();
        PopupMenuItem::element(move |_, _| {
            let selector = selector.clone();
            div()
                .id(selector.clone())
                .debug_selector(move || selector.to_string())
                .child(label)
        })
    }

    fn toggle_menu(&mut self, kind: AppMenu, window: &mut Window, cx: &mut Context<Self>) {
        if self.menu.as_ref().is_some_and(|(open, _)| *open == kind) {
            self.menu = None;
            window.focus(self.menu_focus(kind));
            cx.notify();
            return;
        }

        let workbench = cx.weak_entity();
        let bee = self.bee.downgrade();
        let menu = PopupMenu::build(window, cx, |mut menu, _, _| {
            menu = menu.min_w(px(180.0));
            if kind == AppMenu::File {
                return menu.item(Self::menu_item("Exit", "exit-bee").on_click(
                    move |_, window, cx| {
                        if Self::request_close(&bee, window, cx) {
                            window.remove_window();
                        }
                    },
                ));
            }

            for (index, panel) in PanelKind::ALL.into_iter().enumerate() {
                let workbench = workbench.clone();
                menu = menu.item(
                    Self::menu_item(panel.title(), format!("show-{}", panel.id())).on_click(
                        move |_, window, cx| {
                            workbench
                                .update(cx, |this, cx| this.show_panel(index, window, cx))
                                .ok();
                        },
                    ),
                );
            }
            menu.separator()
                .item(Self::menu_item("Reset Layout", "reset-workbench").on_click(
                    move |_, window, cx| {
                        workbench.update(cx, |this, cx| this.reset(window, cx)).ok();
                    },
                ))
        });
        cx.subscribe_in(
            &menu,
            window,
            move |this, menu, _: &DismissEvent, window, cx| {
                if this.menu.as_ref().is_some_and(|(_, open)| open == menu) {
                    if menu.read(cx).focus_handle(cx).contains_focused(window, cx) {
                        window.focus(this.menu_focus(kind));
                    }
                    this.menu = None;
                    cx.notify();
                }
            },
        )
        .detach();
        window.focus(&menu.read(cx).focus_handle(cx));
        self.menu = Some((kind, menu));
        cx.notify();
    }

    fn menu_button(&self, kind: AppMenu, cx: &Context<Self>) -> impl IntoElement {
        let (id, title) = match kind {
            AppMenu::File => ("file-menu", "File"),
            AppMenu::View => ("view-menu", "View"),
        };
        let popup = self
            .menu
            .as_ref()
            .filter(|(open, _)| *open == kind)
            .map(|(_, menu)| menu.clone());
        let menu_open = popup.is_some();
        div()
            .relative()
            .child(
                div()
                    .id(id)
                    .debug_selector(move || id.into())
                    .track_focus(self.menu_focus(kind))
                    .tab_index(0)
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
                    .child(title)
                    .child(Icon::new(IconName::ChevronDown).xsmall())
                    .on_mouse_down(
                        MouseButton::Left,
                        cx.listener(move |this, _, _, _| {
                            this.menu_press_was_open = menu_open;
                        }),
                    )
                    .on_click(cx.listener(move |this, _, window, cx| {
                        // 弹出菜单在鼠标按下时就会关闭，松开时避免重新打开同一菜单。
                        if std::mem::take(&mut this.menu_press_was_open) {
                            this.menu = None;
                            window.focus(this.menu_focus(kind));
                            cx.notify();
                        } else {
                            this.toggle_menu(kind, window, cx);
                        }
                    }))
                    .on_key_down(cx.listener(move |this, event: &KeyDownEvent, window, cx| {
                        if matches!(event.keystroke.key.as_str(), "enter" | "space" | "down") {
                            this.toggle_menu(kind, window, cx);
                        }
                    })),
            )
            .when_some(popup, |this, menu| {
                this.child(deferred(
                    anchored()
                        .anchor(Corner::TopLeft)
                        .snap_to_window_with_margin(px(8.0))
                        .child(div().top_1().occlude().child(menu)),
                ))
            })
    }

    fn root_stack(&self, cx: &App) -> Entity<StackPanel> {
        let DockItem::Split { view, .. } = self.dock.read(cx).items() else {
            unreachable!();
        };
        view.clone()
    }

    fn unzoom_other_than(
        &self,
        target: Option<&Entity<TabPanel>>,
        window: &mut Window,
        cx: &mut App,
    ) {
        if let Some(tab) = self.panels.iter().find_map(|panel| {
            let panel = panel.read(cx);
            let tab = panel.tab()?;
            (panel.tab_zoomed && target != Some(&tab)).then_some(tab)
        }) {
            tab.read(cx)
                .focus_handle(cx)
                .dispatch_action(&ToggleZoom, window, cx);
        }
    }

    fn set_drawer_open(&mut self, open: bool, window: &mut Window, cx: &mut Context<Self>) {
        if self.drawer.open == open {
            return;
        }
        let root = self.root_stack(cx);
        let drawer_layout = self.drawer.stack.read(cx).dump(cx);
        let drawer = Arc::new(self.drawer.stack.clone());
        if open {
            let dock = self.dock.downgrade();
            root.update(cx, |root, cx| {
                root.add_panel(drawer, Some(self.drawer.height), dock, window, cx);
            });
        } else {
            if let Some(height) = root
                .read(cx)
                .dump(cx)
                .info
                .sizes()
                .and_then(|sizes| sizes.last())
            {
                self.drawer.height = *height;
            }
            root.update(cx, |root, cx| root.remove_panel(drawer, window, cx));
        }
        self.drawer.open = open;
        if contains_panel(&drawer_layout, PanelKind::Viewport) {
            let viewport = &self.panels[PanelKind::Viewport as usize];
            let active = open
                && viewport.read(cx).tab().is_some_and(|tab| {
                    tab.read(cx)
                        .active_panel(cx)
                        .is_some_and(|panel| panel.view().entity_id() == viewport.entity_id())
                });
            viewport.update(cx, |panel, cx| {
                panel.set_viewport_active(active, window, cx)
            });
        }
        cx.notify();
    }

    fn show_panel(&mut self, index: usize, window: &mut Window, cx: &mut Context<Self>) {
        let panel = self.panels[index].clone();
        let tab = panel.read(cx).tab();
        let drawer = self.drawer.stack.read(cx).dump(cx);
        let in_drawer = contains_panel(&drawer, PanelKind::ALL[index]);
        self.unzoom_other_than(tab.as_ref(), window, cx);
        if let Some(tab) = tab {
            if in_drawer {
                self.set_drawer_open(true, window, cx);
            }
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
        } else if index >= PanelKind::DRAWER_START {
            let tabs = &self.drawer.tabs;
            let empty = tabs.read(cx).active_panel(cx).is_none();
            tabs.update(cx, |tabs, cx| {
                tabs.add_panel(Arc::new(panel.clone()), window, cx)
            });
            if empty {
                self.drawer.stack.update(cx, |stack, cx| {
                    stack.add_panel(
                        Arc::new(tabs.clone()),
                        None,
                        self.dock.downgrade(),
                        window,
                        cx,
                    );
                });
            }
            self.set_drawer_open(true, window, cx);
        } else if let Some(tab) = self.panels.iter().find_map(|panel| {
            let panel = panel.read(cx);
            (!contains_panel(&drawer, panel.kind))
                .then(|| panel.tab())
                .flatten()
        }) {
            tab.update(cx, |tab, cx| {
                tab.add_panel(Arc::new(panel.clone()), window, cx);
            });
        } else {
            let dock = self.dock.downgrade();
            let item = DockItem::tab(panel.clone(), &dock, window, cx);
            self.root_stack(cx).update(cx, |root, cx| {
                root.insert_panel_before(item.view(), 0, None, dock, window, cx);
            });
        }
        window.focus(&panel.read(cx).focus_handle(cx));
        cx.notify();
    }

    fn collapse_drawer(&mut self, index: usize, window: &mut Window, cx: &mut Context<Self>) {
        let drawer = self.drawer.stack.read(cx).dump(cx);
        if self.panels.iter().any(|panel| {
            let panel = panel.read(cx);
            panel.tab_zoomed && contains_panel(&drawer, panel.kind)
        }) {
            self.unzoom_other_than(None, window, cx);
        }
        self.set_drawer_open(false, window, cx);
        window.focus(&self.drawer_focus[index - PanelKind::DRAWER_START]);
    }

    fn drawer_bar(&self, cx: &Context<Self>) -> impl IntoElement {
        let layout = self.drawer.stack.read(cx).dump(cx);
        let entries = PanelKind::ALL
            .into_iter()
            .enumerate()
            .skip(PanelKind::DRAWER_START);
        let selected = |index: usize, kind| {
            self.drawer.open
                && contains_panel(&layout, kind)
                && self.panels[index].read(cx).tab().is_some_and(|tab| {
                    tab.read(cx)
                        .active_panel(cx)
                        .is_some_and(|panel| panel.panel_name(cx) == kind.title())
                })
        };
        let collapse_target = entries
            .clone()
            .find(|&(index, kind)| selected(index, kind))
            .map_or(PanelKind::DRAWER_START, |(index, _)| index);
        div()
            .flex()
            .items_center()
            .h(px(30.0))
            .flex_shrink_0()
            .px_2()
            .gap_1()
            .border_t_1()
            .border_color(rgb(BORDER))
            .tab_index(2)
            .tab_stop(false)
            .tab_group()
            .children(entries.map(|(index, kind)| {
                let active = selected(index, kind);
                let selector = format!("drawer-{}", kind.id());
                let activate =
                    move |this: &mut Self, window: &mut Window, cx: &mut Context<Self>| {
                        if active {
                            this.collapse_drawer(index, window, cx);
                        } else {
                            this.show_panel(index, window, cx);
                        }
                    };
                div()
                    .id(SharedString::from(selector.clone()))
                    .debug_selector(move || selector)
                    .track_focus(&self.drawer_focus[index - PanelKind::DRAWER_START])
                    .tab_index(0)
                    .flex()
                    .items_center()
                    .h(px(24.0))
                    .px_2()
                    .rounded_sm()
                    .border_1()
                    .border_color(rgb(BACKGROUND))
                    .text_color(rgb(if active { TEXT } else { MUTED }))
                    .cursor_pointer()
                    .when(active, |this| this.bg(rgb(SELECTED)))
                    .hover(|style| style.bg(rgb(if active { SELECTED } else { HOVER })))
                    .focus(|style| style.border_color(rgb(ACCENT)))
                    .child(kind.title())
                    .on_click(cx.listener(move |this, _, window, cx| activate(this, window, cx)))
                    .on_key_down(cx.listener(move |this, event: &KeyDownEvent, window, cx| {
                        if matches!(event.keystroke.key.as_str(), "enter" | "space") {
                            activate(this, window, cx);
                        }
                    }))
            }))
            .when(self.drawer.open, |bar| {
                bar.child(div().flex_1()).child(
                    Button::new("collapse-drawer")
                        .debug_selector(|| "collapse-drawer".into())
                        .icon(IconName::ChevronDown)
                        .small()
                        .ghost()
                        .tooltip("Collapse drawer")
                        .on_click(cx.listener(move |this, _, window, cx| {
                            this.collapse_drawer(collapse_target, window, cx);
                        }))
                        .on_key_down(cx.listener(move |this, event: &KeyDownEvent, window, cx| {
                            if matches!(event.keystroke.key.as_str(), "enter" | "space") {
                                this.collapse_drawer(collapse_target, window, cx);
                            }
                        })),
                )
            })
    }
}

impl Render for Workbench {
    fn render(&mut self, _: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
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
                    .h(px(34.0))
                    .flex_shrink_0()
                    .gap_3()
                    .px_3()
                    .border_b_1()
                    .border_color(rgb(BORDER))
                    .tab_group()
                    .child(div().font_semibold().text_color(rgb(ACCENT)).child("Bee"))
                    .child(self.menu_button(AppMenu::File, cx))
                    .child(self.menu_button(AppMenu::View, cx)),
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
            .child(self.drawer_bar(cx))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use gpui::{
        point, size, KeyBinding, Modifiers, MouseButton, ScrollDelta, ScrollWheelEvent,
        VisualTestContext,
    };
    use gpui_component::{dock::ClosePanel, PixelsExt};

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
            let trigger = cx.debug_bounds("view-menu").unwrap().center();
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

    fn assert_drawer(
        cx: &mut VisualTestContext,
        workbench: &Entity<Workbench>,
        active: Option<PanelKind>,
    ) {
        cx.update(|window, cx| {
            let wb = workbench.read(cx);
            assert_eq!(wb.drawer.open, active.is_some());
            let layout = wb.dock.read(cx).dump(cx).center;
            assert_eq!(layout.children.len(), if active.is_some() { 2 } else { 1 });
            if let Some(kind) = active {
                let panel = wb.panels[kind as usize].read(cx);
                assert_eq!(panel.tab(), Some(wb.drawer.tabs.clone()));
                assert_eq!(
                    wb.drawer
                        .tabs
                        .read(cx)
                        .active_panel(cx)
                        .unwrap()
                        .panel_name(cx),
                    kind.title()
                );
                assert!(panel.focus.contains_focused(window, cx));
            }
        });
    }

    #[test]
    fn workbench_flow_edits_drive_the_same_viewport() {
        let mut app = gpui::TestAppContext::single();
        app.update(|cx| {
            gpui_component::init(cx);
            init_theme(cx);
        });
        let (workbench, cx) = app.add_window_view(|window, cx| {
            let bee = cx.new(|_| Bee::failure("test device".into()));
            Workbench::new(bee, window, cx)
        });
        cx.update(|window, cx| {
            workbench.update(cx, |workbench, cx| {
                workbench.dock.update(cx, |dock, cx| {
                    let weak = cx.entity().downgrade();
                    let viewport = DockItem::tab(workbench.panels[1].clone(), &weak, window, cx);
                    let flow = DockItem::tab(workbench.panels[5].clone(), &weak, window, cx);
                    dock.set_center(
                        DockItem::h_split(vec![viewport, flow], &weak, window, cx),
                        window,
                        cx,
                    );
                });
            });
        });
        let dimensions = size(px(1200.0), px(1400.0));
        cx.simulate_resize(dimensions);
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| {
            workbench.clone()
        });
        cx.run_until_parked();
        let bee = cx.update(|_, cx| workbench.read(cx).panels[1].read(cx).bee.clone());
        let click = |cx: &mut VisualTestContext, selector: &'static str| {
            let bounds = cx.debug_bounds(selector).expect(selector);
            cx.simulate_click(bounds.center(), Modifiers::none());
            cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| {
                workbench.clone()
            });
            cx.run_until_parked();
        };

        click(cx, "flow-add-raster");
        assert_eq!(
            cx.update(|_, cx| bee.read(cx).viewport.flow.draft().nodes().count()),
            5
        );
        click(cx, "flow-remove-4");
        click(cx, "flow-exposure-up-2");
        click(cx, "flow-apply");
        let active = cx.update(|_, cx| {
            let bee = bee.read(cx);
            assert_eq!(bee.state.snapshot().display.exposure_ev, 0.5);
            assert_eq!(bee.viewport.flow.active().display_source().unwrap().1, 0.5);
            bee.viewport.flow.active().clone()
        });

        click(cx, "flow-disconnect-2");
        click(cx, "flow-apply");
        bee.update(cx, |bee, _| {
            bee.console.drain();
            assert!(bee.viewport.flow.error().is_some());
            assert!(bee
                .console
                .visible()
                .any(|message| message.source == "Render Flow"
                    && message.message.contains("required input")));
        });
        cx.update(|_, cx| {
            let bee = bee.read(cx);
            assert_eq!(bee.viewport.flow.active(), &active);
            assert_ne!(bee.viewport.flow.draft(), &active);
            assert_eq!(bee.state.snapshot().display.exposure_ev, 0.5);
        });
    }

    #[test]
    fn viewport_suspends_and_recovers_with_panel_changes() {
        let mut app = gpui::TestAppContext::single();
        app.update(|cx| {
            gpui_component::init(cx);
            init_theme(cx);
            cx.bind_keys([KeyBinding::new("ctrl-w", ClosePanel, None)]);
        });
        let (workbench, cx) = app.add_window_view(|window, cx| {
            let bee = cx.new(|_| Bee::failure("test device".into()));
            Workbench::new(bee, window, cx)
        });
        cx.simulate_resize(size(px(960.0), px(640.0)));
        redraw(cx, &workbench);
        let bee = cx.update(|_, cx| workbench.read(cx).panels[1].read(cx).bee.clone());
        assert!(cx.update(|_, cx| bee.read(cx).viewport.extent().is_some()));

        cx.update(|window, cx| window.focus(&workbench.read(cx).panels[0].read(cx).focus));
        redraw(cx, &workbench);
        cx.dispatch_action(gpui_component::dock::ToggleZoom);
        redraw(cx, &workbench);
        redraw(cx, &workbench);
        assert_eq!(cx.update(|_, cx| bee.read(cx).viewport.extent()), None);
        cx.dispatch_action(gpui_component::dock::ToggleZoom);
        redraw(cx, &workbench);
        redraw(cx, &workbench);
        assert!(cx.update(|_, cx| bee.read(cx).viewport.extent().is_some()));

        cx.update(|window, cx| window.focus(&workbench.read(cx).panels[0].read(cx).focus));
        redraw(cx, &workbench);
        cx.dispatch_action(gpui_component::dock::ToggleZoom);
        redraw(cx, &workbench);
        redraw(cx, &workbench);
        assert_eq!(cx.update(|_, cx| bee.read(cx).viewport.extent()), None);
        cx.update(|window, cx| {
            let panel = workbench.read(cx).panels[0].clone();
            let tab = panel.read(cx).tab.as_ref().unwrap().upgrade().unwrap();
            tab.update(cx, |tab, cx| {
                tab.remove_panel(Arc::new(panel), window, cx);
            });
        });
        redraw(cx, &workbench);
        redraw(cx, &workbench);
        assert!(cx.update(|_, cx| workbench.read(cx).panels[0].read(cx).tab.is_none()));
        assert!(cx.update(|_, cx| bee.read(cx).viewport.extent().is_some()));
        click(cx, &workbench, "show-scene");
        click(cx, &workbench, "reset-workbench");

        cx.update(|window, cx| window.focus(&workbench.read(cx).panels[0].read(cx).focus));
        redraw(cx, &workbench);
        cx.dispatch_action(gpui_component::dock::ToggleZoom);
        redraw(cx, &workbench);
        redraw(cx, &workbench);
        assert_eq!(cx.update(|_, cx| bee.read(cx).viewport.extent()), None);
        click(cx, &workbench, "reset-workbench");
        assert!(cx.update(|_, cx| bee.read(cx).viewport.extent().is_some()));

        cx.update(|window, cx| window.focus(&workbench.read(cx).panels[1].read(cx).focus));
        redraw(cx, &workbench);
        cx.simulate_keystrokes("ctrl-w");
        redraw(cx, &workbench);
        assert_eq!(cx.update(|_, cx| bee.read(cx).viewport.extent()), None);
        click(cx, &workbench, "show-viewport");
        assert!(cx.update(|_, cx| bee.read(cx).viewport.extent().is_some()));
        click(cx, &workbench, "show-scene");
        assert_eq!(cx.update(|_, cx| bee.read(cx).viewport.extent()), None);
        click(cx, &workbench, "show-viewport");
        assert!(cx.update(|_, cx| bee.read(cx).viewport.extent().is_some()));
        click(cx, &workbench, "reset-workbench");
        assert!(cx.update(|_, cx| bee.read(cx).viewport.extent().is_some()));

        let previous_extent = cx.update(|_, cx| bee.read(cx).viewport.extent());
        let scene = cx.debug_bounds("panel-content-scene").unwrap();
        let start = point(scene.right(), scene.center().y);
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
        assert_ne!(
            cx.update(|_, cx| bee.read(cx).viewport.extent()),
            previous_extent
        );

        let start = cx.debug_bounds("tab-viewport").unwrap().center();
        let target = cx.debug_bounds("panel-content-scene").unwrap().center();
        cx.simulate_mouse_move(start, None, Modifiers::none());
        cx.simulate_mouse_down(start, MouseButton::Left, Modifiers::none());
        cx.simulate_mouse_move(
            point(start.x + px(20.0), start.y),
            MouseButton::Left,
            Modifiers::none(),
        );
        cx.simulate_mouse_move(target, MouseButton::Left, Modifiers::none());
        redraw(cx, &workbench);
        cx.simulate_mouse_up(target, MouseButton::Left, Modifiers::none());
        redraw(cx, &workbench);
        cx.update(|_, cx| {
            let workbench = workbench.read(cx);
            let scene = workbench.panels[0].read(cx).tab.as_ref().unwrap().upgrade();
            let viewport = workbench.panels[1].read(cx).tab.as_ref().unwrap().upgrade();
            assert_eq!(scene, viewport);
        });
        click(cx, &workbench, "show-scene");
        assert_eq!(cx.update(|_, cx| bee.read(cx).viewport.extent()), None);
        click(cx, &workbench, "show-viewport");
        assert!(cx.update(|_, cx| bee.read(cx).viewport.extent().is_some()));

        click(cx, &workbench, "reset-workbench");
        let start = cx.debug_bounds("tab-viewport").unwrap().center();
        let scene_tab = cx.debug_bounds("tab-scene").unwrap();
        let target = point(scene_tab.right() + px(50.0), scene_tab.center().y);
        cx.simulate_mouse_move(start, None, Modifiers::none());
        cx.simulate_mouse_down(start, MouseButton::Left, Modifiers::none());
        cx.simulate_mouse_move(
            point(start.x + px(20.0), start.y),
            MouseButton::Left,
            Modifiers::none(),
        );
        cx.simulate_mouse_move(target, MouseButton::Left, Modifiers::none());
        redraw(cx, &workbench);
        cx.simulate_mouse_up(target, MouseButton::Left, Modifiers::none());
        redraw(cx, &workbench);
        cx.update(|_, cx| {
            let workbench = workbench.read(cx);
            let scene = workbench.panels[0].read(cx).tab.as_ref().unwrap().upgrade();
            let viewport = workbench.panels[1].read(cx).tab.as_ref().unwrap().upgrade();
            assert_eq!(scene, viewport);
            assert_eq!(
                scene
                    .unwrap()
                    .read(cx)
                    .active_panel(cx)
                    .unwrap()
                    .panel_name(cx),
                PanelKind::Scene.title()
            );
            assert_eq!(bee.read(cx).viewport.extent(), None);
        });
        cx.update(|window, cx| window.focus(&workbench.read(cx).panels[0].read(cx).focus));
        redraw(cx, &workbench);
        cx.dispatch_action(gpui_component::dock::ToggleZoom);
        redraw(cx, &workbench);
        redraw(cx, &workbench);
        assert_eq!(cx.update(|_, cx| bee.read(cx).viewport.extent()), None);
        click(cx, &workbench, "tab-viewport");
        assert!(cx.update(|_, cx| bee.read(cx).viewport.extent().is_some()));
        click(cx, &workbench, "tab-scene");
        assert_eq!(cx.update(|_, cx| bee.read(cx).viewport.extent()), None);
        cx.update(|window, cx| window.focus(&workbench.read(cx).panels[0].read(cx).focus));
        redraw(cx, &workbench);
        cx.dispatch_action(gpui_component::dock::ToggleZoom);
        redraw(cx, &workbench);
        redraw(cx, &workbench);
        cx.update(|window, cx| window.focus(&workbench.read(cx).panels[0].read(cx).focus));
        redraw(cx, &workbench);
        cx.simulate_keystrokes("ctrl-w");
        redraw(cx, &workbench);
        assert!(cx.update(|_, cx| bee.read(cx).viewport.extent().is_some()));
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

        cx.update(|window, cx| window.focus(workbench.read(cx).menu_focus(AppMenu::View)));
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
    fn drawer_entries_and_layout_preserve_workbench_state() {
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

        let closed_canvas = cx.debug_bounds("scene-canvas").unwrap().size;
        assert_drawer(cx, &workbench, None);
        assert!(cx.debug_bounds("panel-content-files").is_none());
        assert!(cx.debug_bounds("panel-content-console").is_none());
        assert!(cx.debug_bounds("panel-content-flow").is_none());
        cx.update(|_, cx| {
            let workbench = workbench.read(cx);
            let mut actual = Vec::new();
            names(&workbench.dock.read(cx).dump(cx).center, &mut actual);
            assert_eq!(actual, ["Scene", "Viewport", "Properties"]);
        });

        cx.update(|window, cx| window.focus(workbench.read(cx).menu_focus(AppMenu::File)));
        cx.simulate_keystrokes("shift-tab shift-tab shift-tab");
        assert!(cx.update(|window, cx| workbench.read(cx).drawer_focus[0].is_focused(window)));
        cx.simulate_keystrokes("enter");
        redraw(cx, &workbench);
        assert_drawer(cx, &workbench, Some(PanelKind::Files));
        assert!(cx.debug_bounds("files-choose").is_some());
        let open_canvas = cx.debug_bounds("scene-canvas").unwrap().size;
        assert_eq!(open_canvas.width, closed_canvas.width);
        assert!(closed_canvas.height - open_canvas.height > px(100.0));
        let (bee, files) = cx.update(|_, cx| {
            let wb = workbench.read(cx);
            (wb.bee.clone(), wb.panels[PanelKind::Files as usize].clone())
        });
        let path = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("src");
        files.update(cx, |panel, cx| panel.files.navigate(path.clone(), cx));
        redraw(cx, &workbench);

        click(cx, &workbench, "drawer-files");
        assert_drawer(cx, &workbench, None);
        assert_eq!(cx.debug_bounds("scene-canvas").unwrap().size, closed_canvas);
        assert!(cx.update(|window, cx| workbench.read(cx).drawer_focus[0].is_focused(window)));
        click(cx, &workbench, "show-files");
        assert_drawer(cx, &workbench, Some(PanelKind::Files));
        assert_eq!(
            cx.update(|_, cx| files.read(cx).files.path().map(|path| path.to_owned())),
            Some(path.clone())
        );

        click(cx, &workbench, "drawer-console");
        assert_drawer(cx, &workbench, Some(PanelKind::Console));
        assert!(cx.debug_bounds("console-filter-0").is_some());
        bee.update(cx, |bee, cx| {
            bee.console
                .sender
                .send(bee_vulkan::DiagnosticLevel::Info, "test", "drawer history");
            assert!(bee.console.drain());
            cx.notify();
        });
        redraw(cx, &workbench);
        click(cx, &workbench, "console-filter-0");
        assert!(!cx.update(|_, cx| bee
            .read(cx)
            .console
            .visible()
            .any(|message| message.source == "test")));
        click(cx, &workbench, "collapse-drawer");
        assert_drawer(cx, &workbench, None);
        assert_eq!(cx.debug_bounds("scene-canvas").unwrap().size, closed_canvas);

        click(cx, &workbench, "drawer-flow");
        assert_drawer(cx, &workbench, Some(PanelKind::Flow));
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
        cx.update(|window, cx| {
            bee.update(cx, |bee, cx| {
                bee.viewport.apply_flow(&mut bee.state, window, cx);
                assert_eq!(bee.viewport.flow.active().nodes().count(), 5);
            });
        });
        redraw(cx, &workbench);
        click(cx, &workbench, "flow-add-raster");

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
        let (versions, camera, scene, active, draft) = cx.update(|_, cx| {
            let bee = bee.read(cx);
            (
                bee.state.versions(),
                bee.state.camera(),
                bee.state.snapshot().scene,
                bee.viewport.flow.active().clone(),
                bee.viewport.flow.draft().clone(),
            )
        });
        assert_eq!(draft.nodes().count(), 6);

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

        click(cx, &workbench, "reset-workbench");
        assert_drawer(cx, &workbench, None);
        assert_eq!(cx.debug_bounds("scene-canvas").unwrap().size, closed_canvas);
        cx.update(|_, cx| {
            assert_eq!(workbench.read(cx).panels[PanelKind::Files as usize], files);
            assert_eq!(files.read(cx).files.path(), Some(path.as_path()));
            let bee = bee.read(cx);
            assert_eq!(bee.state.versions(), versions);
            assert_eq!(bee.state.camera(), camera);
            assert!(Arc::ptr_eq(&bee.state.snapshot().scene, &scene));
            assert_eq!(bee.viewport.flow.active(), &active);
            assert_eq!(bee.viewport.flow.draft(), &draft);
            assert!(!bee
                .console
                .visible()
                .any(|message| message.source == "test"));
        });
        click(cx, &workbench, "show-console");
        assert_drawer(cx, &workbench, Some(PanelKind::Console));
        click(cx, &workbench, "console-filter-0");
        assert!(cx.update(|_, cx| bee
            .read(cx)
            .console
            .visible()
            .any(|message| message.message == "drawer history")));

        click(cx, &workbench, "show-viewport");
        cx.dispatch_action(ToggleZoom);
        redraw(cx, &workbench);
        assert!(cx.update(|_, cx| workbench.read(cx).panels[1].read(cx).tab_zoomed));
        click(cx, &workbench, "show-flow");
        assert_drawer(cx, &workbench, Some(PanelKind::Flow));
        assert!(!cx.update(|_, cx| workbench.read(cx).panels[1].read(cx).tab_zoomed));
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
        click(cx, &workbench, "collapse-drawer");
        click(cx, &workbench, "show-scene");
        click(cx, &workbench, "drawer-flow");
        cx.update(|window, cx| {
            let workbench = workbench.read(cx);
            let flow = workbench.panels[PanelKind::Flow as usize].read(cx);
            assert!(!workbench.drawer.open);
            assert_eq!(
                flow.tab(),
                workbench.panels[PanelKind::Scene as usize].read(cx).tab()
            );
            assert!(flow.focus.is_focused(window));
            let mut center = Vec::new();
            names(&workbench.dock.read(cx).dump(cx).center, &mut center);
            assert_eq!(
                center.iter().filter(|name| *name == "Render Flow").count(),
                1
            );
            assert!(!contains_panel(
                &workbench.drawer.stack.read(cx).dump(cx),
                PanelKind::Flow
            ));
        });

        cx.simulate_keystrokes("ctrl-w");
        redraw(cx, &workbench);
        let flow = cx.update(|_, cx| {
            let flow = workbench.read(cx).panels[PanelKind::Flow as usize].clone();
            assert!(flow.read(cx).tab().is_none());
            flow
        });
        click(cx, &workbench, "drawer-flow");
        assert_drawer(cx, &workbench, Some(PanelKind::Flow));
        cx.update(|_, cx| {
            assert_eq!(workbench.read(cx).panels[PanelKind::Flow as usize], flow);
            assert_eq!(bee.read(cx).viewport.flow.draft(), &draft);
        });
        for entry in ["drawer-files", "drawer-console", "show-flow"] {
            click(cx, &workbench, entry);
            cx.simulate_keystrokes("ctrl-w");
            redraw(cx, &workbench);
        }
        assert_drawer(cx, &workbench, None);
        click(cx, &workbench, "show-flow");
        assert_drawer(cx, &workbench, Some(PanelKind::Flow));
    }

    #[test]
    fn app_menus_fit_and_work_with_keyboard_at_small_sizes() {
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

        let view_entries = [
            "show-scene",
            "show-viewport",
            "show-properties",
            "show-files",
            "show-console",
            "show-flow",
            "reset-workbench",
        ];
        for dimensions in [
            size(px(1200.0), px(800.0)),
            size(px(960.0), px(640.0)),
            size(px(640.0), px(426.0)),
        ] {
            cx.simulate_resize(dimensions);
            cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
            cx.run_until_parked();
            assert_eq!(cx.update(|window, _| window.scale_factor()), 2.0);
            let file = cx.debug_bounds("file-menu").unwrap();
            let view = cx.debug_bounds("view-menu").unwrap();
            assert!(file.right() <= view.left());
            assert!(view.right() <= dimensions.width && view.bottom() <= dimensions.height);
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
                let side_scale = dimensions.width / px(1200.0);
                assert!(
                    (scene.size.width - px(200.0) * side_scale).as_f64().abs() <= 4.0,
                    "{scene:?}"
                );
                assert!(
                    (properties.size.width - px(220.0) * side_scale)
                        .as_f64()
                        .abs()
                        <= 4.0,
                    "{properties:?}"
                );
                let row = cx.debug_bounds("scene-camera").unwrap();
                let title = cx.debug_bounds("tab-scene").unwrap();
                assert!(
                    row.size.height <= px(24.0)
                        && row.bottom() <= scene.bottom()
                        && row.right() <= scene.right()
                );
                assert!(title.size.height <= px(16.0) && title.bottom() <= scene.top());
            }
            let mut previous_right = px(0.0);
            for selector in ["drawer-files", "drawer-console", "drawer-flow"] {
                let entry = cx.debug_bounds(selector).expect(selector);
                assert!(
                    entry.left() >= previous_right
                        && entry.top() >= viewport.bottom()
                        && entry.right() <= dimensions.width
                        && entry.bottom() <= dimensions.height,
                    "{selector}: {entry:?} in {dimensions:?}"
                );
                previous_right = entry.right();
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
            cx.simulate_click(file.center(), Modifiers::none());
            cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
            cx.run_until_parked();
            let exit = cx.debug_bounds("exit-bee").expect("File > Exit");
            assert!(exit.left() >= file.left() && exit.top() >= file.bottom());
            assert!(cx.update(|_, cx| workbench
                .read(cx)
                .menu
                .as_ref()
                .is_some_and(|(kind, _)| *kind == AppMenu::File)));

            cx.simulate_click(view.center(), Modifiers::none());
            cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
            cx.run_until_parked();
            assert!(cx.update(|_, cx| workbench
                .read(cx)
                .menu
                .as_ref()
                .is_some_and(|(kind, _)| *kind == AppMenu::View)));
            for selector in view_entries {
                let item = cx.debug_bounds(selector).expect(selector);
                assert!(
                    item.left() >= px(0.0)
                        && item.top() >= view.bottom()
                        && item.right() <= dimensions.width
                        && item.bottom() <= dimensions.height,
                    "{selector}: {item:?} in {dimensions:?}"
                );
            }
            cx.simulate_click(view.center(), Modifiers::none());
            cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| root.clone());
            cx.run_until_parked();
            assert!(cx.update(|_, cx| workbench.read(cx).menu.is_none()));
        }

        cx.update(|window, cx| window.focus(workbench.read(cx).menu_focus(AppMenu::File)));
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(640.0), px(426.0)),
            |_, _| root.clone(),
        );
        cx.simulate_keystrokes("tab");
        assert!(cx.update(|window, cx| workbench
            .read(cx)
            .menu_focus(AppMenu::View)
            .is_focused(window)));
        cx.simulate_keystrokes("tab");
        assert!(cx.update(|window, cx| workbench.read(cx).panels[0]
            .read(cx)
            .focus
            .contains_focused(window, cx)));
        cx.simulate_keystrokes("shift-tab");
        for (kind, selector) in [(AppMenu::View, "show-scene"), (AppMenu::File, "exit-bee")] {
            assert!(cx.update(|window, cx| workbench.read(cx).menu_focus(kind).is_focused(window)));
            cx.simulate_keystrokes("enter");
            cx.draw(
                point(px(0.0), px(0.0)),
                size(px(640.0), px(426.0)),
                |_, _| root.clone(),
            );
            assert!(cx.debug_bounds(selector).is_some(), "{selector}");
            cx.simulate_keystrokes("escape");
            assert!(cx.update(|window, cx| {
                let workbench = workbench.read(cx);
                workbench.menu.is_none() && workbench.menu_focus(kind).is_focused(window)
            }));
            if kind == AppMenu::View {
                cx.simulate_keystrokes("shift-tab");
            }
        }

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
            cx.update(|window, cx| window.focus(workbench.read(cx).menu_focus(AppMenu::View)));
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
            assert!(
                cx.update(|window, cx| {
                    let workbench = workbench.read(cx);
                    workbench.menu.is_none()
                        && workbench.panels[index].read(cx).focus.is_focused(window)
                }),
                "{content}"
            );
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

        cx.update(|window, cx| window.focus(workbench.read(cx).menu_focus(AppMenu::View)));
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
