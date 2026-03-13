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

bool Rules::isAllowed(const AppConfig& config, const WindowContext& context) {
    if (isExcluded(config, context)) {
        return false;
    }

    if (!config.requireAllowedTargets) {
        return true;
    }

    const bool hasKnownTarget =
        !context.appName.trimmed().isEmpty() || !context.windowClass.trimmed().isEmpty() || !context.windowId.trimmed().isEmpty();
    if (!hasKnownTarget) {
        return false;
    }

    if (matchesAnyPattern(context.appName, config.allowedApps)) {
        return true;
    }

    if (matchesAnyPattern(context.windowClass, config.allowedWindowClasses)) {
        return true;
    }

    return false;
}

}  // namespace FlamentineSwitcher::Core
