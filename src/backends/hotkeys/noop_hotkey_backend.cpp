#include "flamentine_switcher/backends/hotkeys/noop_hotkey_backend.h"

namespace FlamentineSwitcher::Backends::Hotkeys {

NoopHotkeyBackend::NoopHotkeyBackend(QObject* parent)
    : IHotkeyBackend(parent)
    , lastError_(QStringLiteral("Global hotkeys are unavailable in the current session")) {
}

bool NoopHotkeyBackend::isSupported() const {
    return false;
}

QString NoopHotkeyBackend::backendName() const {
    return QStringLiteral("noop");
}

bool NoopHotkeyBackend::registerHotkeys(const QMap<FlamentineSwitcher::Core::HotkeyAction, QKeySequence>& shortcuts) {
    Q_UNUSED(shortcuts)
    return false;
}

void NoopHotkeyBackend::unregisterHotkeys() {
}

QString NoopHotkeyBackend::lastError() const {
    return lastError_;
}

}  // namespace FlamentineSwitcher::Backends::Hotkeys

