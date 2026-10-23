# FlamentineSwitcher

FlamentineSwitcher исправляет текст, набранный в неправильной раскладке, в KDE Plasma 6 Wayland.

Готовые релизные сборки рассчитаны на обычный `x86-64` без AVX, AVX2 и AVX-512.

Рабочий сценарий один: выделите текст и нажмите `Meta+End`. Приложение копирует выделение, преобразует его между русской и английской физическими раскладками и вставляет обратно. При включённой настройке оно также переключает раскладку Plasma под результат.

## Что есть в приложении

- KDE Plasma 6 Wayland-first;
- один action: **Конвертировать выделение** (`Meta+End`);
- автозапуск Plasma;
- уведомления;
- опция переключения раскладки после преобразования;
- проверка окружения: `flamentine-switcher --check-wayland`.

При запуске FlamentineSwitcher регистрирует `Meta+End` через KDE Global Shortcuts. Если сочетание занято, назначьте другое в **Параметры системы → Клавиатура → Сочетания клавиш** для действия *Convert selected text*.

## Установка на Arch / CachyOS

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base kglobalaccel libx11 libxkbfile libxtst ydotool
git clone https://github.com/demon-5656/FlamentineSwitcher.git
cd FlamentineSwitcher
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build
cmake --install build
systemctl --user enable --now ydotool.service
flamentine-switcher &
flamentine-switcher --check-wayland
```

После успешной проверки назначьте `Meta+End` в **Параметры системы → Клавиатура → Сочетания клавиш**, если Plasma не подхватила сочетание автоматически. В настройках включите «Запускать вместе с Plasma», если нужен постоянный автозапуск.

### Готовая сборка из релиза

Архив `flamentine-switcher-<version>-linux-x86_64.tar.gz` содержит готовую установку в `/usr`:

```bash
sudo tar -xzf flamentine-switcher-<version>-linux-x86_64.tar.gz -C /
kbuildsycoca6
systemctl --user enable --now ydotool.service
flamentine-switcher &
flamentine-switcher --check-wayland
```

Архив содержит бинарник и документацию. Зависимости Plasma и `ydotool` он не заменяет. Для Arch/CachyOS предпочтительнее установить приложенный `.pkg.tar.zst` через `sudo pacman -U`.

## Сборка и проверки

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
QT_QPA_PLATFORM=offscreen ./build/flamentine-switcher --check-wayland
```

## Ограничения

Wayland не допускает скрытый перехват и замену текста в любых приложениях. Поэтому FlamentineSwitcher работает только по явному действию пользователя и использует обычные `Ctrl+C` / `Ctrl+V` через `ydotool`. Нужны запущенный `ydotoold`, Klipper, служба раскладок и KDE Global Shortcuts. Команда preflight также проверяет установленный launcher и D-Bus-сервис уже запущенного приложения.

## Лицензия

MIT. См. [LICENSE](LICENSE).