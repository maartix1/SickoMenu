use std::fs::{self, File, OpenOptions};
use std::io::{self, Write};
use std::path::{Path, PathBuf};
use std::sync::Mutex;
use std::time::{SystemTime, UNIX_EPOCH};

#[derive(Debug, Clone, Copy)]
pub enum Level {
    Debug,
    Info,
    Warn,
    Error,
}

impl std::fmt::Display for Level {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let label = match self {
            Self::Debug => "DEBUG",
            Self::Info => "INFO",
            Self::Warn => "WARN",
            Self::Error => "ERROR",
        };
        formatter.write_str(label)
    }
}

pub struct Logger {
    file: Mutex<File>,
    path: PathBuf,
}

impl Logger {
    /// Creates the log directory and rotates the previous log, if present.
    pub fn new(path: impl AsRef<Path>) -> io::Result<Self> {
        let path = path.as_ref().to_path_buf();

        if let Some(parent) = path.parent() {
            fs::create_dir_all(parent)?;
        }

        if path.exists() {
            let previous = path.with_file_name("sicko-prev-log.txt");
            if previous.exists() {
                fs::remove_file(&previous)?;
            }
            fs::rename(&path, previous)?;
        }

        let file = OpenOptions::new()
            .create(true)
            .append(true)
            .open(&path)?;

        Ok(Self {
            file: Mutex::new(file),
            path,
        })
    }

    pub fn path(&self) -> &Path {
        &self.path
    }

    pub fn log(&self, level: Level, source: &str, message: &str) -> io::Result<()> {
        let elapsed = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default();

        let line = format!(
            "[{}.{:03}][{} - {}] {}",
            elapsed.as_secs(),
            elapsed.subsec_millis(),
            level,
            source,
            message
        );

        writeln!(io::stdout().lock(), "{line}")?;

        let mut file = self.file.lock()
            .map_err(|_| io::Error::other("logger mutex was poisoned"))?;

        writeln!(file, "{line}")?;
        file.flush()
    }
}
