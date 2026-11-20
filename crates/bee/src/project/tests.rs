use super::*;
use tempfile::{tempdir, TempDir};

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
struct Content {
    revision: u32,
}

impl SceneContent for Content {
    fn validate(&self) -> std::result::Result<(), String> {
        if self.revision == 0 {
            return Err("invalid content".into());
        }
        Ok(())
    }
}

fn content(revision: u32) -> Content {
    Content { revision }
}

fn project() -> (TempDir, Project<Content>, Uuid) {
    let dir = tempdir().unwrap();
    let project = Project::create(
        ProjectLocation::NewDirectoryIn(dir.path()),
        "Project",
        "First",
        &content(1),
    )
    .unwrap();
    let id = project.manifest().startup_scene;
    (dir, project, id)
}

fn save(project: &Project<Content>, id: Uuid, revision: u32) {
    let mut scene = project.read_scene(id).unwrap();
    scene.content = content(revision);
    project.save_scene(&mut scene).unwrap();
}

#[test]
fn manifest_checks_version_json_identity_and_startup() {
    let (_dir, project, id) = project();
    let path = project.root.join(MANIFEST);
    let initial = fs::read(&path).unwrap();
    for (bytes, reason) in [
        (
            br#"{"format_version":2}"#.to_vec(),
            "unsupported format version 2",
        ),
        (br#"{"format_version":"#.to_vec(), "EOF"),
        (
            {
                let mut manifest = project.manifest.clone();
                manifest.scenes.push(manifest.scenes[0].clone());
                encode(&path, &manifest).unwrap()
            },
            "duplicate scene id",
        ),
        (
            {
                let mut manifest = project.manifest.clone();
                manifest.startup_scene = Uuid::new_v4();
                encode(&path, &manifest).unwrap()
            },
            "startup scene is not listed",
        ),
    ] {
        fs::write(&path, &bytes).unwrap();
        let error = Project::<Content>::open(&project.root).unwrap_err();
        assert!(error.to_string().contains(reason), "{error}");
        assert_eq!(fs::read(&path).unwrap(), bytes);
    }
    fs::write(&path, initial).unwrap();

    let scene = project.scene_path(id).unwrap();
    assert_eq!(scene.file_name().unwrap(), scene_name(id).as_str());
    fs::write(&scene, b"damaged").unwrap();
    let opened = Project::<Content>::open(&project.root).unwrap();
    assert!(matches!(opened.read_scene(id), Err(Error::Invalid { .. })));
    assert!(matches!(
        opened.scene_path(Uuid::new_v4()),
        Err(Error::Invalid { .. })
    ));
}

#[test]
fn opening_a_directory_without_a_manifest_is_read_only() {
    let dir = tempdir().unwrap();
    assert!(matches!(
        Project::<Content>::open(dir.path()),
        Err(Error::Io { source, .. }) if source.kind() == io::ErrorKind::NotFound
    ));
    assert_eq!(fs::read_dir(dir.path()).unwrap().count(), 0);
}

#[cfg(windows)]
#[test]
fn outside_windows_junction_is_rejected_on_open_and_reuse() {
    use std::os::windows::process::CommandExt;
    use std::process::Command;

    const CREATE_NO_WINDOW: u32 = 0x0800_0000;
    let (dir, project, id) = project();
    let outside = tempdir().unwrap();
    fs::write(outside.path().join(scene_name(id)), b"untouched").unwrap();
    let root = dir.path().join("Project");
    for name in [SCENES, ASSETS] {
        let original = root.join(format!("{name}-original"));
        let link = root.join(name);
        fs::rename(&link, &original).unwrap();
        let output = Command::new("cmd.exe")
            .args(["/d", "/c", "mklink", "/J"])
            .arg(&link)
            .arg(outside.path())
            .creation_flags(CREATE_NO_WINDOW)
            .output()
            .unwrap();
        assert!(output.status.success(), "{output:?}");
        assert!(matches!(
            Project::<Content>::open(&root),
            Err(Error::Invalid { .. })
        ));
        let error = if name == SCENES {
            project.read_scene(id).unwrap_err()
        } else {
            project.assets_dir().unwrap_err()
        };
        assert!(matches!(error, Error::Invalid { .. }));
        fs::remove_dir(&link).unwrap();
        fs::rename(&original, &link).unwrap();
    }
    assert_eq!(
        fs::read(outside.path().join(scene_name(id))).unwrap(),
        b"untouched"
    );
    assert_eq!(project.read_scene(id).unwrap().content, content(1));
}

#[test]
fn saves_advance_the_expected_bytes_and_keep_only_the_previous_revision() {
    let (_dir, project, id) = project();
    let mut scene = project.read_scene(id).unwrap();
    for revision in [2, 3] {
        scene.content = content(revision);
        project.save_scene(&mut scene).unwrap();
        assert_eq!(project.read_scene(id).unwrap().content, content(revision));
        assert_eq!(
            project.read_backup(id).unwrap().content(),
            &content(revision - 1)
        );
    }
    scene.content = content(0);
    assert!(matches!(
        project.save_scene(&mut scene),
        Err(Error::Invalid { .. })
    ));
    assert_eq!(project.read_scene(id).unwrap().content, content(3));
    assert_eq!(project.read_backup(id).unwrap().content(), &content(2));
}

#[test]
fn scene_and_manifest_external_changes_leave_files_and_unsaved_content_intact() {
    let (_dir, mut project, id) = project();
    save(&project, id, 2);
    let path = project.scene_path(id).unwrap();
    let backup = backup_path(&path);
    let saved_backup = fs::read(&backup).unwrap();
    let mut pending = project.read_scene(id).unwrap();
    pending.content = content(9);
    let external = encode_scene(&path, id, &content(7)).unwrap();
    fs::write(&path, &external).unwrap();
    assert!(
        matches!(project.save_scene(&mut pending), Err(Error::Changed(changed)) if changed == path)
    );
    assert_eq!(pending.content, content(9));
    assert_eq!(fs::read(&path).unwrap(), external);
    assert_eq!(fs::read(&backup).unwrap(), saved_backup);

    let manifest = project.root.join(MANIFEST);
    let mut external_manifest = fs::read(&manifest).unwrap();
    external_manifest.push(b'\n');
    fs::write(&manifest, &external_manifest).unwrap();
    assert!(
        matches!(project.rename_scene(id, "Local name"), Err(Error::Changed(changed)) if changed == manifest)
    );
    assert_eq!(fs::read(&manifest).unwrap(), external_manifest);
    assert_eq!(fs::read(&path).unwrap(), external);
    assert_eq!(project.manifest().scenes[0].name, "First");
}

#[test]
fn restores_missing_and_damaged_scenes_and_keeps_the_backup() {
    let (_dir, project, id) = project();
    save(&project, id, 2);
    let path = project.scene_path(id).unwrap();
    let backup = backup_path(&path);
    let expected = fs::read(&backup).unwrap();
    fs::remove_file(&path).unwrap();

    for damage in [None, Some(&b"{ broken"[..])] {
        if let Some(damage) = damage {
            fs::write(&path, damage).unwrap();
        }
        let reopened = Project::<Content>::open(&project.root).unwrap();
        assert!(reopened.read_scene(id).is_err());
        let offered = reopened.read_backup(id).unwrap();
        let restored = reopened.restore_backup(offered).unwrap();
        assert_eq!(restored.id(), id);
        assert_eq!(restored.content, content(1));
        assert_eq!(fs::read(&path).unwrap(), expected);
        assert_eq!(fs::read(&backup).unwrap(), expected);
    }
}

#[test]
fn backup_identity_version_and_content_are_validated() {
    let (_dir, project, id) = project();
    let path = project.scene_path(id).unwrap();
    let backup = backup_path(&path);
    fs::remove_file(&path).unwrap();
    for (bytes, reason) in [
        (
            encode_scene(&backup, Uuid::new_v4(), &content(1)).unwrap(),
            "scene id",
        ),
        (
            encode(
                &backup,
                &SceneFile {
                    format_version: FORMAT_VERSION,
                    scene_id: id,
                    content: content(0),
                },
            )
            .unwrap(),
            "invalid content",
        ),
        (
            br#"{"format_version":2}"#.to_vec(),
            "unsupported format version 2",
        ),
        (b"{ torn backup".to_vec(), "key must be a string"),
    ] {
        fs::write(&backup, &bytes).unwrap();
        let error = project.read_backup(id).unwrap_err();
        assert!(error.to_string().contains(reason), "{error}");
        assert_eq!(fs::read(&backup).unwrap(), bytes);
        assert!(!path.exists());
    }
}

#[test]
fn recovery_rejects_external_changes_and_protects_newer_format() {
    let (_dir, project, id) = project();
    save(&project, id, 2);
    let path = project.scene_path(id).unwrap();
    let backup_path = backup_path(&path);
    fs::remove_file(&path).unwrap();
    let offered = project.read_backup(id).unwrap();
    fs::write(&path, b"external file").unwrap();
    assert!(
        matches!(project.restore_backup(offered), Err(Error::Changed(changed)) if changed == path)
    );
    assert_eq!(fs::read(&path).unwrap(), b"external file");

    let offered = project.read_backup(id).unwrap();
    let external_backup = encode_scene(&backup_path, id, &content(5)).unwrap();
    fs::write(&backup_path, &external_backup).unwrap();
    assert!(
        matches!(project.restore_backup(offered), Err(Error::Changed(changed)) if changed == backup_path)
    );
    let newer = br#"{"format_version":2}"#;
    fs::write(&path, newer).unwrap();
    assert!(matches!(
        project.restore_backup(project.read_backup(id).unwrap()),
        Err(Error::UnsupportedVersion { version: 2, .. })
    ));
    assert_eq!(fs::read(&backup_path).unwrap(), external_backup);
    assert_eq!(fs::read(&path).unwrap(), newer);
}

#[test]
fn creation_rules_and_no_clobber_preserve_existing_files() {
    let dir = tempdir().unwrap();
    let root = dir.path().join("Project");
    fs::create_dir(&root).unwrap();
    let create = |location, name| Project::create(location, name, "First", &content(1));
    assert!(create(ProjectLocation::NewDirectoryIn(dir.path()), "Project").is_err());
    assert!(create(ProjectLocation::NewDirectoryIn(dir.path()), "../outside").is_err());
    assert!(create(ProjectLocation::NewDirectoryIn(dir.path()), "NUL.txt").is_err());
    fs::write(root.join("keep"), b"existing data").unwrap();
    assert!(create(ProjectLocation::ExistingEmptyDirectory(&root), "Project").is_err());
    assert_eq!(fs::read(root.join("keep")).unwrap(), b"existing data");
    assert_eq!(fs::read_dir(&root).unwrap().count(), 1);

    fs::remove_file(root.join("keep")).unwrap();
    let project = create(ProjectLocation::ExistingEmptyDirectory(&root), "Project").unwrap();
    assert!(project.assets_dir().unwrap().is_dir());
    let path = project
        .scene_path(project.manifest().startup_scene)
        .unwrap();
    let original = fs::read(&path).unwrap();
    assert!(files::commit(stage(&path, b"collision").unwrap(), &path, false).is_err());
    assert_eq!(fs::read(&path).unwrap(), original);
}

#[test]
fn scene_operations_keep_stable_ids_and_only_copy_saved_content() {
    let (_dir, mut project, startup) = project();
    let source = project.scene_path(startup).unwrap();
    let mut unsaved = project.read_scene(startup).unwrap();
    unsaved.content = content(9);
    let copy = project.copy_scene(startup, "Copy").unwrap();
    assert_ne!(startup, copy);
    assert_eq!(project.read_scene(copy).unwrap().content, content(1));
    let added = project.new_scene("Another", &content(3)).unwrap();
    assert_eq!(project.read_scene(added).unwrap().content, content(3));

    project.rename_scene(startup, "Renamed").unwrap();
    assert_eq!(project.scene_path(startup).unwrap(), source);
    let reopened = Project::<Content>::open(&project.root).unwrap();
    assert_eq!(reopened.manifest().scenes[0].name, "Renamed");
    assert_eq!(reopened.manifest().scenes[0].id, startup);
    assert!(project.delete_scene(startup, None).is_err());
    let added_path = project.scene_path(added).unwrap();
    fs::write(&added_path, b"bad replacement").unwrap();
    assert!(project.delete_scene(startup, Some(added)).is_err());
    assert!(project.set_startup_scene(added).is_err());
    fs::write(
        &added_path,
        encode_scene(&added_path, added, &content(3)).unwrap(),
    )
    .unwrap();
    let expected = fs::read(&source).unwrap();

    let DeletedScene::Recycled(recycled) = project.delete_scene(startup, Some(copy)).unwrap()
    else {
        panic!("scene was not recycled");
    };
    assert_eq!(recycled, project.root.join(TRASH).join(scene_name(startup)));
    assert_eq!(fs::read(recycled).unwrap(), expected);
    assert!(!source.exists());
    assert_eq!(project.manifest().startup_scene, copy);
    assert!(project.scene_path(startup).is_err());

    project.set_startup_scene(added).unwrap();
    project.delete_scene(copy, None).unwrap();
    assert!(project.delete_scene(added, None).is_err());
    assert_eq!(
        fs::read(&added_path).unwrap(),
        encode_scene(&added_path, added, &content(3)).unwrap()
    );
    let reopened = Project::<Content>::open(&project.root).unwrap();
    assert_eq!(reopened.manifest().startup_scene, added);
    assert_eq!(reopened.manifest().scenes.len(), 1);
}

#[test]
fn recycle_failure_reports_retained_path_after_manifest_commit() {
    let (_dir, mut project, startup) = project();
    let other = project.new_scene("Second", &content(2)).unwrap();
    let path = project.scene_path(other).unwrap();
    let original = fs::read(&path).unwrap();
    let trash = project.root.join(TRASH);
    fs::create_dir(&trash).unwrap();
    let collision = trash.join(scene_name(other));
    fs::write(&collision, b"previously recycled").unwrap();
    match project.delete_scene(other, None).unwrap() {
        DeletedScene::Retained { path: retained, .. } => assert_eq!(retained, path),
        other => panic!("unexpected deletion outcome: {other:?}"),
    }
    assert_eq!(fs::read(path).unwrap(), original);
    assert_eq!(fs::read(collision).unwrap(), b"previously recycled");
    let reopened = Project::<Content>::open(&project.root).unwrap();
    assert_eq!(reopened.manifest().scenes.len(), 1);
    assert_eq!(reopened.manifest().startup_scene, startup);
}

#[cfg(windows)]
fn block_replacement(path: &Path) -> File {
    use std::os::windows::fs::OpenOptionsExt;
    use windows_sys::Win32::Storage::FileSystem::{FILE_SHARE_READ, FILE_SHARE_WRITE};

    File::options()
        .read(true)
        .share_mode(FILE_SHARE_READ | FILE_SHARE_WRITE)
        .open(path)
        .unwrap()
}

#[cfg(windows)]
#[test]
fn occupied_scene_keeps_formal_and_previous_backup() {
    let (_dir, project, id) = project();
    save(&project, id, 2);
    let path = project.scene_path(id).unwrap();
    let original = fs::read(&path).unwrap();
    let backup = backup_path(&path);
    let saved_backup = fs::read(&backup).unwrap();
    let _blocker = block_replacement(&path);
    let mut pending = project.read_scene(id).unwrap();
    pending.content = content(9);
    assert!(project.save_scene(&mut pending).is_err());
    assert_eq!(pending.content, content(9));
    assert_eq!(fs::read(&path).unwrap(), original);
    assert_eq!(fs::read(backup).unwrap(), saved_backup);
}

#[cfg(windows)]
#[test]
fn failed_recovery_write_keeps_a_readable_backup() {
    let (_dir, project, id) = project();
    save(&project, id, 2);
    let path = project.scene_path(id).unwrap();
    let backup = backup_path(&path);
    let expected = fs::read(&backup).unwrap();
    fs::write(&path, b"damaged scene").unwrap();
    let blocker = block_replacement(&path);
    let offered = project.read_backup(id).unwrap();
    assert!(project.restore_backup(offered).is_err());
    assert_eq!(fs::read(&path).unwrap(), b"damaged scene");
    assert_eq!(fs::read(&backup).unwrap(), expected);
    drop(blocker);
    assert_eq!(project.read_backup(id).unwrap().content(), &content(1));
}

#[cfg(windows)]
#[test]
fn manifest_replace_failure_leaves_existing_manifest_and_scene_files() {
    let (_dir, mut project, startup) = project();
    let added = project.new_scene("Second", &content(2)).unwrap();
    let path = project.root.join(MANIFEST);
    let before = fs::read(&path).unwrap();
    let original = project.scene_path(added).unwrap();
    let saved_scene = fs::read(&original).unwrap();
    let _blocker = block_replacement(&path);
    assert!(project.new_scene("Uncommitted", &content(3)).is_err());
    assert!(project.rename_scene(startup, "Uncommitted").is_err());
    assert!(project.delete_scene(added, None).is_err());
    assert_eq!(fs::read(&path).unwrap(), before);
    assert_eq!(fs::read(&original).unwrap(), saved_scene);
    assert_eq!(fs::read_dir(original.parent().unwrap()).unwrap().count(), 2);
    assert_eq!(project.manifest().scenes.len(), 2);
    let reopened = Project::<Content>::open(&project.root).unwrap();
    assert_eq!(reopened.manifest().scenes.len(), 2);
    assert_eq!(reopened.manifest().startup_scene, startup);
}

#[cfg(windows)]
const CHILD_ROOT: &str = "BEE_STORAGE_TEST_CHILD_ROOT";
#[cfg(windows)]
const CHILD_SIGNAL: &str = "BEE_STORAGE_TEST_CHILD_SIGNAL";
#[cfg(windows)]
const CHILD_PHASE: &str = "BEE_STORAGE_TEST_CHILD_PHASE";
#[cfg(windows)]
const CHILD_EXPECT_CONFLICT: &str = "BEE_STORAGE_TEST_CHILD_EXPECT_CONFLICT";

#[cfg(windows)]
pub(super) fn checkpoint(phase: &str) {
    if std::env::var(CHILD_PHASE).as_deref() != Ok(phase) {
        return;
    }
    let signal = PathBuf::from(std::env::var_os(CHILD_SIGNAL).unwrap());
    fs::File::create(signal.join("paused")).unwrap();
    while !signal.join("continue").exists() {
        std::thread::park_timeout(std::time::Duration::from_millis(10));
    }
}

#[cfg(windows)]
#[test]
fn scene_writer_subprocess() {
    let Some(root) = std::env::var_os(CHILD_ROOT) else {
        return;
    };
    let project = Project::<Content>::open(Path::new(&root)).unwrap();
    let mut scene = project
        .read_scene(project.manifest().startup_scene)
        .unwrap();
    scene.content = content(3);
    let result = project.save_scene(&mut scene);
    if std::env::var_os(CHILD_EXPECT_CONFLICT).is_some() {
        assert!(matches!(result, Err(Error::Changed(_))), "{result:?}");
    } else {
        result.unwrap();
    }
}

#[cfg(windows)]
struct PausedWriter {
    process: std::process::Child,
    signal: TempDir,
}

#[cfg(windows)]
impl PausedWriter {
    fn start(root: &Path, phase: &str, expect_conflict: bool) -> Self {
        use std::os::windows::process::CommandExt;
        use std::process::{Command, Stdio};
        use std::time::{Duration, Instant};

        let signal = tempdir().unwrap();
        let mut command = Command::new(std::env::current_exe().unwrap());
        command
            .args([
                "--exact",
                "project::tests::scene_writer_subprocess",
                "--nocapture",
            ])
            .env(CHILD_ROOT, root)
            .env(CHILD_SIGNAL, signal.path())
            .env(CHILD_PHASE, phase)
            .env_remove(CHILD_EXPECT_CONFLICT)
            .stdin(Stdio::null())
            .stdout(Stdio::null())
            .creation_flags(0x0800_0000); // CREATE_NO_WINDOW
        if expect_conflict {
            command.env(CHILD_EXPECT_CONFLICT, "1");
        }
        let mut writer = Self {
            process: command.spawn().unwrap(),
            signal,
        };
        let deadline = Instant::now() + Duration::from_secs(15);
        while !writer.signal.path().join("paused").exists() {
            assert!(
                writer.process.try_wait().unwrap().is_none(),
                "{phase}: child exited early"
            );
            assert!(
                Instant::now() < deadline,
                "{phase}: child did not reach checkpoint"
            );
            std::thread::sleep(Duration::from_millis(10));
        }
        writer
    }

    fn stop(&mut self) {
        self.process.kill().unwrap();
        assert!(!self.process.wait().unwrap().success());
    }

    fn resume(&mut self) {
        use std::time::{Duration, Instant};

        fs::File::create(self.signal.path().join("continue")).unwrap();
        let deadline = Instant::now() + Duration::from_secs(15);
        loop {
            if let Some(status) = self.process.try_wait().unwrap() {
                assert!(status.success(), "{status}");
                break;
            }
            assert!(Instant::now() < deadline, "child did not finish");
            std::thread::sleep(Duration::from_millis(10));
        }
    }
}

#[cfg(windows)]
impl Drop for PausedWriter {
    fn drop(&mut self) {
        let _ = self.process.kill();
        let _ = self.process.wait();
    }
}

#[cfg(windows)]
#[test]
fn windows_process_interruption_preserves_commits_and_ignores_temporary_files() {
    for (phase, revision, previous) in [
        ("during-write", 2, 1),
        ("before-replace", 2, 1),
        ("after-replace", 3, 2),
    ] {
        let (_dir, mut project, id) = project();
        save(&project, id, 2);
        let path = project.scene_path(id).unwrap();
        let mut writer = PausedWriter::start(&project.root, phase, false);
        let formal = fs::read(&path).unwrap();
        let backup = fs::read(backup_path(&path)).unwrap();
        let manifest = fs::read(project.root.join(MANIFEST)).unwrap();

        assert!(matches!(
            project.rename_scene(id, "Busy"),
            Err(Error::Busy(_))
        ));
        assert_eq!(fs::read(&path).unwrap(), formal);
        assert_eq!(fs::read(backup_path(&path)).unwrap(), backup);
        assert_eq!(fs::read(project.root.join(MANIFEST)).unwrap(), manifest);
        writer.stop();

        let mut reopened = Project::<Content>::open(&project.root).unwrap();
        assert_eq!(reopened.read_scene(id).unwrap().content, content(revision));
        assert_eq!(
            reopened.read_backup(id).unwrap().content(),
            &content(previous)
        );
        reopened.rename_scene(id, "Lock released").unwrap();

        let temps: Vec<_> = fs::read_dir(path.parent().unwrap())
            .unwrap()
            .map(|entry| entry.unwrap().path())
            .filter(|path| path.extension().is_some_and(|ext| ext == "tmp"))
            .collect();
        if phase == "after-replace" {
            assert!(temps.is_empty());
        } else {
            assert_eq!(temps.len(), 1);
            let staged = fs::read(&temps[0]).unwrap();
            let decoded = decode_scene::<Content>(&temps[0], id, &staged);
            if phase == "during-write" {
                assert!(decoded.is_err());
            } else {
                assert_eq!(decoded.unwrap(), content(3));
            }
            fs::remove_file(&path).unwrap();
            assert!(matches!(
                reopened.read_scene(id),
                Err(Error::Io { source, .. }) if source.kind() == io::ErrorKind::NotFound
            ));
            let offered = reopened.read_backup(id).unwrap();
            assert_eq!(
                reopened.restore_backup(offered).unwrap().content,
                content(previous)
            );
        }
    }
}

#[cfg(windows)]
#[test]
fn windows_detects_external_changes_after_staging_without_touching_the_backup() {
    let (_dir, project, id) = project();
    save(&project, id, 2);
    let path = project.scene_path(id).unwrap();
    let backup = backup_path(&path);
    let old_backup = fs::read(&backup).unwrap();
    let mut writer = PausedWriter::start(&project.root, "before-replace", true);
    let external = encode_scene(&path, id, &content(7)).unwrap();
    fs::write(&path, &external).unwrap();
    writer.resume();
    assert_eq!(fs::read(&path).unwrap(), external);
    assert_eq!(fs::read(&backup).unwrap(), old_backup);
    assert_eq!(fs::read_dir(path.parent().unwrap()).unwrap().count(), 2);
}
