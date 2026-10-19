#include "flamentine_switcher/ui/settings_window.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace FlamentineSwitcher::Ui {

SettingsWindow::SettingsWindow(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("FlamentineSwitcher — Настройки"));
    setMinimumWidth(460);

    auto* rootLayout = new QVBoxLayout(this);
    auto* helpLabel = new QLabel(
        QStringLiteral("Для преобразования выделите текст и нажмите Meta+End.\n"
                       "Горячая клавиша регистрируется через KDE Plasma и может быть изменена в его настройках."));
    helpLabel->setWordWrap(true);
    rootLayout->addWidget(helpLabel);

    autostartCheckBox_ = new QCheckBox(QStringLiteral("Запускать вместе с Plasma"));
    notificationsCheckBox_ = new QCheckBox(QStringLiteral("Показывать уведомления"));
    switchLayoutAfterConversionCheckBox_ = new QCheckBox(QStringLiteral("Переключать раскладку после преобразования"));
    rootLayout->addWidget(autostartCheckBox_);
    rootLayout->addWidget(notificationsCheckBox_);
    rootLayout->addWidget(switchLayoutAfterConversionCheckBox_);
    rootLayout->addStretch();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Close);
    rootLayout->addWidget(buttons);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsWindow::apply);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::hide);
}

void SettingsWindow::loadFromConfig(const FlamentineSwitcher::Core::AppConfig& config) {
    notificationsCheckBox_->setChecked(config.notificationsEnabled);
    autostartCheckBox_->setChecked(config.autoStart);
    switchLayoutAfterConversionCheckBox_->setChecked(config.conversion.switchLayoutAfterConversion);
}

FlamentineSwitcher::Core::AppConfig SettingsWindow::buildConfig() const {
    auto config = FlamentineSwitcher::Core::AppConfig::defaults();
    config.notificationsEnabled = notificationsCheckBox_->isChecked();
    config.autoStart = autostartCheckBox_->isChecked();
    config.conversion.switchLayoutAfterConversion = switchLayoutAfterConversionCheckBox_->isChecked();
    return config;
}

void SettingsWindow::apply() {
    emit configApplied(buildConfig());
}

}  // namespace FlamentineSwitcher::Ui