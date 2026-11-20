use super::super::{ACCENT, BORDER, MUTED};
use super::{RecentProject, Workbench};
use bee::{project::SceneEntry, scene_document::SceneDocument};
use gpui::{
    div, prelude::*, px, rgb, AnyElement, App, Context, Entity, SharedString, WeakEntity, Window,
};
use gpui_component::{
    button::{Button, ButtonVariants},
    dialog::DialogButtonProps,
    input::{Input, InputState},
    menu::{PopupMenu, PopupMenuItem},
    select::{Select, SelectItem, SelectState},
    tooltip::Tooltip,
    IndexPath, Sizable, StyledExt, WindowExt,
};
use std::path::PathBuf;
use uuid::Uuid;

#[derive(Clone)]
enum Form {
    Project(PathBuf),
    Scene,
    Copy,
    Rename,
}

#[derive(Clone)]
struct SceneChoice {
    id: Uuid,
    name: SharedString,
}

impl SelectItem for SceneChoice {
    type Value = Uuid;

    fn title(&self) -> SharedString {
        self.name.clone()
    }

    fn value(&self) -> &Uuid {
        &self.id
    }
}

fn select<T: SelectItem + 'static>(
    options: Vec<T>,
    window: &mut Window,
    cx: &mut App,
) -> Entity<SelectState<Vec<T>>> {
    let initial = (!options.is_empty()).then_some(IndexPath::default());
    cx.new(|cx| SelectState::new(options, initial, window, cx))
}

fn action_button(id: &'static str, label: &'static str) -> Button {
    Button::new(id)
        .debug_selector(move || id.into())
        .label(label)
        .small()
}

fn confirm_button(button: AnyElement) -> AnyElement {
    div()
        .id("confirm-project-form")
        .debug_selector(|| "confirm-project-form".into())
        .child(button)
        .into_any_element()
}

impl Workbench {
    pub(in super::super) fn project_titles(&self) -> Option<(SharedString, SharedString)> {
        let session = self.active_session()?;
        let manifest = session.project.manifest();
        let current = manifest
            .scenes
            .iter()
            .find(|scene| scene.id == session.saved.id())?;
        Some((manifest.name.clone().into(), current.name.clone().into()))
    }

    pub(in super::super) fn project_status(&self) -> Option<impl IntoElement> {
        let (text, error) = self.projects.as_ref()?.notice.as_ref()?;
        let text = SharedString::from(text.clone());
        Some(
            div()
                .id("project-status")
                .debug_selector(|| "project-status".into())
                .px_3()
                .py_1()
                .border_t_1()
                .border_color(rgb(BORDER))
                .text_color(rgb(if *error { 0xffa8a8 } else { ACCENT }))
                .text_ellipsis()
                .tooltip({
                    let text = text.clone();
                    move |window, cx| Tooltip::new(text.clone()).build(window, cx)
                })
                .child(text),
        )
    }

    pub(in super::super) fn project_chooser(&self, cx: &Context<Self>) -> impl IntoElement {
        let projects = self.projects.as_ref().unwrap();
        div()
            .id("project-chooser")
            .debug_selector(|| "project-chooser".into())
            .flex_1()
            .min_h_0()
            .overflow_y_scroll()
            .child(
                div()
                    .max_w(px(740.0))
                    .mx_auto()
                    .p_5()
                    .flex()
                    .flex_col()
                    .gap_4()
                    .child(
                        div()
                            .text_size(px(22.0))
                            .font_semibold()
                            .child("Bee Projects"),
                    )
                    .child(
                        div()
                            .flex()
                            .flex_wrap()
                            .gap_2()
                            .child(
                                action_button("new-project", "New Project")
                                    .primary()
                                    .on_click(cx.listener(|this, _, window, cx| {
                                        this.choose_project_location(window, cx);
                                    })),
                            )
                            .child(action_button("open-project", "Open Project").on_click(
                                cx.listener(|this, _, window, cx| this.choose_project(window, cx)),
                            )),
                    )
                    .child(div().font_semibold().child("Recent Projects"))
                    .when(projects.recent.is_empty(), |this| {
                        this.child(div().text_color(rgb(MUTED)).child("No recent projects"))
                    })
                    .children(projects.recent.iter().enumerate().map(|(index, recent)| {
                        let id = recent.id;
                        let path = recent.directory.clone();
                        div()
                            .id(index)
                            .flex()
                            .flex_wrap()
                            .items_center()
                            .gap_2()
                            .py_2()
                            .border_t_1()
                            .border_color(rgb(BORDER))
                            .child(
                                div()
                                    .flex_1()
                                    .min_w(px(160.0))
                                    .child(div().text_ellipsis().child(recent.name.clone()))
                                    .child(
                                        div()
                                            .text_ellipsis()
                                            .text_color(rgb(MUTED))
                                            .child(path.display().to_string()),
                                    ),
                            )
                            .child(action_button("open-recent", "Open").on_click(cx.listener(
                                move |this, _, window, cx| {
                                    this.open_project(path.clone(), Some(id), None, window, cx);
                                },
                            )))
                            .child(
                                action_button("locate-recent", "Locate").on_click(cx.listener(
                                    move |this, _, window, cx| {
                                        this.choose_folder(
                                            "Locate Bee Project",
                                            move |this, path, window, cx| {
                                                this.open_project(path, Some(id), None, window, cx);
                                            },
                                            window,
                                            cx,
                                        );
                                    },
                                )),
                            )
                            .child(action_button("remove-recent", "Remove").on_click(
                                cx.listener(move |this, _, _, cx| this.remove_recent(id, cx)),
                            ))
                    })),
            )
    }

