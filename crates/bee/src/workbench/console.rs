use super::MUTED;
use crate::Bee;
use bee_vulkan::{
    diagnostic_channel, DiagnosticLevel, DiagnosticMessage, DiagnosticReceiver, DiagnosticSender,
};
use gpui::{div, prelude::*, rgb, Entity, KeyDownEvent, SharedString};
use gpui_component::{button::Button, Selectable, Sizable};
use std::collections::VecDeque;

const HISTORY_CAPACITY: usize = 1000;

pub(crate) struct Console {
    pub(crate) sender: DiagnosticSender,
    receiver: DiagnosticReceiver,
    history: VecDeque<DiagnosticMessage>,
    enabled: [bool; 3],
    dropped: u64,
}

impl Default for Console {
    fn default() -> Self {
        let (sender, receiver) = diagnostic_channel();
        Self {
            sender,
            receiver,
            history: VecDeque::with_capacity(HISTORY_CAPACITY),
            enabled: [true; 3],
            dropped: 0,
        }
    }
}

fn level_index(level: DiagnosticLevel) -> usize {
    match level {
        DiagnosticLevel::Info => 0,
        DiagnosticLevel::Warning => 1,
        DiagnosticLevel::Error => 2,
    }
}

fn level_label(level: DiagnosticLevel) -> &'static str {
    match level {
        DiagnosticLevel::Info => "Info",
        DiagnosticLevel::Warning => "Warning",
        DiagnosticLevel::Error => "Error",
    }
}

impl Console {
    pub(crate) fn drain(&mut self) -> bool {
        let mut changed = false;
        for message in self.receiver.drain() {
            if self.history.len() == HISTORY_CAPACITY {
                self.history.pop_front();
                self.dropped = self.dropped.saturating_add(1);
            }
            self.history.push_back(message);
            changed = true;
        }
        let dropped = self.receiver.take_dropped();
        self.dropped = self.dropped.saturating_add(dropped);
        changed || dropped != 0
    }

    pub(crate) fn visible(&self) -> impl Iterator<Item = &DiagnosticMessage> {
        self.history
            .iter()
            .filter(|message| self.enabled[level_index(message.level)])
    }

    pub(super) fn render(&self, bee: Entity<Bee>) -> impl IntoElement {
        div()
            .tab_group()
            .flex()
            .flex_col()
            .gap_1()
            .child(
                div().flex().flex_wrap().gap_1().children(
                    [
                        DiagnosticLevel::Info,
                        DiagnosticLevel::Warning,
                        DiagnosticLevel::Error,
                    ]
                    .into_iter()
                    .map(|level| {
                        let bee = bee.clone();
                        let key_bee = bee.clone();
                        let index = level_index(level);
                        let selector = format!("console-filter-{index}");
                        div()
                            .id(SharedString::from(selector.clone()))
                            .debug_selector(move || selector)
                            .child(
                                Button::new(("console-level", index))
                                    .small()
                                    .selected(self.enabled[index])
                                    .label(format!(
                                        "{}: {}",
                                        level_label(level),
                                        if self.enabled[index] { "on" } else { "off" }
                                    ))
                                    .on_key_down(move |event: &KeyDownEvent, _, cx| {
                                        if matches!(event.keystroke.key.as_str(), "enter" | "space")
                                        {
                                            key_bee.update(cx, |bee, cx| {
                                                bee.console.enabled[index] =
                                                    !bee.console.enabled[index];
                                                cx.notify();
                                            });
                                        }
                                    })
                                    .on_click(move |_, _, cx| {
                                        bee.update(cx, |bee, cx| {
                                            bee.console.enabled[index] =
                                                !bee.console.enabled[index];
                                            cx.notify();
                                        });
                                    }),
                            )
                    }),
                ),
            )
            .child(div().text_color(rgb(MUTED)).child(format!(
                "{} / {HISTORY_CAPACITY} messages retained",
                self.history.len()
            )))
            .when(self.dropped != 0, |this| {
                this.child(
                    div()
                        .id("console-dropped")
                        .debug_selector(|| "console-dropped".into())
                        .text_color(rgb(0xffd28a))
                        .child(format!(
                            "Warning: {} messages dropped (queue or history capacity)",
                            self.dropped
                        )),
                )
            })
            .children(self.visible().enumerate().map(|(index, message)| {
                let selector = format!("console-message-{index}");
                div()
                    .id(SharedString::from(selector.clone()))
                    .debug_selector(move || selector)
                    .text_color(rgb(match message.level {
                        DiagnosticLevel::Info => super::TEXT,
                        DiagnosticLevel::Warning => 0xffd28a,
                        DiagnosticLevel::Error => 0xffa8a8,
                    }))
                    .child(format!(
                        "[{}] {}: {}",
                        level_label(message.level),
                        message.source,
                        message.message
                    ))
            }))
    }
}

