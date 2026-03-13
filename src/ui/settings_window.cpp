#include "flamentine_switcher/ui/settings_window.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include "flamentine_switcher/core/config.h"
#include "flamentine_switcher/utils/string_utils.h"

namespace {

QWidget* makePage(QObject* parent, QLayout* layout) {
    QWidget* widget = new QWidget(static_cast<QWidget*>(parent));
    widget->setLayout(layout);
    return widget;
}

QString displayValue(const QString& value) {
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? QStringLiteral("—") : trimmed;
}

}  // namespace

namespace FlamentineSwitcher::Ui {

SettingsWindow::SettingsWindow(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("FlamentineSwitcher Settings"));
    resize(760, 620);

    QTabWidget* tabs = new QTabWidget(this);

    enabledCheckBox_ = new QCheckBox(QStringLiteral("Enable application"));
    notificationsCheckBox_ = new QCheckBox(QStringLiteral("Enable notifications"));
    autostartCheckBox_ = new QCheckBox(QStringLiteral("Start automatically in the Plasma session"));
    auto* generalLayout = new QVBoxLayout();
    generalLayout->addWidget(enabledCheckBox_);
    generalLayout->addWidget(notificationsCheckBox_);
    generalLayout->addWidget(autostartCheckBox_);
    generalLayout->addStretch();
    tabs->addTab(makePage(tabs, generalLayout), QStringLiteral("General"));

    layoutsEdit_ = new QLineEdit();
    layoutsEdit_->setPlaceholderText(QStringLiteral("us, ru"));
    defaultLayoutCombo_ = new QComboBox();
    defaultLayoutCombo_->setEditable(true);
    auto* layoutsForm = new QFormLayout();
    layoutsForm->addRow(QStringLiteral("Active layouts"), layoutsEdit_);
    layoutsForm->addRow(QStringLiteral("Default layout"), defaultLayoutCombo_);
    tabs->addTab(makePage(tabs, layoutsForm), QStringLiteral("Layouts"));

    toggleLayoutShortcut_ = new QKeySequenceEdit();
    convertLastWordShortcut_ = new QKeySequenceEdit();
    convertSelectionShortcut_ = new QKeySequenceEdit();
    openSettingsShortcut_ = new QKeySequenceEdit();
    toggleEnabledShortcut_ = new QKeySequenceEdit();
    auto* hotkeysForm = new QFormLayout();
    hotkeysForm->addRow(QStringLiteral("Toggle layout"), toggleLayoutShortcut_);
    hotkeysForm->addRow(QStringLiteral("Convert last word"), convertLastWordShortcut_);
    hotkeysForm->addRow(QStringLiteral("Convert selection"), convertSelectionShortcut_);
    hotkeysForm->addRow(QStringLiteral("Open settings"), openSettingsShortcut_);
    hotkeysForm->addRow(QStringLiteral("Toggle enabled"), toggleEnabledShortcut_);
    tabs->addTab(makePage(tabs, hotkeysForm), QStringLiteral("Hotkeys"));

    targetPolicyCombo_ = new QComboBox();
    targetPolicyCombo_->addItem(QStringLiteral("Explicit allowlist only"), true);
    targetPolicyCombo_->addItem(QStringLiteral("Allow everywhere except blocked"), false);
    targetPolicyHintLabel_ = new QLabel();
    targetPolicyHintLabel_->setWordWrap(true);
    currentTargetStatusLabel_ = new QLabel();
    currentTargetStatusLabel_->setWordWrap(true);
    currentTargetAppValueLabel_ = new QLabel(QStringLiteral("—"));
    currentTargetWindowClassValueLabel_ = new QLabel(QStringLiteral("—"));
    currentTargetWindowIdValueLabel_ = new QLabel(QStringLiteral("—"));
    currentTargetFullscreenValueLabel_ = new QLabel(QStringLiteral("—"));
    addCurrentAppButton_ = new QPushButton(QStringLiteral("Allow Current App Only"));
    addCurrentWindowClassButton_ = new QPushButton(QStringLiteral("Allow Current WM_CLASS Only"));
    addCurrentTargetButton_ = new QPushButton(QStringLiteral("Allow App + WM_CLASS"));

