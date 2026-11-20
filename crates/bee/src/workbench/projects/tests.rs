use super::*;
use crate::workbench::{init_theme, AppMenu};
use gpui::{
    point, px, size, AppContext, Entity, Focusable, KeyBinding, Modifiers, TestAppContext,
    VisualTestContext,
};
use gpui_component::Root;
use std::path::Path;

struct Editor<'a> {
    root: Entity<Root>,
    view: Entity<Workbench>,
    cx: &'a mut VisualTestContext,
}

impl<'a> Editor<'a> {
    fn new(app: &'a mut TestAppContext, settings: Option<PathBuf>) -> Self {
        app.update(|cx| {
            gpui_component::init(cx);
            init_theme(cx);
            cx.bind_keys([
                KeyBinding::new("tab", crate::FocusNext, None),
                KeyBinding::new("shift-tab", crate::FocusPrevious, None),
                KeyBinding::new("ctrl-s", super::super::super::SaveScene, None),
            ]);
        });
        let mut view = None;
        let (root, cx) = app.add_window_view(|window, cx| {
            let bee = cx.new(|_| Bee::failure("test device".into()));
            let workbench = cx.new(|cx| {
                Workbench::with_projects(bee, Some(Projects::with_settings(settings)), window, cx)
            });
            view = Some(workbench.clone());
            Root::new(workbench, window, cx)
        });
        cx.simulate_resize(size(px(1200.0), px(800.0)));
        let mut editor = Self {
            root,
            view: view.unwrap(),
            cx,
        };
        editor.draw();
        editor
    }

    fn draw(&mut self) {
        let dimensions = self.cx.update(|window, _| window.viewport_size());
        self.cx.draw(point(px(0.0), px(0.0)), dimensions, |_, _| {
            self.root.clone()
        });
        self.cx.run_until_parked();
    }

    fn click(&mut self, selector: &'static str) {
        let bounds = self
            .cx
            .debug_bounds(selector)
            .unwrap_or_else(|| panic!("{selector}"));
        self.cx.simulate_click(bounds.center(), Modifiers::none());
        self.draw();
    }

    fn menu(&mut self, trigger: &'static str, item: &'static str) {
        self.click(trigger);
        self.click(item);
    }

    fn keys(&mut self, keys: &str) {
        self.cx.simulate_keystrokes(keys);
        self.draw();
    }

    fn folder(&mut self, path: Option<&Path>) {
        self.cx
            .cx
            .simulate_new_path_selection(|_| path.map(Path::to_owned));
        self.draw();
    }

    fn open(&mut self, path: &Path) {
        if self.current().is_some() {
            self.menu("file-menu", "file-open-project");
        } else {
            self.click("open-project");
        }
        self.folder(Some(path));
    }

    fn respond(&mut self, answer: &str) {
        self.cx.cx.simulate_prompt_answer(answer);
        self.draw();
    }

    fn current(&mut self) -> Option<Uuid> {
        self.cx.update(|_, cx| {
            self.view
                .read(cx)
                .active_session()
                .map(|session| session.saved.id())
        })
    }

    fn dirty(&mut self) -> bool {
        self.cx.update(|_, cx| self.view.read(cx).scene_dirty(cx))
    }

    fn notice(&mut self) -> String {
        self.cx.update(|_, cx| {
            self.view
                .read(cx)
                .projects
                .as_ref()
                .unwrap()
                .notice
                .as_ref()
                .unwrap()
                .0
                .clone()
        })
    }

    fn form_name(&mut self, name: &str) {
        self.click("scene-name-input");
        self.keys("ctrl-a");
        self.cx.simulate_input(name);
        self.draw();
    }

    fn edit_draft(&mut self) {
        let bee = self.cx.update(|_, cx| self.view.read(cx).bee.clone());
        bee.update(self.cx, |bee, cx| {
            bee.viewport
                .flow
                .draft_mut()
                .add(bee_renderer::Module::Rasterization);
            cx.notify();
        });
        self.draw();
    }
}

fn project(path: &Path, name: &str) -> SceneProject {
    SceneProject::create(
        ProjectLocation::ExistingEmptyDirectory(path),
        name,
        "Example",
        &SceneDocument::builtin(),
    )
    .unwrap()
}

