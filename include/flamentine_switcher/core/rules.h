#pragma once

#include "flamentine_switcher/core/config.h"
#include "flamentine_switcher/core/models.h"

namespace FlamentineSwitcher::Core {

class Rules {
public:
    static bool matchesAnyPattern(const QString& value, const QStringList& patterns);
    static bool isExcluded(const AppConfig& config, const WindowContext& context);
    static bool isAllowed(const AppConfig& config, const WindowContext& context);
};

}  // namespace FlamentineSwitcher::Core
