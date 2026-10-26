#pragma once

#include <QDialog>
#include <QString>

#include "flamentine_switcher/core/config.h"

class QCheckBox;
class QLabel;

namespace FlamentineSwitcher::Ui {

class SettingsWindow : public QDialog {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget* parent = nullptr);

    void loadFromConfig(const FlamentineSwitcher::Core::AppConfig& config);
    FlamentineSwitcher::Core::AppConfig buildConfig() const;
    void showSaveResult(bool success, const QString& message);

signals:
    void configApplied(const FlamentineSwitcher::Core::AppConfig& config);

private slots:
    void apply();

private:
    QCheckBox* notificationsCheckBox_ = nullptr;
    QCheckBox* autostartCheckBox_ = nullptr;
    QCheckBox* switchLayoutAfterConversionCheckBox_ = nullptr;
    QLabel* saveStatusLabel_ = nullptr;
    ::FlamentineSwitcher::Core::AppConfig currentConfig_ = ::FlamentineSwitcher::Core::AppConfig::defaults();
};

}  // namespace FlamentineSwitcher::Ui