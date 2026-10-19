# KDE Plasma 6 Wayland

## Первичная проверка

После установки запустите:

```bash
systemctl --user enable --now ydotool.service
flamentine-switcher --check-wayland
```

Перед preflight запустите **FlamentineSwitcher** из меню Plasma или включите его автозапуск. Успешный preflight подтверждает `ydotool`, его виртуальное устройство ввода, Klipper, службу раскладок Plasma, установленный shortcut launcher, KDE Global Shortcuts и D-Bus-сервис приложения.

## Горячая клавиша

Запущенный FlamentineSwitcher регистрирует действие **Convert selected text** через KDE Global Shortcuts. Для совместимости desktop launcher **Transliterate selected text** также вызывает:

```text
flamentine-switcher --transliterate-selection
```

По умолчанию назначается `Meta+End`. Если сочетание занято, измените его через **Параметры системы → Клавиатура → Сочетания клавиш**. Launcher обращается к уже запущенному FlamentineSwitcher по D-Bus и не создаёт второй фоновый процесс.

## Как работает конвертация

1. Пользователь выделяет текст.
2. Plasma shortcut запускает client-команду.
3. Основной процесс отправляет `Ctrl+C` через `ydotool`.
4. Получает текст через Klipper, преобразует физическую раскладку и вставляет результат `Ctrl+V`.
5. При включённой настройке переключает активную раскладку Plasma.

Если preflight проходит, но конкретная программа не заменяет текст, проверьте, принимает ли она обычные `Ctrl+C` и `Ctrl+V`.