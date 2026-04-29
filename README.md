# ⌨️ AHK-Linux

NOT OFFICIAL AutoHotkey-style text expansion and hotstrings for Linux. 

AHK-Linux reads physical keyboard input via the Linux input subsystem (`evdev`), intercepts triggers from `.ahkl` scripts, and injects text into the active window using `uinput` and native clipboards. It runs below the X11/Wayland display server level.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](#)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Platform: Linux](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](#)

---

## 💥 The Problem

Linux lacks a native, low-level equivalent to AutoHotkey for simple text expansion. Existing solutions either:
- Rely on X11 (breaks on Wayland)
- Require bloated GUI frameworks
- Intercept keys at the wrong level, causing latency or missed strokes

## 🛠️ The Solution

AHK-Linux directly grabs `/dev/input/eventX`. When a hotstring like `rrd1` is typed, the daemon:
1. Erases the trigger via backspaces.
2. Injects the replacement text using the system clipboard (`Ctrl+V`).
3. Sends optional tail keystrokes (e.g., `{Left 10}`).

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
:?*:ккв1::Куплю квартиру в ЛТО. Бюджет: Свободный.
:?*:пкв1::Продам квартиру в . Цена: Договорная.{Left 19}
```

### 2. Run it

```bash
AHK-Linux-open my.ahkl
```
*(AHK-Linux will prompt for `sudo` to access input devices).*

### 3. Trigger it
Open any text field and type `rrd1`. It will instantly be replaced.

> **Note:** AHK-Linux translates triggers to physical keycodes. `rrd1` on a Russian layout is evaluated exactly like `rrd1` on an English QWERTY layout. Keep your layout in mind.

---

## 🎯 CLI Reference

```bash
# Auto-detect keyboard and run script
sudo AHK-Linuxd script.ahkl

# Specify a physical device manually
sudo AHK-Linuxd --device /dev/input/event0 script.ahkl
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
