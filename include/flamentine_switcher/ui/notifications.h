#pragma once

#include <QObject>

class QSystemTrayIcon;

namespace FlamentineSwitcher::Ui {

class Notifications : public QObject {
    Q_OBJECT

public:
    explicit Notifications(QSystemTrayIcon* trayIcon, QObject* parent = nullptr);

    void info(const QString& message) const;
    void warning(const QString& message) const;
    void error(const QString& message) const;

private:
    QSystemTrayIcon* trayIcon_ = nullptr;
};

}  // namespace FlamentineSwitcher::Ui

