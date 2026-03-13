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
- tray icon and settings window
- text converter for `us <-> ru`
- D-Bus service skeleton
- unit tests for config, conversion and heuristics

The current text conversion workflow is intentionally conservative:

- selection conversion operates on X11 selection clipboard or regular clipboard
- last-word conversion currently transforms the last clipboard token
- when deny-by-default mode is enabled, conversion is blocked unless the active target is explicitly allowed
- no unsafe global keylogging is used
- `autoConvertDelayMs` is already stored in config, but live typed-text observation is still pending a dedicated X11-only backend

## Build

Install dependencies on Arch Linux:

```bash
sudo pacman -S --needed base-devel cmake qt6-base libx11 libxkbfile
```

Configure and build:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run:

```bash
./build/flamentine-switcher
```

## Architecture

- `include/flamentine_switcher/core`: config, settings manager, orchestration
- `include/flamentine_switcher/backends`: layout, hotkeys, window context abstractions
- `include/flamentine_switcher/conversion`: layout maps, converter, heuristics
- `include/flamentine_switcher/ui`: tray icon, settings window, notifications
- `include/flamentine_switcher/services`: D-Bus and autostart services
- `docs/`: architecture notes, config format, D-Bus API, Wayland limitations, Caramba assessment

## X11 vs Wayland

X11 is still the primary target for the full MVP:

- current layout detection
- layout switching
- global hotkeys

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
