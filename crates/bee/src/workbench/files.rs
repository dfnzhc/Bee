use super::{WorkbenchPanel, MUTED};
use gpui::{div, prelude::*, rgb, Context, KeyDownEvent, PathPromptOptions};
use gpui_component::{button::Button, Disableable, Sizable};
use std::path::{Path, PathBuf};

#[derive(Debug)]
struct Entry {
    path: PathBuf,
    directory: bool,
}

#[derive(Default)]
pub(super) struct Files {
    path: Option<PathBuf>,
    entries: Vec<Entry>,
    loading: bool,
    error: Option<String>,
    generation: u64,
}

fn read_directory(path: &Path) -> Result<Vec<Entry>, String> {
    let read = || -> std::io::Result<Vec<Entry>> {
        let mut entries = std::fs::read_dir(path)?
            .map(|entry| {
                let entry = entry?;
                Ok(Entry {
                    directory: entry.file_type()?.is_dir(),
                    path: entry.path(),
                })
            })
            .collect::<std::io::Result<Vec<_>>>()?;
        entries.sort_by(|a, b| {
            b.directory
                .cmp(&a.directory)
                .then_with(|| a.path.file_name().cmp(&b.path.file_name()))
        });
        Ok(entries)
    };
    read().map_err(|error| format!("Could not read {}: {error}", path.display()))
}

impl Files {
    fn choose(cx: &mut Context<WorkbenchPanel>) {
        let receiver = cx.prompt_for_paths(PathPromptOptions {
            files: false,
            directories: true,
            multiple: false,
            prompt: Some("Choose a folder".into()),
        });
        cx.spawn(async move |panel, cx| {
            let result = receiver.await;
            panel
                .update(cx, |panel, cx| match result {
                    Ok(Ok(paths)) => panel.files.selected(paths, cx),
                    error => {
                        panel.files.error = Some(format!("Folder selection failed: {error:?}"));
                        cx.notify();
                    }
                })
                .ok();
        })
        .detach();
    }

    fn selected(&mut self, paths: Option<Vec<PathBuf>>, cx: &mut Context<WorkbenchPanel>) {
        if let Some(path) = paths.and_then(|paths| paths.into_iter().next()) {
            self.navigate(path, cx);
        }
    }

    fn navigate(&mut self, path: PathBuf, cx: &mut Context<WorkbenchPanel>) {
        self.generation = self.generation.wrapping_add(1);
        let generation = self.generation;
        self.path = Some(path.clone());
        self.entries.clear();
        self.error = None;
        self.loading = true;
        cx.notify();

        // 文件系统读取留在后台，旧位置的结果不得覆盖当前目录。
        let task = cx.background_spawn(async move { read_directory(&path) });
        cx.spawn(async move |panel, cx| {
            let result = task.await;
            panel
                .update(cx, |panel, cx| {
                    if panel.files.complete(generation, result) {
                        cx.notify();
                    }
                })
                .ok();
        })
        .detach();
    }

    fn complete(&mut self, generation: u64, result: Result<Vec<Entry>, String>) -> bool {
        if self.generation != generation {
            return false;
        }
        self.loading = false;
        match result {
            Ok(entries) => self.entries = entries,
            Err(error) => self.error = Some(error),
        }
        true
    }

