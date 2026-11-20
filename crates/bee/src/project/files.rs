use super::{Error, Result, LOCK};
use std::fs::{self, File, Metadata, OpenOptions, TryLockError};
use std::io::{self, Read, Write};
use std::path::{Path, PathBuf};
use tempfile::{Builder, NamedTempFile};

#[cfg(windows)]
use std::os::windows::{
    ffi::OsStrExt,
    fs::{MetadataExt, OpenOptionsExt},
};
#[cfg(windows)]
use windows_sys::Win32::{
    Foundation::{ERROR_LOCK_VIOLATION, ERROR_SHARING_VIOLATION},
    Storage::FileSystem::{
        MoveFileExW, ReplaceFileW, SetFileAttributesW, FILE_ATTRIBUTE_NORMAL,
        FILE_ATTRIBUTE_REPARSE_POINT, FILE_FLAG_OPEN_REPARSE_POINT, FILE_SHARE_READ,
        FILE_SHARE_WRITE,
    },
};

pub(super) fn io_at<T>(path: &Path, result: io::Result<T>) -> Result<T> {
    result.map_err(|source| {
        #[cfg(windows)]
        if source.raw_os_error().is_some_and(|code| {
            matches!(code as u32, ERROR_SHARING_VIOLATION | ERROR_LOCK_VIOLATION)
        }) {
            return Error::Busy(path.to_owned());
        }
        Error::Io {
            path: path.to_owned(),
            source,
        }
    })
}

fn check_type(path: &Path, metadata: &Metadata, directory: bool) -> Result<()> {
    let correct_type = if directory {
        metadata.is_dir()
    } else {
        metadata.is_file()
    };
    #[cfg(windows)]
    let correct_type =
        correct_type && metadata.file_attributes() & FILE_ATTRIBUTE_REPARSE_POINT == 0;
    if !correct_type {
        return Err(Error::invalid(
            path,
            "unexpected file type or link in project",
        ));
    }
    Ok(())
}

pub(super) fn check_dir(root: &Path, name: &str, allow_missing: bool) -> Result<PathBuf> {
    let path = root.join(name);
    match fs::symlink_metadata(&path) {
        Err(error) if allow_missing && error.kind() == io::ErrorKind::NotFound => return Ok(path),
        result => check_type(&path, &io_at(&path, result)?, true)?,
    }
    let canonical = io_at(&path, path.canonicalize())?;
    if canonical == root || !canonical.starts_with(root) {
        return Err(Error::invalid(&path, "directory leaves the project root"));
    }
    Ok(canonical)
}

pub(super) fn check_file(path: &Path) -> Result<()> {
    match fs::symlink_metadata(path) {
        Err(error) if error.kind() == io::ErrorKind::NotFound => Ok(()),
        result => check_type(path, &io_at(path, result)?, false),
    }
}

pub(super) fn lock(root: &Path, new: bool) -> Result<File> {
    if io_at(root, root.canonicalize())? != root {
        return Err(Error::invalid(root, "project root has moved"));
    }
    let path = root.join(LOCK);
    check_file(&path)?;
    let mut options = OpenOptions::new();
    options.read(true).write(true).create(!new).create_new(new);
    // 保留锁文件名，避免两个进程锁住不同的文件实体。
    #[cfg(windows)]
    options
        .share_mode(FILE_SHARE_READ | FILE_SHARE_WRITE)
        .custom_flags(FILE_FLAG_OPEN_REPARSE_POINT);
    let file = io_at(&path, options.open(&path))?;
    check_type(&path, &io_at(&path, file.metadata())?, false)?;
    match file.try_lock() {
        Ok(()) => Ok(file),
        Err(TryLockError::WouldBlock) => Err(Error::Busy(path)),
        Err(TryLockError::Error(error)) => io_at(&path, Err(error)),
    }
}

