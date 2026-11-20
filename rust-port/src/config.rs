use serde::{Deserialize, Serialize};
use std::fs;
use std::io;
use std::path::Path;

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct AppConfig {
    pub show_menu_on_startup: bool,
    pub theme: ThemeConfig,
    pub shortcuts: ShortcutConfig,
}

impl Default for AppConfig {
    fn default() -> Self {
        Self {
            show_menu_on_startup: true,
            theme: ThemeConfig::default(),
            shortcuts: ShortcutConfig::default(),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct ThemeConfig {
    /// RGB hex colour, with or without a leading '#'.
    pub accent_hex: String,
    /// Window opacity in the inclusive range 0.1..=1.0.
    pub opacity: f32,
    /// UI scale in the inclusive range 0.5..=2.0.
    pub scale: f32,
    pub animations_enabled: bool,
    pub show_borders: bool,
}

impl Default for ThemeConfig {
    fn default() -> Self {
        Self {
            accent_hex: "#8B5CF6".to_owned(),
            opacity: 1.0,
            scale: 1.0,
            animations_enabled: true,
            show_borders: true,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(default)]
pub struct ShortcutConfig {
    /// Preferences for the standalone settings application only.
    pub toggle_settings: String,
    pub toggle_diagnostics: String,
}

impl Default for ShortcutConfig {
    fn default() -> Self {
        Self {
            toggle_settings: "Delete".to_owned(),
            toggle_diagnostics: "Home".to_owned(),
        }
    }
}

impl AppConfig {
    pub fn validate(&self) -> io::Result<()> {
        let colour = self.theme.accent_hex.strip_prefix('#')
            .unwrap_or(self.theme.accent_hex.as_str());

        if colour.len() != 6 || !colour.chars().all(|c| c.is_ascii_hexdigit()) {
            return Err(invalid_config("theme.accent_hex must be a six-digit RGB hex colour"));
        }

        if !self.theme.opacity.is_finite()
            || !(0.1..=1.0).contains(&self.theme.opacity)
        {
            return Err(invalid_config("theme.opacity must be between 0.1 and 1.0"));
        }

        if !self.theme.scale.is_finite()
            || !(0.5..=2.0).contains(&self.theme.scale)
        {
            return Err(invalid_config("theme.scale must be between 0.5 and 2.0"));
        }

        if self.shortcuts.toggle_settings.trim().is_empty()
            || self.shortcuts.toggle_diagnostics.trim().is_empty()
        {
            return Err(invalid_config("shortcut names cannot be empty"));
        }

        Ok(())
    }
}

/// Loads an existing configuration or returns defaults on the first run.
/// The boolean is true when no configuration file existed.
pub fn load_or_default(path: &Path) -> io::Result<(AppConfig, bool)> {
    match fs::read_to_string(path) {
        Ok(contents) => {
            let config: AppConfig = serde_json::from_str(&contents)
                .map_err(|error| invalid_config(format!("invalid JSON: {error}")))?;
            config.validate()?;
            Ok((config, false))
        }
        Err(error) if error.kind() == io::ErrorKind::NotFound => {
            Ok((AppConfig::default(), true))
        }
        Err(error) => Err(error),
    }
}

/// Saves through a temporary file so a partially written JSON document is
/// not exposed as the primary configuration file.
pub fn save(path: &Path, config: &AppConfig) -> io::Result<()> {
    config.validate()?;

    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }

    let json = serde_json::to_vec_pretty(config)
        .map_err(|error| invalid_config(format!("could not serialize config: {error}")))?;

    let mut temporary = path.as_os_str().to_owned();
    temporary.push(".tmp");
    let temporary = std::path::PathBuf::from(temporary);

    fs::write(&temporary, json)?;

    // On Windows, rename does not replace an existing destination. Remove
    // the old file first; the temporary file still protects against partial
    // writes during serialization and writing.
    if path.exists() {
        fs::remove_file(path)?;
    }

    if let Err(error) = fs::rename(&temporary, path) {
        let _ = fs::remove_file(&temporary);
        return Err(error);
    }

    Ok(())
}

fn invalid_config(message: impl Into<String>) -> io::Error {
    io::Error::new(io::ErrorKind::InvalidData, message.into())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn defaults_are_valid() {
        AppConfig::default().validate().expect("default config should be valid");
    }

    #[test]
    fn rejects_invalid_opacity() {
        let mut config = AppConfig::default();
        config.theme.opacity = 1.5;
        assert!(config.validate().is_err());
    }

    #[test]
    fn rejects_invalid_colour() {
        let mut config = AppConfig::default();
        config.theme.accent_hex = "purple".to_owned();
        assert!(config.validate().is_err());
    }

    #[test]
    fn serde_uses_defaults_for_missing_fields() {
        let config: AppConfig = serde_json::from_str("{}").expect("empty object is valid");
        config.validate().expect("defaults should validate");
        assert_eq!(config.theme.scale, 1.0);
    }
}