#[test]
fn chooser_creates_reopens_locates_and_removes_projects() {
    let location = tempfile::tempdir().unwrap();
    let settings = location
        .path()
        .join("settings")
        .join("recent-projects.json");
    let empty = tempfile::tempdir().unwrap();
    let mut app = TestAppContext::single();
    let mut editor = Editor::new(&mut app, Some(settings.clone()));
    assert!(editor.cx.debug_bounds("project-chooser").is_some());
    assert!(editor.cx.debug_bounds("scene-canvas").is_none());
    assert_eq!(editor.current(), None);
    editor.click("open-project");
    editor.folder(None);
    assert!(!settings.exists());
    editor.open(empty.path());
    assert_eq!(editor.current(), None);
    assert!(editor.notice().contains("No Bee project"));
    assert!(fs::read_dir(empty.path()).unwrap().next().is_none());

    editor.click("new-project");
    editor.folder(Some(location.path()));
    assert!(!location.path().join("Studio").exists());
    editor.form_name("Studio");
    editor.click("confirm-project-form");
    let root = location.path().join("Studio");
    let opened = SceneProject::open(&root).unwrap();
    let startup = opened.manifest().startup_scene;
    assert_eq!(editor.current(), Some(startup));
    assert!(root.join("Assets").is_dir());
    assert!(!opened
        .read_scene(startup)
        .unwrap()
        .content
        .scene
        .instances
        .is_empty());
    assert_eq!(
        Projects::with_settings(Some(settings.clone())).recent[0].directory,
        root
    );

    editor.menu("file-menu", "file-close-project");
    assert!(editor.cx.debug_bounds("project-chooser").is_some());
    let moved = location.path().join("Moved");
    fs::rename(&root, &moved).unwrap();
    editor.click("open-recent");
    assert!(editor.notice().contains("Studio"));
    assert_eq!(editor.current(), None);
    editor.click("locate-recent");
    editor.folder(Some(&moved));
    assert_eq!(editor.current(), Some(startup));
    assert_eq!(
        Projects::with_settings(Some(settings.clone())).recent[0].directory,
        moved
    );
    editor.menu("file-menu", "file-close-project");
    editor.click("remove-recent");
    assert!(Projects::with_settings(Some(settings)).recent.is_empty());
    assert!(moved.join("project.beeproj").is_file());

    editor.click("new-project");
    editor.folder(Some(empty.path()));
    editor.click("scene-template");
    editor.keys("down enter");
    editor.click("project-placement");
    editor.keys("down enter");
    editor.click("confirm-project-form");
    let in_place = SceneProject::open(empty.path()).unwrap();
    let id = in_place.manifest().startup_scene;
    assert_eq!(editor.current(), Some(id));
    assert!(in_place
        .read_scene(id)
        .unwrap()
        .content
        .scene
        .instances
        .is_empty());
}

#[test]
fn damaged_startup_offers_an_alternative_and_confirmed_backup_recovery() {
    let directory = tempfile::tempdir().unwrap();
    let mut project = project(directory.path(), "Recoverable");
    let startup = project.manifest().startup_scene;
    let alternative = project.new_scene("Blank", &SceneDocument::blank()).unwrap();
    let mut saved = project.read_scene(startup).unwrap();
    let original = saved.content.clone();
    saved.content.scene.environment = [0.25; 3];
    project.save_scene(&mut saved).unwrap();
    let scene_path = project.scene_path(startup).unwrap();
    fs::write(&scene_path, b"damaged").unwrap();

    let mut app = TestAppContext::single();
    let mut editor = Editor::new(&mut app, None);
    editor.open(directory.path());
    assert_eq!(editor.current(), None);
    assert!(editor.notice().contains(".beescene"));
    assert!(editor.cx.debug_bounds("restore-backup").is_some());
    editor.click("confirm-project-form");
    assert_eq!(editor.current(), Some(alternative));
    assert_eq!(fs::read(&scene_path).unwrap(), b"damaged");
    editor.menu("file-menu", "file-close-project");

    editor.open(directory.path());
    editor.click("restore-backup");
    editor.respond("Cancel");
    assert_eq!(editor.current(), None);
    assert_eq!(fs::read(&scene_path).unwrap(), b"damaged");
    editor.open(directory.path());
    editor.click("restore-backup");
    editor.respond("Restore Backup");
    assert_eq!(editor.current(), Some(startup));
    assert_eq!(project.read_scene(startup).unwrap().content, original);
    assert_eq!(project.read_backup(startup).unwrap().content(), &original);
}