    auto* detectedGroup = new QGroupBox(QStringLiteral("Detected Current Target"));
    auto* detectedForm = new QFormLayout(detectedGroup);
    detectedForm->addRow(QStringLiteral("Application"), currentTargetAppValueLabel_);
    detectedForm->addRow(QStringLiteral("WM_CLASS"), currentTargetWindowClassValueLabel_);
    detectedForm->addRow(QStringLiteral("Window ID"), currentTargetWindowIdValueLabel_);
    detectedForm->addRow(QStringLiteral("Fullscreen"), currentTargetFullscreenValueLabel_);

    auto* quickAddLayout = new QHBoxLayout();
    quickAddLayout->addWidget(addCurrentAppButton_);
    quickAddLayout->addWidget(addCurrentWindowClassButton_);
    quickAddLayout->addWidget(addCurrentTargetButton_);

    allowedAppsEdit_ = new QPlainTextEdit();
    allowedAppsEdit_->setPlaceholderText(QStringLiteral("org.telegram.desktop\ncode\nfirefox"));
    allowedWindowClassesEdit_ = new QPlainTextEdit();
    allowedWindowClassesEdit_->setPlaceholderText(QStringLiteral("code\nfirefox"));
    excludedAppsEdit_ = new QPlainTextEdit();
    excludedAppsEdit_->setPlaceholderText(QStringLiteral("konsole\nalacritty\nsteam_app_*"));
    excludedWindowClassesEdit_ = new QPlainTextEdit();
    excludedWindowClassesEdit_->setPlaceholderText(QStringLiteral("yakuake"));
    excludeTerminalsCheckBox_ = new QCheckBox(QStringLiteral("Exclude terminals"));
    excludeFullscreenCheckBox_ = new QCheckBox(QStringLiteral("Exclude fullscreen applications"));

    auto* policyForm = new QFormLayout();
    policyForm->addRow(QStringLiteral("Policy mode"), targetPolicyCombo_);

    auto* allowedGroup = new QGroupBox(QStringLiteral("Allowed Targets"));
    auto* allowedLayout = new QVBoxLayout(allowedGroup);
    allowedLayout->addWidget(new QLabel(QStringLiteral("Process names or masks")));
    allowedLayout->addWidget(allowedAppsEdit_);
    allowedLayout->addWidget(new QLabel(QStringLiteral("Window classes or masks")));
    allowedLayout->addWidget(allowedWindowClassesEdit_);

    auto* blockedGroup = new QGroupBox(QStringLiteral("Blocked Targets"));
    auto* blockedLayout = new QVBoxLayout(blockedGroup);
    blockedLayout->addWidget(new QLabel(QStringLiteral("Process names or masks")));
    blockedLayout->addWidget(excludedAppsEdit_);
    blockedLayout->addWidget(new QLabel(QStringLiteral("Window classes or masks")));
    blockedLayout->addWidget(excludedWindowClassesEdit_);
    blockedLayout->addWidget(excludeTerminalsCheckBox_);
    blockedLayout->addWidget(excludeFullscreenCheckBox_);

    auto* targetsLayout = new QVBoxLayout();
    targetsLayout->addLayout(policyForm);
    targetsLayout->addWidget(targetPolicyHintLabel_);
    targetsLayout->addWidget(currentTargetStatusLabel_);
    targetsLayout->addWidget(detectedGroup);
    targetsLayout->addLayout(quickAddLayout);
    targetsLayout->addWidget(allowedGroup);
    targetsLayout->addWidget(blockedGroup);
    tabs->addTab(makePage(tabs, targetsLayout), QStringLiteral("Targets"));

    rememberPerWindowCheckBox_ = new QCheckBox(QStringLiteral("Remember layout per window"));
    rememberPerAppCheckBox_ = new QCheckBox(QStringLiteral("Remember layout per application"));
    preserveCaseCheckBox_ = new QCheckBox(QStringLiteral("Preserve case"));
    preservePunctuationCheckBox_ = new QCheckBox(QStringLiteral("Preserve punctuation"));
    autoConvertCheckBox_ = new QCheckBox(QStringLiteral("Enable delayed automatic conversion on X11"));
    heuristicsCheckBox_ = new QCheckBox(QStringLiteral("Enable layout heuristics"));
    autoConvertDelayCombo_ = new QComboBox();
    autoConvertDelayCombo_->addItem(QStringLiteral("150 ms"), 150);
    autoConvertDelayCombo_->addItem(QStringLiteral("250 ms"), 250);
    autoConvertDelayCombo_->addItem(QStringLiteral("350 ms"), 350);
    autoConvertDelayCombo_->addItem(QStringLiteral("450 ms"), 450);
    autoConvertDelayCombo_->addItem(QStringLiteral("600 ms"), 600);
    autoConvertDelayCombo_->addItem(QStringLiteral("800 ms"), 800);
    auto* behaviorLayout = new QVBoxLayout();
    behaviorLayout->addWidget(rememberPerWindowCheckBox_);
    behaviorLayout->addWidget(rememberPerAppCheckBox_);
    behaviorLayout->addWidget(preserveCaseCheckBox_);
    behaviorLayout->addWidget(preservePunctuationCheckBox_);
    behaviorLayout->addWidget(autoConvertCheckBox_);
    behaviorLayout->addWidget(heuristicsCheckBox_);
    behaviorLayout->addWidget(new QLabel(QStringLiteral("Automatic conversion delay")));
    behaviorLayout->addWidget(autoConvertDelayCombo_);
    behaviorLayout->addStretch();
    tabs->addTab(makePage(tabs, behaviorLayout), QStringLiteral("Behavior"));

