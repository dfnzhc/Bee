mod ui;

#[cfg(test)]
mod tests;

use super::Workbench;
use crate::Bee;
use bee::{
    project::{DeletedScene, Error, LoadedScene, Project, ProjectLocation},
    scene_document::{PreparedScene, SceneDocument},
};
use bee_scene::Camera;
use gpui::{App, Context, PromptButton, PromptLevel, Window};
use serde::{Deserialize, Serialize};
use std::{
    fs,
    io::{self, Write},
    path::PathBuf,
};
use uuid::Uuid;

type SceneProject = Project<SceneDocument>;
type SavedScene = LoadedScene<SceneDocument>;

pub(super) struct Projects {
    session: Option<Session>,
    settings: Option<PathBuf>,
    recent: Vec<RecentProject>,
    busy: bool,
    notice: Option<(String, bool)>,
}

#[derive(Clone, Deserialize, Serialize)]
struct RecentProject {
    id: Uuid,
    name: String,
    directory: PathBuf,
}

struct OpenProject {
    project: SceneProject,
    directory: PathBuf,
}

struct Session {
    project: SceneProject,
    saved: SavedScene,
    scene_version: u64,
    start_camera: Camera,
}

impl Session {
    fn dirty(&self, bee: &Bee) -> bool {
        let saved = &self.saved.content;
        self.start_camera != saved.start_camera
            || (bee.state.versions().scene != self.scene_version
                && bee.state.scene() != &saved.scene)
            || bee.state.snapshot().render != saved.render
            || bee.viewport.flow.active() != &saved.applied_flow
            || bee.viewport.flow.draft() != &saved.draft_flow
    }

    fn save(&mut self, bee: &Bee) -> Result<(), Error> {
        let snapshot = bee.state.snapshot();
        let previous = std::mem::replace(
            &mut self.saved.content,
            SceneDocument {
                scene: (*snapshot.scene).clone(),
                start_camera: self.start_camera,
                render: snapshot.render,
                applied_flow: bee.viewport.flow.active().clone(),
                draft_flow: bee.viewport.flow.draft().clone(),
            },
        );
        if let Err(error) = self.project.save_scene(&mut self.saved) {
            self.saved.content = previous;
            return Err(error);
        }
        self.scene_version = snapshot.versions.scene;
        Ok(())
    }
}

impl Projects {
    pub(super) fn load() -> Self {
        Self::with_settings(
            std::env::var_os("LOCALAPPDATA")
                .map(|path| PathBuf::from(path).join("Bee").join("recent-projects.json")),
        )
    }

    fn with_settings(settings: Option<PathBuf>) -> Self {
        let mut projects = Self {
            session: None,
            settings,
            recent: Vec::new(),
            busy: false,
            notice: None,
        };
        if let Some(path) = &projects.settings {
            match fs::read(path) {
                Ok(bytes) => match serde_json::from_slice(&bytes) {
                    Ok(recent) => projects.recent = recent,
                    Err(error) => {
                        projects.notice =
                            Some((format!("Could not read {}: {error}", path.display()), true));
                    }
                },
                Err(error) if error.kind() == io::ErrorKind::NotFound => {}
                Err(error) => {
                    projects.notice =
                        Some((format!("Could not read {}: {error}", path.display()), true));
                }
            }
        }
        projects
    }

    pub(super) fn has_session(&self) -> bool {
        self.session.is_some()
    }

    fn remember(&mut self, project: RecentProject) -> Result<(), String> {
        self.recent.retain(|entry| entry.id != project.id);
        self.recent.insert(0, project);
        self.recent.truncate(12);
        self.write_recent()
    }

    fn write_recent(&self) -> Result<(), String> {
        let Some(path) = &self.settings else {
            return Ok(());
        };
        let write = || -> io::Result<()> {
            let parent = path.parent().unwrap();
            fs::create_dir_all(parent)?;
            let mut file = tempfile::NamedTempFile::new_in(parent)?;
            serde_json::to_writer_pretty(&mut file, &self.recent)?;
            file.flush()?;
            file.persist(path).map_err(|error| error.error)?;
            Ok(())
        };
        write().map_err(|error| format!("Could not update {}: {error}", path.display()))
    }
}