    fn project_menu_item(
        label: impl Into<SharedString>,
        selector: impl Into<SharedString>,
        workbench: &WeakEntity<Self>,
        action: impl Fn(&mut Self, &mut Window, &mut Context<Self>) + Clone + 'static,
    ) -> PopupMenuItem {
        let workbench = workbench.clone();
        Self::menu_item(label, selector).on_click(move |_, window, cx| {
            let workbench = workbench.clone();
            let action = action.clone();
            window.defer(cx, move |window, cx| {
                let _ = workbench.update(cx, |this, cx| {
                    if !this.project_busy() && !this.closing {
                        action(this, window, cx);
                    }
                });
            });
        })
    }

    pub(in super::super) fn file_menu_items(
        &self,
        menu: PopupMenu,
        workbench: &WeakEntity<Self>,
    ) -> PopupMenu {
        menu.item(Self::project_menu_item(
            "New Project...",
            "file-new-project",
            workbench,
            Self::choose_project_location,
        ))
        .item(Self::project_menu_item(
            "Open Project...",
            "file-open-project",
            workbench,
            Self::choose_project,
        ))
        .item(Self::project_menu_item(
            "Close Project",
            "file-close-project",
            workbench,
            Self::close_project,
        ))
        .separator()
        .item(Self::project_menu_item(
            "Save Scene   Ctrl+S",
            "file-save-scene",
            workbench,
            |this, _, cx| this.save_project_scene(cx),
        ))
        .separator()
    }

    pub(in super::super) fn scene_menu_items(
        &self,
        mut menu: PopupMenu,
        workbench: &WeakEntity<Self>,
    ) -> PopupMenu {
        let session = self.active_session().unwrap();
        let manifest = session.project.manifest();
        let current = session.saved.id();
        menu = menu.min_w(px(226.0)).max_w(px(340.0)).scrollable(true);
        for (index, entry) in manifest.scenes.iter().enumerate() {
            let id = entry.id;
            let marker = match (id == current, id == manifest.startup_scene) {
                (true, true) => "[Current, Startup] ",
                (true, false) => "[Current] ",
                (false, true) => "[Startup] ",
                (false, false) => "",
            };
            menu = menu.item(Self::project_menu_item(
                format!("{marker}{}", entry.name),
                format!("open-scene-{index}"),
                workbench,
                move |this, window, cx| this.open_scene(None, id, window, cx),
            ));
        }
        menu.separator()
            .item(Self::project_menu_item(
                "New Scene...",
                "new-scene",
                workbench,
                |this, window, cx| {
                    this.scene_form(Form::Scene, window, cx);
                },
            ))
            .item(Self::project_menu_item(
                "Copy Saved Scene...",
                "copy-scene",
                workbench,
                |this, window, cx| {
                    this.scene_form(Form::Copy, window, cx);
                },
            ))
            .item(Self::project_menu_item(
                "Rename Scene...",
                "rename-scene",
                workbench,
                |this, window, cx| {
                    this.scene_form(Form::Rename, window, cx);
                },
            ))
            .item(
                Self::project_menu_item(
                    "Set as Startup Scene",
                    "set-startup-scene",
                    workbench,
                    move |this, _, cx| {
                        let result = this
                            .edit_session()
                            .project
                            .set_startup_scene(current)
                            .map(|()| None)
                            .map_err(|error| error.to_string());
                        this.project_notice(result, cx);
                    },
                )
                .disabled(current == manifest.startup_scene),
            )
            .item(Self::project_menu_item(
                "Set Current View as Start View",
                "capture-start-camera",
                workbench,
                |this, _, cx| this.capture_start_camera(cx),
            ))
            .item(
                Self::project_menu_item(
                    "Delete Current Scene...",
                    "delete-scene",
                    workbench,
                    |this, window, cx| this.delete_scene_dialog(window, cx),
                )
                .disabled(manifest.scenes.len() == 1),
            )
    }

