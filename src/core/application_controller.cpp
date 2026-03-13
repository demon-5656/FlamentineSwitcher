#include "flamentine_switcher/core/application_controller.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMap>
#include <utility>

#include "flamentine_switcher/backends/hotkeys/ihotkey_backend.h"
#include "flamentine_switcher/backends/layout/ilayout_backend.h"
#include "flamentine_switcher/backends/text/itext_input_backend.h"
#include "flamentine_switcher/backends/window/iwindow_backend.h"
#include "flamentine_switcher/conversion/heuristics.h"
#include "flamentine_switcher/core/rules.h"
#include "flamentine_switcher/core/settings_manager.h"
#include "flamentine_switcher/services/autostart_service.h"
#include "flamentine_switcher/ui/notifications.h"
#include "flamentine_switcher/ui/settings_window.h"
#include "flamentine_switcher/ui/tray_icon.h"
#include "flamentine_switcher/utils/logging.h"

namespace FlamentineSwitcher::Core {

namespace {

bool sameWindowTarget(const WindowContext& left, const WindowContext& right) {
    return left.windowId == right.windowId && left.appName == right.appName && left.windowClass == right.windowClass
        && left.fullscreen == right.fullscreen;
}

bool appendIfMissing(QStringList& values, const QString& value) {
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    for (const QString& existing : std::as_const(values)) {
        if (existing.compare(trimmed, Qt::CaseInsensitive) == 0) {
            return false;
        }
    }

    values.append(trimmed);
    return true;
}

}  // namespace

ApplicationController::ApplicationController(SettingsManager& settingsManager,
                                             Services::AutostartService& autostartService,
                                             Backends::Layout::ILayoutBackend& layoutBackend,
                                             Backends::Hotkeys::IHotkeyBackend& hotkeyBackend,
                                             Backends::Text::ITextInputBackend& textInputBackend,
                                             Backends::Window::IWindowBackend& windowBackend,
                                             Ui::TrayIcon& trayIcon,
                                             Ui::SettingsWindow& settingsWindow,
                                             Ui::Notifications& notifications,
                                             QObject* parent)
    : QObject(parent)
    , settingsManager_(settingsManager)
    , autostartService_(autostartService)
    , layoutBackend_(layoutBackend)
    , hotkeyBackend_(hotkeyBackend)
    , textInputBackend_(textInputBackend)
    , windowBackend_(windowBackend)
    , trayIcon_(trayIcon)
    , settingsWindow_(settingsWindow)
    , notifications_(notifications) {
    refreshTimer_.setInterval(900);
    autoConvertTimer_.setSingleShot(true);
    layoutMemoryPersistTimer_.setSingleShot(true);
    layoutMemoryPersistTimer_.setInterval(700);

    connect(&refreshTimer_, &QTimer::timeout, this, &ApplicationController::refreshLayout);
    connect(&autoConvertTimer_, &QTimer::timeout, this, &ApplicationController::processPendingAutoConversion);
    connect(&layoutMemoryPersistTimer_, &QTimer::timeout, this, &ApplicationController::persistLayoutMemory);
    connect(&trayIcon_, &Ui::TrayIcon::toggleLayoutRequested, this, &ApplicationController::toggleLayout);
    connect(&trayIcon_, &Ui::TrayIcon::convertLastWordRequested, this, &ApplicationController::convertLastWord);
    connect(&trayIcon_, &Ui::TrayIcon::convertSelectionRequested, this, &ApplicationController::convertSelection);
    connect(&trayIcon_, &Ui::TrayIcon::openSettingsRequested, this, &ApplicationController::openSettings);
    connect(&trayIcon_, &Ui::TrayIcon::allowCurrentTargetRequested, this, &ApplicationController::allowCurrentTarget);
    connect(&trayIcon_, &Ui::TrayIcon::enabledToggled, this, [this](const bool enabled) {
        setEnabled(enabled);
    });
    connect(&trayIcon_, &Ui::TrayIcon::quitRequested, qApp, &QCoreApplication::quit);
    connect(&settingsWindow_, &Ui::SettingsWindow::configApplied, this, &ApplicationController::applyConfig);
    connect(&settingsWindow_, &Ui::SettingsWindow::allowCurrentTargetRequested, this, &ApplicationController::allowCurrentTarget);
    connect(&settingsWindow_, &Ui::SettingsWindow::importRequested, this, &ApplicationController::importConfig);
    connect(&settingsWindow_, &Ui::SettingsWindow::exportRequested, this, &ApplicationController::exportConfig);
    connect(&hotkeyBackend_, &Backends::Hotkeys::IHotkeyBackend::hotkeyTriggered, this, &ApplicationController::handleHotkeyAction);
    connect(&layoutBackend_, &Backends::Layout::ILayoutBackend::layoutChanged, this, &ApplicationController::refreshLayout);
    connect(&textInputBackend_,
            &Backends::Text::ITextInputBackend::wordCommitted,
            this,
            &ApplicationController::handleObservedWordCommitted);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &ApplicationController::persistLayoutMemory);
}

