#pragma once

#include <QDialog>

#include "flamentine_switcher/core/config.h"

class QCheckBox;

namespace FlamentineSwitcher::Ui {

class SettingsWindow : public QDialog {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget* parent = nullptr);

    void loadFromConfig(const FlamentineSwitcher::Core::AppConfig& config);
    FlamentineSwitcher::Core::AppConfig buildConfig() const;

signals:
    void configApplied(const FlamentineSwitcher::Core::AppConfig& config);

private slots:
    void apply();

private:
    QCheckBox* notificationsCheckBox_ = nullptr;
    QCheckBox* autostartCheckBox_ = nullptr;
    QCheckBox* switchLayoutAfterConversionCheckBox_ = nullptr;
};

}  // namespace FlamentineSwitcher::Ui