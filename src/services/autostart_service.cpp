#include "flamentine_switcher/services/autostart_service.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace FlamentineSwitcher::Services {

bool AutostartService::isEnabled() const {
    return QFile::exists(desktopFilePath());
}

bool AutostartService::setEnabled(const bool enabled) const {
    lastError_.clear();

    if (enabled) {
        return writeDesktopFile();
    }

    QFile file(desktopFilePath());
    if (!file.exists()) {
        return true;
    }

    if (!file.remove()) {
        lastError_ = QStringLiteral("Failed to remove autostart entry: %1").arg(file.errorString());
        return false;
    }

    return true;
}

QString AutostartService::desktopFilePath() const {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/autostart/org.flamentineswitcher.desktop");
}

QString AutostartService::lastError() const {
    return lastError_;
}

bool AutostartService::writeDesktopFile() const {
    const QString path = desktopFilePath();
    QFileInfo info(path);
    QDir().mkpath(info.dir().absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        lastError_ = QStringLiteral("Failed to open autostart file: %1").arg(file.errorString());
        return false;
    }

    file.write(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=FlamentineSwitcher\n"
        "Comment=Keyboard layout switcher and text conversion utility\n"
        "Exec=flamentine-switcher\n"
        "Icon=input-keyboard\n"
        "OnlyShowIn=KDE;\n"
        "X-KDE-autostart-phase=2\n");
    file.close();
    return true;
}

}  // namespace FlamentineSwitcher::Services

