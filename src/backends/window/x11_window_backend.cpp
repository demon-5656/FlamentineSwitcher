#include "flamentine_switcher/backends/window/x11_window_backend.h"

#include <cstring>
#include <optional>

#include <QFile>
#include <QFileInfo>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace {

Atom internAtom(Display* display, const char* name) {
    return XInternAtom(display, name, False);
}

std::optional<QByteArray> readWindowProperty(Display* display, Window window, Atom property, Atom expectedType) {
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char* data = nullptr;

    const int status = XGetWindowProperty(display,
                                          window,
                                          property,
                                          0,
                                          1024,
                                          False,
                                          expectedType,
                                          &actualType,
                                          &actualFormat,
                                          &itemCount,
                                          &bytesAfter,
                                          &data);
    if (status != Success || actualType == None || data == nullptr) {
        if (data != nullptr) {
            XFree(data);
        }
        return std::nullopt;
    }

    const size_t bytes = actualFormat == 32 ? itemCount * sizeof(long) : itemCount * (actualFormat / 8);
    QByteArray result(reinterpret_cast<const char*>(data), static_cast<qsizetype>(bytes));
    XFree(data);
    return result;
}

QString processNameFromPid(const long pid) {
    if (pid <= 0) {
        return {};
    }

    QFile commFile(QStringLiteral("/proc/%1/comm").arg(pid));
    if (commFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString name = QString::fromUtf8(commFile.readAll()).trimmed();
        if (!name.isEmpty()) {
            return name;
        }
    }

    const QFileInfo exeInfo(QStringLiteral("/proc/%1/exe").arg(pid));
    return exeInfo.symLinkTarget().isEmpty() ? QString() : QFileInfo(exeInfo.symLinkTarget()).baseName();
}

bool hasStableIdentity(const FlamentineSwitcher::Core::WindowContext& context) {
    return !context.appName.trimmed().isEmpty() || !context.windowClass.trimmed().isEmpty();
}

}  // namespace

namespace FlamentineSwitcher::Backends::Window {

X11WindowBackend::X11WindowBackend(QObject* parent)
    : IWindowBackend(parent) {
    display_ = XOpenDisplay(nullptr);
    if (display_ == nullptr) {
        lastError_ = QStringLiteral("Unable to connect to the X11 display");
    }
}

X11WindowBackend::~X11WindowBackend() {
    if (display_ != nullptr) {
        XCloseDisplay(display_);
    }
}

bool X11WindowBackend::isSupported() const {
    return display_ != nullptr;
}

QString X11WindowBackend::backendName() const {
    return QStringLiteral("x11-active-window");
}

FlamentineSwitcher::Core::WindowContext X11WindowBackend::currentContext() const {
    using FlamentineSwitcher::Core::WindowContext;

    WindowContext context;
    if (display_ == nullptr) {
        lastError_ = QStringLiteral("X11 display is not available");
        return context;
    }

    const ::Window root = DefaultRootWindow(display_);
    const Atom activeWindowAtom = internAtom(display_, "_NET_ACTIVE_WINDOW");
    const auto activeWindowProperty = readWindowProperty(display_, root, activeWindowAtom, XA_WINDOW);
    if (!activeWindowProperty.has_value() || activeWindowProperty->size() < static_cast<int>(sizeof(::Window))) {
        lastError_ = QStringLiteral("Unable to query active X11 window");
        return context;
    }

    ::Window activeWindow = 0;
    std::memcpy(&activeWindow, activeWindowProperty->constData(), sizeof(::Window));
    if (activeWindow == 0) {
        lastError_ = QStringLiteral("X11 reported no active window");
        return context;
    }

    if (activeWindow == cachedWindow_ && hasStableIdentity(cachedContext_)) {
        lastError_.clear();
        return cachedContext_;
    }

    context.windowId = QStringLiteral("0x%1").arg(static_cast<qulonglong>(activeWindow), 0, 16);

    XClassHint classHint {};
    if (XGetClassHint(display_, activeWindow, &classHint) != 0) {
        if (classHint.res_class != nullptr) {
            context.windowClass = QString::fromLocal8Bit(classHint.res_class).trimmed();
            XFree(classHint.res_class);
        }
        if (classHint.res_name != nullptr) {
            if (context.appName.isEmpty()) {
                context.appName = QString::fromLocal8Bit(classHint.res_name).trimmed();
            }
            XFree(classHint.res_name);
        }
    }

    const Atom pidAtom = internAtom(display_, "_NET_WM_PID");
    const auto pidProperty = readWindowProperty(display_, activeWindow, pidAtom, XA_CARDINAL);
    if (pidProperty.has_value() && pidProperty->size() >= static_cast<int>(sizeof(long))) {
        long pid = 0;
        std::memcpy(&pid, pidProperty->constData(), sizeof(long));
        const QString processName = processNameFromPid(pid);
        if (!processName.isEmpty()) {
            context.appName = processName;
        }
    }

    const Atom stateAtom = internAtom(display_, "_NET_WM_STATE");
    const Atom fullscreenAtom = internAtom(display_, "_NET_WM_STATE_FULLSCREEN");
    const auto stateProperty = readWindowProperty(display_, activeWindow, stateAtom, XA_ATOM);
    if (stateProperty.has_value()) {
        const auto* atoms = reinterpret_cast<const Atom*>(stateProperty->constData());
        const int count = stateProperty->size() / static_cast<int>(sizeof(Atom));
        for (int index = 0; index < count; ++index) {
            if (atoms[index] == fullscreenAtom) {
                context.fullscreen = true;
                break;
            }
        }
    }

    if (context.appName.isEmpty()) {
        context.appName = context.windowClass;
    }

    if (hasStableIdentity(context)) {
        cachedWindow_ = activeWindow;
        cachedContext_ = context;
        lastError_.clear();
    } else {
        cachedWindow_ = 0;
        cachedContext_ = {};
        lastError_ = QStringLiteral("Unable to identify the active X11 window");
    }
    return context;
}

QString X11WindowBackend::lastError() const {
    return lastError_;
}

}  // namespace FlamentineSwitcher::Backends::Window
