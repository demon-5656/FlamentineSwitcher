#pragma once

#include <QObject>
#include <QSystemTrayIcon>

#include "flamentine_switcher/core/models.h"

class QAction;
class QMenu;

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
    void convertSelectionRequested();
    void openSettingsRequested();
    void quitRequested();

private:
    void rebuildIcon();
    void rebuildMenuLabels();

    QString currentLayout_ = QStringLiteral("--");
    bool enabled_ = true;
    QSystemTrayIcon trayIcon_;
    QMenu* menu_ = nullptr;
    QAction* statusAction_ = nullptr;
    QAction* convertSelectionAction_ = nullptr;
    QAction* openSettingsAction_ = nullptr;
    QAction* quitAction_ = nullptr;
};

}  // namespace FlamentineSwitcher::Ui