impl OpenProject {
    fn read(directory: PathBuf, expected: Option<Uuid>) -> Result<Self, String> {
        let project = SceneProject::open(&directory).map_err(|error| match &error {
            Error::Io { path, source }
                if path
                    .file_name()
                    .is_some_and(|name| name == "project.beeproj")
                    && source.kind() == io::ErrorKind::NotFound =>
            {
                format!("No Bee project in {} ({error})", directory.display())
            }
            _ => error.to_string(),
        })?;
        if expected.is_some_and(|id| id != project.manifest().project_id) {
            return Err(format!(
                "{} belongs to a different Bee project",
                directory.display()
            ));
        }
        Ok(Self { project, directory })
    }

    fn recent(&self) -> RecentProject {
        RecentProject {
            id: self.project.manifest().project_id,
            name: self.project.manifest().name.clone(),
            directory: self.directory.clone(),
        }
    }
}

impl Workbench {
    fn active_session(&self) -> Option<&Session> {
        self.projects.as_ref()?.session.as_ref()
    }

    fn edit_session(&mut self) -> &mut Session {
        self.projects.as_mut().unwrap().session.as_mut().unwrap()
    }

    fn project_notice(&mut self, result: Result<Option<String>, String>, cx: &mut Context<Self>) {
        self.projects.as_mut().unwrap().notice = match result {
            Ok(text) => text.map(|text| (text, false)),
            Err(error) => Some((error, true)),
        };
        cx.notify();
    }

    fn project_busy(&self) -> bool {
        self.projects.as_ref().is_some_and(|projects| projects.busy)
    }

    pub(super) fn save_project_scene(&mut self, cx: &mut Context<Self>) {
        if self.active_session().is_some() && !self.project_busy() {
            let result = self.save_scene(cx).map(|()| Some("Scene saved".into()));
            self.project_notice(result, cx);
        }
    }

    fn save_scene(&mut self, cx: &App) -> Result<(), String> {
        let bee = self.bee.clone();
        self.edit_session()
            .save(bee.read(cx))
            .map_err(|error| error.to_string())
    }

    fn capture_start_camera(&mut self, cx: &mut Context<Self>) {
        let camera = self.bee.read(cx).state.camera();
        self.edit_session().start_camera = camera;
        self.project_notice(Ok(None), cx);
    }

    fn choose_folder(
        &mut self,
        title: &'static str,
        chosen: impl FnOnce(&mut Self, PathBuf, &mut Window, &mut Context<Self>) + 'static,
        window: &mut Window,
        cx: &mut Context<Self>,
    ) {
        if self.project_busy() {
            return;
        }
        self.projects.as_mut().unwrap().busy = true;
        #[cfg(not(test))]
        let receiver = cx.prompt_for_paths(gpui::PathPromptOptions {
            files: false,
            directories: true,
            multiple: false,
            prompt: Some(title.into()),
        });
        // GPUI 的测试平台仅实现单路径选择。
        #[cfg(test)]
        let receiver = cx.prompt_for_new_path(std::path::Path::new(title), None);
        cx.spawn_in(window, async move |view, cx| {
            #[cfg(not(test))]
            let result = receiver
                .await
                .map(|result| result.map(|paths| paths.and_then(|paths| paths.into_iter().next())));
            #[cfg(test)]
            let result = receiver.await;
            let _ = view.update_in(cx, |this, window, cx| {
                this.projects.as_mut().unwrap().busy = false;
                match result {
                    Ok(Ok(Some(path))) => chosen(this, path, window, cx),
                    Ok(Ok(None)) => {}
                    error => {
                        this.project_notice(Err(format!("Folder selection failed: {error:?}")), cx)
                    }
                }
            });
        })
        .detach();
    }