void ApplicationController::initialize() {
    config_ = settingsManager_.load();
    Utils::Logging::initialize(config_.logging);
    settingsWindow_.loadFromConfig(config_);
    trayIcon_.setEnabledState(config_.enabled);
    autostartService_.setEnabled(config_.autoStart);
    registerHotkeys();
    updateTextInputBackendState();
    trayIcon_.show();
    if (config_.rememberLayoutPerWindow || config_.rememberLayoutPerApp) {
        const auto state = settingsManager_.loadLayoutMemoryState();
        if (state.has_value()) {
            layoutMemory_.restoreState(state.value());
        } else if (!settingsManager_.lastError().isEmpty()) {
            notifyWarning(settingsManager_.lastError());
        }
    }
    refreshLayout();
    refreshTimer_.start();

    if (!layoutBackend_.isSupported()) {
        notifyWarning(QStringLiteral("Layout backend is limited: %1").arg(layoutBackend_.lastError()));
    }
    if (!hotkeyBackend_.isSupported()) {
        notifyWarning(QStringLiteral("Hotkey backend is limited: %1").arg(hotkeyBackend_.lastError()));
    }
    if (!windowBackend_.isSupported() && config_.requireAllowedTargets) {
        notifyWarning(QStringLiteral("Window backend is unavailable, deny-by-default policy will block conversion actions"));
    }
    if (config_.conversion.autoConvertEnabled && !textInputBackend_.isSupported()) {
        notifyWarning(QStringLiteral("Delayed automatic conversion is unavailable: %1").arg(textInputBackend_.lastError()));
    }
}

QString ApplicationController::currentLayout() const {
    return layoutBackend_.currentLayoutId();
}

const AppConfig& ApplicationController::config() const {
    return config_;
}

QString ApplicationController::lastError() const {
    return lastError_;
}

bool ApplicationController::setLayout(const QString& layoutId) {
    if (!layoutBackend_.setLayout(layoutId)) {
        lastError_ = layoutBackend_.lastError();
        notifyWarning(lastError_);
        return false;
    }

    rememberCurrentTargetLayout(layoutBackend_.currentLayoutId());
    refreshLayout();
    return true;
}

bool ApplicationController::toggleLayout() {
    if (!layoutBackend_.toggleLayout()) {
        lastError_ = layoutBackend_.lastError();
        notifyWarning(lastError_);
        return false;
    }

    rememberCurrentTargetLayout(layoutBackend_.currentLayoutId());
    refreshLayout();
    return true;
}

QString ApplicationController::convertLastWord() {
    if (!config_.enabled) {
        notifyInfo(QStringLiteral("Application is disabled"));
        return {};
    }

    const WindowContext context = windowBackend_.currentContext();
    if (!Rules::isAllowed(config_, context)) {
        notifyInfo(
            context.appName.isEmpty() && context.windowClass.isEmpty()
                ? QStringLiteral("Current application is not identified or not allowed for conversion")
                : QStringLiteral("Current application is not allowed for conversion: %1")
                      .arg(!context.appName.isEmpty() ? context.appName : context.windowClass));
        return {};
    }

    const QString sourceText = clipboardText();
    if (sourceText.trimmed().isEmpty()) {
        notifyWarning(QStringLiteral("Clipboard is empty, nothing to convert"));
        return {};
    }

    const QString converted = textConverter_.convertLastWordInText(sourceText);
    replaceClipboardText(converted);
    notifyInfo(QStringLiteral("Last clipboard token converted"));
    return converted;
}