    loggingLevelCombo_ = new QComboBox();
    loggingLevelCombo_->addItems({
        QStringLiteral("trace"),
        QStringLiteral("debug"),
        QStringLiteral("info"),
        QStringLiteral("warning"),
        QStringLiteral("error"),
    });
    logFileCheckBox_ = new QCheckBox(QStringLiteral("Write log file"));
    auto* logsForm = new QFormLayout();
    logsForm->addRow(QStringLiteral("Logging level"), loggingLevelCombo_);
    logsForm->addRow(QString(), logFileCheckBox_);
    tabs->addTab(makePage(tabs, logsForm), QStringLiteral("Logs"));

    QLabel* aboutLabel = new QLabel(
        QStringLiteral("FlamentineSwitcher\n\n"
                       "Qt6/C++ keyboard layout switcher for KDE Plasma.\n"
                       "This initial implementation keeps platform-specific code\n"
                       "behind backends and treats Wayland limitations honestly."));
    aboutLabel->setWordWrap(true);
    auto* aboutLayout = new QVBoxLayout();
    aboutLayout->addWidget(aboutLabel);
    aboutLayout->addStretch();
    tabs->addTab(makePage(tabs, aboutLayout), QStringLiteral("About"));

    importButton_ = new QPushButton(QStringLiteral("Import"));
    exportButton_ = new QPushButton(QStringLiteral("Export"));
    applyButton_ = new QPushButton(QStringLiteral("Apply"));
    QPushButton* closeButton = new QPushButton(QStringLiteral("Close"));

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(importButton_);
    buttonsLayout->addWidget(exportButton_);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(applyButton_);
    buttonsLayout->addWidget(closeButton);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->addWidget(tabs);
    rootLayout->addLayout(buttonsLayout);

    connect(layoutsEdit_, &QLineEdit::textChanged, this, &SettingsWindow::syncDefaultLayoutChoices);
    connect(targetPolicyCombo_, &QComboBox::currentIndexChanged, this, &SettingsWindow::syncTargetPolicyUi);
    connect(addCurrentAppButton_, &QPushButton::clicked, this, &SettingsWindow::allowCurrentAppRequested);
    connect(addCurrentWindowClassButton_, &QPushButton::clicked, this, &SettingsWindow::allowCurrentWindowClassRequested);
    connect(addCurrentTargetButton_, &QPushButton::clicked, this, &SettingsWindow::allowCurrentTargetRequested);
    connect(importButton_, &QPushButton::clicked, this, &SettingsWindow::chooseImportFile);
    connect(exportButton_, &QPushButton::clicked, this, &SettingsWindow::chooseExportFile);
    connect(applyButton_, &QPushButton::clicked, this, &SettingsWindow::apply);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::hide);

    syncTargetPolicyUi();
    setCurrentTargetContext({});
}