    fn confirm_project_action(
        &mut self,
        message: &str,
        detail: Option<&str>,
        buttons: &[PromptButton],
        chosen: impl FnOnce(&mut Self, usize, &mut Window, &mut Context<Self>) + 'static,
        window: &mut Window,
        cx: &mut Context<Self>,
    ) {
        if self.project_busy() {
            return;
        }
        self.projects.as_mut().unwrap().busy = true;
        let receiver = window.prompt(PromptLevel::Warning, message, detail, buttons, cx);
        cx.spawn_in(window, async move |view, cx| {
            let response = receiver.await;
            let _ = view.update_in(cx, |this, window, cx| {
                this.projects.as_mut().unwrap().busy = false;
                if let Ok(response) = response {
                    chosen(this, response, window, cx);
                }
            });
        })
        .detach();
    }

    fn leave_scene(
        &mut self,
        commit: impl FnOnce(&mut Self, &mut Window, &mut Context<Self>) -> Result<Option<String>, String>
            + 'static,
        window: &mut Window,
        cx: &mut Context<Self>,
    ) {
        if self.project_busy() {
            return;
        }
        if self.scene_dirty(cx) {
            self.confirm_project_action(
                "Save changes to the current scene?",
                None,
                &[
                    PromptButton::ok("Save"),
                    PromptButton::new("Discard"),
                    PromptButton::cancel("Cancel"),
                ],
                |this, answer, window, cx| {
                    match answer {
                        0 => {
                            if let Err(error) = this.save_scene(cx) {
                                this.project_notice(Err(error), cx);
                                return;
                            }
                        }
                        1 => {}
                        _ => return,
                    };
                    let result = commit(this, window, cx);
                    this.project_notice(result, cx);
                },
                window,
                cx,
            );
        } else {
            let result = commit(self, window, cx);
            self.project_notice(result, cx);
        }
    }

    pub(super) fn scene_dirty(&self, cx: &App) -> bool {
        self.active_session()
            .is_some_and(|session| session.dirty(self.bee.read(cx)))
    }

    fn shutdown(&mut self, window: &mut Window, cx: &mut Context<Self>) -> bool {
        self.closing = true;
        self.bee
            .update(cx, |bee, _| bee.viewport.request_close(window))
    }

    pub(super) fn request_exit(&mut self, window: &mut Window, cx: &mut Context<Self>) -> bool {
        if self.closing {
            return self.shutdown(window, cx);
        }
        if self.project_busy() {
            return false;
        }
        if !self.scene_dirty(cx) {
            return self.shutdown(window, cx);
        }
        self.leave_scene(
            |this, window, cx| {
                if this.shutdown(window, cx) {
                    window.remove_window();
                }
                Ok(None)
            },
            window,
            cx,
        );
        false
    }

    pub(super) fn close_project(&mut self, window: &mut Window, cx: &mut Context<Self>) {
        self.leave_scene(
            |this, window, cx| {
                this.projects.as_mut().unwrap().session = None;
                this.clear_selection(cx);
                this.bee.update(cx, |bee, cx| {
                    bee.viewport.set_active(false, &bee.state, window, cx);
                    let empty = bee
                        .viewport
                        .prepare_scene(&SceneDocument::blank())
                        .expect("blank template");
                    bee.load_scene(empty, window, cx);
                });
                window.blur();
                Ok(None)
            },
            window,
            cx,
        );
    }

    fn clear_selection(&self, cx: &mut Context<Self>) {
        self.selection.update(cx, |selection, cx| {
            selection.clear();
            cx.notify();
        });
    }

    pub(super) fn choose_project(&mut self, window: &mut Window, cx: &mut Context<Self>) {
        self.choose_folder(
            "Open Bee Project",
            |this, path, window, cx| this.open_project(path, None, None, window, cx),
            window,
            cx,
        );
    }

    fn open_project(
        &mut self,
        directory: PathBuf,
        expected: Option<Uuid>,
        scene: Option<Uuid>,
        window: &mut Window,
        cx: &mut Context<Self>,
    ) {
        match OpenProject::read(directory, expected) {
            Ok(opened) => {
                let id = scene.unwrap_or(opened.project.manifest().startup_scene);
                self.open_scene(Some(opened), id, window, cx);
            }
            Err(error) => self.project_notice(Err(error), cx),
        }
    }

