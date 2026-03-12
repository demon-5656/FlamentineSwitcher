#pragma once

#include "flamentine_switcher/backends/hotkeys/ihotkey_backend.h"

namespace FlamentineSwitcher::Backends::Hotkeys {

class NoopHotkeyBackend : public IHotkeyBackend {
    Q_OBJECT

public:
    explicit NoopHotkeyBackend(QObject* parent = nullptr);

    bool isSupported() const override;
    QString backendName() const override;
    bool registerHotkeys(const QMap<FlamentineSwitcher::Core::HotkeyAction, QKeySequence>& shortcuts) override;
    void unregisterHotkeys() override;
    QString lastError() const override;

private:
    QString lastError_;
};

}  // namespace FlamentineSwitcher::Backends::Hotkeys

