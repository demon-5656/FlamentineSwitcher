#include "flamentine_switcher/ui/tray_icon.h"

#include <QAction>
#include <QMenu>
#include <QPainter>
#include <QPixmap>

namespace FlamentineSwitcher::Ui {

TrayIcon::TrayIcon(QObject* parent)
    : QObject(parent)
    , menu_(new QMenu()) {
    statusAction_ = menu_->addAction(QStringLiteral("Раскладка: --"));
    statusAction_->setEnabled(false);
    menu_->addSeparator();
    convertSelectionAction_ = menu_->addAction(QStringLiteral("Конвертировать выделение (Meta+End)"));
    openSettingsAction_ = menu_->addAction(QStringLiteral("Настройки"));
    menu_->addSeparator();
    quitAction_ = menu_->addAction(QStringLiteral("Выход"));

    trayIcon_.setContextMenu(menu_);
    rebuildIcon();
    rebuildMenuLabels();

    connect(convertSelectionAction_, &QAction::triggered, this, &TrayIcon::convertSelectionRequested);
    connect(openSettingsAction_, &QAction::triggered, this, &TrayIcon::openSettingsRequested);
    connect(quitAction_, &QAction::triggered, this, &TrayIcon::quitRequested);
    connect(&trayIcon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            emit convertSelectionRequested();
        }
    });
}

void TrayIcon::setCurrentLayout(const QString& layoutId) {
    currentLayout_ = layoutId.isEmpty() ? QStringLiteral("--") : layoutId.left(2).toUpper();
    rebuildMenuLabels();
    rebuildIcon();
}

void TrayIcon::setCurrentTargetContext(const FlamentineSwitcher::Core::WindowContext& context, const QString& backendStatus) {
    Q_UNUSED(context);
    Q_UNUSED(backendStatus);
}

void TrayIcon::setEnabledState(const bool enabled) {
    enabled_ = enabled;
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
    statusAction_->setText(QStringLiteral("Раскладка: %1").arg(currentLayout_));
}

}  // namespace FlamentineSwitcher::Ui