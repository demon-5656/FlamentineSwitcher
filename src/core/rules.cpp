#include "flamentine_switcher/core/rules.h"

#include <QRegularExpression>

#include "flamentine_switcher/utils/process_info.h"

namespace FlamentineSwitcher::Core {

bool Rules::matchesAnyPattern(const QString& value, const QStringList& patterns) {
    if (value.trimmed().isEmpty()) {
        return false;
    }

    for (const QString& pattern : patterns) {
        const QString normalized = pattern.trimmed();
        if (normalized.isEmpty()) {
            continue;
        }

        const QRegularExpression expression(
            QRegularExpression::wildcardToRegularExpression(normalized),
            QRegularExpression::CaseInsensitiveOption);
        if (expression.match(value).hasMatch()) {
            return true;
        }
    }

    return false;
}

bool Rules::isExcluded(const AppConfig& config, const WindowContext& context) {
    if (config.excludeFullscreen && context.fullscreen) {
        return true;
    }

    if (matchesAnyPattern(context.appName, config.excludedApps)) {
        return true;
    }

    if (matchesAnyPattern(context.windowClass, config.excludedWindowClasses)) {
        return true;
    }

    return config.excludeTerminals && Utils::ProcessInfo::isTerminalApp(context.appName);
}

}  // namespace FlamentineSwitcher::Core

