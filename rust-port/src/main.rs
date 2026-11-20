use sickomenu_rust_foundation::config;
use sickomenu_rust_foundation::logger::{Level, Logger};
use std::error::Error;
use std::fs;
use std::path::PathBuf;

fn main() -> Result<(), Box<dyn Error>> {
    let executable = std::env::current_exe()?;
    let executable_dir = executable.parent().unwrap_or_else(|| std::path::Path::new("."));
    let data_dir: PathBuf = executable_dir.join("SickoMenu");
    fs::create_dir_all(&data_dir)?;

    let logger = Logger::new(data_dir.join("sicko-log.txt"))?;
    let config_path = data_dir.join("config.json");

    let (settings, first_run) = config::load_or_default(&config_path)?;

    if first_run {
        config::save(&config_path, &settings)?;
        logger.log(Level::Info, "bootstrap", "Created default configuration")?;
    } else {
        logger.log(Level::Info, "bootstrap", "Loaded configuration")?;
    }

    logger.log(
        Level::Info,
        "bootstrap",
        &format!(
            "Settings ready (scale: {:.2}, opacity: {:.2}, animations: {})",
            settings.theme.scale,
            settings.theme.opacity,
            settings.theme.animations_enabled
        ),
    )?;

    Ok(())
}
