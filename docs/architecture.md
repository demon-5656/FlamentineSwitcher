# Architecture Overview

FlamentineSwitcher is split into clear layers so platform code can evolve without rewriting the whole application.

## Layers

### Core

- `AppConfig` and related data models hold persistent settings.
- `SettingsManager` loads, validates and stores JSON configuration.
- `ApplicationController` coordinates UI, backends and service entry points.
- `Rules` contains future-safe logic for exclusions and window-based behavior.
- `LayoutMemory` keeps per-window and per-app layout state outside the backend layer and persists it through `SettingsManager`.

### Backends

- `ILayoutBackend` abstracts keyboard layout operations.
- `X11LayoutBackend` implements the MVP path through XKB/Xlib/Xkbfile.
- `WaylandLayoutBackend` is a partial backend that reports unsupported features instead of pretending.
- `IHotkeyBackend` abstracts global shortcuts.
- `X11HotkeyBackend` uses passive grabs on X11.
- `IWindowBackend` is reserved for per-window and per-app memory logic.
- `ITextInputBackend` abstracts typed-word observation and delayed replacement.
- `X11TextInputBackend` observes committed words on X11 and only keeps state for allowlisted targets.

### Conversion

- `LayoutMaps` owns physical-key layout maps.
- `TextConverter` performs transliteration-like key remapping without language translation.
- `LayoutHeuristics` estimates when a word was probably typed in the wrong layout.

### UI and Services

- `TrayIcon` exposes fast actions, current status, a live target inspector and separate quick allowlisting actions for app-only, `WM_CLASS`-only or both.
- `SettingsWindow` edits the full application config, shows the currently detected target metadata and can add that target to the allowlist.
- `Notifications` centralizes user-facing status messages.
- `DbusService` provides remote control entry points.
- `AutostartService` manages a desktop autostart entry in the user session.

## Runtime Flow

1. `main.cpp` creates the Qt application and loads config.
2. The controller selects session-appropriate backends.
3. Tray, settings, D-Bus and hotkeys are connected to controller slots.
4. The layout backend refreshes the current layout state and the tray reflects it.
5. Manual conversion requests still use the clipboard-safe workflow.
6. On X11, the text-input backend can observe committed words in allowed targets and schedule delayed replacement after a pause.
7. On X11, the controller can restore remembered layouts when focus moves between allowed windows or applications.
8. The settings window is refreshed with the latest detected target context so allowlist rules can be created without guessing process or `WM_CLASS` values.

## Why this split

- X11-specific code is isolated in backend implementations.
- Wayland limitations stay localized to backend capability checks.
- The conversion engine is testable without any UI.
- Settings format and migration logic stay independent from widgets.