    pub(super) fn render(&self, cx: &mut Context<WorkbenchPanel>) -> impl IntoElement {
        let parent = self
            .path
            .as_deref()
            .and_then(Path::parent)
            .map(Path::to_path_buf);
        let path = self.path.clone();
        let key_parent = parent.clone();
        let key_path = path.clone();
        div()
            .tab_group()
            .flex()
            .flex_col()
            .gap_2()
            .child(
                div()
                    .flex()
                    .flex_wrap()
                    .gap_1()
                    .child(
                        Button::new("files-choose")
                            .small()
                            .label("Browse Folder…")
                            .debug_selector(|| "files-choose".into())
                            .on_key_down(cx.listener(|_, event: &KeyDownEvent, _, cx| {
                                if matches!(event.keystroke.key.as_str(), "enter" | "space") {
                                    Self::choose(cx);
                                }
                            }))
                            .on_click(cx.listener(|_, _, _, cx| Self::choose(cx))),
                    )
                    .child(
                        Button::new("files-up")
                            .small()
                            .label("Up")
                            .debug_selector(|| "files-up".into())
                            .disabled(parent.is_none())
                            .on_key_down(cx.listener(move |panel, event: &KeyDownEvent, _, cx| {
                                if matches!(event.keystroke.key.as_str(), "enter" | "space") {
                                    if let Some(parent) = &key_parent {
                                        panel.files.navigate(parent.clone(), cx);
                                    }
                                }
                            }))
                            .on_click(cx.listener(move |panel, _, _, cx| {
                                if let Some(parent) = &parent {
                                    panel.files.navigate(parent.clone(), cx);
                                }
                            })),
                    )
                    .child(
                        Button::new("files-refresh")
                            .small()
                            .label("Refresh")
                            .debug_selector(|| "files-refresh".into())
                            .disabled(path.is_none())
                            .on_key_down(cx.listener(move |panel, event: &KeyDownEvent, _, cx| {
                                if matches!(event.keystroke.key.as_str(), "enter" | "space") {
                                    if let Some(path) = &key_path {
                                        panel.files.navigate(path.clone(), cx);
                                    }
                                }
                            }))
                            .on_click(cx.listener(move |panel, _, _, cx| {
                                if let Some(path) = &path {
                                    panel.files.navigate(path.clone(), cx);
                                }
                            })),
                    ),
            )
            .child(
                div()
                    .id("files-path")
                    .debug_selector(|| "files-path".into())
                    .child(self.path.as_ref().map_or_else(
                        || "No folder selected".to_string(),
                        |path| path.display().to_string(),
                    )),
            )
            .child(div().text_color(rgb(MUTED)).child("Read-only browsing"))
            .when(self.loading, |this| this.child("Loading folder..."))
            .when_some(self.error.clone(), |this, error| {
                this.child(div().text_color(rgb(0xffa8a8)).child(error))
            })
            .when(
                self.path.is_some()
                    && !self.loading
                    && self.error.is_none()
                    && self.entries.is_empty(),
                |this| this.child("Empty folder"),
            )
            .children(self.entries.iter().enumerate().map(|(index, entry)| {
                let name = entry.path.file_name().unwrap_or_default().to_string_lossy();
                if entry.directory {
                    let path = entry.path.clone();
                    let key_path = path.clone();
                    let selector = format!("files-folder-{index}");
                    div()
                        .child(
                            Button::new(("files-entry", index))
                                .small()
                                .debug_selector(move || selector)
                                .label(format!("[Folder] {name}"))
                                .on_key_down(cx.listener(
                                    move |panel, event: &KeyDownEvent, _, cx| {
                                        if matches!(event.keystroke.key.as_str(), "enter" | "space")
                                        {
                                            panel.files.navigate(key_path.clone(), cx);
                                        }
                                    },
                                ))
                                .on_click(cx.listener(move |panel, _, _, cx| {
                                    panel.files.navigate(path.clone(), cx);
                                })),
                        )
                        .into_any_element()
                } else {
                    div().child(format!("[File] {name}")).into_any_element()
                }
            }))
    }
}

#[cfg(test)]
mod tests {
    use super::super::scene;
    use super::*;
    use crate::Bee;
    use gpui::{point, px, size, Modifiers};
    use std::{fs, time::SystemTime};

    struct Directory(PathBuf);

    impl Directory {
        fn new() -> Self {
            let unique = SystemTime::now()
                .duration_since(SystemTime::UNIX_EPOCH)
                .unwrap()
                .as_nanos();
            let path =
                std::env::temp_dir().join(format!("bee-files-{}-{unique}", std::process::id()));
            fs::create_dir(&path).unwrap();
            Self(path)
        }
    }

    impl Drop for Directory {
        fn drop(&mut self) {
            fs::remove_dir_all(&self.0).unwrap();
        }
    }

