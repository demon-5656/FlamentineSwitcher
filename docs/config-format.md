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
    "heuristicsEnabled": true
  },
  "logging": {
    "level": "info",
    "fileEnabled": true
  }
}
```