QString ApplicationController::convertSelection() {
    if (!config_.enabled) {
        notifyInfo(QStringLiteral("Application is disabled"));
        return {};
    }

    const WindowContext context = windowBackend_.currentContext();
    if (!Rules::isAllowed(config_, context)) {
        notifyInfo(
            context.appName.isEmpty() && context.windowClass.isEmpty()
                ? QStringLiteral("Current application is not identified or not allowed for conversion")
                : QStringLiteral("Current application is not allowed for conversion: %1")
                      .arg(!context.appName.isEmpty() ? context.appName : context.windowClass));
        return {};
    }

    const QString sourceText = clipboardText();
    if (sourceText.trimmed().isEmpty()) {
        notifyWarning(QStringLiteral("Clipboard is empty, nothing to convert"));
        return {};
    }

    const QString converted = textConverter_.convertText(sourceText);
    replaceClipboardText(converted);
    notifyInfo(QStringLiteral("Converted text copied back to the clipboard"));
    return converted;
}

bool ApplicationController::enable() {
    return setEnabled(true);
}

bool ApplicationController::disable() {
    return setEnabled(false);
}

bool ApplicationController::openSettings() {
    settingsWindow_.show();
    settingsWindow_.raise();
    settingsWindow_.activateWindow();
    return true;
}

bool ApplicationController::allowCurrentTarget() {
    const WindowContext context = windowBackend_.currentContext();
    if (context.appName.trimmed().isEmpty() && context.windowClass.trimmed().isEmpty()) {
        notifyWarning(windowBackend_.lastError().isEmpty() ? QStringLiteral("Unable to identify the active target")
                                                           : windowBackend_.lastError());
        return false;
    }

    AppConfig updatedConfig = config_;
    QStringList addedEntries;
    if (appendIfMissing(updatedConfig.allowedApps, context.appName)) {
        addedEntries.append(QStringLiteral("app=%1").arg(context.appName.trimmed()));
    }
    if (appendIfMissing(updatedConfig.allowedWindowClasses, context.windowClass)) {
        addedEntries.append(QStringLiteral("class=%1").arg(context.windowClass.trimmed()));
    }

    if (addedEntries.isEmpty()) {
        notifyInfo(QStringLiteral("Current target is already present in the allowlist"));
        return true;
    }

    applyConfig(updatedConfig);
    notifyInfo(QStringLiteral("Added current target to allowlist: %1").arg(addedEntries.join(QStringLiteral(", "))));
    return true;
}

void ApplicationController::applyConfig(const AppConfig& config) {
    config_ = config;
    Utils::Logging::initialize(config_.logging);

    if (!settingsManager_.save(config_)) {
        notifyWarning(settingsManager_.lastError());
    }
    if (!autostartService_.setEnabled(config_.autoStart)) {
        notifyWarning(autostartService_.lastError());
    }

    settingsWindow_.loadFromConfig(config_);
    trayIcon_.setEnabledState(config_.enabled);
    registerHotkeys();
    updateTextInputBackendState();
    refreshLayout();
    emit enabledChanged(config_.enabled);
}

void ApplicationController::importConfig(const QString& filePath) {
    const auto imported = settingsManager_.importConfig(filePath);
    if (!imported.has_value()) {
        notifyWarning(settingsManager_.lastError());
        return;
    }

    applyConfig(imported.value());
    notifyInfo(QStringLiteral("Configuration imported"));
}

void ApplicationController::exportConfig(const QString& filePath) {
    if (!settingsManager_.exportConfig(filePath, config_)) {
        notifyWarning(settingsManager_.lastError());
        return;
    }

    notifyInfo(QStringLiteral("Configuration exported"));
}

void ApplicationController::refreshLayout() {
    const QString currentLayoutId = layoutBackend_.currentLayoutId();
    if (!syncingRememberedLayout_) {
        syncRememberedLayout(windowBackend_.currentContext(), currentLayoutId);
    }

    const QString layoutId = layoutBackend_.currentLayoutId();
    trayIcon_.setCurrentLayout(layoutId.isEmpty() ? QStringLiteral("--") : layoutId);
    emit layoutChanged(layoutId);
}

