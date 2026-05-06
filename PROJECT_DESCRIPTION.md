# AHKUnix: AutoHotkey-Style Automation for Linux

## Overview

AHKUnix is a lightweight, low-level automation tool for Linux systems, inspired by AutoHotkey (AHK). It provides text expansion (hotstrings) and hotkey functionality that works consistently across different desktop environments (Wayland, X11) and even in TTY consoles. Unlike GUI-based solutions, AHKUnix operates below the display server layer using Linux's input subsystem (`evdev`) and virtual input devices (`uinput`), ensuring reliable performance without compatibility issues.

The project consists of a daemon (`ahkunixd`) that runs in the background, monitoring keyboard input and executing user-defined scripts written in `.ahkl` files (AHK-like scripting language).

## Key Features

### Core Functionality
- **Hotstrings**: Automatic text replacement triggers (e.g., `:*?:btw::by the way`)
- **Hotkeys**: Keyboard shortcuts with modifiers (e.g., `Ctrl & F1::SendInput Hello World`)
- **Script Execution**: Support for command blocks including `SendInput`, `Sleep`, `Random`, and conditional logic with `if/else`
- **Caret Control**: Precise cursor positioning after text injection using commands like `{Left}`, `{Right}`, `{Home}`, `{End}`

### Technical Advantages
- **Low-Level Input Handling**: Uses `libevdev` to read from `/dev/input/eventX` devices and `uinput` for injection
- **Cross-Platform Compatibility**: Works on Wayland, X11, and TTY environments
- **Clipboard Integration**: Leverages native clipboard tools (`wl-clipboard`, `xclip`, `xsel`) for fast text injection
- **Device Safety**: Automatically excludes touchpad and mouse devices from keyboard detection
- **Daemon Architecture**: Runs as a detached background process with proper signal handling

### Supported Keys and Modifiers
- **Function Keys**: F1-F12
- **Numpad Keys**: NumPad0-NumPad9, NumPadAdd, NumPadSub, etc.
- **Navigation Keys**: Home, End, PageUp, PageDown, Insert, Delete
- **Modifiers**: Ctrl, Alt, Shift, Meta/Super
- **Special Keys**: Enter, Tab, Backspace, Escape, Space

## Architecture

### Components
1. **Daemon (ahkunixd)**: Core executable that monitors input and executes scripts
2. **Script Parser**: Parses `.ahkl` files with strict syntax rules
3. **Keyboard Detection**: Automatically identifies and monitors keyboard devices
4. **Virtual Keyboard**: Creates uinput devices for text/key injection
5. **Clipboard Manager**: Interfaces with system clipboard for text expansion

### Workflow
1. User starts the daemon with a script file
2. Daemon grabs the target keyboard device exclusively
3. Monitors key events via `libevdev`
4. Matches input against defined triggers
5. Erases trigger text using backspaces
6. Injects replacement text via clipboard paste (`Ctrl+V`) and virtual keyboard
7. Applies any tail commands (cursor movement, etc.)

## Installation

### Prerequisites
- Linux kernel with evdev and uinput support
- C++20 compatible compiler (GCC 10+ or Clang 10+)
- CMake 3.20+
- `libevdev` development libraries
- Clipboard utilities:
  - Wayland: `wl-clipboard`
  - X11: `xclip` or `xsel`

### Build Process
```bash
git clone https://github.com/Heysh1n/AHKUnix.git
cd AHKUnix
cmake -S . -B build
cmake --build build
```

### Packaging
The project supports Debian package generation via CPack:
```bash
cmake --build build --target package-deb
```

### Local Installation
```bash
cmake --build build --target install-local
```

## Usage

### Basic Syntax
Scripts use `.ahkl` extension and follow AHK-inspired syntax:

```
; Hotstring example
:*?:btw::by the way

; Hotkey example
Ctrl & F1::
    SendInput Hello World{Enter}
    return

; Conditional logic
if (condition)
    SendInput True
else
    SendInput False
```

