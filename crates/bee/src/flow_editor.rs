use crate::Bee;
use bee_renderer::{Module, ModuleGraph, NodeId, PortType, PreparedFlow, RenderFlow};
use gpui::{div, prelude::*, rgb, Context, Div, KeyDownEvent, SharedString, Stateful, Window};

pub struct FlowEditor {
    flow: RenderFlow,
    prepared: PreparedFlow,
    preview_reason: Option<String>,
    selected_output: Option<NodeId>,
    error: Option<String>,
}

impl FlowEditor {
    pub(crate) fn new(
        flow: RenderFlow,
        prepared: PreparedFlow,
        preview_reason: Option<String>,
    ) -> Self {
        Self {
            prepared,
            flow,
            preview_reason,
            selected_output: None,
            error: None,
        }
    }

    pub(crate) fn draft(&self) -> &ModuleGraph {
        self.flow.draft()
    }

    #[cfg(test)]
    pub(crate) fn draft_mut(&mut self) -> &mut ModuleGraph {
        self.edit()
    }

    pub fn active(&self) -> &ModuleGraph {
        self.flow.active().expect("the initial flow is valid")
    }

    pub(crate) fn prepared(&self) -> &PreparedFlow {
        &self.prepared
    }

    pub(crate) fn preview_reason(&self) -> Option<&str> {
        self.preview_reason.as_deref()
    }

    pub fn error(&self) -> Option<&str> {
        self.error.as_deref()
    }

    pub fn select_source(&mut self, module: Module) -> Result<(), String> {
        let graph = self.edit();
        let output = graph
            .nodes()
            .find_map(|(id, node)| (node == Module::ViewportOutput).then_some(id));
        let tone = output.and_then(|output| {
            graph.connections().iter().find_map(|&(from, to)| {
                (to == output && matches!(graph.module(from), Some(Module::ToneMapping { .. })))
                    .then_some(from)
            })
        });
        let Some(tone) = tone else {
            let error = "Connect tone mapping to the viewport output first".to_string();
            self.error = Some(error.clone());
            return Err(error);
        };
        let source = graph
            .nodes()
            .find_map(|(id, node)| (node == module).then_some(id));
        let source = source.unwrap_or_else(|| graph.add(module));
        let result = graph.connect(source, tone);
        self.error = result.as_ref().err().cloned();
        result
    }

    pub fn apply(&mut self, path_unavailable: Option<String>) -> Result<ModuleGraph, String> {
        let result = self.flow.draft().display_source().and_then(|(source, _)| {
            if self.flow.draft().module(source) == Some(Module::PathTracing) {
                if let Some(reason) = path_unavailable {
                    return Err(format!("Path tracing unavailable: {reason}"));
                }
            }
            let prepared = PreparedFlow::prepare(self.flow.draft(), Some(&self.prepared))?;
            self.flow.apply()?;
            self.prepared = prepared;
            self.preview_reason = None;
            Ok(self.active().clone())
        });
        self.error = result.as_ref().err().cloned();
        result
    }

    fn edit(&mut self) -> &mut ModuleGraph {
        self.error = None;
        self.flow.edit()
    }

