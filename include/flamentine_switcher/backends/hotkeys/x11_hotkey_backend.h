#pragma once

#include <QHash>
#include <QMap>
#include <QSocketNotifier>

#include "flamentine_switcher/backends/hotkeys/ihotkey_backend.h"

struct _XDisplay;

namespace FlamentineSwitcher::Backends::Hotkeys {

class X11HotkeyBackend : public IHotkeyBackend {
    Q_OBJECT

public:
    explicit X11HotkeyBackend(QObject* parent = nullptr);
    ~X11HotkeyBackend() override;

    bool isSupported() const override;
    QString backendName() const override;
    bool registerHotkeys(const QMap<FlamentineSwitcher::Core::HotkeyAction, QKeySequence>& shortcuts) override;
    void unregisterHotkeys() override;
    QString lastError() const override;

private slots:
    void processXEvents();

private:
    struct Registration {
        int keycode = 0;
        unsigned int modifiers = 0;
    };

    static quint64 signature(int keycode, unsigned int modifiers);
    unsigned int resolveNumLockMask() const;
    bool resolveShortcut(const QKeySequence& shortcut, Registration& registration);

    _XDisplay* display_ = nullptr;
    QSocketNotifier* notifier_ = nullptr;
    mutable QString lastError_;
    unsigned long rootWindow_ = 0;
    unsigned int numLockMask_ = 0;
    QMap<FlamentineSwitcher::Core::HotkeyAction, Registration> registrations_;
    QHash<quint64, FlamentineSwitcher::Core::HotkeyAction> actionsBySignature_;
};

}  // namespace FlamentineSwitcher::Backends::Hotkeys
