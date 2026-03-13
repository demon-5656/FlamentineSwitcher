# Config Format

The application stores its persistent state as JSON under the user config directory.

Default path:

```text
$XDG_CONFIG_HOME/FlamentineSwitcher/config.json
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
  "requireAllowedTargets": true,
  "allowedApps": [
    "org.telegram.desktop",
    "code",
    "firefox"
  ],
  "allowedWindowClasses": [
    "code",
    "firefox"
  ],
  "hotkeys": {
    "toggleLayout": "Meta+Space",
    "convertLastWord": "Pause",
    "convertSelection": "Meta+Pause",
    "openSettings": "Meta+Alt+K",
    "toggleEnabled": "Meta+Alt+S"
  },
  "excludedApps": [
    "konsole",
    "alacritty",
    "steam_app_*"
  ],
  "excludedWindowClasses": [
    "yakuake"
  ],
  "conversion": {
    "preserveCase": true,
    "preservePunctuation": true,
    "autoConvertEnabled": false,
    "heuristicsEnabled": true,
    "autoConvertDelayMs": 450
  },
  "logging": {
    "level": "info",
    "fileEnabled": true
  }
}
```

`autoConvertDelayMs` is a policy field for the future delayed auto-correction pipeline. The current repository stores it in config and exposes it in settings, but live typed-text observation is not implemented yet.