    pub fn render(&self, cx: &mut Context<Bee>) -> impl IntoElement {
        let mut nodes = Vec::new();
        for (id, module) in self.flow.draft().nodes() {
            let mut card = div()
                .flex()
                .flex_col()
                .gap_1()
                .p_2()
                .border_1()
                .border_color(rgb(0x52645f))
                .bg(rgb(0x252d2b))
                .child(
                    div()
                        .flex()
                        .justify_between()
                        .child(format!("{} #{}", module_name(module), id.index()))
                        .child(button(
                            cx,
                            format!("flow-remove-{}", id.index()),
                            "Remove".into(),
                            move |view, _, cx| {
                                if view.viewport.flow.selected_output == Some(id) {
                                    view.viewport.flow.selected_output = None;
                                }
                                view.viewport.flow.edit().remove(id);
                                cx.notify();
                            },
                        )),
                );
            if let Some(input) = module.input_type() {
                let source = self
                    .flow
                    .draft()
                    .connections()
                    .iter()
                    .find(|&&(_, target)| target == id)
                    .and_then(|&(source, _)| {
                        self.flow
                            .draft()
                            .module(source)
                            .map(|module| (source, module))
                    });
                let connection = source.map_or_else(
                    || "unconnected".to_string(),
                    |(source, module)| format!("{} #{}", module_name(module), source.index()),
                );
                card = card.child(button(
                    cx,
                    format!("flow-input-{}", id.index()),
                    format!("Input {} ← {connection}", port_name(input)),
                    move |view, _, cx| {
                        let editor = &mut view.viewport.flow;
                        if let Some(source) = editor.selected_output.take() {
                            editor.error = editor.edit().connect(source, id).err();
                        } else {
                            editor.error = Some("Select an output port first".into());
                        }
                        if let Some(error) = editor.error() {
                            eprintln!("Render Flow: {error}");
                            view.console.sender.send(
                                bee_vulkan::DiagnosticLevel::Error,
                                "Render Flow",
                                error.to_owned(),
                            );
                        }
                        cx.notify();
                    },
                ));
                if source.is_some() {
                    card = card.child(button(
                        cx,
                        format!("flow-disconnect-{}", id.index()),
                        "Disconnect input".into(),
                        move |view, _, cx| {
                            view.viewport.flow.edit().disconnect(id);
                            cx.notify();
                        },
                    ));
                }
            }
            if let Some(output) = module.output_type() {
                let selected = if self.selected_output == Some(id) {
                    " (selected)"
                } else {
                    ""
                };
                card = card.child(button(
                    cx,
                    format!("flow-output-{}", id.index()),
                    format!("Output {}{selected}", port_name(output)),
                    move |view, _, cx| {
                        view.viewport.flow.selected_output = Some(id);
                        view.viewport.flow.error = None;
                        cx.notify();
                    },
                ));
            }
            if let Module::ToneMapping { exposure_ev } = module {
                card = card.child(
                    div()
                        .flex()
                        .gap_2()
                        .items_center()
                        .child(button(
                            cx,
                            format!("flow-exposure-down-{}", id.index()),
                            "−".into(),
                            move |view, _, cx| {
                                if let Some(Module::ToneMapping { exposure_ev }) =
                                    view.viewport.flow.edit().module_mut(id)
                                {
                                    *exposure_ev -= 0.5;
                                }
                                cx.notify();
                            },
                        ))
                        .child(format!("Exposure {exposure_ev:.1} EV"))
                        .child(button(
                            cx,
                            format!("flow-exposure-up-{}", id.index()),
                            "+".into(),
                            move |view, _, cx| {
                                if let Some(Module::ToneMapping { exposure_ev }) =
                                    view.viewport.flow.edit().module_mut(id)
                                {
                                    *exposure_ev += 0.5;
                                }
                                cx.notify();
                            },
                        )),
                );
            }
            nodes.push(card);
        }
        div()
            .flex()
            .flex_col()
            .gap_2()
            .child("Render flow")
            .child(
                div()
                    .text_color(rgb(0xa8c9bc))
                    .child(if self.flow.is_applied() {
                        "Flow applied"
                    } else {
                        "Draft not applied; previous flow remains active"
                    }),
            )
            .children(
                self.error
                    .clone()
                    .map(|error| div().text_color(rgb(0xffa8a8)).child(error)),
            )
            .child("Select an output, then an input to connect.")
            .child(button(
                cx,
                "flow-apply".into(),
                "Apply flow".into(),
                |view, window, cx| {
                    view.viewport.apply_flow(&mut view.state, window, cx);
                },
            ))
            .child(
                div().flex().flex_wrap().gap_1().children(
                    [
                        ("flow-add-raster", "Add raster", Module::Rasterization),
                        ("flow-add-path", "Add path", Module::PathTracing),
                        (
                            "flow-add-tone",
                            "Add tone",
                            Module::ToneMapping { exposure_ev: 0.0 },
                        ),
                        ("flow-add-output", "Add output", Module::ViewportOutput),
                    ]
                    .into_iter()
                    .map(|(id, label, module)| {
                        button(cx, id.to_string(), label.into(), move |view, _, cx| {
                            view.viewport.flow.edit().add(module);
                            cx.notify();
                        })
                    }),
                ),
            )
            .children(nodes)
    }
}

fn module_name(module: Module) -> &'static str {
    match module {
        Module::Rasterization => "Raster",
        Module::PathTracing => "Path tracing",
        Module::ToneMapping { .. } => "Tone mapping",
        Module::ViewportOutput => "Viewport output",
    }
}

fn port_name(port: PortType) -> &'static str {
    match port {
        PortType::LinearSceneColor => "linear color",
        PortType::DisplayColor => "display color",
    }
}