    fn target_project<'a>(&'a self, opened: &'a Option<OpenProject>) -> &'a SceneProject {
        if let Some(opened) = opened {
            &opened.project
        } else {
            &self.active_session().unwrap().project
        }
    }

    fn prepare_scene(
        &self,
        project: &SceneProject,
        id: Uuid,
        cx: &App,
    ) -> Result<(SavedScene, PreparedScene), String> {
        let saved = project.read_scene(id).map_err(|error| error.to_string())?;
        let prepared = self
            .bee
            .read(cx)
            .viewport
            .prepare_scene(&saved.content)
            .map_err(|error| {
                project
                    .scene_path(id)
                    .map_or(error.clone(), |path| format!("{}: {error}", path.display()))
            })?;
        Ok((saved, prepared))
    }

    fn open_scene(
        &mut self,
        opened: Option<OpenProject>,
        id: Uuid,
        window: &mut Window,
        cx: &mut Context<Self>,
    ) {
        let project = self.target_project(&opened);
        match self.prepare_scene(project, id, cx) {
            Ok((saved, prepared)) => {
                // 同一场景已经打开时保留内存编辑，避免换回提示前读到的旧版本。
                if self.active_session().is_some_and(|session| {
                    session.saved.id() == id && session.project.root() == project.root()
                }) {
                    let result = opened.map_or(Ok(()), |opened| {
                        self.projects.as_mut().unwrap().remember(opened.recent())
                    });
                    self.project_notice(result.map(|()| None), cx);
                    return;
                }
                self.leave_scene(
                    |this, window, cx| {
                        this.activate_scene(opened, saved, prepared, window, cx)?;
                        Ok(None)
                    },
                    window,
                    cx,
                );
            }
            Err(error) => {
                let scenes = project.manifest().scenes.clone();
                let has_backup = project.read_backup(id).is_ok();
                let recent = opened.as_ref().map(OpenProject::recent);
                self.project_notice(Err(error.clone()), cx);
                self.show_recovery(recent, id, &scenes, has_backup, error, window, cx);
            }
        }
    }

    fn restore_scene(
        &mut self,
        recent: Option<RecentProject>,
        id: Uuid,
        window: &mut Window,
        cx: &mut Context<Self>,
    ) {
        let result = (|| {
            let opened = recent
                .map(|recent| OpenProject::read(recent.directory, Some(recent.id)))
                .transpose()?;
            let backup = self
                .target_project(&opened)
                .read_backup(id)
                .map_err(|error| error.to_string())?;
            let prepared = self.bee.read(cx).viewport.prepare_scene(backup.content())?;
            Ok((opened, backup, prepared))
        })();
        match result {
            Ok((opened, backup, prepared)) => self.confirm_project_action(
                "Restore the previous saved scene?",
                Some("The backup will replace the damaged or missing scene file."),
                &[
                    PromptButton::new("Restore Backup"),
                    PromptButton::cancel("Cancel"),
                ],
                |this, answer, window, cx| {
                    if answer != 0 {
                        return;
                    }
                    this.leave_scene(
                        |this, window, cx| {
                            let saved = this
                                .target_project(&opened)
                                .restore_backup(backup)
                                .map_err(|error| error.to_string())?;
                            this.activate_scene(opened, saved, prepared, window, cx)?;
                            Ok(Some("Scene restored from backup".into()))
                        },
                        window,
                        cx,
                    );
                },
                window,
                cx,
            ),
            Err(error) => self.project_notice(Err(error), cx),
        }
    }

