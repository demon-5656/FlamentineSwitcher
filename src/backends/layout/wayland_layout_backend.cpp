#include "flamentine_switcher/backends/layout/wayland_layout_backend.h"

namespace FlamentineSwitcher::Backends::Layout {

WaylandLayoutBackend::WaylandLayoutBackend(QObject* parent)
    : ILayoutBackend(parent)
    , lastError_(QStringLiteral("Wayland layout backend is not implemented yet")) {
}

bool WaylandLayoutBackend::isSupported() const {
    return false;
}

QString WaylandLayoutBackend::backendName() const {
    return QStringLiteral("wayland-stub");
}

QList<FlamentineSwitcher::Core::LayoutInfo> WaylandLayoutBackend::listLayouts() const {
    return {};
}

QString WaylandLayoutBackend::currentLayoutId() const {
    return {};
}

bool WaylandLayoutBackend::setLayout(const QString& layoutId) {
    Q_UNUSED(layoutId)
    return false;
}

bool WaylandLayoutBackend::toggleLayout() {
    return false;
}

QString WaylandLayoutBackend::lastError() const {
    return lastError_;
}

}  // namespace FlamentineSwitcher::Backends::Layout

