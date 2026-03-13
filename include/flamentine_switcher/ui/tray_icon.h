#pragma once

#include <QObject>
#include <QPointer>
#include <QSystemTrayIcon>

#include "flamentine_switcher/core/models.h"

class QAction;
class QDialog;
class QMenu;
class QPlainTextEdit;

namespace FlamentineSwitcher::Ui {

class TrayIcon : public QObject {
    Q_OBJECT

public:
    explicit TrayIcon(QObject* parent = nullptr);

    void setCurrentLayout(const QString& layoutId);
    void setCurrentTargetContext(const FlamentineSwitcher::Core::WindowContext& context, const QString& backendStatus = QString());
    void setEnabledState(bool enabled);
    void show();
    QSystemTrayIcon* systemTrayIcon();

signals:
    void toggleLayoutRequested();
    void convertLastWordRequested();
    void convertSelectionRequested();
    void openSettingsRequested();
    void copyCurrentTargetInfoRequested();
    void allowCurrentTargetRequested();
    void allowCurrentAppRequested();
    void allowCurrentWindowClassRequested();
    void enabledToggled(bool enabled);
    void quitRequested();

private:
    void rebuildIcon();
    void rebuildMenuLabels();
    void appendTargetHistoryEntry(const FlamentineSwitcher::Core::WindowContext& context, const QString& backendStatus);
    void refreshTargetHistoryDialog();
    void showTargetHistory();
    void clearTargetHistory();

    QString currentLayout_ = QStringLiteral("--");
    FlamentineSwitcher::Core::WindowContext currentTargetContext_;
    QString currentTargetStatus_;
    QStringList targetHistoryEntries_;
    bool enabled_ = true;
    QSystemTrayIcon trayIcon_;
    QMenu* menu_ = nullptr;
    QMenu* currentTargetMenu_ = nullptr;
    QAction* statusAction_ = nullptr;
    QAction* toggleEnabledAction_ = nullptr;
    QAction* toggleLayoutAction_ = nullptr;
    QAction* currentTargetStatusAction_ = nullptr;
    QAction* currentTargetAppAction_ = nullptr;
    QAction* currentTargetWindowClassAction_ = nullptr;
    QAction* currentTargetWindowIdAction_ = nullptr;
    QAction* currentTargetFullscreenAction_ = nullptr;
    QAction* copyCurrentTargetInfoAction_ = nullptr;
    QAction* showTargetHistoryAction_ = nullptr;
    QAction* allowCurrentAppAction_ = nullptr;
    QAction* allowCurrentWindowClassAction_ = nullptr;
    QAction* convertLastWordAction_ = nullptr;
    QAction* convertSelectionAction_ = nullptr;
    QAction* allowCurrentTargetAction_ = nullptr;
    QAction* openSettingsAction_ = nullptr;
    QAction* quitAction_ = nullptr;
    QPointer<QDialog> targetHistoryDialog_;
    QPointer<QPlainTextEdit> targetHistoryTextEdit_;
};

}  // namespace FlamentineSwitcher::Ui