### Command Line Options
- `--script <file.ahkl>`: Specify script file
- `--device <path>`: Target keyboard device (auto-detected if not specified)
- `--lint`: Validate script syntax without running
- `--strict`: Enforce strict parsing rules
- `--verbose`: Enable detailed logging

### Running the Daemon
```bash
# Auto-detect keyboard and run script
ahkunixd --script myscript.ahkl

# Specify device explicitly
ahkunixd --script myscript.ahkl --device /dev/input/event3
```

## Script Language Reference

### Hotstrings
- `:*?:trigger::replacement` - Case-insensitive hotstring
- `::trigger::replacement` - Case-sensitive hotstring
- `:*C?:trigger::replacement` - Case-sensitive with C suffix

### Hotkeys
- `Key::action` - Single key trigger
- `Mod & Key::action` - Modifier combination
- Supported modifiers: Ctrl, Alt, Shift, Meta

### Commands
- `SendInput <text>` - Send keystrokes/text
- `Sleep <milliseconds>` - Pause execution
- `Random <min>,<max>` - Generate random number
- `if (condition) / else` - Conditional execution

### Special Keys in SendInput
- `{Enter}`, `{Tab}`, `{Space}`, `{Backspace}`
- `{Left}`, `{Right}`, `{Up}`, `{Down}`
- `{Home}`, `{End}`, `{PageUp}`, `{PageDown}`
- `{Delete}`, `{Insert}`
- `{F1}` through `{F12}`
- `{NumPad0}` through `{NumPad9}`

## Development

### Project Structure
```
├── CMakeLists.txt          # Build configuration
├── include/ahkunix/        # Header files
├── src/                    # Source code
│   ├── commands/           # Script command implementations
│   └── *.cpp               # Core components
├── examples/               # Sample scripts
├── packaging/              # Debian packaging files
├── scripts/                # Build/installation scripts
├── tests/                  # Test suite
└── data/                   # Desktop integration files
```

### Building from Source
```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Run tests
cmake --build build --target test

# Install locally
cmake --build build --target install-local
```

### Code Standards
- C++20 standard
- Strict compiler warnings enabled
- Comprehensive test coverage
- Clean, documented code

## Limitations and Known Issues

### Compatibility
- Not a full AutoHotkey implementation - only core features supported
- Requires root/sudo for device access (or proper udev rules)
- Some advanced AHK features not implemented (GUI, COM, etc.)

### Performance Considerations
- Exclusive device grabbing may interfere with other input monitoring tools
- Clipboard-based injection may have slight latency vs direct key injection
- Memory usage scales with script complexity

### Security
- Runs with elevated privileges for device access
- Scripts execute with daemon permissions
- No sandboxing - malicious scripts can perform any allowed operations

## Contributing

### Development Setup
1. Fork the repository
2. Create a feature branch
3. Make changes with tests
4. Submit pull request

### Testing
```bash
# Run unit tests
cmake --build build --target test

# Manual testing
ahkunixd --script test.ahkl --lint  # Syntax check
ahkunixd --script test.ahkl --verbose  # Debug run
```

### Code Style
- Follow existing C++ conventions
- Use meaningful variable names
- Add documentation for new features
- Include unit tests for new functionality

## License

MIT License - see LICENSE file for details.

## Acknowledgments

- Inspired by AutoHotkey for Windows
- Built using libevdev and Linux input subsystem
- Community contributions and feedback

## Version History

- **0.1.0**: Initial release with basic hotstring support
- **0.2.0**: Added hotkey support and modifier combinations
- **0.3.0**: Implemented command blocks and conditional logic
- **0.4.0**: Enhanced parser, added caret control, improved stability

---

This project aims to bring the power and simplicity of AutoHotkey-style automation to the Linux ecosystem, providing a reliable alternative for users who need consistent keyboard automation across different environments.