#[cfg(test)]
mod tests {
    use super::super::{init_theme, Workbench};
    use super::*;
    use bee_vulkan::DIAGNOSTIC_QUEUE_CAPACITY;
    use gpui::{point, px, size, Modifiers};
    use std::sync::Arc;
    use std::time::Duration;

    #[test]
    #[ignore = "requires a Vulkan graphics device"]
    fn initialized_device_report_is_visible_in_console() {
        let mut app = gpui::TestAppContext::single();
        app.update(|cx| {
            gpui_component::init(cx);
            init_theme(cx);
        });
        let (panel, cx) = app.add_window_view(|_, cx| {
            let bee = cx.new(|_| Bee::new());
            let selection = cx.new(|_| super::super::scene::Selection::default());
            super::super::WorkbenchPanel::new(super::super::PanelKind::Console, bee, selection, cx)
        });
        let count = cx.update(|_, cx| {
            let bee = panel.read(cx).bee.read(cx);
            assert!(!bee.failed, "{:?}", bee.lines);
            for line in &bee.lines {
                assert!(bee
                    .console
                    .visible()
                    .any(|message| message.level == DiagnosticLevel::Info
                        && message.source == "Device"
                        && &message.message == line));
            }
            assert!(bee.lines.iter().any(|line| line.starts_with("Device: ")));
            assert!(bee
                .lines
                .iter()
                .any(|line| line.starts_with("Validation: ")));
            bee.console.visible().count()
        });
        let dimensions = size(px(960.0), px(640.0));
        cx.simulate_resize(dimensions);
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| panel.clone());
        cx.run_until_parked();
        assert!(count >= 6);
        for selector in [
            "console-message-0",
            "console-message-1",
            "console-message-3",
            "console-message-5",
        ] {
            assert!(cx.debug_bounds(selector).is_some());
        }
    }

    #[test]
    fn workbench_polls_background_messages_and_filtering_keeps_history_and_loss_notice() {
        let mut app = gpui::TestAppContext::single();
        app.update(|cx| {
            gpui_component::init(cx);
            init_theme(cx);
        });
        let (workbench, cx) = app.add_window_view(|window, cx| {
            let bee = cx.new(|_| Bee::failure("test device".into()));
            Workbench::new(bee, window, cx)
        });
        let bee = cx.update(|_, cx| workbench.read(cx).panels[4].read(cx).bee.clone());
        let dimensions = size(px(960.0), px(640.0));
        cx.simulate_resize(dimensions);
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| {
            workbench.clone()
        });
        cx.run_until_parked();
        let button = cx.debug_bounds("drawer-console").unwrap().center();
        cx.simulate_click(button, Modifiers::none());
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| {
            workbench.clone()
        });
        cx.run_until_parked();
        assert!(cx.debug_bounds("console-message-0").is_some());
        let sender = cx.update(|_, cx| bee.read(cx).console.sender.clone());
        std::thread::spawn(move || {
            sender.send(DiagnosticLevel::Info, "test", "runtime info");
            sender.send(DiagnosticLevel::Warning, "test", "runtime warning");
            sender.send(DiagnosticLevel::Error, "test", "runtime error");
        })
        .join()
        .unwrap();
        cx.executor().advance_clock(Duration::from_millis(100));
        cx.run_until_parked();
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| {
            workbench.clone()
        });
        cx.update(|_, cx| {
            let console = &bee.read(cx).console;
            assert_eq!(console.history.len(), 4);
            assert!(console
                .visible()
                .any(|message| message.message.contains("Vulkan initialization failed")));
            assert!(console
                .visible()
                .any(|message| message.message == "runtime warning"));
        });
        for selector in ["console-filter-0", "console-filter-1"] {
            let bounds = cx.debug_bounds(selector).unwrap();
            cx.simulate_click(bounds.center(), Modifiers::none());
            cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| {
                workbench.clone()
            });
            cx.run_until_parked();
        }
        cx.update(|_, cx| {
            let console = &bee.read(cx).console;
            assert_eq!(console.visible().count(), 2);
            assert!(console
                .visible()
                .all(|message| message.level == DiagnosticLevel::Error));
            assert_eq!(console.history.len(), 4);
        });
        // 恢复信息筛选后关闭面板；后台消息仍应被接收。
        let bounds = cx.debug_bounds("console-filter-0").unwrap();
        cx.simulate_click(bounds.center(), Modifiers::none());
        cx.update(|window, cx| {
            assert!(bee.read(cx).console.enabled[0]);
            let panel = workbench.read(cx).panels[4].clone();
            let tab = panel.read(cx).tab.as_ref().unwrap().upgrade().unwrap();
            tab.update(cx, |tab, cx| {
                tab.remove_panel(Arc::new(panel.clone()), window, cx)
            });
            let sender = &bee.read(cx).console.sender;
            for _ in 0..DIAGNOSTIC_QUEUE_CAPACITY + 1 {
                sender.send(DiagnosticLevel::Warning, "test", "burst while closed");
            }
        });
        cx.executor().advance_clock(Duration::from_millis(100));
        cx.run_until_parked();
        cx.update(|window, cx| {
            assert_eq!(bee.read(cx).console.dropped, 1);
            assert_eq!(
                bee.read(cx).console.history.len(),
                4 + DIAGNOSTIC_QUEUE_CAPACITY
            );
            workbench.update(cx, |workbench, cx| workbench.show_panel(4, window, cx));
        });
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| {
            workbench.clone()
        });
        cx.run_until_parked();
        assert!(cx.debug_bounds("console-dropped").is_some());
        cx.update(|_, cx| {
            let console = &bee.read(cx).console;
            assert!(!console.enabled[1]);
            assert_eq!(console.visible().count(), 3);
        });
    }

    #[test]
    fn filtering_preserves_history_and_capacity_drops_remain_visible() {
        let mut console = Console::default();
        for level in [
            DiagnosticLevel::Info,
            DiagnosticLevel::Warning,
            DiagnosticLevel::Error,
        ] {
            console.sender.send(level, "test", level_label(level));
        }
        assert!(console.drain());
        console.enabled = [false, false, true];
        assert_eq!(
            console
                .visible()
                .map(|message| message.level)
                .collect::<Vec<_>>(),
            vec![DiagnosticLevel::Error]
        );
        assert_eq!(console.history.len(), 3);
        console.enabled = [true; 3];
        assert_eq!(console.visible().count(), 3);
        assert!(!console.drain());
        for _ in 0..DIAGNOSTIC_QUEUE_CAPACITY + 7 {
            console.sender.send(DiagnosticLevel::Info, "test", "burst");
        }
        assert!(console.drain());
        assert_eq!(console.dropped, 7);
        for _ in 0..4 {
            for _ in 0..DIAGNOSTIC_QUEUE_CAPACITY {
                console
                    .sender
                    .send(DiagnosticLevel::Info, "test", "more messages");
            }
            assert!(console.drain());
        }
        assert_eq!(console.history.len(), HISTORY_CAPACITY);
        assert_eq!(
            console.dropped,
            7 + 3 + (5 * DIAGNOSTIC_QUEUE_CAPACITY - HISTORY_CAPACITY) as u64
        );
    }
}
