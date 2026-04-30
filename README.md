# ⌨️ AHK-Linux (AHKUnix)

NOT OFFICIAL AutoHotkey-style text expansion and hotstrings for Linux. 

AHK-Linux reads physical keyboard input via the Linux input subsystem (`evdev`), intercepts triggers from `.ahkl` scripts, and injects text into the active window using `uinput` and native clipboards. It runs below the X11/Wayland display server level.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](#)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Platform: Linux](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](#)

---

## ⚡ Current State & Capabilities

The daemon is currently focused on text expansion. Advanced hotkeys are in active development.

| Feature | Status | Description / Example |
| :--- | :---: | :--- |
| **Hotstrings** | ✅ YES | Auto-replacing typed triggers (e.g., `hlo` → `Hello World!`). |
| **Caret Control** | ✅ YES | Moving the cursor left after injection (e.g., `{Left 16}`). |
| **Modifier Hotkeys** | ❌ WIP | Bindings requiring held keys like `Alt`, `Ctrl`, or `Super`. |
| **Complex Scripting** | ❌ WIP | Executing shell commands or complex logic via triggers. |

---

## 🗺️ Roadmap

The codebase is being prepared to support full AutoHotkey-like functionality natively on Linux. Upcoming features include:

- [ ] **Modifier Keys Parsing:** Tracking the state of `Ctrl` (`^`), `Alt` (`!`), `Shift` (`+`), and `Super` (`#`) via `libevdev` state queries.
- [ ] **Shell Execution:** Running external commands directly from `.ahkl` triggers (e.g., opening a terminal or browser).
- [ ] **Dynamic Layout Detection:** Moving away from hardcoded physical key mappings to support arbitrary keyboard layouts dynamically.
- [ ] **Multi-line Replacements:** Clean syntax for injecting massive multi-line text blocks.
- [ ] **More abstraction:** Abstractin code for creating ANY script to ANY lines. 

---

## 💥 The Problem

Linux lacks a native, low-level equivalent to AutoHotkey for simple text expansion. Existing solutions either:
- Rely on X11 (breaks on Wayland)
- Require bloated GUI frameworks
- Intercept keys at the wrong level, causing latency or missed strokes

## 🛠️ The Solution

AHK-Linux directly grabs `/dev/input/eventX`. When a hotstring like `hlo` is typed, the daemon:
1. Erases the trigger via backspaces.
2. Injects the replacement text using the system clipboard (`Ctrl+V`).
3. Sends optional tail keystrokes (e.g., `{Left 16}`).

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| **Low-Level Intercept** | Uses `libevdev` and `uinput`. Works on Wayland, X11, and TTY. |
| **Hotstring Parsing** | Define triggers and replacements in simple `.ahkl` files. |
| **Cursor Control** | Supports moving the cursor after expansion (e.g., `{Left N}`). |
| **Auto-Detect** | Automatically finds the correct keyboard event device. |
| **Fast Injection** | Bypasses character-by-character typing using native clipboards (`wl-copy`, `xclip`, `xsel`). |
| **Zero Bloat** | Pure C++20 daemon. |

---

## 📦 Installation

### Prerequisites

- C++20 compiler & CMake 3.20+
- `libevdev`
- Clipboard backend: `wl-clipboard` (Wayland) or `xclip` / `xsel` (X11)

### Quick Setup
```bash
git clone https://github.com/Heysh1n/AHK-Linux.git
cd AHK-Linux
make setup
```

This will attempt to install dependencies via your package manager, build the daemon, and install the binaries to `~/.local/bin`.

### Debian / Ubuntu (.deb)

```bash
make deb
sudo apt install ./ahkunix-0.1.0-Linux.deb
```

---

## 🚀 Quick Start

### 1. Create a script (`my.ahkl`)

```ahk
; Basic text replacement
:?*:hlo::Hello World!

; Replacement with cursor shift (cursor lands between the quotes)
:?*:cout::std::cout << " " << std::endl;{Left 16}
```

### 2. Run it

```bash
ahkunixd-open my.ahkl
```
*(AHK-Linux will prompt for `sudo` to access input devices).*

### 3. Trigger it
Open any text field and type `cout`. The trigger will instantly be erased, replaced by `std::cout << " " << std::endl;`, and the cursor will jump 16 characters to the left.

> **Note:** AHK-Linux translates triggers to physical keycodes. A trigger on a non-QWERTY layout is evaluated exactly like its physical equivalent on an English QWERTY layout. Keep your active layout in mind.

---

## 🎯 CLI Reference

```bash
# Auto-detect keyboard and run script
sudo AHK-Linuxd script.ahkl

# Specify a physical device manually
sudo ahkunixd --device /dev/input/event0 script.ahkl
```

### Finding your device manually

If auto-detect fails (e.g., grabs the Power Button instead of the keyboard):

```bash
cat /proc/bus/input/devices | grep -E 'Name=|Handlers=.*kbd'
```

---

## 🏗️ Architecture

```text
AHK-Linux/
├── include/AHK-Linux/     # Public headers
├── src/                 # Implementation (C++20)
│   ├── Daemon.cpp       # Main event loop (poll)
│   ├── EvdevKeyboard.cpp# EVIOCGRAB wrapper
│   ├── UinputKeyboard.cpp# Virtual key injection
│   └── Clipboard.cpp    # Native clipboard bindings
├── scripts/             # Launch & install scripts
└── examples/            # Example .ahkl files
```

**Data Flow:**
`Physical Key` → `/dev/input` → `libevdev` → `RingBuffer` → `Hotstring Matcher` → `Clipboard + Ctrl+V` → `Virtual /dev/uinput`

---

## 🔨 Development

```bash
make help             # Show available targets
make build            # Compile AHK-Linuxd
make test             # Run smoke tests
make clean-full       # Nuke all build artifacts
```

---

## 📜 License

[MIT](LICENSE) — © 2026 (Heysh1n) AHK-Linux contributors
<p align="center">
  Made with ❤️ by Heysh1n
</p>
