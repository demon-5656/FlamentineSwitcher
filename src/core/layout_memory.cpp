#include "flamentine_switcher/core/layout_memory.h"

namespace {

QString normalizedKey(const QString& value) {
    return value.trimmed().toLower();
}

}  // namespace

namespace FlamentineSwitcher::Core {

void LayoutMemory::remember(const AppConfig& config, const WindowContext& context, const QString& layoutId) {
    const QString normalizedLayoutId = normalizedKey(layoutId);
    if (normalizedLayoutId.isEmpty()) {
        return;
    }

    if (config.rememberLayoutPerWindow) {
        const QString key = windowKey(context);
        if (!key.isEmpty()) {
            layoutsByWindow_.insert(key, normalizedLayoutId);
        }
    }

    if (config.rememberLayoutPerApp) {
        const QString key = appKey(context);
        if (!key.isEmpty()) {
            layoutsByApp_.insert(key, normalizedLayoutId);
        }
    }
}

std::optional<QString> LayoutMemory::recall(const AppConfig& config, const WindowContext& context) const {
    if (config.rememberLayoutPerWindow) {
        const QString key = windowKey(context);
        const auto windowIt = layoutsByWindow_.constFind(key);
        if (windowIt != layoutsByWindow_.cend()) {
            return windowIt.value();
        }
    }

    if (config.rememberLayoutPerApp) {
        const QString key = appKey(context);
        const auto appIt = layoutsByApp_.constFind(key);
        if (appIt != layoutsByApp_.cend()) {
            return appIt.value();
        }
    }

    return std::nullopt;
}

void LayoutMemory::clear() {
    layoutsByWindow_.clear();
    layoutsByApp_.clear();
}

QString LayoutMemory::windowKey(const WindowContext& context) {
    return normalizedKey(context.windowId);
}

QString LayoutMemory::appKey(const WindowContext& context) {
    const QString appName = normalizedKey(context.appName);
    if (!appName.isEmpty()) {
        return appName;
    }

    return normalizedKey(context.windowClass);
}

}  // namespace FlamentineSwitcher::Core
