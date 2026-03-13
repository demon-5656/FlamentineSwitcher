#pragma once

#include <optional>

#include <QHash>
#include <QJsonObject>
#include <QString>

#include "flamentine_switcher/core/config.h"
#include "flamentine_switcher/core/models.h"

namespace FlamentineSwitcher::Core {

struct LayoutMemoryState {
    QHash<QString, QString> layoutsByWindow;
    QHash<QString, QString> layoutsByApp;

    static LayoutMemoryState fromJsonObject(const QJsonObject& object);
    QJsonObject toJsonObject() const;
    bool isEmpty() const;
};

class LayoutMemory {
public:
    bool remember(const AppConfig& config, const WindowContext& context, const QString& layoutId);
    std::optional<QString> recall(const AppConfig& config, const WindowContext& context) const;
    void clear();
    void restoreState(const LayoutMemoryState& state);
    LayoutMemoryState exportState() const;

private:
    static QString windowKey(const WindowContext& context);
    static QString appKey(const WindowContext& context);

    QHash<QString, QString> layoutsByWindow_;
    QHash<QString, QString> layoutsByApp_;
};

}  // namespace FlamentineSwitcher::Core
