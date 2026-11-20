# SickoMenu Rust foundation

This directory starts a **separate Rust implementation** alongside the original C++ project. It does not replace or modify the existing C++ build.

## Included in this first migration step

- typed application settings for theme, scale, opacity and local UI shortcuts;
- JSON configuration loading, validation and saving;
- a small file-and-console logger with previous-log rotation;
- a standalone bootstrap executable.

The settings and shortcuts in this crate belong to a standalone desktop application. This crate does not install global keyboard hooks or connect to the game.

## Deliberately out of scope

This safe foundation does not implement process injection, runtime hooks, IL2CPP memory access, anti-cheat bypasses, packet/RPC manipulation, wallhack/ESP, teleportation or kill automation, lobby disruption, spam, or mass moderation actions.

## Build

Requires Rust 1.85 or newer:

```sh
cd rust-port
cargo check
cargo test
cargo run
```

The executable creates a `SickoMenu` directory beside itself, writes `config.json`, and records diagnostics in `sicko-log.txt`.

## Migration status

This is **not a feature-complete port**. It is the first, independent Rust foundation. The original C++ project remains intact. Future safe work can add a standalone settings UI, more configuration fields, and diagnostics based on local/mock data.

The parent repository is GPL-3.0-or-later; this Rust component is distributed under the same license.