fn button(
    cx: &mut Context<Bee>,
    id: String,
    label: String,
    action: impl Fn(&mut Bee, &mut Window, &mut Context<Bee>) + Clone + 'static,
) -> Stateful<Div> {
    let selector = id.clone();
    let click = action.clone();
    div()
        .id(SharedString::from(id))
        .debug_selector(move || selector)
        .tab_index(0)
        .cursor_pointer()
        .px_2()
        .border_1()
        .border_color(rgb(0x647972))
        .focus(|style| style.border_color(rgb(0xffffff)))
        .child(label)
        .on_click(cx.listener(move |view, _, window, cx| click(view, window, cx)))
        .on_key_down(cx.listener(move |view, event: &KeyDownEvent, window, cx| {
            if matches!(event.keystroke.key.as_str(), "enter" | "space") {
                action(view, window, cx);
            }
        }))
}

#[cfg(test)]
mod tests {
    use super::*;
    use gpui::{point, px, size, Modifiers, VisualTestContext};

    fn click(cx: &mut VisualTestContext, selector: &'static str) {
        let bounds = cx.debug_bounds(selector).expect(selector);
        cx.simulate_click(bounds.center(), Modifiers::none());
        cx.run_until_parked();
    }

    #[test]
    fn gpui_edits_nodes_ports_parameters_and_rejects_invalid_flows() {
        let mut app = gpui::TestAppContext::single();
        let (view, cx) = app.add_window_view(|_, _| Bee::failure("test device".into()));
        cx.simulate_resize(size(px(1100.0), px(1400.0)));
        cx.draw(
            point(px(0.0), px(0.0)),
            size(px(1100.0), px(1400.0)),
            |_, _| view.clone(),
        );
        cx.run_until_parked();

        let original = cx.update(|_, cx| view.read(cx).viewport.flow.active().clone());
        click(cx, "flow-add-raster");
        cx.update(|_, cx| {
            let editor = &view.read(cx).viewport.flow;
            assert_eq!(editor.flow.draft().nodes().count(), 5);
            assert!(!editor.flow.is_applied());
            assert_eq!(editor.active(), &original);
        });
        click(cx, "flow-remove-4");
        cx.update(|_, cx| {
            let editor = &view.read(cx).viewport.flow;
            assert_eq!(editor.flow.draft().nodes().count(), 4);
            assert_eq!(editor.active(), &original);
        });

        // 错误连接和断开的必需输入都不能替换已生效流程。
        click(cx, "flow-output-0");
        click(cx, "flow-input-3");
        click(cx, "flow-apply");
        cx.update(|_, cx| {
            let editor = &view.read(cx).viewport.flow;
            assert!(editor.error.as_deref().unwrap().contains("incompatible"));
            assert_eq!(editor.active(), &original);
        });
        click(cx, "flow-output-2");
        click(cx, "flow-input-3");
        click(cx, "flow-disconnect-2");
        click(cx, "flow-apply");
        cx.update(|_, cx| {
            let editor = &view.read(cx).viewport.flow;
            assert!(editor.error.as_deref().unwrap().contains("required input"));
            assert_eq!(editor.active(), &original);
        });
        click(cx, "flow-output-2");
        click(cx, "flow-input-2");
        click(cx, "flow-apply");
        cx.update(|_, cx| {
            let editor = &view.read(cx).viewport.flow;
            assert!(editor.error.as_deref().unwrap().contains("cycle"));
            assert_eq!(editor.active(), &original);
        });
        click(cx, "flow-output-0");
        click(cx, "flow-input-2");
        click(cx, "flow-exposure-up-2");
        click(cx, "flow-apply");
        cx.update(|_, cx| {
            let view = view.read(cx);
            assert!(view.viewport.flow.flow.is_applied());
            assert_eq!(view.viewport.flow.active().display_source().unwrap().1, 0.5);
            assert_eq!(view.state.snapshot().display.exposure_ev, 0.5);
        });

        // 不支持光追时可编辑连接，但不能把该流程设为生效。
        click(cx, "flow-output-1");
        click(cx, "flow-input-2");
        click(cx, "flow-apply");
        cx.update(|_, cx| {
            let editor = &view.read(cx).viewport.flow;
            assert!(editor
                .error
                .as_deref()
                .unwrap()
                .contains("Path tracing unavailable"));
            let (source, _) = editor.active().display_source().unwrap();
            assert_eq!(editor.active().module(source), Some(Module::Rasterization));
            assert!(!editor.flow.is_applied());
        });
    }
}