    #[test]
    fn file_panel_buttons_enter_and_return_and_keyboard_opens_a_folder() {
        let directory = Directory::new();
        let child = directory.0.join("子目录");
        fs::create_dir(&child).unwrap();
        fs::write(directory.0.join("材质.txt"), "original contents").unwrap();
        let mut app = gpui::TestAppContext::single();
        app.update(|cx| {
            gpui_component::init(cx);
            super::super::init_theme(cx);
        });
        let (panel, cx) = app.add_window_view(|_, cx| {
            let bee = cx.new(|_| Bee::failure("test device".into()));
            let selection = cx.new(|_| scene::Selection::default());
            WorkbenchPanel::new(super::super::PanelKind::Files, bee, selection, cx)
        });
        let dimensions = size(px(960.0), px(640.0));
        cx.simulate_resize(dimensions);
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| panel.clone());
        cx.run_until_parked();
        assert!(cx.debug_bounds("files-choose").is_some());
        assert!(cx.debug_bounds("files-path").is_some());
        panel.update(cx, |panel, cx| {
            panel.files.selected(Some(vec![directory.0.clone()]), cx)
        });
        cx.run_until_parked();
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| panel.clone());
        let folder = cx.debug_bounds("files-folder-0").unwrap();
        cx.simulate_click(folder.center(), Modifiers::none());
        cx.run_until_parked();
        cx.update(|_, cx| assert_eq!(panel.read(cx).files.path.as_ref(), Some(&child)));
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| panel.clone());
        let up = cx.debug_bounds("files-up").unwrap();
        cx.simulate_click(up.center(), Modifiers::none());
        cx.run_until_parked();
        cx.update(|_, cx| assert_eq!(panel.read(cx).files.path.as_ref(), Some(&directory.0)));
        cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| panel.clone());
        cx.update(|window, cx| {
            window.focus(&panel.read(cx).focus);
            for _ in 0..4 {
                window.focus_next();
            }
        });
        cx.simulate_keystrokes("enter");
        cx.run_until_parked();
        cx.update(|_, cx| {
            assert_eq!(panel.read(cx).files.path.as_ref(), Some(&child));
            assert!(panel.read(cx).files.entries.is_empty());
        });
        assert_eq!(
            fs::read_to_string(directory.0.join("材质.txt")).unwrap(),
            "original contents"
        );
    }

    #[test]
    fn lists_real_entries_without_modifying_them_and_reports_read_errors() {
        let directory = Directory::new();
        let child = directory.0.join("空目录");
        fs::create_dir(&child).unwrap();
        let file = directory.0.join("材质.txt");
        fs::write(&file, "original contents").unwrap();
        let entries = read_directory(&directory.0).unwrap();
        assert_eq!(entries.len(), 2);
        assert!(entries[0].directory);
        assert_eq!(entries[0].path, child);
        assert!(!entries[1].directory);
        assert_eq!(entries[1].path, file);
        assert!(read_directory(&child).unwrap().is_empty());
        assert_eq!(fs::read_to_string(&file).unwrap(), "original contents");
        assert!(read_directory(&file)
            .unwrap_err()
            .contains("Could not read"));
        fs::remove_dir(&child).unwrap();
        assert!(read_directory(&child)
            .unwrap_err()
            .contains("Could not read"));
    }

    #[test]
    fn selection_cancel_navigation_and_out_of_order_reads_preserve_current_location() {
        let directory = Directory::new();
        let child = directory.0.join("子目录");
        fs::create_dir(&child).unwrap();
        let mut app = gpui::TestAppContext::single();
        let panel = app.new(|cx| {
            let bee = cx.new(|_| Bee::failure("test device".into()));
            let selection = cx.new(|_| scene::Selection::default());
            WorkbenchPanel::new(super::super::PanelKind::Files, bee, selection, cx)
        });
        panel.update(&mut app, |panel, cx| {
            assert!(panel.files.path.is_none());
            panel.files.selected(None, cx);
            assert!(panel.files.path.is_none());
            panel.files.selected(Some(vec![directory.0.clone()]), cx);
        });
        app.run_until_parked();
        panel.update(&mut app, |panel, cx| {
            assert_eq!(panel.files.entries.len(), 1);
            panel.files.selected(None, cx);
            assert_eq!(panel.files.path.as_ref(), Some(&directory.0));
            assert_eq!(panel.files.entries.len(), 1);
            let stale = panel.files.generation;
            panel.files.navigate(child.clone(), cx);
            assert!(panel.files.entries.is_empty());
            assert!(!panel.files.complete(stale, read_directory(&directory.0)));
            assert!(!panel.files.complete(stale, Err("stale failure".into())));
            assert!(panel.files.loading);
            assert!(panel.files.error.is_none());
        });
        app.run_until_parked();
        panel.update(&mut app, |panel, cx| {
            assert_eq!(panel.files.path.as_ref(), Some(&child));
            assert!(panel.files.entries.is_empty());
            assert!(!panel.files.loading);
            fs::remove_dir(&child).unwrap();
            panel.files.navigate(child.clone(), cx);
        });
        app.run_until_parked();
        panel.update(&mut app, |panel, cx| {
            assert!(panel
                .files
                .error
                .as_ref()
                .unwrap()
                .contains("Could not read"));
            assert!(!panel.files.loading);
            let parent = panel
                .files
                .path
                .as_deref()
                .unwrap()
                .parent()
                .unwrap()
                .to_path_buf();
            panel.files.navigate(parent, cx);
            panel.files.navigate(child.clone(), cx);
            panel.files.navigate(directory.0.clone(), cx);
        });
        app.run_until_parked();
        panel.update(&mut app, |panel, _| {
            assert_eq!(panel.files.path.as_ref(), Some(&directory.0));
            assert!(panel.files.error.is_none());
            assert!(!panel.files.loading);
            assert!(panel.files.entries.is_empty());
        });
    }
}
