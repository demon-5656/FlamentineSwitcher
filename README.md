# FlamentineSwitcher

FlamentineSwitcher is a Qt6/C++ keyboard layout switcher and mistyped text conversion utility aimed at KDE Plasma 6 on Arch Linux.

The project is a greenfield Plasma-focused reinterpretation of the Caramba Switcher idea:

- Linux-native architecture
- X11-first MVP with honest Wayland limitations
- tray icon and settings window
- layout backend abstraction
- text conversion engine separated from UI and platform code
- D-Bus control surface and Arch packaging

## Status

This repository now contains the initial project skeleton plus an MVP-oriented foundation:

- Qt6/CMake application layout
- JSON config loading and saving
- X11 layout backend based on XKB/Xlib
- KDE Plasma layout backend on D-Bus for the active system layout in Plasma sessions
- deny-by-default target policy with explicit allowlists for applications and window classes
- X11 active-window backend for app-aware policy decisions
- global X11 hotkey backend based on `XGrabKey`
- delayed X11 typed-word observation and replacement for allowed applications only
- persistent per-window and per-app layout memory for allowed X11 targets
- tray icon and settings window
- text converter for `us <-> ru`
- D-Bus service skeleton
- unit tests for config, conversion and heuristics

The current text conversion workflow is intentionally conservative:

- selection conversion operates on X11 selection clipboard or regular clipboard
- last-word conversion currently transforms the last clipboard token
- when deny-by-default mode is enabled, conversion is blocked unless the active target is explicitly allowed
- X11 delayed auto-conversion only buffers committed words in explicitly allowed targets and cancels replacement if typing continues
- target policy UI now exposes an explicit allowlist-only mode instead of a hidden checkbox
- tray and settings can add the currently focused target to the allowlist in one action
- Wayland stays in the safe/manual mode without low-level text interception

## Build

Install dependencies on Arch Linux:

```bash
sudo pacman -S --needed base-devel cmake qt6-base libx11 libxkbfile libxi libxtst
```

Configure and build:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Controlled X11 end-to-end check for delayed replacement:

```bash
tests/run_x11_delayed_replacement_e2e.sh ./build/flamentine-switcher ./build/x11_text_target "$DISPLAY"
```

Controlled X11 end-to-end check for remember-per-window:

```bash
tests/run_x11_remember_per_window_e2e.sh ./build/flamentine-switcher ./build/x11_text_target "$DISPLAY"
```

The remember-per-window smoke test assumes a real Plasma X11 session where the harness can transfer focus to the target window. On Xwayland-driven displays it exits early with a clear message instead of pretending the check passed.

Run:

```bash
./build/flamentine-switcher
```

## Architecture

- `include/flamentine_switcher/core`: config, settings manager, orchestration
- `include/flamentine_switcher/backends`: layout, hotkeys, window context abstractions
- `include/flamentine_switcher/backends/text`: typed-text observation and delayed replacement backends
- `include/flamentine_switcher/conversion`: layout maps, converter, heuristics
- `include/flamentine_switcher/ui`: tray icon, settings window, notifications
- `include/flamentine_switcher/services`: D-Bus and autostart services
- `docs/`: architecture notes, config format, D-Bus API, Wayland limitations, Caramba assessment

## X11 vs Wayland

X11 is still the primary target for the full MVP:

- current layout detection
- layout switching
- global hotkeys
- delayed automatic word replacement in explicitly allowed applications
- remembered layouts per window or per application

Wayland support in KDE Plasma is now usable for safe/manual flows:

- current layout detection through `org.kde.KeyboardLayouts`
- manual layout switching through `org.kde.KeyboardLayouts`
- tray and settings

Wayland limitations still remain:

- no global X11-style hotkeys in the current implementation
- no transparent low-level text interception

See [docs/wayland-limitations.md](docs/wayland-limitations.md) for details.

## License

MIT. See [LICENSE](LICENSE).