#[test]
fn scene_menu_switches_and_manages_scenes_and_saves_explicit_edits() {
    let directory = tempfile::tempdir().unwrap();
    let mut project = project(directory.path(), &"Project with a long name ".repeat(12));
    let first = project.manifest().startup_scene;
    let second = project
        .new_scene("Second", &SceneDocument::builtin())
        .unwrap();
    let mut app = TestAppContext::single();
    let mut editor = Editor::new(&mut app, None);
    editor.open(directory.path());
    editor.cx.simulate_resize(size(px(640.0), px(426.0)));
    editor.draw();
    editor.cx.update(|window, cx| {
        assert_eq!(window.scale_factor(), 2.0);
        window.focus(editor.view.read(cx).menu_focus(AppMenu::File));
    });
    let header =
        ["file-menu", "view-menu", "scene-menu"].map(|id| editor.cx.debug_bounds(id).unwrap());
    assert!(header[0].right() <= header[1].left() && header[1].right() <= header[2].left());
    assert!(header[2].right() <= px(640.0));
    editor.keys("tab tab");
    assert!(editor.cx.update(|window, cx| editor
        .view
        .read(cx)
        .menu_focus(AppMenu::Scenes)
        .is_focused(window)));
    editor.cx.simulate_keystrokes("space");
    editor.cx.update(|window, cx| {
        let view = editor.view.read(cx);
        let (kind, popup) = view.menu.as_ref().expect("scene menu is open");
        assert!(*kind == AppMenu::Scenes);
        assert!(
            popup.read(cx).focus_handle(cx).is_focused(window),
            "popup focus: {:?}, window focus: {:?}, panels: {:?}",
            popup.read(cx).focus_handle(cx),
            window.focused(cx),
            view.panels
                .iter()
                .map(|panel| &panel.read(cx).focus)
                .collect::<Vec<_>>(),
        );
    });
    editor.draw();
    assert!(editor.cx.debug_bounds("open-scene-1").is_some());
    editor.keys("escape");
    assert_eq!(
        editor.cx.update(|window, cx| {
            let view = editor.view.read(cx);
            (
                view.menu.is_none(),
                view.menu_focus(AppMenu::Scenes).is_focused(window),
            )
        }),
        (true, true)
    );
    editor.click("scene-camera");
    editor
        .cx
        .update(|window, cx| window.focus(editor.view.read(cx).menu_focus(AppMenu::Scenes)));
    editor.keys("enter");
    editor.keys("down down enter");
    assert_eq!(editor.current(), Some(second));
    assert!(editor.cx.update(|_, cx| {
        let view = editor.view.read(cx);
        view.selection
            .read(cx)
            .current(&view.bee.read(cx).state)
            .is_none()
    }));
    editor.cx.simulate_resize(size(px(1200.0), px(800.0)));
    editor.draw();

    let bee = editor.cx.update(|_, cx| editor.view.read(cx).bee.clone());
    let camera = bee.update(editor.cx, |bee, cx| {
        let mut camera = bee.state.camera();
        assert!(camera.orbit(30.0, 10.0));
        assert!(bee.state.set_camera(camera));
        cx.notify();
        camera
    });
    editor.click("scene-camera");
    assert!(!editor.dirty());
    editor.menu("scene-menu", "capture-start-camera");
    assert!(editor.cx.debug_bounds("scene-dirty").is_some());
    editor.keys("ctrl-s");
    assert!(!editor.dirty());
    assert_eq!(
        project.read_scene(second).unwrap().content.start_camera,
        camera
    );

    editor.edit_draft();
    assert!(editor.dirty());
    editor.keys("ctrl-s");
    assert!(!editor.dirty());
    bee.update_in(editor.cx, |bee, window, cx| {
        bee.viewport.apply_flow(&mut bee.state, window, cx);
    });
    assert!(editor.dirty());
    editor.keys("ctrl-s");
    let baseline = project.read_scene(second).unwrap().content;
    assert_eq!(baseline.applied_flow.nodes().count(), 5);
    assert!(!editor.dirty());
    editor.edit_draft();
    editor.menu("scene-menu", "copy-scene");
    editor.click("confirm-project-form");
    let updated = SceneProject::open(directory.path()).unwrap();
    let copied = updated.manifest().scenes.last().unwrap().id;
    assert_ne!(copied, second);
    assert_eq!(updated.read_scene(copied).unwrap().content, baseline);
    assert!(editor.dirty());
    editor.menu("scene-menu", "rename-scene");
    editor.form_name("Renamed");
    editor.click("confirm-project-form");
    assert_eq!(editor.current(), Some(second));
    editor.menu("scene-menu", "set-startup-scene");
    let updated = SceneProject::open(directory.path()).unwrap();
    assert_eq!(updated.manifest().startup_scene, second);
    assert_eq!(updated.manifest().scenes[1].name, "Renamed");

    editor.menu("scene-menu", "new-scene");
    editor.click("scene-template");
    editor.keys("down enter");
    editor.click("confirm-project-form");
    editor.respond("Discard");
    let blank = editor.current().unwrap();
    let updated = SceneProject::open(directory.path()).unwrap();
    assert!(updated
        .read_scene(blank)
        .unwrap()
        .content
        .scene
        .instances
        .is_empty());
    editor.menu("scene-menu", "open-scene-1");
    assert_eq!(editor.current(), Some(second));
    editor.cx.update(|_, cx| {
        let bee = bee.read(cx);
        assert_eq!(bee.state.camera(), camera);
        assert_eq!(bee.viewport.flow.active(), &baseline.applied_flow);
    });

    editor.menu("scene-menu", "delete-scene");
    editor.click("confirm-project-form");
    assert_eq!(editor.current(), Some(first));
    let updated = SceneProject::open(directory.path()).unwrap();
    assert_eq!(updated.manifest().startup_scene, first);
    assert!(updated
        .manifest()
        .scenes
        .iter()
        .all(|entry| entry.id != second));
    assert_eq!(
        editor.notice(),
        format!(
            "Deleted scene moved to {}",
            updated
                .root()
                .join("Trash")
                .join(format!("{second}.beescene"))
                .display()
        )
    );
    assert!(directory
        .path()
        .join("Trash")
        .join(format!("{second}.beescene"))
        .is_file());
}

