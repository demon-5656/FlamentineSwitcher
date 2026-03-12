#include "flamentine_switcher/services/dbus_service.h"

#include <QDBusConnection>
#include <QDBusError>

#include "flamentine_switcher/core/application_controller.h"

namespace FlamentineSwitcher::Services {

DbusService::DbusService(FlamentineSwitcher::Core::ApplicationController& controller, QObject* parent)
    : QObject(parent)
    , controller_(controller) {
    connect(&controller_, &FlamentineSwitcher::Core::ApplicationController::layoutChanged, this, &DbusService::LayoutChanged);
    connect(&controller_, &FlamentineSwitcher::Core::ApplicationController::enabledChanged, this, &DbusService::EnabledChanged);
}

bool DbusService::start() {
    QDBusConnection connection = QDBusConnection::sessionBus();

    if (!connection.registerService(QStringLiteral("org.flamentineswitcher.Control"))) {
        lastError_ = connection.lastError().message();
        return false;
    }

    if (!connection.registerObject(QStringLiteral("/org/flamentineswitcher/Control"),
                                   this,
                                   QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
        lastError_ = connection.lastError().message();
        return false;
    }

    lastError_.clear();
    return true;
}

QString DbusService::lastError() const {
    return lastError_;
}

QString DbusService::GetCurrentLayout() const {
    return controller_.currentLayout();
}

bool DbusService::SetLayout(const QString& layoutId) {
    return controller_.setLayout(layoutId);
}

bool DbusService::ToggleLayout() {
    return controller_.toggleLayout();
}

QString DbusService::ConvertLastWord() {
    return controller_.convertLastWord();
}

QString DbusService::ConvertSelection() {
    return controller_.convertSelection();
}

bool DbusService::Enable() {
    return controller_.enable();
}

bool DbusService::Disable() {
    return controller_.disable();
}

bool DbusService::OpenSettings() {
    return controller_.openSettings();
}

}  // namespace FlamentineSwitcher::Services
