#include "flamentine_switcher/utils/process_info.h"

#include <QSet>

namespace FlamentineSwitcher::Utils::ProcessInfo {

SessionType currentSessionType() {
    const QString sessionType = QString::fromUtf8(qgetenv("XDG_SESSION_TYPE")).trimmed().toLower();
    if (sessionType == QStringLiteral("x11")) {
        return SessionType::X11;
    }
    if (sessionType == QStringLiteral("wayland")) {
        return SessionType::Wayland;
    }
    return SessionType::Unknown;
}

QString sessionTypeName(SessionType type) {
    switch (type) {
    case SessionType::X11:
        return QStringLiteral("x11");
    case SessionType::Wayland:
        return QStringLiteral("wayland");
    case SessionType::Unknown:
    default:
        return QStringLiteral("unknown");
    }
}

bool isTerminalApp(const QString& appName) {
    static const QSet<QString> terminalApps = {
        QStringLiteral("alacritty"),
        QStringLiteral("foot"),
        QStringLiteral("gnome-terminal"),
        QStringLiteral("kitty"),
        QStringLiteral("konsole"),
        QStringLiteral("wezterm"),
        QStringLiteral("xterm"),
        QStringLiteral("yakuake"),
    };

    return terminalApps.contains(appName.trimmed().toLower());
}

}  // namespace FlamentineSwitcher::Utils::ProcessInfo

