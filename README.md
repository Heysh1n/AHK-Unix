# ⌨️ AHK UNIX (AHK Linux)

NOT OFFICIAL AutoHotkey-style text expansion and hotkeys for Linux. 

AHKUnix reads physical keyboard input via the Linux input subsystem (`evdev`), parses triggers from `.ahkl` scripts, and injects text/keys into the active window using `uinput` + native clipboard backends. 

With version **0.5.7**, it runs as a robust Client-Server architecture (Daemon + CLI Controller) strictly below the X11/Wayland display server layer, featuring multithreaded macro execution and a zero-race-condition synchronous clipboard pipeline.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](#)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Platform: Linux](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](#)

---

## ⚡ Current State & Capabilities

| Feature | Status | Description |
| :--- | :---: | :--- |
| **Hotstrings** | ✅ YES | Text trigger replacement (`:*?:hlo::Hello`) |
| **Hotkeys (NumPad/F-keys)** | ✅ YES | `NumPad1::...`, `F6::...` |
| **Modifier Hotkeys** | ✅ YES | `Ctrl & NumPad1::...`, `Alt & F1::...` |
| **SendInput / Sleep / Random** | ✅ YES | Script command blocks supported |
| **Worker-Thread Execution** | ✅ YES | Long scripts run in the background without freezing the `evdev` loop |
| **Interrupt Commands** | ✅ YES | Safely kill hanging macros via `Cancel` or `Pause` hotkeys |
| **Synchronous Clipboard** | ✅ YES | 100% race-condition-free paste pipeline. Restores user data instantly |
| **Caret Control** | ✅ YES | Moving the cursor left after injection (e.g., `{Left 16}`) |
| **Hot-Reloading** | ✅ YES | Reload scripts via IPC socket without ungrabbing device |
| **Full AHK compatibility** | ❌ NO | Project is not a full AutoHotkey interpreter |

---

## 🧠 Parser Rules (Important)

AHKUnix supports command blocks, and the parser is now **case-insensitive** for command tokens, making it much more resilient.

### Supported Comments
- Single-line: `;` or `#`
- Multi-line block: `/* ... */`

### Command Casing Policy
Command tokens (`SendInput`, `Sleep`, `If`, `Else`, `Random`, `Return`) are normalized automatically.
- `SENDINPUT`, `sendinput`, and `SendInput` are all treated the same.
- **String arguments** (the actual text you want to inject) strictly preserve their original case.

---

## 💥 The Problem

Linux lacks a native, low-level equivalent to AutoHotkey that works consistently across Wayland/X11/TTY without GUI bloat. Existing solutions either break on Wayland, require heavy frameworks, or intercept keys at the wrong level (causing latency). Furthermore, running a monolithic key-grabber risks killing your keyboard input if the parent terminal is closed or if a long macro freezes the event loop.

## 🛠️ The Solution (Client-Server Architecture)

AHKUnix is split into two parts communicating via a Unix Domain Socket (`/tmp/ahkunix.sock`):
1. **`ahkunixd` (Daemon):** Grabs `/dev/input/eventX` exclusively, runs completely in the background, parses scripts, and delegates macro execution to a dedicated worker thread to prevent system freezes.
2. **`ahkunixctl` (Client):** A lightweight CLI tool to send commands (Load, Stop, Ping, Lint) to the daemon safely.

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| **Low-Level Intercept** | Uses `libevdev` + `uinput`. Works on Wayland, X11, TTY. |
| **Strictly Sync Pipeline** | Safe text injection. Saves clipboard, injects via `Ctrl+V`, hard-blocks for 75ms, and instantly restores your original copied data. |
| **Key Modifiers** | `Alt`, `Ctrl`, `Shift`, `Meta/Super` combos. |
| **Tail Key Commands** | `{Left}`, `{Right}`, `{Home}`, `{End}`, `{Delete}`, `{Enter}`, etc. |
| **Touchpad Safe** | Touchpad/mouse devices are excluded from keyboard autodetect. |
| **Daemon Isolation** | Double-fork detached background process. Survives terminal closure. |
| **Zero Bloat** | Pure C++20. No GUI frameworks required. |

---

## 📦 Installation

### Prerequisites
- C++20 compiler
- CMake 3.20+
- `libevdev`
- Clipboard backend: 
  - Wayland: `wl-clipboard`
  - X11: `xclip` or `xsel`

### Quick Setup
```bash
git clone https://github.com/Heysh1n/AHKUnix.git
cd AHKUnix
make setup

```

### Build Debian Package

```bash
make clean-artifacts
make deb
sudo apt install ./*.deb

```

---

## 🚀 Quick Start

### 1. Create script (`my.ahkl`)

```ahk
/* AHKUnix 0.5.7 Script
    Demonstrating case-insensitivity and interrupts
*/
:*?:hlo::Hello World!

; Emergency kill switch for hanging macros
Ctrl & F12::
Cancel
Return

:*?:weather1::
sEnDiNpUt, {Esc}
sleep, 300
Random, variant, 1, 2
IF (variant = 1) {
    SendInput, Forecast variant one.{Enter}
} else {
    sendinput, Forecast variant two.{Enter}
}
Return

```

### 2. Validate script first

Use the client to lint the script locally (does not require sudo or running daemon):

```bash
ahkunixctl lint my.ahkl
ahkunixctl lint --strict my.ahkl

```

### 3. Run the Daemon

```bash
# Start the server in the background
sudo ahkunixd my.ahkl

```

### 4. Control the Daemon via Client

```bash
# Check if daemon is alive
ahkunixctl ping

# Hot-reload a different script without losing keyboard grab
ahkunixctl load /absolute/path/to/another.ahkl

# Safely stop the daemon and ungrab the keyboard
ahkunixctl stop

```

---

## 🎯 Daemon (`ahkunixd`) CLI Reference

```bash
# Run in background daemon mode (default)
sudo ahkunixd script.ahkl

# Run in foreground (for debugging/logs)
sudo ahkunixd --foreground script.ahkl

# Explicit input device
sudo ahkunixd --device /dev/input/event0 script.ahkl

```

If auto-detect fails, find your device manually:

```bash
cat /proc/bus/input/devices | grep -E 'Name=|Handlers=.*kbd'

```

---

## 🏗️ Architecture

```text
AHKUnix/
├── include/ahkunix/
│   ├── core/      (Parser, Commands, Clipboard)
│   └── daemon/    (IpcServer, Daemonizer)
├── src/
│   ├── daemon/    (ahkunixd server)
│   ├── cli/       (ahkunixctl client)
│   └── commands/  (AST Execution & Threading)
├── scripts/
├── examples/
└── packaging/

```

---

## ⚠️ Limitations

AHKUnix is not a full AutoHotkey interpreter. Not fully supported:

* Full AHK language semantics & variables (only basic variables)
* Loops (`while`, `for`)
* Window management APIs & Mouse automation
* GUI scripting
* `SendMessage` / `Input` commands are parsed but ignored for compatibility.

## 🧪 Troubleshooting

**fatal: connect /tmp/ahkunix.sock: Permission denied**
The daemon is running as `root`, so the socket is restricted. Run `ahkunixctl` with `sudo`, or configure proper udev rules and run both as a standard user.

**Permission denied on `/dev/input/eventX**`
Run `ahkunixd` with `sudo` or configure proper udev group permissions for the `input` group.

**no clipboard backend found**
Install `wl-clipboard`, `xclip`, or `xsel`.

---

## 📜 License

[MIT](LICENSE) — © 2026 AutoHotKey Contributors & Heysh1n 

<p align="center">
  Made with ❤️ by Heysh1n
</p>