# Config Format

The application stores its persistent state as JSON under the user config directory.

Default path:

```text
$XDG_CONFIG_HOME/FlamentineSwitcher/config.json
$XDG_CONFIG_HOME/FlamentineSwitcher/state.json
```

Example:

```json
{
  "version": 1,
  "enabled": true,
  "layouts": ["us", "ru"],
  "defaultLayout": "us",
  "rememberLayoutPerWindow": true,
  "rememberLayoutPerApp": false,
  "notificationsEnabled": true,
  "autoStart": false,
  "requireAllowedTargets": false,
  "allowedApps": [],
  "allowedWindowClasses": [],
  "hotkeys": {
    "toggleLayout": "Meta+Space",
    "convertLastWord": "Pause",
    "convertSelection": "Meta+End",
    "openSettings": "Meta+Alt+K",
    "toggleEnabled": "Meta+Alt+S"
  },
  "excludedApps": [],
  "excludedWindowClasses": [],
  "conversion": {
    "preserveCase": true,
    "preservePunctuation": true,
    "switchLayoutAfterConversion": true,
    "autoConvertEnabled": false,
    "heuristicsEnabled": true
  },
  "logging": {
    "level": "info",
    "fileEnabled": true
  }
}
```

`autoConvertEnabled` is retained only for compatibility with earlier configuration files.  It is ignored: FlamentineSwitcher converts only when its manual action is invoked.

`rememberLayoutPerWindow` and `rememberLayoutPerApp` control remembered layouts for allowed X11 targets. The user-facing settings live in `config.json`, while the last known per-window and per-app layout state is persisted separately in `state.json` so it survives restarts.

The settings window also exposes the currently detected target metadata so the allowlist can be filled from real `appName` and `WM_CLASS` values instead of guessing them manually.
