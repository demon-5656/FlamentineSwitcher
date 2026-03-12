#pragma once

#include <QString>

namespace FlamentineSwitcher::Utils::ProcessInfo {

enum class SessionType {
    X11,
    Wayland,
    Unknown,
};

SessionType currentSessionType();
QString sessionTypeName(SessionType type);
bool isTerminalApp(const QString& appName);

}  // namespace FlamentineSwitcher::Utils::ProcessInfo

