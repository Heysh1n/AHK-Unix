# AHKUnix Project Description

## Version

0.5.7

## Summary

AHKUnix is a focused AutoHotkey-style automation daemon for Linux. It provides hotstrings, hotkeys, and small command blocks for keyboard-driven workflows on Wayland, X11, and TTY-oriented environments.

The project is written in C++20 and works below the display server layer:

- `libevdev` reads physical keyboard events from `/dev/input/eventX`
- `EVIOCGRAB` gives the daemon exclusive control of the selected keyboard
- `uinput` injects virtual keyboard events
- `wl-clipboard`, `xclip`, or `xsel` handle text payload insertion through the system clipboard

## Design Goals

- Keep the keyboard event loop responsive.
- Avoid GUI-framework dependencies.
- Preserve clipboard content after text injection.
- Make scripts easy to lint before loading.
- Support practical AHK v1.1-style syntax without pretending to be a full AutoHotkey interpreter.

## Runtime Architecture

```text
ahkunixd
    input device detector
    evdev reader
    trigger ring buffer
    parser and AST model
    macro dispatcher queue
    worker thread
    uinput injector
    clipboard pipeline

ahkunixctl
    ping
    stop
    load
    lint
```

The daemon owns the physical input device and keeps reading it in the main event loop. When a trigger matches, the parsed macro is submitted to a worker thread. The worker traverses the AST and executes commands. This keeps long macros and `Sleep` commands from blocking `libevdev` polling.

## Core Features

### Hotstrings

```ahk
:*?:11::221B Baker Street
```

### Hotkeys

```ahk
F6::
SendInput, Hello{Enter}
Return

Ctrl & F12::
Cancel
Return
```

### Command Blocks

Supported commands:

- `SendInput`
- `Sleep`
- `Random`
- `if`
- `else`
- `Cancel`
- `Pause`
- `Return`

`SendMessage` and `Input` are accepted as compatibility stubs and ignored.

### Case-Insensitive Command Tokens

The tokenizer normalizes recognized command tokens before AST construction:

```ahk
SendInput, Upper command spelling
sendinput, Lower command spelling
sEnDiNpUt, Mixed command spelling
If (variant = 1) {
    Sleep, 100
} else {
    sleep, 100
}
```

Only command tokens are normalized. String arguments preserve their original casing.

### Worker-Thread AST Execution

Macro execution runs in a dedicated worker thread. The main evdev loop continues reading and forwarding keyboard events while long scripts execute.

### Cancel/Pause Interrupts

Users define their own interrupt hotkeys:

```ahk
F12::
Cancel
Return
```

When a matched macro contains `Cancel` or `Pause`, it is not queued. The main thread immediately sets `stop_requested`, clears queued macro jobs, and lets the running worker exit at the next cancellation checkpoint.

### Synchronous Clipboard Pipeline

Text insertion uses a strict synchronous pipeline:

1. Save the current clipboard text into C++ memory.
2. Write macro text to the system clipboard.
3. Send `Ctrl+V` through `uinput`.
4. Sleep for 75 milliseconds to let X11/Wayland process paste.
5. Restore the saved clipboard text immediately.

This removes the race condition caused by delayed clipboard cleanup timers.

## Parser Features

- Single-line comments with `;` or `#`
- Multi-line comments with `/* ... */`
- New hotkey format: `Key::` and `Modifier & Key::`
- Legacy hotstring format: `:*?:11::replacement`
- Nested `if`/`else` blocks with braces
- Random integer assignment through `Random, name, min, max`
- Variable comparisons such as `if (name = 2)`

## Command Line Tools

### Daemon

```bash
sudo ahkunixd script.ahkl
sudo ahkunixd --foreground script.ahkl
sudo ahkunixd --device /dev/input/event3 script.ahkl
sudo ahkunixd --strict script.ahkl
```

### Client

```bash
ahkunixctl ping
ahkunixctl stop
ahkunixctl load /absolute/path/script.ahkl
ahkunixctl lint script.ahkl
ahkunixctl lint --strict script.ahkl
```

## Limitations

AHKUnix is not a full AutoHotkey runtime. It does not currently support:

- Loops
- Functions
- GUI automation
- Mouse automation
- Window management
- Full AHK expression semantics
- Sandboxed script execution

## Repository Layout

```text
include/ahkunix/          Public headers
src/                      Core implementation
src/commands/             Script command AST nodes
src/daemon/               Daemon entrypoint and IPC server
src/cli/                  Client controller
examples/                 Example .ahkl scripts
AHK-Unix.wiki/            User-facing documentation pages
packaging/                Debian packaging hooks
tests/                    CLI smoke tests
```

## Status

0.5.7 is the stable architecture baseline for:

- responsive evdev polling during long macro execution
- configurable script-level interruption through `Cancel`/`Pause`
- case-insensitive command parsing
- synchronous clipboard save/paste/restore behavior