#[test]
fn leave_confirmation_preserves_the_session_on_cancel_or_failure() {
    let directory = tempfile::tempdir().unwrap();
    let other = tempfile::tempdir().unwrap();
    let mut source = project(directory.path(), "Source");
    let first = source.manifest().startup_scene;
    let second = source.new_scene("Second", &SceneDocument::blank()).unwrap();
    let destination = project(other.path(), "Destination");
    let mut app = TestAppContext::single();
    let mut editor = Editor::new(&mut app, None);
    editor.open(directory.path());
    editor.edit_draft();

    editor.menu("scene-menu", "open-scene-1");
    editor.respond("Cancel");
    editor.open(other.path());
    editor.respond("Cancel");
    editor.menu("scene-menu", "new-scene");
    editor.click("confirm-project-form");
    editor.respond("Cancel");
    editor.menu("scene-menu", "delete-scene");
    editor.click("confirm-project-form");
    editor.respond("Cancel");
    editor.menu("file-menu", "file-close-project");
    editor.respond("Cancel");
    editor.menu("file-menu", "exit-bee");
    editor.respond("Cancel");
    assert!(!editor.cx.simulate_close());
    editor.respond("Cancel");
    assert_eq!(editor.current(), Some(first));
    assert!(editor.dirty());
    assert_eq!(
        SceneProject::open(directory.path())
            .unwrap()
            .manifest()
            .scenes
            .len(),
        2
    );

    let second_path = source.scene_path(second).unwrap();
    let valid = fs::read(&second_path).unwrap();
    fs::write(&second_path, b"broken").unwrap();
    editor.menu("scene-menu", "open-scene-1");
    assert!(editor.cx.debug_bounds("recovery-error").is_some());
    assert!(!editor.cx.cx.has_pending_prompt());
    editor.keys("escape");
    editor.menu("scene-menu", "delete-scene");
    editor.click("confirm-project-form");
    assert!(!editor.cx.cx.has_pending_prompt());
    assert_eq!(editor.current(), Some(first));
    assert!(editor.dirty());
    fs::write(&second_path, valid).unwrap();

    let first_path = source.scene_path(first).unwrap();
    let saved = fs::read(&first_path).unwrap();
    fs::write(&first_path, b"external change").unwrap();
    editor.open(other.path());
    editor.respond("Save");
    assert_eq!(editor.current(), Some(first));
    assert!(editor.dirty());
    assert!(editor.notice().contains("contents changed on disk"));
    assert_eq!(fs::read(&first_path).unwrap(), b"external change");
    fs::write(&first_path, saved).unwrap();

    editor.open(other.path());
    editor.respond("Save");
    assert_eq!(editor.current(), Some(destination.manifest().startup_scene));
    assert!(!editor.dirty());
    let saved = source.read_scene(first).unwrap().content;
    assert_eq!(saved.draft_flow.nodes().count(), 5);
    assert_eq!(saved.applied_flow.nodes().count(), 4);
}

#[test]
fn confirmed_exit_from_the_menu_and_window_honors_the_save_decision() {
    for from_menu in [false, true] {
        let directory = tempfile::tempdir().unwrap();
        let project = project(directory.path(), "Exit");
        let startup = project.manifest().startup_scene;
        let mut app = TestAppContext::single();
        let mut editor = Editor::new(&mut app, None);
        editor.open(directory.path());
        editor.edit_draft();
        if from_menu {
            editor.menu("file-menu", "exit-bee");
        } else {
            assert!(!editor.cx.simulate_close());
        }
        editor
            .cx
            .cx
            .simulate_prompt_answer(if from_menu { "Save" } else { "Discard" });
        editor.cx.run_until_parked();
        assert!(editor.cx.cx.update(|cx| cx.windows().is_empty()));
        let saved = project.read_scene(startup).unwrap().content;
        assert_eq!(
            saved.draft_flow.nodes().count(),
            if from_menu { 5 } else { 4 }
        );
    }
}
