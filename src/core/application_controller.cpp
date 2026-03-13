#include "flamentine_switcher/core/application_controller.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMap>

#include "flamentine_switcher/backends/hotkeys/ihotkey_backend.h"
#include "flamentine_switcher/backends/layout/ilayout_backend.h"
#include "flamentine_switcher/backends/window/iwindow_backend.h"
#include "flamentine_switcher/core/rules.h"
#include "flamentine_switcher/core/settings_manager.h"
#include "flamentine_switcher/services/autostart_service.h"
#include "flamentine_switcher/ui/notifications.h"
#include "flamentine_switcher/ui/settings_window.h"
#include "flamentine_switcher/ui/tray_icon.h"
#include "flamentine_switcher/utils/logging.h"

namespace FlamentineSwitcher::Core {

ApplicationController::ApplicationController(SettingsManager& settingsManager,
                                             Services::AutostartService& autostartService,
                                             Backends::Layout::ILayoutBackend& layoutBackend,
                                             Backends::Hotkeys::IHotkeyBackend& hotkeyBackend,
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
    , windowBackend_(windowBackend)
    , trayIcon_(trayIcon)
    , settingsWindow_(settingsWindow)
    , notifications_(notifications) {
    refreshTimer_.setInterval(900);

    connect(&refreshTimer_, &QTimer::timeout, this, &ApplicationController::refreshLayout);
    connect(&trayIcon_, &Ui::TrayIcon::toggleLayoutRequested, this, &ApplicationController::toggleLayout);
    connect(&trayIcon_, &Ui::TrayIcon::convertLastWordRequested, this, &ApplicationController::convertLastWord);
    connect(&trayIcon_, &Ui::TrayIcon::convertSelectionRequested, this, &ApplicationController::convertSelection);
    connect(&trayIcon_, &Ui::TrayIcon::openSettingsRequested, this, &ApplicationController::openSettings);
    connect(&trayIcon_, &Ui::TrayIcon::enabledToggled, this, [this](const bool enabled) {
        setEnabled(enabled);
    });
    connect(&trayIcon_, &Ui::TrayIcon::quitRequested, qApp, &QCoreApplication::quit);
    connect(&settingsWindow_, &Ui::SettingsWindow::configApplied, this, &ApplicationController::applyConfig);
    connect(&settingsWindow_, &Ui::SettingsWindow::importRequested, this, &ApplicationController::importConfig);
    connect(&settingsWindow_, &Ui::SettingsWindow::exportRequested, this, &ApplicationController::exportConfig);
    connect(&hotkeyBackend_, &Backends::Hotkeys::IHotkeyBackend::hotkeyTriggered, this, &ApplicationController::handleHotkeyAction);
    connect(&layoutBackend_, &Backends::Layout::ILayoutBackend::layoutChanged, this, &ApplicationController::refreshLayout);
}

void ApplicationController::initialize() {
    config_ = settingsManager_.load();
    Utils::Logging::initialize(config_.logging);
    settingsWindow_.loadFromConfig(config_);
    trayIcon_.setEnabledState(config_.enabled);
    autostartService_.setEnabled(config_.autoStart);
    registerHotkeys();
    trayIcon_.show();
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

    refreshLayout();
    return true;
}

bool ApplicationController::toggleLayout() {
    if (!layoutBackend_.toggleLayout()) {
        lastError_ = layoutBackend_.lastError();
        notifyWarning(lastError_);
        return false;
    }

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
    trayIcon_.setEnabledState(enabled);
    settingsWindow_.loadFromConfig(config_);
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