pub(super) fn read(path: &Path) -> Result<Vec<u8>> {
    check_file(path)?;
    let mut options = OpenOptions::new();
    options.read(true);
    #[cfg(windows)]
    options.custom_flags(FILE_FLAG_OPEN_REPARSE_POINT);
    let mut file = io_at(path, options.open(path))?;
    check_type(path, &io_at(path, file.metadata())?, false)?;
    let mut bytes = Vec::new();
    io_at(path, file.read_to_end(&mut bytes))?;
    Ok(bytes)
}

pub(super) fn read_optional(path: &Path) -> Result<Option<Vec<u8>>> {
    match read(path) {
        Ok(bytes) => Ok(Some(bytes)),
        Err(Error::Io { source, .. }) if source.kind() == io::ErrorKind::NotFound => Ok(None),
        Err(error) => Err(error),
    }
}

pub(super) fn check_unchanged(path: &Path, bytes: Option<&[u8]>) -> Result<()> {
    if read_optional(path)?.as_deref() != bytes {
        return Err(Error::Changed(path.to_owned()));
    }
    Ok(())
}

pub(super) fn stage(path: &Path, bytes: &[u8]) -> Result<NamedTempFile> {
    let temp = io_at(
        path,
        Builder::new()
            .prefix(".bee-")
            .suffix(".tmp")
            .tempfile_in(path.parent().unwrap()),
    )?;
    #[cfg(all(test, windows))]
    let bytes = {
        let (first, rest) = bytes.split_at(bytes.len() / 2);
        io_at(temp.path(), temp.as_file().write_all(first))?;
        super::tests::checkpoint("during-write");
        rest
    };
    io_at(temp.path(), temp.as_file().write_all(bytes))?;
    io_at(temp.path(), temp.as_file().sync_all())?;
    Ok(temp)
}

pub(super) fn commit(temp: NamedTempFile, path: &Path, replace: bool) -> Result<()> {
    let temp = temp.into_temp_path();
    let result = if replace {
        temp.persist(path)
    } else {
        temp.persist_noclobber(path)
    };
    io_at(path, result.map_err(|error| error.error))
}

#[cfg(windows)]
fn wide(path: &Path) -> Vec<u16> {
    path.as_os_str().encode_wide().chain([0]).collect()
}

#[cfg(windows)]
pub(super) fn replace_scene(temp: NamedTempFile, path: &Path, backup: &Path) -> Result<()> {
    let temp = temp.into_temp_path();
    let replacement = wide(&temp);
    let original = wide(path);
    let backup = wide(backup);
    // 临时文件先刷新并关闭，交给 Windows 替换正式文件并保留旧版本。
    if unsafe { SetFileAttributesW(replacement.as_ptr(), FILE_ATTRIBUTE_NORMAL) } == 0 {
        return io_at(&temp, Err(io::Error::last_os_error()));
    }
    if unsafe {
        ReplaceFileW(
            original.as_ptr(),
            replacement.as_ptr(),
            backup.as_ptr(),
            0,
            std::ptr::null(),
            std::ptr::null(),
        )
    } == 0
    {
        return io_at(path, Err(io::Error::last_os_error()));
    }
    Ok(())
}

#[cfg(not(windows))]
pub(super) fn replace_scene(temp: NamedTempFile, path: &Path, backup: &Path) -> Result<()> {
    commit(stage(backup, &read(path)?)?, backup, true)?;
    commit(temp, path, true)
}

#[cfg(windows)]
pub(super) fn move_new(path: &Path, destination: &Path) -> Result<()> {
    if unsafe { MoveFileExW(wide(path).as_ptr(), wide(destination).as_ptr(), 0) } == 0 {
        return io_at(path, Err(io::Error::last_os_error()));
    }
    Ok(())
}

#[cfg(not(windows))]
pub(super) fn move_new(path: &Path, destination: &Path) -> Result<()> {
    io_at(destination, fs::hard_link(path, destination))?;
    io_at(path, fs::remove_file(path))
}

pub(super) fn remove_unchanged(path: &Path, expected: &[u8]) {
    if check_unchanged(path, Some(expected)).is_ok() {
        let _ = fs::remove_file(path);
    }
}