void ApplicationController::handleHotkeyAction(const FlamentineSwitcher::Core::HotkeyAction action) {
    switch (action) {
    case HotkeyAction::ToggleLayout:
        toggleLayout();
        break;
    case HotkeyAction::ConvertLastWord:
        convertLastWord();
        break;
    case HotkeyAction::ConvertSelection:
        convertSelection();
        break;
    case HotkeyAction::OpenSettings:
        openSettings();
        break;
    case HotkeyAction::ToggleEnabled:
        setEnabled(!config_.enabled);
        break;
    }
}

bool ApplicationController::setEnabled(const bool enabled) {
    config_.enabled = enabled;
    pendingAutoConversion_ = {};
    autoConvertTimer_.stop();
    trayIcon_.setEnabledState(enabled);
    settingsWindow_.loadFromConfig(config_);
    updateTextInputBackendState();
    if (!settingsManager_.save(config_)) {
        notifyWarning(settingsManager_.lastError());
    }
    emit enabledChanged(enabled);
    return true;
}

QString ApplicationController::clipboardText() const {
    QClipboard* clipboard = QGuiApplication::clipboard();
    QString text = clipboard->text(QClipboard::Selection);
    if (text.trimmed().isEmpty()) {
        text = clipboard->text(QClipboard::Clipboard);
    }
    return text;
}

void ApplicationController::replaceClipboardText(const QString& text) const {
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(text, QClipboard::Clipboard);
    clipboard->setText(text, QClipboard::Selection);
}

void ApplicationController::registerHotkeys() {
    QMap<HotkeyAction, QKeySequence> shortcuts;
    shortcuts.insert(HotkeyAction::ToggleLayout, QKeySequence::fromString(config_.hotkeys.toggleLayout, QKeySequence::PortableText));
    shortcuts.insert(HotkeyAction::ConvertLastWord, QKeySequence::fromString(config_.hotkeys.convertLastWord, QKeySequence::PortableText));
    shortcuts.insert(HotkeyAction::ConvertSelection, QKeySequence::fromString(config_.hotkeys.convertSelection, QKeySequence::PortableText));
    shortcuts.insert(HotkeyAction::OpenSettings, QKeySequence::fromString(config_.hotkeys.openSettings, QKeySequence::PortableText));
    shortcuts.insert(HotkeyAction::ToggleEnabled, QKeySequence::fromString(config_.hotkeys.toggleEnabled, QKeySequence::PortableText));

    if (!hotkeyBackend_.registerHotkeys(shortcuts) && hotkeyBackend_.isSupported()) {
        notifyWarning(hotkeyBackend_.lastError());
    }
}

void ApplicationController::rememberCurrentTargetLayout(const QString& layoutId) {
    const WindowContext context = windowBackend_.currentContext();
    if (Rules::isAllowed(config_, context) && layoutMemory_.remember(config_, context, layoutId)) {
        scheduleLayoutMemoryPersist();
    }
}

void ApplicationController::syncRememberedLayout(const WindowContext& context, const QString& currentLayoutId) {
    const bool rememberEnabled = config_.rememberLayoutPerWindow || config_.rememberLayoutPerApp;
    if (!rememberEnabled) {
        lastWindowContext_ = context;
        return;
    }

    const bool targetChanged = !sameWindowTarget(lastWindowContext_, context);
    if (targetChanged) {
        lastWindowContext_ = context;
        if (!Rules::isAllowed(config_, context)) {
            return;
        }

        const auto rememberedLayout = layoutMemory_.recall(config_, context);
        if (!rememberedLayout.has_value() || rememberedLayout.value() == currentLayoutId) {
            return;
        }

        syncingRememberedLayout_ = true;
        layoutBackend_.setLayout(rememberedLayout.value());
        syncingRememberedLayout_ = false;
        return;
    }

    if (Rules::isAllowed(config_, context) && !currentLayoutId.trimmed().isEmpty()
        && layoutMemory_.remember(config_, context, currentLayoutId)) {
        scheduleLayoutMemoryPersist();
    }
}

