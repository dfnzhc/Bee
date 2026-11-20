mod files;

#[cfg(test)]
mod tests;

use self::files::{check_dir, check_unchanged, io_at, read, read_optional, stage};
use serde::{de::DeserializeOwned, Deserialize, Serialize};
use std::collections::HashSet;
use std::fs::{self, File};
use std::io;
use std::marker::PhantomData;
use std::path::{Path, PathBuf};
use uuid::Uuid;

const FORMAT_VERSION: u32 = 1;
const MANIFEST: &str = "project.beeproj";
const SCENES: &str = "Scenes";
const ASSETS: &str = "Assets";
const TRASH: &str = "Trash";
const LOCK: &str = ".bee.lock";

#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("{path}: {source}")]
    Io {
        path: PathBuf,
        #[source]
        source: io::Error,
    },
    #[error("{path}: {reason}")]
    Invalid { path: PathBuf, reason: String },
    #[error("{path}: unsupported format version {version}")]
    UnsupportedVersion { path: PathBuf, version: u32 },
    #[error("{0}: another writer holds the file")]
    Busy(PathBuf),
    #[error("{0}: contents changed on disk")]
    Changed(PathBuf),
}

impl Error {
    fn invalid(path: &Path, reason: impl ToString) -> Self {
        Self::Invalid {
            path: path.to_owned(),
            reason: reason.to_string(),
        }
    }
}

type Result<T> = std::result::Result<T, Error>;

#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct Manifest {
    format_version: u32,
    pub project_id: Uuid,
    pub name: String,
    pub scenes: Vec<SceneEntry>,
    pub startup_scene: Uuid,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct SceneEntry {
    pub id: Uuid,
    pub name: String,
}

impl Manifest {
    fn validate(&self, path: &Path) -> Result<()> {
        if self.project_id.is_nil() || self.name.trim().is_empty() {
            return Err(Error::invalid(path, "invalid project id or name"));
        }
        let mut ids = HashSet::new();
        for scene in &self.scenes {
            if scene.id.is_nil() || !ids.insert(scene.id) {
                return Err(Error::invalid(path, "invalid or duplicate scene id"));
            }
            if scene.name.trim().is_empty() {
                return Err(Error::invalid(path, "scene name is empty"));
            }
        }
        if !ids.contains(&self.startup_scene) {
            return Err(Error::invalid(path, "startup scene is not listed"));
        }
        Ok(())
    }
}

// 场景层负责数据约束，存储层对正式文件和备份使用同一套校验。
pub trait SceneContent: Serialize + DeserializeOwned {
    fn validate(&self) -> std::result::Result<(), String>;
}

#[derive(Debug, Deserialize, Serialize)]
struct SceneFile<C> {
    format_version: u32,
    scene_id: Uuid,
    content: C,
}

#[derive(Debug)]
pub struct LoadedScene<C> {
    id: Uuid,
    pub content: C,
    saved: Vec<u8>,
}

impl<C> LoadedScene<C> {
    pub fn id(&self) -> Uuid {
        self.id
    }
}

#[derive(Debug)]
pub struct SceneBackup<C> {
    scene: LoadedScene<C>,
    original: Option<Vec<u8>>,
}

impl<C> SceneBackup<C> {
    pub fn content(&self) -> &C {
        &self.scene.content
    }
}