    fn choose_project_location(&mut self, window: &mut Window, cx: &mut Context<Self>) {
        self.choose_folder(
            "Choose a location for the Bee project",
            |this, directory, window, cx| this.scene_form(Form::Project(directory), window, cx),
            window,
            cx,
        );
    }

    fn scene_form(&self, form: Form, window: &mut Window, cx: &mut Context<Self>) {
        let current = self
            .project_titles()
            .map(|(_, scene)| scene)
            .unwrap_or_default();
        let (title, command, suggestion) = match &form {
            Form::Project(_) => ("New Project", "Create", "New Project".into()),
            Form::Scene => ("New Scene", "Create", "New Scene".into()),
            Form::Copy => ("Copy Saved Scene", "Copy", format!("{current} Copy").into()),
            Form::Rename => ("Rename Scene", "Rename", current),
        };
        let name = cx.new(|cx| InputState::new(window, cx).default_value(suggestion));
        let template = matches!(&form, Form::Project(_) | Form::Scene)
            .then(|| select(vec!["Built-in Example", "Blank"], window, cx));
        let placement = matches!(&form, Form::Project(_)).then(|| {
            select(
                vec![
                    "Create a project subfolder",
                    "Use the selected empty folder",
                ],
                window,
                cx,
            )
        });
        let workbench = cx.weak_entity();
        let width = (window.viewport_size().width - px(24.0)).min(px(480.0));
        window.open_dialog(cx, move |dialog, _, _| {
            let body = div()
                .flex()
                .flex_col()
                .gap_2()
                .child("Name")
                .child(
                    div()
                        .id("scene-name-input")
                        .debug_selector(|| "scene-name-input".into())
                        .child(Input::new(&name)),
                )
                .when_some(template.as_ref(), |body, template| {
                    body.child("Template").child(
                        div()
                            .id("scene-template")
                            .debug_selector(|| "scene-template".into())
                            .child(Select::new(template)),
                    )
                })
                .when_some(placement.as_ref(), |body, placement| {
                    let Form::Project(directory) = &form else {
                        unreachable!();
                    };
                    body.child("Location")
                        .child(div().text_ellipsis().child(directory.display().to_string()))
                        .child(
                            div()
                                .id("project-placement")
                                .debug_selector(|| "project-placement".into())
                                .child(Select::new(placement)),
                        )
                });
            dialog
                .w(width)
                .title(title)
                .confirm()
                .footer(|ok, cancel, window, cx| {
                    vec![cancel(window, cx), confirm_button(ok(window, cx))]
                })
                .button_props(DialogButtonProps::default().ok_text(command))
                .child(body)
                .on_ok({
                    let workbench = workbench.clone();
                    let form = form.clone();
                    let name = name.clone();
                    let template = template.clone();
                    let placement = placement.clone();
                    move |_, window, cx| {
                        let name = name.read(cx).value().trim().to_string();
                        let blank = template
                            .as_ref()
                            .and_then(|state| state.read(cx).selected_value().copied())
                            == Some("Blank");
                        let in_place = placement
                            .as_ref()
                            .and_then(|state| state.read(cx).selected_value().copied())
                            == Some("Use the selected empty folder");
                        let workbench = workbench.clone();
                        let form = form.clone();
                        window.defer(cx, move |window, cx| {
                            let _ = workbench.update(cx, |this, cx| {
                                this.submit_scene_form(form, name, blank, in_place, window, cx);
                            });
                        });
                        true
                    }
                })
        });
    }

    fn submit_scene_form(
        &mut self,
        form: Form,
        name: String,
        blank: bool,
        in_place: bool,
        window: &mut Window,
        cx: &mut Context<Self>,
    ) {
        if name.is_empty() {
            self.project_notice(Err("Name is required".into()), cx);
            return;
        }
        let document = if blank {
            SceneDocument::blank
        } else {
            SceneDocument::builtin
        };
        match form {
            Form::Project(directory) => {
                self.new_project(directory, in_place, name, document(), window, cx)
            }
            Form::Scene => self.new_scene(name, document(), window, cx),
            Form::Copy | Form::Rename => {
                let session = self.edit_session();
                let id = session.saved.id();
                let result = if matches!(form, Form::Copy) {
                    session.project.copy_scene(id, &name).map(|_| ())
                } else {
                    session.project.rename_scene(id, &name)
                };
                self.project_notice(result.map(|()| None).map_err(|error| error.to_string()), cx);
            }
        }
    }