void ApplicationController::updateTextInputBackendState() {
    textInputBackend_.applyConfig(config_);
    const bool shouldObserve = config_.enabled && config_.conversion.autoConvertEnabled;
    if (!shouldObserve) {
        pendingAutoConversion_ = {};
        autoConvertTimer_.stop();
    }
    if (!config_.rememberLayoutPerWindow && !config_.rememberLayoutPerApp) {
        layoutMemory_.clear();
        persistLayoutMemory();
    }
    textInputBackend_.setEnabled(shouldObserve);
}

void ApplicationController::scheduleLayoutMemoryPersist() {
    if (!(config_.rememberLayoutPerWindow || config_.rememberLayoutPerApp)) {
        return;
    }

    layoutMemoryPersistTimer_.start();
}

void ApplicationController::persistLayoutMemory() {
    if (!settingsManager_.saveLayoutMemoryState(layoutMemory_.exportState())) {
        notifyWarning(settingsManager_.lastError());
    }
}

QString ApplicationController::targetLayoutIdForDirection(const Conversion::ConversionDirection direction) const {
    return direction == Conversion::ConversionDirection::UsToRu ? QStringLiteral("ru") : QStringLiteral("us");
}

void ApplicationController::handleObservedWordCommitted(const quint64 tokenId,
                                                        const QString& word,
                                                        const FlamentineSwitcher::Core::WindowContext& context) {
    if (!config_.enabled || !config_.conversion.autoConvertEnabled) {
        return;
    }

    if (!Rules::isAllowed(config_, context)) {
        return;
    }

    pendingAutoConversion_.tokenId = tokenId;
    pendingAutoConversion_.word = word;
    pendingAutoConversion_.context = context;
    pendingAutoConversion_.valid = true;
    autoConvertTimer_.start(qMax(0, config_.conversion.autoConvertDelayMs));
}

void ApplicationController::processPendingAutoConversion() {
    if (!pendingAutoConversion_.valid || !config_.enabled || !config_.conversion.autoConvertEnabled) {
        pendingAutoConversion_ = {};
        return;
    }

    using namespace FlamentineSwitcher::Conversion;

    ConversionDirection direction = ConversionDirection::AutoDetect;
    if (config_.conversion.heuristicsEnabled) {
        const LayoutHeuristics heuristics;
        const LayoutAssessment assessment = heuristics.assessWord(pendingAutoConversion_.word);
        if (!assessment.looksMistyped) {
            pendingAutoConversion_ = {};
            return;
        }

        direction = assessment.target == SuggestedLayout::Ru ? ConversionDirection::UsToRu : ConversionDirection::RuToUs;
    } else {
        direction = textConverter_.resolveDirection(pendingAutoConversion_.word);
    }

    const QString converted = textConverter_.convertText(pendingAutoConversion_.word, direction);
    if (converted.isEmpty() || converted == pendingAutoConversion_.word) {
        pendingAutoConversion_ = {};
        return;
    }

    const QString targetLayoutId = targetLayoutIdForDirection(direction);
    if (!config_.layouts.contains(targetLayoutId)) {
        qWarning().noquote() << QStringLiteral("Skipping delayed replacement because layout '%1' is not configured").arg(targetLayoutId);
        pendingAutoConversion_ = {};
        return;
    }

    if (currentLayout() != targetLayoutId && !setLayout(targetLayoutId)) {
        qWarning().noquote() << QStringLiteral("Skipping delayed replacement because layout switch failed: %1").arg(lastError_);
        pendingAutoConversion_ = {};
        return;
    }

    if (!textInputBackend_.replacePendingWord(pendingAutoConversion_.tokenId, converted)) {
        const QString backendError = textInputBackend_.lastError();
        if (!backendError.contains(QStringLiteral("Typing continued"), Qt::CaseInsensitive)
            && !backendError.contains(QStringLiteral("Active window changed"), Qt::CaseInsensitive)) {
            qWarning().noquote() << backendError;
        }
    }

    pendingAutoConversion_ = {};
}

void ApplicationController::notifyInfo(const QString& message) const {
    if (config_.notificationsEnabled) {
        notifications_.info(message);
    }
}

void ApplicationController::notifyWarning(const QString& message) const {
    if (config_.notificationsEnabled) {
        notifications_.warning(message);
    }
}

}  // namespace FlamentineSwitcher::Core
