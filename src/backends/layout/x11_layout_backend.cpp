#include "flamentine_switcher/backends/layout/x11_layout_backend.h"

#include <cstdlib>

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XKBrules.h>

namespace {

QStringList splitXkbList(const char* value) {
    if (value == nullptr) {
        return {};
    }

    QStringList items = QString::fromUtf8(value).split(',', Qt::KeepEmptyParts);
    for (QString& item : items) {
        item = item.trimmed();
    }
    return items;
}

}  // namespace

namespace FlamentineSwitcher::Backends::Layout {

X11LayoutBackend::X11LayoutBackend(QObject* parent)
    : ILayoutBackend(parent) {
    display_ = XOpenDisplay(nullptr);
    if (display_ == nullptr) {
        lastError_ = QStringLiteral("Unable to connect to the X11 display");
    }
}

X11LayoutBackend::~X11LayoutBackend() {
    if (display_ != nullptr) {
        XCloseDisplay(display_);
    }
}

bool X11LayoutBackend::isSupported() const {
    return display_ != nullptr;
}

QString X11LayoutBackend::backendName() const {
    return QStringLiteral("x11-xkb");
}

QList<FlamentineSwitcher::Core::LayoutInfo> X11LayoutBackend::listLayouts() const {
    refreshCachedLayouts();
    return cachedLayouts_;
}

QString X11LayoutBackend::currentLayoutId() const {
    if (display_ == nullptr) {
        return {};
    }

    if (!refreshCachedLayouts()) {
        return {};
    }

    XkbStateRec state {};
    if (XkbGetState(display_, XkbUseCoreKbd, &state) != Success) {
        lastError_ = QStringLiteral("Failed to query XKB state");
        return {};
    }

    const int group = qBound(0, static_cast<int>(state.group), cachedLayouts_.size() - 1);
    return cachedLayouts_.at(group).id;
}

bool X11LayoutBackend::setLayout(const QString& layoutId) {
    if (!refreshCachedLayouts()) {
        return false;
    }

    for (int index = 0; index < cachedLayouts_.size(); ++index) {
        if (cachedLayouts_.at(index).id != layoutId) {
            continue;
        }

        if (!XkbLockGroup(display_, XkbUseCoreKbd, index)) {
            lastError_ = QStringLiteral("Failed to switch to layout '%1'").arg(layoutId);
            return false;
        }

        XFlush(display_);
        emit layoutChanged(layoutId);
        lastError_.clear();
        return true;
    }

    lastError_ = QStringLiteral("Layout '%1' is not configured in X11").arg(layoutId);
    return false;
}

bool X11LayoutBackend::toggleLayout() {
    if (!refreshCachedLayouts() || cachedLayouts_.isEmpty()) {
        return false;
    }

    const QString current = currentLayoutId();
    int currentIndex = 0;
    for (int index = 0; index < cachedLayouts_.size(); ++index) {
        if (cachedLayouts_.at(index).id == current) {
            currentIndex = index;
            break;
        }
    }

    const int nextIndex = (currentIndex + 1) % cachedLayouts_.size();
    return setLayout(cachedLayouts_.at(nextIndex).id);
}

QString X11LayoutBackend::lastError() const {
    return lastError_;
}

bool X11LayoutBackend::refreshCachedLayouts() const {
    cachedLayouts_.clear();

    if (display_ == nullptr) {
        lastError_ = QStringLiteral("X11 display is not available");
        return false;
    }

    char* rulesFile = nullptr;
    XkbRF_VarDefsRec definitions {};
    if (!XkbRF_GetNamesProp(display_, &rulesFile, &definitions)) {
        lastError_ = QStringLiteral("Unable to query XKB rules from the X server");
        return false;
    }

    const QStringList layouts = splitXkbList(definitions.layout);
    const QStringList variants = splitXkbList(definitions.variant);

    for (int index = 0; index < layouts.size(); ++index) {
        const QString layout = layouts.at(index);
        if (layout.isEmpty()) {
            continue;
        }

        const QString variant = index < variants.size() ? variants.at(index) : QString();
        const QString displayName = variant.isEmpty()
            ? layout
            : QStringLiteral("%1(%2)").arg(layout, variant);
        cachedLayouts_.append({layout, displayName});
    }

    if (rulesFile != nullptr) {
        std::free(rulesFile);
    }
    XkbRF_FreeVarDefs(&definitions, False);

    if (cachedLayouts_.isEmpty()) {
        lastError_ = QStringLiteral("No configured X11 layouts were found");
        return false;
    }

    lastError_.clear();
    return true;
}

}  // namespace FlamentineSwitcher::Backends::Layout

