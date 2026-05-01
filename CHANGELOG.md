# Changelog

All notable changes to AHKUnix are documented in this file.

## [0.4.0] - 2026-05-01

### Added
- `--lint` mode to validate `.ahkl` scripts without device grab or daemon start.
- `--strict` mode for strict parser validation rules.
- Strict If/Else policy:
  - Allowed forms: `if/else` and `IF/ELSE`.
  - Other case variants are warnings in normal mode and fatal errors in strict mode.
- Multi-line action block parsing with `Return` terminator support.
- Improved command pipeline with context binding for script commands.

### Changed
- Improved trigger/action discrimination to avoid false trigger parsing when `::` appears inside text.
- Improved parser behavior for mixed script styles and real-world macro blocks.
- Better handling of key-hotkey behavior via `erase_trigger` semantics.

### Fixed
- Fixed touchpad/mouse capture issue in keyboard autodetection.
- Fixed daemon process isolation and parent-session dependency issues.
- Fixed AHK escaped literals handling in braces (`{!}`, `{+}`, `{^}`, `{#}`, `{}`, etc. as text literals where applicable).
- Fixed unstable `If/Else If/Else` chain behavior in multi-line scripts.
- Fixed parser edge cases causing unexpected execution of multiple branches.
- Fixed launch-time parse failures on complex broadcast-style scripts.

### Notes
- AHKUnix remains Linux-first.
- Not a full AutoHotkey interpreter.

## [0.3.0] - 2026-04-30

### Added
- Command-based architecture in `commands/`.
- `SendInput`, `Sleep`, `IfCommand`, `ScriptParser` modules.
- Initial `Context` support for random/variables in command execution path.

### Changed
- Internal parser and daemon integration expanded to support action-block scripts.

## [0.2.0] - 2026-04-30

### Added
- NumPad triggers.
- Function key triggers (F1-F12).
- Modifier key combinations (`Alt`, `Ctrl`, `Shift`, `Meta/Super`).
- Extended tail commands (`{Left}`, `{Right}`, `{Home}`, `{End}`, `{Delete}`, etc.).
- `--no-daemon` debug mode.

### Fixed
- Touchpad/mouse false capture.
- Basic daemonization and session detachment.

## [0.1.0] - Initial

### Added
- Basic Linux hotstring daemon.
- Legacy hotstring parsing and text replacement.
- Clipboard-based text injection via `wl-copy`/`xclip`/`xsel`.