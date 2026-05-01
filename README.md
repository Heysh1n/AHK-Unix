# ⌨️ AHK UNIX (AHK Linux)

NOT OFFICIAL AutoHotkey-style text expansion and hotkeys for Linux. 

AHK Linux reads physical keyboard input via the Linux input subsystem (`evdev`), parses triggers from `.ahkl` scripts, and injects text/keys into the active window using `uinput` + native clipboard backends. It runs strictly below the X11/Wayland display server layer.

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
| **If/Else logic** | ✅ YES | Supported with parser rules |
| **Caret Control** | ✅ YES | Moving the cursor left after injection (e.g., `{Left 16}`) |
| **Lint validation** | ✅ YES | `--lint` checks script without grabbing device |
| **Strict mode** | ✅ YES | `--strict` enforces strict parser policy |
| **Full AHK compatibility** | ❌ NO | Project is not a full AutoHotkey interpreter |

---

## 🧠 Parser Rules (Important)

AHK Linux supports command blocks, but the parser is intentionally strict for stability.

### If/Else case policy
Allowed forms:
- `if / else`
- `IF / ELSE`

Other case variants:
- Normal mode: warning + line ignored
- `--strict` mode: fatal parser error

---

## 💥 The Problem

Linux lacks a native, low-level equivalent to AutoHotkey that works consistently across Wayland/X11/TTY without GUI bloat. Existing solutions either break on Wayland, require heavy frameworks, or intercept keys at the wrong level (causing latency).

## 🛠️ The Solution

AHK Linux works below the display server layer:
1. Grabs keyboard events from `/dev/input/eventX`
2. Matches the trigger sequence
3. Erases the trigger via backspaces
4. Injects replacement through system clipboard + virtual keyboard (`Ctrl+V`)
5. Applies tail keys (`{Left}`, `{Enter}`, etc.)

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| **Low-Level Intercept** | Uses `libevdev` + `uinput`. Works on Wayland, X11, TTY. |
| **Trigger Formats** | Legacy hotstrings and key-based hotkeys. |
| **NumPad & Function Keys** | `NumPad0..9`, `F1..F12`, navigation/special keys. |
| **Key Modifiers** | `Alt`, `Ctrl`, `Shift`, `Meta/Super` combos. |
| **Tail Key Commands** | `{Left}`, `{Right}`, `{Home}`, `{End}`, `{Delete}`, `{Enter}`, etc. |
| **Action Blocks** | `SendInput`, `Sleep`, `Random`, `if/else` / `IF/ELSE`. |
| **Touchpad Safe** | Touchpad/mouse devices are excluded from keyboard autodetect. |
| **Daemon Isolation** | Detached background process with proper daemonization. |
| **Fast Injection** | Clipboard-based paste via `wl-copy`, `xclip`, or `xsel`. |
| **Zero Bloat** | Pure C++20 daemon. No GUI frameworks required. |

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
git clone https://github.com/Heysh1n/AHK Linux.git
cd AHK Linux
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
; Legacy hotstring
:*?:hlo::Hello World!

; Key hotkey
NumPad9::Quick message{Enter}

; Modifier hotkey
Ctrl & NumPad1::Control action{Enter}

; Action block
:*?:weather1::
SendInput, {Esc}
Sleep, 300
Random, variant, 1, 2
if (variant = 1) {
    SendInput, Forecast variant one.{Enter}
} else {
    SendInput, Forecast variant two.{Enter}
}
Return
```

### 2. Validate script first (recommended)

```bash
./build/ahkunixd --lint my.ahkl
./build/ahkunixd --lint --strict my.ahkl
```

### 3. Run daemon

```bash
# Background daemon
sudo ./build/ahkunixd my.ahkl

# Foreground debugging
sudo ./build/ahkunixd --no-daemon my.ahkl

# Launcher shortcut
ahkunix-open my.ahkl
```

---

## 🎯 CLI Reference

```bash
# Lint only, no device grab
./build/ahkunixd --lint script.ahkl

# Lint in strict parser mode
./build/ahkunixd --lint --strict script.ahkl

# Run in background daemon mode (default)
sudo ./build/ahkunixd script.ahkl

# Explicit input device
sudo ahkunixd --device /dev/input/event0 script.ahkl

# Combine options
sudo ahkunixd --device /dev/input/event0 --no-daemon --strict script.ahkl
```

### Finding your device manually
If auto-detect fails:
```bash
cat /proc/bus/input/devices | grep -E 'Name=|Handlers=.*kbd'
```

---

## 🏗️ Architecture

```text
AHKUnix/
├── include/ahkunix/
│   ├── core headers
│   └── commands/
│       ├── Command.hpp
│       ├── SendInputCommand.hpp
│       ├── SleepCommand.hpp
│       ├── IfCommand.hpp
│       └── ScriptParser.hpp
├── src/
│   ├── core runtime
│   └── commands/
├── scripts/
├── examples/
└── packaging/
```

**Data flow:**
`Physical Key` → `/dev/input` → `libevdev` → `RingBuffer` → `matcher` → `Clipboard + uinput`

---

## 🔨 Development

```bash
make help
make build
make test
make clean-artifacts
make clean-full
```

---

## ⚠️ Limitations

AHK Linux is not a full AutoHotkey interpreter. Not fully supported:
- Full AHK language semantics & variables
- Loops (`while`, `for`)
- Window management APIs & Mouse automation
- GUI scripting
- Full `SendMessage` / `Input` behavior parity with Windows AHK

## 🧪 Troubleshooting

**Permission denied on `/dev/input/eventX`**
Run with `sudo` or configure proper udev permissions.

**no clipboard backend found**
Install `wl-clipboard`, `xclip`, or `xsel`.

**Script passes normal mode but fails strict mode**
Check `If/Else` case policy and parser warnings.

---

## 📜 License

[MIT](LICENSE) — © 2026 AutoHotKey Contributors & Heysh1n 

<p align="center">
  Made with ❤️ by Heysh1n
</p>