    fn activate_scene(
        &mut self,
        opened: Option<OpenProject>,
        saved: SavedScene,
        prepared: PreparedScene,
        window: &mut Window,
        cx: &mut Context<Self>,
    ) -> Result<(), String> {
        let was_chooser = self.active_session().is_none();
        let start_camera = saved.content.start_camera;
        let scene_version = prepared.state.versions().scene;
        let recent = opened.as_ref().map(OpenProject::recent);
        if let Some(opened) = opened {
            self.projects.as_mut().unwrap().session = Some(Session {
                project: opened.project,
                saved,
                start_camera,
                scene_version,
            });
        } else {
            let session = self.edit_session();
            session.saved = saved;
            session.start_camera = start_camera;
            session.scene_version = scene_version;
        }
        self.clear_selection(cx);
        let viewport_is_visible = was_chooser && self.viewport_panel_visible(cx);
        self.bee.update(cx, |bee, cx| {
            bee.load_scene(prepared, window, cx);
            if viewport_is_visible {
                bee.viewport.set_active(true, &bee.state, window, cx);
            }
        });
        if was_chooser {
            window.focus(self.menu_focus(super::AppMenu::File));
        }
        recent.map_or(Ok(()), |recent| {
            self.projects.as_mut().unwrap().remember(recent)
        })
    }

    fn new_project(
        &mut self,
        directory: PathBuf,
        use_empty_directory: bool,
        name: String,
        document: SceneDocument,
        window: &mut Window,
        cx: &mut Context<Self>,
    ) {
        let prepared = match self.bee.read(cx).viewport.prepare_scene(&document) {
            Ok(prepared) => prepared,
            Err(error) => return self.project_notice(Err(error), cx),
        };
        self.leave_scene(
            move |this, window, cx| {
                let location = if use_empty_directory {
                    ProjectLocation::ExistingEmptyDirectory(&directory)
                } else {
                    ProjectLocation::NewDirectoryIn(&directory)
                };
                let project = SceneProject::create(location, &name, "Scene", &document)
                    .map_err(|error| error.to_string())?;
                let saved = project
                    .read_scene(project.manifest().startup_scene)
                    .map_err(|error| error.to_string())?;
                let directory = if use_empty_directory {
                    directory
                } else {
                    directory.join(name)
                };
                this.activate_scene(
                    Some(OpenProject { project, directory }),
                    saved,
                    prepared,
                    window,
                    cx,
                )?;
                Ok(None)
            },
            window,
            cx,
        );
    }

    fn new_scene(
        &mut self,
        name: String,
        document: SceneDocument,
        window: &mut Window,
        cx: &mut Context<Self>,
    ) {
        let prepared = match self.bee.read(cx).viewport.prepare_scene(&document) {
            Ok(prepared) => prepared,
            Err(error) => return self.project_notice(Err(error), cx),
        };
        self.leave_scene(
            move |this, window, cx| {
                let project = &mut this.edit_session().project;
                let id = project
                    .new_scene(&name, &document)
                    .map_err(|error| error.to_string())?;
                let saved = project.read_scene(id).map_err(|error| error.to_string())?;
                this.activate_scene(None, saved, prepared, window, cx)?;
                Ok(None)
            },
            window,
            cx,
        );
    }

    fn delete_current_scene(
        &mut self,
        replacement: Uuid,
        window: &mut Window,
        cx: &mut Context<Self>,
    ) {
        let session = self.active_session().unwrap();
        let id = session.saved.id();
        let startup = (session.project.manifest().startup_scene == id).then_some(replacement);
        let (saved, prepared) = match self.prepare_scene(&session.project, replacement, cx) {
            Ok(scene) => scene,
            Err(error) => return self.project_notice(Err(error), cx),
        };
        self.leave_scene(
            move |this, window, cx| {
                let result = this
                    .edit_session()
                    .project
                    .delete_scene(id, startup)
                    .map_err(|error| error.to_string())?;
                this.activate_scene(None, saved, prepared, window, cx)?;
                Ok(Some(match result {
                    DeletedScene::Recycled(path) => {
                        format!("Deleted scene moved to {}", path.display())
                    }
                    DeletedScene::Retained { path, error } => {
                        format!(
                            "Scene file kept at {}. Could not move it: {error}",
                            path.display()
                        )
                    }
                }))
            },
            window,
            cx,
        );
    }

    fn remove_recent(&mut self, id: Uuid, cx: &mut Context<Self>) {
        let projects = self.projects.as_mut().unwrap();
        projects.recent.retain(|entry| entry.id != id);
        let result = projects.write_recent();
        self.project_notice(result.map(|()| None), cx);
    }
}
