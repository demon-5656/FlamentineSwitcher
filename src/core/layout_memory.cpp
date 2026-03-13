#include "flamentine_switcher/core/layout_memory.h"

#include <QJsonObject>

namespace {

QString normalizedKey(const QString& value) {
    return value.trimmed().toLower();
}

QHash<QString, QString> jsonObjectToHash(const QJsonValue& value) {
    QHash<QString, QString> entries;
    const QJsonObject object = value.toObject();
    for (auto it = object.begin(); it != object.end(); ++it) {
        const QString key = normalizedKey(it.key());
        const QString layoutId = normalizedKey(it.value().toString());
        if (!key.isEmpty() && !layoutId.isEmpty()) {
            entries.insert(key, layoutId);
        }
    }
    return entries;
}

QJsonObject hashToJsonObject(const QHash<QString, QString>& entries) {
    QJsonObject object;
    for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
        if (!it.key().isEmpty() && !it.value().isEmpty()) {
            object.insert(it.key(), it.value());
        }
    }
    return object;
}

}  // namespace

namespace FlamentineSwitcher::Core {

LayoutMemoryState LayoutMemoryState::fromJsonObject(const QJsonObject& object) {
    LayoutMemoryState state;
    state.layoutsByWindow = jsonObjectToHash(object.value(QStringLiteral("layoutsByWindow")));
    state.layoutsByApp = jsonObjectToHash(object.value(QStringLiteral("layoutsByApp")));
    return state;
}

QJsonObject LayoutMemoryState::toJsonObject() const {
    return {
        {QStringLiteral("layoutsByWindow"), hashToJsonObject(layoutsByWindow)},
        {QStringLiteral("layoutsByApp"), hashToJsonObject(layoutsByApp)},
    };
}

bool LayoutMemoryState::isEmpty() const {
    return layoutsByWindow.isEmpty() && layoutsByApp.isEmpty();
}

bool LayoutMemory::remember(const AppConfig& config, const WindowContext& context, const QString& layoutId) {
    const QString normalizedLayoutId = normalizedKey(layoutId);
    if (normalizedLayoutId.isEmpty()) {
        return false;
    }

    bool changed = false;
    if (config.rememberLayoutPerWindow) {
        const QString key = windowKey(context);
        if (!key.isEmpty()) {
            const auto current = layoutsByWindow_.constFind(key);
            if (current == layoutsByWindow_.cend() || current.value() != normalizedLayoutId) {
                layoutsByWindow_.insert(key, normalizedLayoutId);
                changed = true;
            }
        }
    }

    if (config.rememberLayoutPerApp) {
        const QString key = appKey(context);
        if (!key.isEmpty()) {
            const auto current = layoutsByApp_.constFind(key);
            if (current == layoutsByApp_.cend() || current.value() != normalizedLayoutId) {
                layoutsByApp_.insert(key, normalizedLayoutId);
                changed = true;
            }
        }
    }

    return changed;
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

void LayoutMemory::restoreState(const LayoutMemoryState& state) {
    layoutsByWindow_ = state.layoutsByWindow;
    layoutsByApp_ = state.layoutsByApp;
}

LayoutMemoryState LayoutMemory::exportState() const {
    return {layoutsByWindow_, layoutsByApp_};
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
