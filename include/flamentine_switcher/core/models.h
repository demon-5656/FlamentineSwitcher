#pragma once

#include <QString>

namespace FlamentineSwitcher::Core {

enum class HotkeyAction {
    ToggleLayout,
    ConvertLastWord,
    ConvertSelection,
    OpenSettings,
    ToggleEnabled,
};

struct LayoutInfo {
    QString id;
    QString displayName;
};

struct WindowContext {
    QString windowId;
    QString windowClass;
    QString appName;
    bool fullscreen = false;
};

}  // namespace FlamentineSwitcher::Core

Q_DECLARE_METATYPE(FlamentineSwitcher::Core::HotkeyAction)
Q_DECLARE_METATYPE(FlamentineSwitcher::Core::LayoutInfo)
Q_DECLARE_METATYPE(FlamentineSwitcher::Core::WindowContext)

