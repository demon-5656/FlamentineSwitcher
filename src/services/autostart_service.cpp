#include "flamentine_switcher/services/autostart_service.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSaveFile>
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
    if (file.exists() && !file.remove()) {
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

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        lastError_ = QStringLiteral("Failed to open autostart file: %1").arg(file.errorString());
        return false;
    }

    const QString executable = QCoreApplication::applicationFilePath();
    if (executable.isEmpty()) {
        lastError_ = QStringLiteral("Unable to determine the FlamentineSwitcher executable path");
        return false;
    }

    const QString desktopEntry = QStringLiteral(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=FlamentineSwitcher\n"
        "Comment=Keyboard layout switcher and text conversion utility\n"
        "Exec=%1\n"
        "Icon=input-keyboard\n"
        "OnlyShowIn=KDE;\n"
        "X-KDE-autostart-phase=2\n"
        "Terminal=false\n"
        "NoDisplay=true\n")
                                     .arg(executable);
    if (file.write(desktopEntry.toUtf8()) != desktopEntry.toUtf8().size() || !file.commit()) {
        lastError_ = QStringLiteral("Failed to save autostart file: %1").arg(file.errorString());
        return false;
    }
    return true;
}

}  // namespace FlamentineSwitcher::Services
