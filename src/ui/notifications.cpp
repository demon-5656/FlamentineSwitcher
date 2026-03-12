#include "flamentine_switcher/ui/notifications.h"

#include <QDebug>
#include <QSystemTrayIcon>

namespace FlamentineSwitcher::Ui {

Notifications::Notifications(QSystemTrayIcon* trayIcon, QObject* parent)
    : QObject(parent)
    , trayIcon_(trayIcon) {
}

void Notifications::info(const QString& message) const {
    qInfo().noquote() << message;
    if (trayIcon_ != nullptr) {
        trayIcon_->showMessage(QStringLiteral("FlamentineSwitcher"), message, QSystemTrayIcon::Information, 3000);
    }
}

void Notifications::warning(const QString& message) const {
    qWarning().noquote() << message;
    if (trayIcon_ != nullptr) {
        trayIcon_->showMessage(QStringLiteral("FlamentineSwitcher"), message, QSystemTrayIcon::Warning, 4000);
    }
}

void Notifications::error(const QString& message) const {
    qCritical().noquote() << message;
    if (trayIcon_ != nullptr) {
        trayIcon_->showMessage(QStringLiteral("FlamentineSwitcher"), message, QSystemTrayIcon::Critical, 5000);
    }
}

}  // namespace FlamentineSwitcher::Ui

