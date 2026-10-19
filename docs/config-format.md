# Конфигурация

Конфигурация находится в:

```text
$XDG_CONFIG_HOME/FlamentineSwitcher/config.json
```

Версия 2 сохраняет только настройки, которые действительно нужны для KDE Wayland:

```json
{
  "version": 2,
  "notificationsEnabled": true,
  "autoStart": false,
  "conversion": {
    "switchLayoutAfterConversion": true
  }
}
```

Старые поля конфигурации (allowlist, исключения приложений, память раскладки, отладочные логи и комбинации X11) игнорируются. После применения настроек они не записываются снова.