void SettingsWindow::loadFromConfig(const FlamentineSwitcher::Core::AppConfig& config) {
    enabledCheckBox_->setChecked(config.enabled);
    layoutsEdit_->setText(config.layouts.join(QStringLiteral(", ")));
    syncDefaultLayoutChoices();
    defaultLayoutCombo_->setCurrentText(config.defaultLayout);
    rememberPerWindowCheckBox_->setChecked(config.rememberLayoutPerWindow);
    rememberPerAppCheckBox_->setChecked(config.rememberLayoutPerApp);
    notificationsCheckBox_->setChecked(config.notificationsEnabled);
    autostartCheckBox_->setChecked(config.autoStart);
    const int policyIndex = qMax(0, targetPolicyCombo_->findData(config.requireAllowedTargets));
    targetPolicyCombo_->setCurrentIndex(policyIndex);
    excludeTerminalsCheckBox_->setChecked(config.excludeTerminals);
    excludeFullscreenCheckBox_->setChecked(config.excludeFullscreen);
    toggleLayoutShortcut_->setKeySequence(QKeySequence::fromString(config.hotkeys.toggleLayout, QKeySequence::PortableText));
    convertLastWordShortcut_->setKeySequence(QKeySequence::fromString(config.hotkeys.convertLastWord, QKeySequence::PortableText));
    convertSelectionShortcut_->setKeySequence(QKeySequence::fromString(config.hotkeys.convertSelection, QKeySequence::PortableText));
    openSettingsShortcut_->setKeySequence(QKeySequence::fromString(config.hotkeys.openSettings, QKeySequence::PortableText));
    toggleEnabledShortcut_->setKeySequence(QKeySequence::fromString(config.hotkeys.toggleEnabled, QKeySequence::PortableText));
    allowedAppsEdit_->setPlainText(config.allowedApps.join('\n'));
    allowedWindowClassesEdit_->setPlainText(config.allowedWindowClasses.join('\n'));
    excludedAppsEdit_->setPlainText(config.excludedApps.join('\n'));
    excludedWindowClassesEdit_->setPlainText(config.excludedWindowClasses.join('\n'));
    preserveCaseCheckBox_->setChecked(config.conversion.preserveCase);
    preservePunctuationCheckBox_->setChecked(config.conversion.preservePunctuation);
    autoConvertCheckBox_->setChecked(config.conversion.autoConvertEnabled);
    heuristicsCheckBox_->setChecked(config.conversion.heuristicsEnabled);
    const int delayIndex = qMax(0, autoConvertDelayCombo_->findData(config.conversion.autoConvertDelayMs));
    autoConvertDelayCombo_->setCurrentIndex(delayIndex);
    loggingLevelCombo_->setCurrentText(FlamentineSwitcher::Core::toString(config.logging.level));
    logFileCheckBox_->setChecked(config.logging.fileEnabled);
    syncTargetPolicyUi();
}

void SettingsWindow::setCurrentTargetContext(const FlamentineSwitcher::Core::WindowContext& context, const QString& backendStatus) {
    const bool hasIdentifiedTarget =
        !context.appName.trimmed().isEmpty() || !context.windowClass.trimmed().isEmpty() || !context.windowId.trimmed().isEmpty();

    currentTargetAppValueLabel_->setText(displayValue(context.appName));
    currentTargetWindowClassValueLabel_->setText(displayValue(context.windowClass));
    currentTargetWindowIdValueLabel_->setText(displayValue(context.windowId));
    currentTargetFullscreenValueLabel_->setText(
        hasIdentifiedTarget ? (context.fullscreen ? QStringLiteral("Yes") : QStringLiteral("No")) : QStringLiteral("—"));

    const bool hasApp = !context.appName.trimmed().isEmpty();
    const bool hasWindowClass = !context.windowClass.trimmed().isEmpty();
    addCurrentAppButton_->setEnabled(hasApp);
    addCurrentWindowClassButton_->setEnabled(hasWindowClass);
    addCurrentTargetButton_->setEnabled(hasApp || hasWindowClass);
    if (!backendStatus.trimmed().isEmpty() && !hasIdentifiedTarget) {
        currentTargetStatusLabel_->setText(QStringLiteral("Window backend status: %1").arg(backendStatus.trimmed()));
    } else if (hasIdentifiedTarget) {
        currentTargetStatusLabel_->setText(
            QStringLiteral("The currently focused target is detected below. You can add it to the allowlist in one action."));
    } else {
        currentTargetStatusLabel_->setText(QStringLiteral("No focused target is detected yet."));
    }
}

