#pragma once

#include <QKeySequence>
#include <QMap>
#include <QObject>

#include "flamentine_switcher/core/models.h"

namespace FlamentineSwitcher::Backends::Hotkeys {

class IHotkeyBackend : public QObject {
    Q_OBJECT

public:
    explicit IHotkeyBackend(QObject* parent = nullptr)
        : QObject(parent) {
    }

    virtual ~IHotkeyBackend() = default;

    virtual bool isSupported() const = 0;
    virtual QString backendName() const = 0;
    virtual bool registerHotkeys(const QMap<FlamentineSwitcher::Core::HotkeyAction, QKeySequence>& shortcuts) = 0;
    virtual void unregisterHotkeys() = 0;
    virtual QString lastError() const = 0;

signals:
    void hotkeyTriggered(FlamentineSwitcher::Core::HotkeyAction action);
};

}  // namespace FlamentineSwitcher::Backends::Hotkeys

