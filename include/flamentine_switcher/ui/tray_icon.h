#pragma once

#include <QObject>
#include <QSystemTrayIcon>

class QAction;
class QMenu;

namespace FlamentineSwitcher::Ui {

class TrayIcon : public QObject {
    Q_OBJECT

public:
    explicit TrayIcon(QObject* parent = nullptr);

    void setCurrentLayout(const QString& layoutId);
    void setEnabledState(bool enabled);
    void show();
    QSystemTrayIcon* systemTrayIcon();

signals:
    void toggleLayoutRequested();
    void convertLastWordRequested();
    void convertSelectionRequested();
    void openSettingsRequested();
    void enabledToggled(bool enabled);
    void quitRequested();

private:
    void rebuildIcon();
    void rebuildMenuLabels();

    QString currentLayout_ = QStringLiteral("--");
    bool enabled_ = true;
    QSystemTrayIcon trayIcon_;
    QMenu* menu_ = nullptr;
    QAction* statusAction_ = nullptr;
    QAction* toggleEnabledAction_ = nullptr;
    QAction* toggleLayoutAction_ = nullptr;
    QAction* convertLastWordAction_ = nullptr;
    QAction* convertSelectionAction_ = nullptr;
    QAction* openSettingsAction_ = nullptr;
    QAction* quitAction_ = nullptr;
};

}  // namespace FlamentineSwitcher::Ui

