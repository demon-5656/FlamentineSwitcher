#include "flamentine_switcher/backends/hotkeys/x11_hotkey_backend.h"

#include <array>

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

namespace {

KeySym keySymFromQtKey(const Qt::Key key) {
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        return XK_A + (key - Qt::Key_A);
    }
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return XK_0 + (key - Qt::Key_0);
    }
    if (key >= Qt::Key_F1 && key <= Qt::Key_F12) {
        return XK_F1 + (key - Qt::Key_F1);
    }

    switch (key) {
    case Qt::Key_Space:
        return XK_space;
    case Qt::Key_Pause:
        return XK_Pause;
    case Qt::Key_Tab:
        return XK_Tab;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return XK_Return;
    case Qt::Key_Escape:
        return XK_Escape;
    case Qt::Key_Backspace:
        return XK_BackSpace;
    case Qt::Key_Insert:
        return XK_Insert;
    case Qt::Key_Delete:
        return XK_Delete;
    case Qt::Key_Home:
        return XK_Home;
    case Qt::Key_End:
        return XK_End;
    case Qt::Key_PageUp:
        return XK_Page_Up;
    case Qt::Key_PageDown:
        return XK_Page_Down;
    case Qt::Key_Left:
        return XK_Left;
    case Qt::Key_Right:
        return XK_Right;
    case Qt::Key_Up:
        return XK_Up;
    case Qt::Key_Down:
        return XK_Down;
    default:
        return NoSymbol;
    }
}

unsigned int xModifiersFromQt(const Qt::KeyboardModifiers modifiers) {
    unsigned int result = 0;
    if (modifiers.testFlag(Qt::ShiftModifier)) {
        result |= ShiftMask;
    }
    if (modifiers.testFlag(Qt::ControlModifier)) {
        result |= ControlMask;
    }
    if (modifiers.testFlag(Qt::AltModifier)) {
        result |= Mod1Mask;
    }
    if (modifiers.testFlag(Qt::MetaModifier)) {
        result |= Mod4Mask;
    }
    return result;
}

}  // namespace

namespace FlamentineSwitcher::Backends::Hotkeys {

X11HotkeyBackend::X11HotkeyBackend(QObject* parent)
    : IHotkeyBackend(parent) {
    display_ = XOpenDisplay(nullptr);
    if (display_ == nullptr) {
        lastError_ = QStringLiteral("Unable to connect to the X11 display for global hotkeys");
        return;
    }

    rootWindow_ = DefaultRootWindow(display_);
    numLockMask_ = resolveNumLockMask();
    notifier_ = new QSocketNotifier(ConnectionNumber(display_), QSocketNotifier::Read, this);
    connect(notifier_, &QSocketNotifier::activated, this, &X11HotkeyBackend::processXEvents);
}

X11HotkeyBackend::~X11HotkeyBackend() {
    unregisterHotkeys();
    if (display_ != nullptr) {
        XCloseDisplay(display_);
    }
}

bool X11HotkeyBackend::isSupported() const {
    return display_ != nullptr;
}

QString X11HotkeyBackend::backendName() const {
    return QStringLiteral("x11-xgrabkey");
}

bool X11HotkeyBackend::registerHotkeys(const QMap<FlamentineSwitcher::Core::HotkeyAction, QKeySequence>& shortcuts) {
    unregisterHotkeys();

    if (display_ == nullptr) {
        return false;
    }

    static const std::array<unsigned int, 4> extraMasks = {
        0U,
        LockMask,
        numLockMask_,
        static_cast<unsigned int>(LockMask | numLockMask_),
    };

    for (auto it = shortcuts.cbegin(); it != shortcuts.cend(); ++it) {
        if (it.value().isEmpty()) {
            continue;
        }

        Registration registration;
        if (!resolveShortcut(it.value(), registration)) {
            continue;
        }

        for (const unsigned int mask : extraMasks) {
            XGrabKey(display_,
                     registration.keycode,
                     registration.modifiers | mask,
                     rootWindow_,
                     False,
                     GrabModeAsync,
                     GrabModeAsync);
        }

        registrations_.insert(it.key(), registration);
        actionsBySignature_.insert(signature(registration.keycode, registration.modifiers), it.key());
    }

    XFlush(display_);
    return !registrations_.isEmpty();
}

void X11HotkeyBackend::unregisterHotkeys() {
    if (display_ == nullptr) {
        return;
    }

    static const std::array<unsigned int, 4> extraMasks = {
        0U,
        LockMask,
        numLockMask_,
        static_cast<unsigned int>(LockMask | numLockMask_),
    };

    for (auto it = registrations_.cbegin(); it != registrations_.cend(); ++it) {
        for (const unsigned int mask : extraMasks) {
            XUngrabKey(display_, it.value().keycode, it.value().modifiers | mask, rootWindow_);
        }
    }

    registrations_.clear();
    actionsBySignature_.clear();
    XFlush(display_);
}

QString X11HotkeyBackend::lastError() const {
    return lastError_;
}

void X11HotkeyBackend::processXEvents() {
    if (display_ == nullptr) {
        return;
    }

    while (XPending(display_) > 0) {
        XEvent event {};
        XNextEvent(display_, &event);
        if (event.type != KeyPress) {
            continue;
        }

        const XKeyEvent& keyEvent = event.xkey;
        const unsigned int modifiers = keyEvent.state & ~(LockMask | numLockMask_);
        const quint64 eventSignature = signature(keyEvent.keycode, modifiers);
        const auto it = actionsBySignature_.constFind(eventSignature);
        if (it != actionsBySignature_.constEnd()) {
            emit hotkeyTriggered(it.value());
        }
    }
}

quint64 X11HotkeyBackend::signature(const int keycode, const unsigned int modifiers) {
    return (static_cast<quint64>(keycode) << 32U) | static_cast<quint64>(modifiers);
}

unsigned int X11HotkeyBackend::resolveNumLockMask() const {
    if (display_ == nullptr) {
        return 0;
    }

    unsigned int result = 0;
    XModifierKeymap* modifiers = XGetModifierMapping(display_);
    if (modifiers == nullptr) {
        return 0;
    }

    const KeyCode numLockKeycode = XKeysymToKeycode(display_, XK_Num_Lock);
    for (int modifier = 0; modifier < 8; ++modifier) {
        for (int key = 0; key < modifiers->max_keypermod; ++key) {
            if (modifiers->modifiermap[modifier * modifiers->max_keypermod + key] == numLockKeycode) {
                result = (1U << modifier);
                break;
            }
        }
    }

    XFreeModifiermap(modifiers);
    return result;
}

bool X11HotkeyBackend::resolveShortcut(const QKeySequence& shortcut, Registration& registration) {
    if (shortcut.count() != 1) {
        lastError_ = QStringLiteral("Multi-chord shortcuts are not supported by the X11 backend");
        return false;
    }

    const QKeyCombination combination = shortcut[0];
    const KeySym keySym = keySymFromQtKey(combination.key());
    if (keySym == NoSymbol) {
        lastError_ = QStringLiteral("Unsupported hotkey: %1").arg(shortcut.toString(QKeySequence::PortableText));
        return false;
    }

    registration.keycode = XKeysymToKeycode(display_, keySym);
    registration.modifiers = xModifiersFromQt(combination.keyboardModifiers());

    if (registration.keycode == 0) {
        lastError_ = QStringLiteral("Unable to resolve keycode for: %1").arg(shortcut.toString(QKeySequence::PortableText));
        return false;
    }

    return true;
}

}  // namespace FlamentineSwitcher::Backends::Hotkeys

