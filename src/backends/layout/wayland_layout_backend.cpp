#include "flamentine_switcher/backends/layout/wayland_layout_backend.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>

namespace {

constexpr auto kKdeKeyboardService = "org.kde.keyboard";
constexpr auto kKdeKeyboardPath = "/Layouts";
constexpr auto kKdeKeyboardInterface = "org.kde.KeyboardLayouts";

QDBusInterface makeKeyboardInterface() {
    return QDBusInterface(QString::fromLatin1(kKdeKeyboardService),
                          QString::fromLatin1(kKdeKeyboardPath),
                          QString::fromLatin1(kKdeKeyboardInterface),
                          QDBusConnection::sessionBus());
}

}  // namespace

namespace FlamentineSwitcher::Backends::Layout {

WaylandLayoutBackend::WaylandLayoutBackend(QObject* parent)
    : ILayoutBackend(parent)
    , lastError_(QStringLiteral("KDE keyboard layout service is not available")) {
    QDBusConnection::sessionBus().connect(QString::fromLatin1(kKdeKeyboardService),
                                          QString::fromLatin1(kKdeKeyboardPath),
                                          QString::fromLatin1(kKdeKeyboardInterface),
                                          QStringLiteral("layoutChanged"),
                                          this,
                                          SLOT(handleKdeLayoutChanged(uint)));
    QDBusConnection::sessionBus().connect(QString::fromLatin1(kKdeKeyboardService),
                                          QString::fromLatin1(kKdeKeyboardPath),
                                          QString::fromLatin1(kKdeKeyboardInterface),
                                          QStringLiteral("layoutListChanged"),
                                          this,
                                          SLOT(handleKdeLayoutListChanged()));
}

bool WaylandLayoutBackend::isSupported() const {
    return makeKeyboardInterface().isValid();
}

QString WaylandLayoutBackend::backendName() const {
    return QStringLiteral("plasma-dbus");
}

QList<FlamentineSwitcher::Core::LayoutInfo> WaylandLayoutBackend::listLayouts() const {
    refreshLayouts();
    return cachedLayouts_;
}

QString WaylandLayoutBackend::currentLayoutId() const {
    if (!refreshLayouts()) {
        return {};
    }

    const int index = currentLayoutIndex();
    if (index < 0 || index >= cachedLayouts_.size()) {
        lastError_ = QStringLiteral("Current layout index is out of bounds");
        return {};
    }

    lastError_.clear();
    return cachedLayouts_.at(index).id;
}

bool WaylandLayoutBackend::setLayout(const QString& layoutId) {
    if (!refreshLayouts()) {
        return false;
    }

    int targetIndex = -1;
    for (int index = 0; index < cachedLayouts_.size(); ++index) {
        if (cachedLayouts_.at(index).id == layoutId) {
            targetIndex = index;
            break;
        }
    }

    if (targetIndex < 0) {
        lastError_ = QStringLiteral("Layout '%1' is not configured in KDE Plasma").arg(layoutId);
        return false;
    }

    QDBusInterface keyboard = makeKeyboardInterface();
    if (!keyboard.isValid()) {
        lastError_ = QStringLiteral("KDE keyboard layout service is not available");
        return false;
    }

    const QDBusReply<bool> reply = keyboard.call(QStringLiteral("setLayout"), static_cast<uint>(targetIndex));
    if (!reply.isValid() || !reply.value()) {
        lastError_ = reply.isValid()
            ? QStringLiteral("KDE keyboard layout service rejected the layout change")
            : reply.error().message();
        return false;
    }

    lastError_.clear();
    emit layoutChanged(layoutId);
    return true;
}

bool WaylandLayoutBackend::toggleLayout() {
    QDBusInterface keyboard = makeKeyboardInterface();
    if (!keyboard.isValid()) {
        lastError_ = QStringLiteral("KDE keyboard layout service is not available");
        return false;
    }

    const QDBusMessage reply = keyboard.call(QStringLiteral("switchToNextLayout"));
    if (reply.type() == QDBusMessage::ErrorMessage) {
        lastError_ = reply.errorMessage();
        return false;
    }

    lastError_.clear();
    emit layoutChanged(currentLayoutId());
    return true;
}

QString WaylandLayoutBackend::lastError() const {
    return lastError_;
}

void WaylandLayoutBackend::handleKdeLayoutChanged(const uint index) {
    if (!refreshLayouts() || index >= static_cast<uint>(cachedLayouts_.size())) {
        emit layoutChanged(QString());
        return;
    }

    emit layoutChanged(cachedLayouts_.at(static_cast<int>(index)).id);
}

void WaylandLayoutBackend::handleKdeLayoutListChanged() {
    refreshLayouts();
    emit layoutChanged(currentLayoutId());
}

bool WaylandLayoutBackend::refreshLayouts() const {
    cachedLayouts_.clear();

    QDBusInterface keyboard = makeKeyboardInterface();
    if (!keyboard.isValid()) {
        lastError_ = QStringLiteral("KDE keyboard layout service is not available");
        return false;
    }

    const QDBusMessage reply = keyboard.call(QStringLiteral("getLayoutsList"));
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty()) {
        lastError_ = reply.type() == QDBusMessage::ErrorMessage
            ? reply.errorMessage()
            : QStringLiteral("KDE keyboard layout service returned no layouts");
        return false;
    }

    const QDBusArgument dbusArgument = reply.arguments().constFirst().value<QDBusArgument>();
    dbusArgument.beginArray();
    while (!dbusArgument.atEnd()) {
        QString id;
        QString variant;
        QString displayName;

        dbusArgument.beginStructure();
        dbusArgument >> id >> variant >> displayName;
        dbusArgument.endStructure();

        const QString resolvedDisplayName = variant.isEmpty()
            ? displayName
            : QStringLiteral("%1 (%2)").arg(displayName, variant);
        cachedLayouts_.append({id, resolvedDisplayName});
    }
    dbusArgument.endArray();

    if (cachedLayouts_.isEmpty()) {
        lastError_ = QStringLiteral("KDE Plasma reports no active keyboard layouts");
        return false;
    }

    lastError_.clear();
    return true;
}

int WaylandLayoutBackend::currentLayoutIndex() const {
    QDBusInterface keyboard = makeKeyboardInterface();
    if (!keyboard.isValid()) {
        lastError_ = QStringLiteral("KDE keyboard layout service is not available");
        return -1;
    }

    const QDBusReply<uint> reply = keyboard.call(QStringLiteral("getLayout"));
    if (!reply.isValid()) {
        lastError_ = reply.error().message();
        return -1;
    }

    return static_cast<int>(reply.value());
}

}  // namespace FlamentineSwitcher::Backends::Layout
