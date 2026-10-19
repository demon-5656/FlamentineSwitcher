#include "flamentine_switcher/backends/text/x11_text_input_backend.h"

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>

namespace FlamentineSwitcher::Backends::Text {

X11TextInputBackend::X11TextInputBackend(QObject* parent)
    : ITextInputBackend(parent) {
    display_ = XOpenDisplay(nullptr);
    if (display_ == nullptr) {
        lastError_ = QStringLiteral("Unable to connect to the X11 display for selection conversion");
    }
}

X11TextInputBackend::~X11TextInputBackend() {
    if (display_ != nullptr) {
        XCloseDisplay(display_);
    }
}

bool X11TextInputBackend::isSupported() const {
    return display_ != nullptr;
}

QString X11TextInputBackend::backendName() const {
    return QStringLiteral("x11-text-input");
}

bool X11TextInputBackend::copyActiveSelection() {
    if (display_ == nullptr) {
        lastError_ = QStringLiteral("X11 text input backend is unavailable");
        return false;
    }

    const KeyCode controlKeycode = XKeysymToKeycode(display_, XK_Control_L);
    const KeyCode cKeycode = XKeysymToKeycode(display_, XK_c);
    if (controlKeycode == 0 || cKeycode == 0) {
        lastError_ = QStringLiteral("Unable to resolve Ctrl+C on the X11 server");
        return false;
    }
    XTestFakeKeyEvent(display_, controlKeycode, True, CurrentTime);
    XTestFakeKeyEvent(display_, cKeycode, True, CurrentTime);
    XTestFakeKeyEvent(display_, cKeycode, False, CurrentTime);
    XTestFakeKeyEvent(display_, controlKeycode, False, CurrentTime);
    XFlush(display_);
    lastError_.clear();
    return true;
}

bool X11TextInputBackend::replaceActiveSelection() {
    if (display_ == nullptr) {
        lastError_ = QStringLiteral("X11 text input backend is unavailable");
        return false;
    }

    const KeyCode controlKeycode = XKeysymToKeycode(display_, XK_Control_L);
    const KeyCode vKeycode = XKeysymToKeycode(display_, XK_v);
    if (controlKeycode == 0 || vKeycode == 0) {
        lastError_ = QStringLiteral("Unable to resolve Ctrl+V on the X11 server");
        return false;
    }

    XTestFakeKeyEvent(display_, controlKeycode, True, CurrentTime);
    XTestFakeKeyEvent(display_, vKeycode, True, CurrentTime);
    XTestFakeKeyEvent(display_, vKeycode, False, CurrentTime);
    XTestFakeKeyEvent(display_, controlKeycode, False, CurrentTime);
    XFlush(display_);
    lastError_.clear();
    return true;
}

QString X11TextInputBackend::lastError() const {
    return lastError_;
}

}  // namespace FlamentineSwitcher::Backends::Text