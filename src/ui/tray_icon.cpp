#include "flamentine_switcher/ui/tray_icon.h"

#include <QAction>
#include <QDateTime>
#include <QDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace FlamentineSwitcher::Ui {

namespace {

QString displayValue(const QString& value) {
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? QStringLiteral("—") : trimmed;
}

bool sameTargetContext(const FlamentineSwitcher::Core::WindowContext& left, const FlamentineSwitcher::Core::WindowContext& right) {
    return left.appName == right.appName && left.windowClass == right.windowClass && left.windowId == right.windowId
        && left.fullscreen == right.fullscreen;
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
    copyCurrentTargetInfoAction_ = menu_->addAction(QStringLiteral("Copy Target Info"));
    showTargetHistoryAction_ = menu_->addAction(QStringLiteral("Target History"));
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
    connect(copyCurrentTargetInfoAction_, &QAction::triggered, this, &TrayIcon::copyCurrentTargetInfoRequested);
    connect(showTargetHistoryAction_, &QAction::triggered, this, &TrayIcon::showTargetHistory);
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
    if (!sameTargetContext(currentTargetContext_, context) || currentTargetStatus_ != backendStatus.trimmed()) {
        appendTargetHistoryEntry(context, backendStatus);
    }
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
    copyCurrentTargetInfoAction_->setEnabled(hasIdentifiedTarget);
    showTargetHistoryAction_->setEnabled(!targetHistoryEntries_.isEmpty());
    allowCurrentAppAction_->setEnabled(hasApp);
    allowCurrentWindowClassAction_->setEnabled(hasWindowClass);
    allowCurrentTargetAction_->setEnabled(hasApp || hasWindowClass);
}

void TrayIcon::appendTargetHistoryEntry(const FlamentineSwitcher::Core::WindowContext& context, const QString& backendStatus) {
    QStringList fields;
    fields.append(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")));

    const QString status = backendStatus.trimmed();
    if (!context.appName.trimmed().isEmpty() || !context.windowClass.trimmed().isEmpty() || !context.windowId.trimmed().isEmpty()) {
        fields.append(QStringLiteral("app=%1").arg(displayValue(context.appName)));
        fields.append(QStringLiteral("class=%1").arg(displayValue(context.windowClass)));
        fields.append(QStringLiteral("window=%1").arg(displayValue(context.windowId)));
        fields.append(QStringLiteral("fullscreen=%1").arg(context.fullscreen ? QStringLiteral("yes") : QStringLiteral("no")));
    } else if (!status.isEmpty()) {
        fields.append(QStringLiteral("status=%1").arg(status));
    } else {
        fields.append(QStringLiteral("status=no focused target"));
    }

    targetHistoryEntries_.prepend(fields.join(QStringLiteral(" | ")));
    while (targetHistoryEntries_.size() > 30) {
        targetHistoryEntries_.removeLast();
    }
    refreshTargetHistoryDialog();
}

void TrayIcon::refreshTargetHistoryDialog() {
    if (!targetHistoryTextEdit_) {
        return;
    }

    targetHistoryTextEdit_->setPlainText(targetHistoryEntries_.join('\n'));
}

void TrayIcon::showTargetHistory() {
    if (!targetHistoryDialog_) {
        targetHistoryDialog_ = new QDialog();
        targetHistoryDialog_->setWindowTitle(QStringLiteral("FlamentineSwitcher Target History"));
        targetHistoryDialog_->resize(720, 420);

        auto* rootLayout = new QVBoxLayout(targetHistoryDialog_);
        targetHistoryTextEdit_ = new QPlainTextEdit(targetHistoryDialog_);
        targetHistoryTextEdit_->setReadOnly(true);
        rootLayout->addWidget(targetHistoryTextEdit_);

        auto* buttonsLayout = new QHBoxLayout();
        buttonsLayout->addStretch();
        auto* clearButton = new QPushButton(QStringLiteral("Clear"), targetHistoryDialog_);
        auto* closeButton = new QPushButton(QStringLiteral("Close"), targetHistoryDialog_);
        buttonsLayout->addWidget(clearButton);
        buttonsLayout->addWidget(closeButton);
        rootLayout->addLayout(buttonsLayout);

        connect(clearButton, &QPushButton::clicked, this, &TrayIcon::clearTargetHistory);
        connect(closeButton, &QPushButton::clicked, targetHistoryDialog_, &QDialog::hide);
        connect(targetHistoryDialog_, &QObject::destroyed, this, [this]() {
            targetHistoryDialog_ = nullptr;
            targetHistoryTextEdit_ = nullptr;
        });
    }

    refreshTargetHistoryDialog();
    targetHistoryDialog_->show();
    targetHistoryDialog_->raise();
    targetHistoryDialog_->activateWindow();
}

void TrayIcon::clearTargetHistory() {
    targetHistoryEntries_.clear();
    refreshTargetHistoryDialog();
    rebuildMenuLabels();
}

}  // namespace FlamentineSwitcher::Ui
