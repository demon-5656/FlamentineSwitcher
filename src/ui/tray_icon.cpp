#include "flamentine_switcher/ui/tray_icon.h"

#include <QAction>
#include <QFont>
#include <QMenu>
#include <QPainter>
#include <QPixmap>

namespace FlamentineSwitcher::Ui {

namespace {

QString displayValue(const QString& value) {
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? QStringLiteral("—") : trimmed;
}

}  // namespace

TrayIcon::TrayIcon(QObject* parent)
    : QObject(parent)
    , menu_(new QMenu()) {
    statusAction_ = menu_->addAction(QStringLiteral("Layout: --"));
    statusAction_->setEnabled(false);
    menu_->addSeparator();

    toggleEnabledAction_ = menu_->addAction(QStringLiteral("Enabled"));
    toggleEnabledAction_->setCheckable(true);
    toggleEnabledAction_->setChecked(true);

    toggleLayoutAction_ = menu_->addAction(QStringLiteral("Switch Layout"));
    currentTargetMenu_ = menu_->addMenu(QStringLiteral("Current Target"));
    currentTargetStatusAction_ = currentTargetMenu_->addAction(QStringLiteral("No focused target"));
    currentTargetAppAction_ = currentTargetMenu_->addAction(QStringLiteral("App: —"));
    currentTargetWindowClassAction_ = currentTargetMenu_->addAction(QStringLiteral("WM_CLASS: —"));
    currentTargetWindowIdAction_ = currentTargetMenu_->addAction(QStringLiteral("Window ID: —"));
    currentTargetFullscreenAction_ = currentTargetMenu_->addAction(QStringLiteral("Fullscreen: —"));
    currentTargetMenu_->addSeparator();
    allowCurrentAppAction_ = currentTargetMenu_->addAction(QStringLiteral("Allow Current App Only"));
    allowCurrentWindowClassAction_ = currentTargetMenu_->addAction(QStringLiteral("Allow Current WM_CLASS Only"));
    allowCurrentTargetAction_ = currentTargetMenu_->addAction(QStringLiteral("Allow App + WM_CLASS"));
    currentTargetStatusAction_->setEnabled(false);
    currentTargetAppAction_->setEnabled(false);
    currentTargetWindowClassAction_->setEnabled(false);
    currentTargetWindowIdAction_->setEnabled(false);
    currentTargetFullscreenAction_->setEnabled(false);
    convertLastWordAction_ = menu_->addAction(QStringLiteral("Convert Last Word"));
    convertSelectionAction_ = menu_->addAction(QStringLiteral("Convert Selection"));
    menu_->addSeparator();
    openSettingsAction_ = menu_->addAction(QStringLiteral("Settings"));
    quitAction_ = menu_->addAction(QStringLiteral("Quit"));

    trayIcon_.setContextMenu(menu_);
    rebuildIcon();
    rebuildMenuLabels();

    connect(toggleEnabledAction_, &QAction::toggled, this, &TrayIcon::enabledToggled);
    connect(toggleLayoutAction_, &QAction::triggered, this, &TrayIcon::toggleLayoutRequested);
    connect(allowCurrentAppAction_, &QAction::triggered, this, &TrayIcon::allowCurrentAppRequested);
    connect(allowCurrentWindowClassAction_, &QAction::triggered, this, &TrayIcon::allowCurrentWindowClassRequested);
    connect(allowCurrentTargetAction_, &QAction::triggered, this, &TrayIcon::allowCurrentTargetRequested);
    connect(convertLastWordAction_, &QAction::triggered, this, &TrayIcon::convertLastWordRequested);
    connect(convertSelectionAction_, &QAction::triggered, this, &TrayIcon::convertSelectionRequested);
    connect(openSettingsAction_, &QAction::triggered, this, &TrayIcon::openSettingsRequested);
    connect(quitAction_, &QAction::triggered, this, &TrayIcon::quitRequested);
    connect(&trayIcon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            emit toggleLayoutRequested();
        }
    });
}

void TrayIcon::setCurrentLayout(const QString& layoutId) {
    currentLayout_ = layoutId.isEmpty() ? QStringLiteral("--") : layoutId.left(2).toUpper();
    rebuildMenuLabels();
    rebuildIcon();
}

void TrayIcon::setCurrentTargetContext(const FlamentineSwitcher::Core::WindowContext& context, const QString& backendStatus) {
    currentTargetContext_ = context;
    currentTargetStatus_ = backendStatus.trimmed();
    rebuildMenuLabels();
}

void TrayIcon::setEnabledState(const bool enabled) {
    enabled_ = enabled;
    const bool previous = toggleEnabledAction_->blockSignals(true);
    toggleEnabledAction_->setChecked(enabled_);
    toggleEnabledAction_->blockSignals(previous);
    rebuildMenuLabels();
    rebuildIcon();
}

void TrayIcon::show() {
    trayIcon_.show();
}

QSystemTrayIcon* TrayIcon::systemTrayIcon() {
    return &trayIcon_;
}

void TrayIcon::rebuildIcon() {
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(enabled_ ? QColor(QStringLiteral("#2563eb")) : QColor(QStringLiteral("#6b7280")));
    painter.drawRoundedRect(QRectF(4.0, 4.0, 56.0, 56.0), 14.0, 14.0);

    QFont font;
    font.setBold(true);
    font.setPixelSize(26);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, currentLayout_);
    painter.end();

    trayIcon_.setIcon(QIcon(pixmap));
    trayIcon_.setToolTip(QStringLiteral("FlamentineSwitcher [%1]").arg(currentLayout_));
}

void TrayIcon::rebuildMenuLabels() {
    const bool hasIdentifiedTarget = !currentTargetContext_.appName.trimmed().isEmpty()
        || !currentTargetContext_.windowClass.trimmed().isEmpty() || !currentTargetContext_.windowId.trimmed().isEmpty();

    statusAction_->setText(QStringLiteral("Layout: %1").arg(currentLayout_));
    toggleEnabledAction_->setText(enabled_ ? QStringLiteral("Enabled") : QStringLiteral("Disabled"));
    currentTargetStatusAction_->setText(
        hasIdentifiedTarget ? QStringLiteral("Detected focused target")
                            : (currentTargetStatus_.isEmpty() ? QStringLiteral("No focused target")
                                                              : QStringLiteral("Status: %1").arg(currentTargetStatus_)));
    currentTargetAppAction_->setText(QStringLiteral("App: %1").arg(displayValue(currentTargetContext_.appName)));
    currentTargetWindowClassAction_->setText(
        QStringLiteral("WM_CLASS: %1").arg(displayValue(currentTargetContext_.windowClass)));
    currentTargetWindowIdAction_->setText(QStringLiteral("Window ID: %1").arg(displayValue(currentTargetContext_.windowId)));
    currentTargetFullscreenAction_->setText(
        QStringLiteral("Fullscreen: %1").arg(hasIdentifiedTarget ? (currentTargetContext_.fullscreen ? QStringLiteral("Yes")
                                                                                                     : QStringLiteral("No"))
                                                                 : QStringLiteral("—")));

    const bool hasApp = !currentTargetContext_.appName.trimmed().isEmpty();
    const bool hasWindowClass = !currentTargetContext_.windowClass.trimmed().isEmpty();
    allowCurrentAppAction_->setEnabled(hasApp);
    allowCurrentWindowClassAction_->setEnabled(hasWindowClass);
    allowCurrentTargetAction_->setEnabled(hasApp || hasWindowClass);
}

}  // namespace FlamentineSwitcher::Ui