FlamentineSwitcher::Core::AppConfig SettingsWindow::buildConfig() const {
    using namespace FlamentineSwitcher::Core;

    AppConfig config = AppConfig::defaults();
    config.enabled = enabledCheckBox_->isChecked();
    config.layouts = Utils::StringUtils::splitCommaSeparated(layoutsEdit_->text());
    if (config.layouts.isEmpty()) {
        config.layouts = AppConfig::defaults().layouts;
    }
    config.defaultLayout = defaultLayoutCombo_->currentText().trimmed();
    if (!config.layouts.contains(config.defaultLayout)) {
        config.defaultLayout = config.layouts.constFirst();
    }
    config.rememberLayoutPerWindow = rememberPerWindowCheckBox_->isChecked();
    config.rememberLayoutPerApp = rememberPerAppCheckBox_->isChecked();
    config.notificationsEnabled = notificationsCheckBox_->isChecked();
    config.autoStart = autostartCheckBox_->isChecked();
    config.requireAllowedTargets = targetPolicyCombo_->currentData().toBool();
    config.excludeTerminals = excludeTerminalsCheckBox_->isChecked();
    config.excludeFullscreen = excludeFullscreenCheckBox_->isChecked();
    config.hotkeys.toggleLayout = toggleLayoutShortcut_->keySequence().toString(QKeySequence::PortableText);
    config.hotkeys.convertLastWord = convertLastWordShortcut_->keySequence().toString(QKeySequence::PortableText);
    config.hotkeys.convertSelection = convertSelectionShortcut_->keySequence().toString(QKeySequence::PortableText);
    config.hotkeys.openSettings = openSettingsShortcut_->keySequence().toString(QKeySequence::PortableText);
    config.hotkeys.toggleEnabled = toggleEnabledShortcut_->keySequence().toString(QKeySequence::PortableText);
    config.allowedApps = Utils::StringUtils::splitLines(allowedAppsEdit_->toPlainText());
    config.allowedWindowClasses = Utils::StringUtils::splitLines(allowedWindowClassesEdit_->toPlainText());
    config.excludedApps = Utils::StringUtils::splitLines(excludedAppsEdit_->toPlainText());
    config.excludedWindowClasses = Utils::StringUtils::splitLines(excludedWindowClassesEdit_->toPlainText());
    config.conversion.preserveCase = preserveCaseCheckBox_->isChecked();
    config.conversion.preservePunctuation = preservePunctuationCheckBox_->isChecked();
    config.conversion.autoConvertEnabled = autoConvertCheckBox_->isChecked();
    config.conversion.heuristicsEnabled = heuristicsCheckBox_->isChecked();
    config.conversion.autoConvertDelayMs = autoConvertDelayCombo_->currentData().toInt();
    config.logging.level = FlamentineSwitcher::Core::logLevelFromString(loggingLevelCombo_->currentText());
    config.logging.fileEnabled = logFileCheckBox_->isChecked();
    return config;
}

void SettingsWindow::apply() {
    emit configApplied(buildConfig());
}

void SettingsWindow::chooseImportFile() {
    const QString path = QFileDialog::getOpenFileName(this,
                                                      QStringLiteral("Import Config"),
                                                      QString(),
                                                      QStringLiteral("JSON files (*.json);;All files (*)"));
    if (!path.isEmpty()) {
        emit importRequested(path);
    }
}

void SettingsWindow::chooseExportFile() {
    const QString path = QFileDialog::getSaveFileName(this,
                                                      QStringLiteral("Export Config"),
                                                      QStringLiteral("flamentine-switcher-config.json"),
                                                      QStringLiteral("JSON files (*.json);;All files (*)"));
    if (!path.isEmpty()) {
        emit exportRequested(path);
    }
}

void SettingsWindow::syncDefaultLayoutChoices() {
    const QString current = defaultLayoutCombo_->currentText();
    const QStringList layouts = Utils::StringUtils::splitCommaSeparated(layoutsEdit_->text());

    defaultLayoutCombo_->blockSignals(true);
    defaultLayoutCombo_->clear();
    defaultLayoutCombo_->addItems(layouts);
    defaultLayoutCombo_->setCurrentText(current);
    defaultLayoutCombo_->blockSignals(false);
}

void SettingsWindow::syncTargetPolicyUi() {
    const bool allowlistOnly = targetPolicyCombo_->currentData().toBool();
    allowedAppsEdit_->setEnabled(allowlistOnly);
    allowedWindowClassesEdit_->setEnabled(allowlistOnly);
    targetPolicyHintLabel_->setText(
        allowlistOnly
            ? QStringLiteral("Only listed processes or window classes may trigger layout switching and text conversion. "
                             "Blocked rules still take priority.")
            : QStringLiteral("The application may operate in any target except blocked ones. Allowed lists stay in the config "
                             "but are currently inactive."));
}

}  // namespace FlamentineSwitcher::Ui
