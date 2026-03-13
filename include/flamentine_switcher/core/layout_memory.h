#pragma once

#include <optional>

#include <QHash>
#include <QString>

#include "flamentine_switcher/core/config.h"
#include "flamentine_switcher/core/models.h"

namespace FlamentineSwitcher::Core {

class LayoutMemory {
public:
    void remember(const AppConfig& config, const WindowContext& context, const QString& layoutId);
    std::optional<QString> recall(const AppConfig& config, const WindowContext& context) const;
    void clear();

private:
    static QString windowKey(const WindowContext& context);
    static QString appKey(const WindowContext& context);

    QHash<QString, QString> layoutsByWindow_;
    QHash<QString, QString> layoutsByApp_;
};

}  // namespace FlamentineSwitcher::Core