pub enum ProjectLocation<'a> {
    NewDirectoryIn(&'a Path),
    ExistingEmptyDirectory(&'a Path),
}

#[derive(Debug)]
pub enum DeletedScene {
    Recycled(PathBuf),
    Retained { path: PathBuf, error: Error },
}

#[derive(Debug)]
pub struct Project<C> {
    root: PathBuf,
    manifest: Manifest,
    saved_manifest: Vec<u8>,
    content: PhantomData<C>,
}

impl<C: SceneContent> Project<C> {
    pub fn open(root: &Path) -> Result<Self> {
        let root = io_at(root, root.canonicalize())?;
        let path = root.join(MANIFEST);
        let saved_manifest = read(&path)?;
        let manifest: Manifest = decode(&path, &saved_manifest)?;
        manifest.validate(&path)?;
        check_dir(&root, SCENES, true)?;
        check_dir(&root, ASSETS, true)?;
        Ok(Self {
            root,
            manifest,
            saved_manifest,
            content: PhantomData,
        })
    }

    pub fn create(
        location: ProjectLocation<'_>,
        name: &str,
        first_scene_name: &str,
        first_scene: &C,
    ) -> Result<Self> {
        let (target, new_dir) = match location {
            ProjectLocation::NewDirectoryIn(parent) => {
                check_directory_name(parent, name)?;
                (io_at(parent, parent.canonicalize())?.join(name), true)
            }
            ProjectLocation::ExistingEmptyDirectory(path) => (path.to_owned(), false),
        };
        let id = Uuid::new_v4();
        let manifest = Manifest {
            format_version: FORMAT_VERSION,
            project_id: Uuid::new_v4(),
            name: name.to_owned(),
            scenes: vec![SceneEntry {
                id,
                name: first_scene_name.to_owned(),
            }],
            startup_scene: id,
        };
        manifest.validate(&target.join(MANIFEST))?;
        let scene_bytes = encode_scene(&target.join(SCENES).join(scene_name(id)), id, first_scene)?;
        if new_dir {
            io_at(&target, fs::create_dir(&target))?;
        }
        let result = Self::initialize(&target, manifest, &scene_bytes);
        if new_dir && result.is_err() {
            let _ = fs::remove_dir(&target);
        }
        result
    }

    fn initialize(root: &Path, manifest: Manifest, scene_bytes: &[u8]) -> Result<Self> {
        let root = io_at(root, root.canonicalize())?;
        if io_at(&root, fs::read_dir(&root))?.next().is_some() {
            return Err(Error::invalid(&root, "project directory is not empty"));
        }
        let path = root.join(MANIFEST);
        let saved_manifest = encode(&path, &manifest)?;
        let lock = files::lock(&root, true)?;
        let scene_path = root.join(SCENES).join(scene_name(manifest.startup_scene));
        let mut dirs = Vec::new();
        let mut scene_created = false;
        let result = (|| {
            for name in [SCENES, ASSETS] {
                let dir = root.join(name);
                io_at(&dir, fs::create_dir(&dir))?;
                dirs.push(dir);
            }
            files::commit(stage(&scene_path, scene_bytes)?, &scene_path, false)?;
            scene_created = true;
            files::commit(stage(&path, &saved_manifest)?, &path, false)
        })();
        // 清单是可见提交点，只撤销本次创建且仍归本次操作所有的实体。
        if let Err(error) = result {
            if scene_created {
                files::remove_unchanged(&scene_path, scene_bytes);
            }
            for dir in dirs.into_iter().rev() {
                let _ = fs::remove_dir(dir);
            }
            drop(lock);
            files::remove_unchanged(&root.join(LOCK), b"");
            return Err(error);
        }
        Ok(Self {
            root,
            manifest,
            saved_manifest,
            content: PhantomData,
        })
    }

    pub fn manifest(&self) -> &Manifest {
        &self.manifest
    }

    pub fn assets_dir(&self) -> Result<PathBuf> {
        check_dir(&self.root, ASSETS, true)
    }

    pub fn scene_path(&self, id: Uuid) -> Result<PathBuf> {
        if !self.manifest.scenes.iter().any(|entry| entry.id == id) {
            return Err(Error::invalid(
                &self.root.join(MANIFEST),
                format!("scene {id} is not listed"),
            ));
        }
        Ok(check_dir(&self.root, SCENES, false)?.join(scene_name(id)))
    }

    pub fn read_scene(&self, id: Uuid) -> Result<LoadedScene<C>> {
        let path = self.scene_path(id)?;
        load_scene(&path, id, read(&path)?)
    }

    pub fn save_scene(&self, scene: &mut LoadedScene<C>) -> Result<()> {
        let _lock = self.begin_write()?;
        let path = self.scene_path(scene.id)?;
        let backup = backup_path(&path);
        check_unchanged(&path, Some(&scene.saved))?;
        let bytes = encode_scene(&path, scene.id, &scene.content)?;
        let temp = stage(&path, &bytes)?;

        #[cfg(all(test, windows))]
        tests::checkpoint("before-replace");
        self.check_manifest()?;
        files::check_file(&backup)?;
        check_unchanged(&path, Some(&scene.saved))?;
        files::replace_scene(temp, &path, &backup)?;
        #[cfg(all(test, windows))]
        tests::checkpoint("after-replace");
        scene.saved = bytes;
        Ok(())
    }

    pub fn read_backup(&self, id: Uuid) -> Result<SceneBackup<C>> {
        let path = self.scene_path(id)?;
        let backup = backup_path(&path);
        Ok(SceneBackup {
            scene: load_scene(&backup, id, read(&backup)?)?,
            original: read_optional(&path)?,
        })
    }

    pub fn restore_backup(&self, backup: SceneBackup<C>) -> Result<LoadedScene<C>> {
        let _lock = self.begin_write()?;
        let path = self.scene_path(backup.scene.id)?;
        if let Some(bytes) = &backup.original {
            match decode_scene::<C>(&path, backup.scene.id, bytes) {
                Ok(_) => return Err(Error::invalid(&path, "scene is already valid")),
                Err(Error::Invalid { .. }) => {}
                Err(error) => return Err(error),
            }
        }
        let temp = stage(&path, &backup.scene.saved)?;
        self.check_manifest()?;
        check_unchanged(&backup_path(&path), Some(&backup.scene.saved))?;
        check_unchanged(&path, backup.original.as_deref())?;
        files::commit(temp, &path, backup.original.is_some())?;
        Ok(backup.scene)
    }

    pub fn new_scene(&mut self, name: &str, content: &C) -> Result<Uuid> {
        let _lock = self.begin_write()?;
        self.insert_scene(name, content)
    }

    pub fn copy_scene(&mut self, id: Uuid, name: &str) -> Result<Uuid> {
        let _lock = self.begin_write()?;
        let saved = self.read_scene(id)?;
        self.insert_scene(name, &saved.content)
    }

    fn insert_scene(&mut self, name: &str, content: &C) -> Result<Uuid> {
        let id = Uuid::new_v4();
        let mut next = self.manifest.clone();
        next.scenes.push(SceneEntry {
            id,
            name: name.into(),
        });
        next.validate(&self.root.join(MANIFEST))?;

        let path = check_dir(&self.root, SCENES, false)?.join(scene_name(id));
        let bytes = encode_scene(&path, id, content)?;
        files::commit(stage(&path, &bytes)?, &path, false)?;
        if let Err(error) = self.commit_manifest(next) {
            files::remove_unchanged(&path, &bytes);
            return Err(error);
        }
        Ok(id)
    }

    pub fn rename_scene(&mut self, id: Uuid, name: &str) -> Result<()> {
        let _lock = self.begin_write()?;
        let mut next = self.manifest.clone();
        let entry = next
            .scenes
            .iter_mut()
            .find(|scene| scene.id == id)
            .ok_or_else(|| Error::invalid(&self.root.join(MANIFEST), "scene is not listed"))?;
        entry.name = name.to_owned();
        self.commit_manifest(next)
    }

    pub fn set_startup_scene(&mut self, id: Uuid) -> Result<()> {
        let _lock = self.begin_write()?;
        self.read_scene(id)?;
        let mut next = self.manifest.clone();
        next.startup_scene = id;
        self.commit_manifest(next)
    }

    pub fn delete_scene(
        &mut self,
        id: Uuid,
        replacement_startup: Option<Uuid>,
    ) -> Result<DeletedScene> {
        let _lock = self.begin_write()?;
        let path = self.scene_path(id)?;
        let mut next = self.manifest.clone();
        if next.scenes.len() == 1 {
            return Err(Error::invalid(
                &self.root.join(MANIFEST),
                "cannot delete the last scene",
            ));
        }
        next.scenes.retain(|entry| entry.id != id);
        if id == next.startup_scene {
            let replacement = replacement_startup
                .filter(|replacement| *replacement != id)
                .ok_or_else(|| {
                    Error::invalid(
                        &self.root.join(MANIFEST),
                        "a replacement startup scene is required",
                    )
                })?;
            self.read_scene(replacement)?;
            next.startup_scene = replacement;
        }

        // 清单提交后才回收文件，回收失败也能报告原路径。
        self.commit_manifest(next)?;
        Ok(match self.recycle_scene(&path, id) {
            Ok(destination) => DeletedScene::Recycled(destination),
            Err(error) => DeletedScene::Retained { path, error },
        })
    }

    fn recycle_scene(&self, path: &Path, id: Uuid) -> Result<PathBuf> {
        let trash = check_dir(&self.root, TRASH, true)?;
        match fs::create_dir(&trash) {
            Err(error) if error.kind() == io::ErrorKind::AlreadyExists => {}
            result => io_at(&trash, result)?,
        }
        let destination = check_dir(&self.root, TRASH, false)?.join(scene_name(id));
        files::check_file(path)?;
        files::move_new(path, &destination)?;
        Ok(destination)
    }

    fn begin_write(&self) -> Result<File> {
        let lock = files::lock(&self.root, false)?;
        self.check_manifest()?;
        Ok(lock)
    }

    fn check_manifest(&self) -> Result<()> {
        check_unchanged(&self.root.join(MANIFEST), Some(&self.saved_manifest))
    }

    fn commit_manifest(&mut self, manifest: Manifest) -> Result<()> {
        let path = self.root.join(MANIFEST);
        manifest.validate(&path)?;
        let bytes = encode(&path, &manifest)?;
        let temp = stage(&path, &bytes)?;
        self.check_manifest()?;
        files::commit(temp, &path, true)?;
        self.manifest = manifest;
        self.saved_manifest = bytes;
        Ok(())
    }
}

fn scene_name(id: Uuid) -> String {
    format!("{id}.beescene")
}

fn backup_path(path: &Path) -> PathBuf {
    path.with_extension("beescene.bak")
}

fn check_directory_name(path: &Path, name: &str) -> Result<()> {
    let device = name
        .split('.')
        .next()
        .unwrap_or_default()
        .trim_end()
        .to_ascii_uppercase();
    let numbered_device = device
        .strip_prefix("COM")
        .or_else(|| device.strip_prefix("LPT"));
    if name.trim().is_empty()
        || name.ends_with(['.', ' '])
        || name
            .chars()
            .any(|c| c.is_control() || "<>:\"/\\|?*".contains(c))
        || matches!(
            device.as_str(),
            "CON" | "PRN" | "AUX" | "NUL" | "CONIN$" | "CONOUT$"
        )
        || numbered_device.is_some_and(|suffix| {
            matches!(suffix.as_bytes(), [b'1'..=b'9'])
                || matches!(suffix, "\u{b9}" | "\u{b2}" | "\u{b3}")
        })
    {
        return Err(Error::invalid(path, "invalid project directory name"));
    }
    Ok(())
}

fn encode(path: &Path, value: &impl Serialize) -> Result<Vec<u8>> {
    serde_json::to_vec_pretty(value).map_err(|error| Error::invalid(path, error))
}

fn decode<T: DeserializeOwned>(path: &Path, bytes: &[u8]) -> Result<T> {
    #[derive(Deserialize)]
    struct Header {
        format_version: u32,
    }
    let header: Header =
        serde_json::from_slice(bytes).map_err(|error| Error::invalid(path, error))?;
    if header.format_version != FORMAT_VERSION {
        return Err(Error::UnsupportedVersion {
            path: path.to_owned(),
            version: header.format_version,
        });
    }
    serde_json::from_slice(bytes).map_err(|error| Error::invalid(path, error))
}

fn encode_scene(path: &Path, id: Uuid, content: &impl SceneContent) -> Result<Vec<u8>> {
    content
        .validate()
        .map_err(|error| Error::invalid(path, error))?;
    encode(
        path,
        &SceneFile {
            format_version: FORMAT_VERSION,
            scene_id: id,
            content,
        },
    )
}

fn decode_scene<C: SceneContent>(path: &Path, id: Uuid, bytes: &[u8]) -> Result<C> {
    let scene: SceneFile<C> = decode(path, bytes)?;
    if scene.scene_id != id {
        return Err(Error::invalid(path, "scene id does not match the manifest"));
    }
    scene
        .content
        .validate()
        .map_err(|error| Error::invalid(path, error))?;
    Ok(scene.content)
}

fn load_scene<C: SceneContent>(path: &Path, id: Uuid, saved: Vec<u8>) -> Result<LoadedScene<C>> {
    let content = decode_scene(path, id, &saved)?;
    Ok(LoadedScene { id, content, saved })
}
