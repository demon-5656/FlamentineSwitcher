# D-Bus API

Service name:

```text
org.flamentineswitcher.Control
```

Object path:

```text
/org/flamentineswitcher/Control
```

Interface:

```text
org.flamentineswitcher.Control
```

## Methods

- `GetCurrentLayout() -> s`
- `SetLayout(layoutId: s) -> b`
- `ToggleLayout() -> b`
- `ConvertLastWord() -> s`
- `ConvertSelection() -> s` — transliterates and replaces the active selection.  On Wayland this requires a `ydotoold` daemon for `Ctrl+C`/`Ctrl+V` injection.
- `Enable() -> b`
- `Disable() -> b`
- `OpenSettings() -> b`

## Signals

- `LayoutChanged(layoutId: s)`
- `EnabledChanged(enabled: b)`
