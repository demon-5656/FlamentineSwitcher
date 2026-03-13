#pragma once

#include <QDialog>

#include "flamentine_switcher/core/config.h"

class QCheckBox;
class QComboBox;
class QKeySequenceEdit;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

namespace FlamentineSwitcher::Ui {

class SettingsWindow : public QDialog {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget* parent = nullptr);

    void loadFromConfig(const FlamentineSwitcher::Core::AppConfig& config);
    FlamentineSwitcher::Core::AppConfig buildConfig() const;

signals:
    void configApplied(const FlamentineSwitcher::Core::AppConfig& config);
    void allowCurrentTargetRequested();
    void importRequested(const QString& filePath);
    void exportRequested(const QString& filePath);

private slots:
    void apply();
    void chooseImportFile();
    void chooseExportFile();
    void syncDefaultLayoutChoices();
    void syncTargetPolicyUi();

private:
    QCheckBox* enabledCheckBox_ = nullptr;
    QLineEdit* layoutsEdit_ = nullptr;
    QComboBox* defaultLayoutCombo_ = nullptr;
    QCheckBox* rememberPerWindowCheckBox_ = nullptr;
    QCheckBox* rememberPerAppCheckBox_ = nullptr;
    QCheckBox* notificationsCheckBox_ = nullptr;
    QCheckBox* autostartCheckBox_ = nullptr;
    QComboBox* targetPolicyCombo_ = nullptr;
    QLabel* targetPolicyHintLabel_ = nullptr;
    QPushButton* addCurrentTargetButton_ = nullptr;
    QCheckBox* excludeTerminalsCheckBox_ = nullptr;
    QCheckBox* excludeFullscreenCheckBox_ = nullptr;
    QKeySequenceEdit* toggleLayoutShortcut_ = nullptr;
    QKeySequenceEdit* convertLastWordShortcut_ = nullptr;
    QKeySequenceEdit* convertSelectionShortcut_ = nullptr;
    QKeySequenceEdit* openSettingsShortcut_ = nullptr;
    QKeySequenceEdit* toggleEnabledShortcut_ = nullptr;
    QPlainTextEdit* allowedAppsEdit_ = nullptr;
    QPlainTextEdit* allowedWindowClassesEdit_ = nullptr;
    QPlainTextEdit* excludedAppsEdit_ = nullptr;
    QPlainTextEdit* excludedWindowClassesEdit_ = nullptr;
    QCheckBox* preserveCaseCheckBox_ = nullptr;
    QCheckBox* preservePunctuationCheckBox_ = nullptr;
    QCheckBox* autoConvertCheckBox_ = nullptr;
    QCheckBox* heuristicsCheckBox_ = nullptr;
    QComboBox* autoConvertDelayCombo_ = nullptr;
    QComboBox* loggingLevelCombo_ = nullptr;
    QCheckBox* logFileCheckBox_ = nullptr;
    QPushButton* importButton_ = nullptr;
    QPushButton* exportButton_ = nullptr;
    QPushButton* applyButton_ = nullptr;
};

}  // namespace FlamentineSwitcher::Ui
