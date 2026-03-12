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
- global X11 hotkey backend based on `XGrabKey`
- tray icon and settings window
- text converter for `us <-> ru`
- D-Bus service skeleton
- unit tests for config, conversion and heuristics

The current text conversion workflow is intentionally conservative:

- selection conversion operates on X11 selection clipboard or regular clipboard
- last-word conversion currently transforms the last clipboard token
- no unsafe global keylogging is used

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

X11 is the primary target for the MVP:

- current layout detection
- layout switching
- global hotkeys

Wayland is treated honestly:

- the application still starts
- tray and settings are available
- platform-limited features stay disabled until a safe API exists

See [docs/wayland-limitations.md](docs/wayland-limitations.md) for details.

## License

MIT. See [LICENSE](LICENSE).

