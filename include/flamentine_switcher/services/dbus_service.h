#pragma once

#include <QObject>

namespace FlamentineSwitcher::Core {
class ApplicationController;
}

namespace FlamentineSwitcher::Services {

class DbusService : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.flamentineswitcher.Control")

public:
    explicit DbusService(FlamentineSwitcher::Core::ApplicationController& controller, QObject* parent = nullptr);

    bool start();
    QString lastError() const;

public slots:
    QString GetCurrentLayout() const;
    bool SetLayout(const QString& layoutId);
    bool ToggleLayout();
    QString ConvertLastWord();
    QString ConvertSelection();
    bool Enable();
    bool Disable();
    bool OpenSettings();

signals:
    void LayoutChanged(const QString& layoutId);
    void EnabledChanged(bool enabled);

private:
    FlamentineSwitcher::Core::ApplicationController& controller_;
    QString lastError_;
};

}  // namespace FlamentineSwitcher::Services