    fn delete_scene_dialog(&self, window: &mut Window, cx: &mut Context<Self>) {
        let session = self.active_session().unwrap();
        let current = session.saved.id();
        let manifest = session.project.manifest();
        let name = self.project_titles().unwrap().1;
        let alternatives = manifest.scenes.iter().filter(|entry| entry.id != current);
        let choices = alternatives
            .map(|entry| SceneChoice {
                id: entry.id,
                name: entry.name.clone().into(),
            })
            .collect::<Vec<_>>();
        if choices.is_empty() {
            return;
        }
        let select = select(choices, window, cx);
        let label = if current == manifest.startup_scene {
            "Open and set as startup"
        } else {
            "Scene to open"
        };
        let workbench = cx.weak_entity();
        let width = (window.viewport_size().width - px(24.0)).min(px(480.0));
        window.open_dialog(cx, move |dialog, _, _| {
            dialog
                .w(width)
                .title(div().text_ellipsis().child(format!("Delete {name}?")))
                .confirm()
                .footer(|ok, cancel, window, cx| {
                    vec![cancel(window, cx), confirm_button(ok(window, cx))]
                })
                .button_props(DialogButtonProps::default().ok_text("Delete"))
                .child(
                    div()
                        .flex()
                        .flex_col()
                        .gap_2()
                        .child(label)
                        .child(Select::new(&select)),
                )
                .on_ok({
                    let select = select.clone();
                    let workbench = workbench.clone();
                    move |_, window, cx| {
                        let Some(id) = select.read(cx).selected_value().copied() else {
                            return false;
                        };
                        let workbench = workbench.clone();
                        window.defer(cx, move |window, cx| {
                            let _ = workbench.update(cx, |this, cx| {
                                this.delete_current_scene(id, window, cx);
                            });
                        });
                        true
                    }
                })
        });
    }

    pub(super) fn show_recovery(
        &self,
        project: Option<RecentProject>,
        failed: Uuid,
        entries: &[SceneEntry],
        has_backup: bool,
        error: String,
        window: &mut Window,
        cx: &mut Context<Self>,
    ) {
        let choices: Vec<_> = entries
            .iter()
            .filter(|entry| entry.id != failed)
            .map(|entry| SceneChoice {
                id: entry.id,
                name: entry.name.clone().into(),
            })
            .collect();
        let scenes = (!choices.is_empty()).then(|| select(choices, window, cx));
        let workbench = cx.weak_entity();
        let width = (window.viewport_size().width - px(24.0)).min(px(480.0));
        window.open_dialog(cx, move |dialog, _, _| {
            let body = div()
                .flex()
                .flex_col()
                .gap_2()
                .child(
                    div()
                        .id("recovery-error")
                        .debug_selector(|| "recovery-error".into())
                        .text_ellipsis()
                        .child(error.clone())
                        .tooltip({
                            let error = error.clone();
                            move |window, cx| Tooltip::new(error.clone()).build(window, cx)
                        }),
                )
                .when_some(scenes.as_ref(), |body, scenes| {
                    body.child("Choose another scene")
                        .child(Select::new(scenes))
                });
            dialog
                .w(width)
                .title("Scene could not be opened")
                .confirm()
                .button_props(DialogButtonProps::default().ok_text("Open Scene"))
                .child(body)
                .footer({
                    let workbench = workbench.clone();
                    let project = project.clone();
                    let can_open = scenes.is_some();
                    move |ok, cancel, window, cx| {
                        let mut buttons = vec![cancel(window, cx)];
                        if has_backup {
                            let workbench = workbench.clone();
                            let project = project.clone();
                            buttons.push(
                                action_button("restore-backup", "Restore Backup")
                                    .on_click(move |_, window, cx| {
                                        window.close_dialog(cx);
                                        let _ = workbench.update(cx, |this, cx| {
                                            this.restore_scene(project.clone(), failed, window, cx);
                                        });
                                    })
                                    .into_any_element(),
                            );
                        }
                        if can_open {
                            buttons.push(confirm_button(ok(window, cx)));
                        }
                        buttons
                    }
                })
                .on_ok({
                    let scenes = scenes.clone();
                    let workbench = workbench.clone();
                    let project = project.clone();
                    move |_, window, cx| {
                        let Some(id) = scenes
                            .as_ref()
                            .and_then(|select| select.read(cx).selected_value().copied())
                        else {
                            return false;
                        };
                        let workbench = workbench.clone();
                        let project = project.clone();
                        window.defer(cx, move |window, cx| {
                            let _ = workbench.update(cx, |this, cx| {
                                if let Some(project) = project {
                                    this.open_project(
                                        project.directory,
                                        Some(project.id),
                                        Some(id),
                                        window,
                                        cx,
                                    );
                                } else {
                                    this.open_scene(None, id, window, cx);
                                }
                            });
                        });
                        true
                    }
                })
        });
    }
}
