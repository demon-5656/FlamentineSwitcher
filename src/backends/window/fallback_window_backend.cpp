#include "flamentine_switcher/backends/window/fallback_window_backend.h"

namespace FlamentineSwitcher::Backends::Window {

FallbackWindowBackend::FallbackWindowBackend(QObject* parent)
    : IWindowBackend(parent)
    , lastError_(QStringLiteral("Window context backend is not available in this build")) {
}

bool FallbackWindowBackend::isSupported() const {
    return false;
}

QString FallbackWindowBackend::backendName() const {
    return QStringLiteral("fallback");
}

FlamentineSwitcher::Core::WindowContext FallbackWindowBackend::currentContext() const {
    return {};
}

QString FallbackWindowBackend::lastError() const {
    return lastError_;
}

}  // namespace FlamentineSwitcher::Backends::Window

