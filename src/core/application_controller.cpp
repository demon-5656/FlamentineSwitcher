#include "flamentine_switcher/core/application_controller.h"

#include <QClipboard>
#include <QDBusInterface>
#include <QDBusReply>
#include <QGuiApplication>
#include <QMap>
#include <QUuid>
#include <utility>

#include "flamentine_switcher/backends/hotkeys/ihotkey_backend.h"
#include "flamentine_switcher/backends/layout/ilayout_backend.h"
#include "flamentine_switcher/backends/text/itext_input_backend.h"
#include "flamentine_switcher/backends/window/iwindow_backend.h"
#include "flamentine_switcher/conversion/layout_maps.h"
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

QString targetDeniedMessage(const WindowContext& context) {
    if (context.appName.isEmpty() && context.windowClass.isEmpty()) {
        return QStringLiteral("Current application is not identified or not allowed for conversion");
    }
    return QStringLiteral("Current application is not allowed for conversion: %1")
        .arg(!context.appName.isEmpty() ? context.appName : context.windowClass);
}

std::optional<FlamentineSwitcher::Conversion::ConversionDirection> directionForTrailingScript(const QString& text) {
    for (auto it = text.crbegin(); it != text.crend(); ++it) {
        if (FlamentineSwitcher::Conversion::isLatinLetter(*it)) {
            return FlamentineSwitcher::Conversion::ConversionDirection::UsToRu;
        }
        if (FlamentineSwitcher::Conversion::isCyrillicLetter(*it)) {
            return FlamentineSwitcher::Conversion::ConversionDirection::RuToUs;
        }
    }
    return std::nullopt;
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
    layoutMemoryPersistTimer_.setSingleShot(true);
    layoutMemoryPersistTimer_.setInterval(700);
    selectionClipboardWaitTimer_.setInterval(40);

    connect(&refreshTimer_, &QTimer::timeout, this, &ApplicationController::refreshLayout);
    connect(&layoutMemoryPersistTimer_, &QTimer::timeout, this, &ApplicationController::persistLayoutMemory);
    connect(&selectionClipboardWaitTimer_, &QTimer::timeout, this, &ApplicationController::waitForSelectionClipboard);
    connect(&trayIcon_, &Ui::TrayIcon::convertSelectionRequested, this, &ApplicationController::convertSelection);
    connect(&trayIcon_, &Ui::TrayIcon::openSettingsRequested, this, &ApplicationController::openSettings);
    connect(&trayIcon_, &Ui::TrayIcon::quitRequested, qApp, &QCoreApplication::quit);
    connect(&settingsWindow_, &Ui::SettingsWindow::configApplied, this, &ApplicationController::applyConfig);
    connect(&hotkeyBackend_, &Backends::Hotkeys::IHotkeyBackend::hotkeyTriggered, this, &ApplicationController::handleHotkeyAction);
    connect(&layoutBackend_, &Backends::Layout::ILayoutBackend::layoutChanged, this, &ApplicationController::refreshLayout);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &ApplicationController::persistLayoutMemory);
}

void ApplicationController::initialize() {
    config_ = settingsManager_.load();
    Utils::Logging::initialize(config_.logging);
    settingsWindow_.loadFromConfig(config_);
    trayIcon_.setEnabledState(config_.enabled);
    autostartService_.setEnabled(config_.autoStart);
    registerHotkeys();
    clearLayoutMemoryIfDisabled();
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
    // Wayland uses the installed KDE Global Shortcuts launcher instead of the
    // X11 grab backend, so reporting the latter as unavailable is misleading.
    if (!hotkeyBackend_.isSupported()
        && !QGuiApplication::platformName().contains(QStringLiteral("wayland"), Qt::CaseInsensitive)) {
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
    if (!conversionPermitted()) {
        return {};
    }

    const QString sourceText = clipboardText();
    if (sourceText.trimmed().isEmpty()) {
        notifyWarning(QStringLiteral("Clipboard is empty, nothing to convert"));
        return {};
    }

    const auto direction = resolveDirectionForCurrentLayout();
    if (!direction.has_value()) {
        return {};
    }

    const QString converted = textConverter_.convertLastWordInText(sourceText, direction.value());
    replaceClipboardText(converted);
    notifyInfo(QStringLiteral("Last clipboard token converted"));
    return converted;
}

QString ApplicationController::convertSelection() {
    if (!conversionPermitted()) {
        return {};
    }

    if (selectionConversionPending_) {
        notifyInfo(QStringLiteral("Selection conversion is already in progress"));
        return {};
    }
    selectionConversionPending_ = true;
    // A global shortcut is delivered while Super may still be physically held.
    // Waiting briefly prevents the injected Ctrl+C from becoming Super+Ctrl+C.
    QTimer::singleShot(220, this, &ApplicationController::beginSelectionCopy);
    return QStringLiteral("pending");
}

void ApplicationController::beginSelectionCopy() {
    if (!selectionConversionPending_) {
        return;
    }

    // Isolate this request from every older Klipper history update. The source
    // text is accepted only after it replaces this unique marker.
    clipboardBeforeSelectionCopy_ = QStringLiteral("__flamentine_selection_%1__")
                                      .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    setKlipperClipboardText(clipboardBeforeSelectionCopy_);
    copiedSelectionText_.clear();
    selectionClipboardWaitAttempts_ = 0;
    QTimer::singleShot(60, this, &ApplicationController::sendSelectionCopy);
}

void ApplicationController::sendSelectionCopy() {
    if (!selectionConversionPending_) {
        return;
    }
    if (!textInputBackend_.copyActiveSelection()) {
        selectionConversionPending_ = false;
        notifyWarning(textInputBackend_.lastError());
        return;
    }

    selectionClipboardWaitTimer_.start();
}

void ApplicationController::waitForSelectionClipboard() {
    if (!selectionConversionPending_) {
        selectionClipboardWaitTimer_.stop();
        return;
    }

    const QString currentClipboardText = clipboardText();
    ++selectionClipboardWaitAttempts_;
    // Klipper updates asynchronously on Wayland.  Prefer a confirmed change, but
    // still handle copying the same text that was already in the clipboard.
    if (currentClipboardText != clipboardBeforeSelectionCopy_ && !currentClipboardText.trimmed().isEmpty()) {
        copiedSelectionText_ = currentClipboardText;
        selectionClipboardWaitTimer_.stop();
        finishSelectionConversion();
    } else if (selectionClipboardWaitAttempts_ >= 25) {
        selectionClipboardWaitTimer_.stop();
        selectionConversionPending_ = false;
        notifyWarning(QStringLiteral("Ctrl+C did not put selected text into the clipboard"));
    }
}

void ApplicationController::finishSelectionConversion() {
    selectionConversionPending_ = false;
    const QString sourceText = copiedSelectionText_;
    copiedSelectionText_.clear();
    clipboardBeforeSelectionCopy_.clear();
    const QString converted = convertSelectionClipboardText(sourceText, Conversion::ConversionDirection::AutoDetect);
    if (converted.isEmpty()) {
        return;
    }
    if (converted == sourceText) {
        notifyInfo(QStringLiteral("Selection contains no convertible letters"));
        return;
    }

    if (textInputBackend_.replaceActiveSelection()) {
        if (config_.conversion.switchLayoutAfterConversion) {
            // Auto-detection can convert a mixed-script selection. Keep the
            // active layout aligned with the final convertible segment so the
            // next character continues that segment in the expected layout.
            const auto direction = directionForTrailingScript(sourceText);
            if (direction.has_value()) {
                const QString targetLayoutId = targetLayoutIdForDirection(direction.value());
                if (config_.layouts.contains(targetLayoutId) && currentLayout() != targetLayoutId) {
                    setLayout(targetLayoutId);
                }
            }
        }
        notifyInfo(QStringLiteral("Selection keyboard layout converted"));
    } else {
        notifyWarning(textInputBackend_.lastError());
    }
}

QString ApplicationController::convertSelectionClipboardText(const QString& sourceText, const Conversion::ConversionDirection direction) {
    if (sourceText.trimmed().isEmpty()) {
        notifyWarning(QStringLiteral("Ctrl+C did not put selected text into the clipboard"));
        return {};
    }

    const QString converted = textConverter_.convertText(sourceText, direction);
    replaceClipboardText(converted);
    return converted;
}

bool ApplicationController::enable() {
    return setEnabled(true);
}

bool ApplicationController::disable() {
    return setEnabled(false);
}

bool ApplicationController::openSettings() {
    updateCurrentTargetUi(windowBackend_.currentContext());
    settingsWindow_.show();
    settingsWindow_.raise();
    settingsWindow_.activateWindow();
    return true;
}

bool ApplicationController::copyCurrentTargetInfo() {
    const WindowContext context = windowBackend_.currentContext();
    if (context.appName.trimmed().isEmpty() && context.windowClass.trimmed().isEmpty() && context.windowId.trimmed().isEmpty()) {
        notifyWarning(windowBackend_.lastError().isEmpty() ? QStringLiteral("Unable to identify the active target")
                                                           : windowBackend_.lastError());
        return false;
    }

    const QString info = formatTargetInfo(context);
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(info, QClipboard::Clipboard);
    clipboard->setText(info, QClipboard::Selection);
    notifyInfo(QStringLiteral("Copied current target info to the clipboard"));
    return true;
}

bool ApplicationController::allowCurrentApp() {
    return allowCurrentTargetEntries(true, false);
}

bool ApplicationController::allowCurrentWindowClass() {
    return allowCurrentTargetEntries(false, true);
}

bool ApplicationController::allowCurrentTarget() {
    return allowCurrentTargetEntries(true, true);
}

bool ApplicationController::allowCurrentTargetEntries(const bool allowApp, const bool allowWindowClass) {
    const WindowContext context = windowBackend_.currentContext();
    if (context.appName.trimmed().isEmpty() && context.windowClass.trimmed().isEmpty()) {
        notifyWarning(windowBackend_.lastError().isEmpty() ? QStringLiteral("Unable to identify the active target")
                                                           : windowBackend_.lastError());
        return false;
    }

    AppConfig updatedConfig = config_;
    QStringList addedEntries;
    if (allowApp && appendIfMissing(updatedConfig.allowedApps, context.appName)) {
        addedEntries.append(QStringLiteral("app=%1").arg(context.appName.trimmed()));
    }
    if (allowWindowClass && appendIfMissing(updatedConfig.allowedWindowClasses, context.windowClass)) {
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
    clearLayoutMemoryIfDisabled();
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
    const WindowContext context = windowBackend_.currentContext();
    const QString currentLayoutId = layoutBackend_.currentLayoutId();
    if (!syncingRememberedLayout_) {
        syncRememberedLayout(context, currentLayoutId);
    }

    const QString layoutId = layoutBackend_.currentLayoutId();
    updateCurrentTargetUi(context);
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
    clearLayoutMemoryIfDisabled();
    if (!settingsManager_.save(config_)) {
        notifyWarning(settingsManager_.lastError());
    }
    emit enabledChanged(enabled);
    return true;
}

QString ApplicationController::clipboardText() const {
    // On Plasma Wayland, a background Qt client cannot always read another
    // application's selection. Klipper owns the compositor clipboard and can.
    QDBusInterface klipper(QStringLiteral("org.kde.klipper"),
                           QStringLiteral("/klipper"),
                           QStringLiteral("org.kde.klipper.klipper"));
    if (klipper.isValid()) {
        const QDBusReply<QString> result = klipper.call(QStringLiteral("getClipboardContents"));
        if (result.isValid() && !result.value().trimmed().isEmpty()) {
            return result.value();
        }
    }

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

    setKlipperClipboardText(text);
}

void ApplicationController::setKlipperClipboardText(const QString& text) const {
    QDBusInterface klipper(QStringLiteral("org.kde.klipper"),
                           QStringLiteral("/klipper"),
                           QStringLiteral("org.kde.klipper.klipper"));
    if (klipper.isValid()) {
        klipper.call(QStringLiteral("setClipboardContents"), text);
    }
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
    updateCurrentTargetUi(context);
}

QString ApplicationController::formatTargetInfo(const WindowContext& context) const {
    return QStringLiteral("app=%1\nwm_class=%2\nwindow_id=%3\nfullscreen=%4")
        .arg(context.appName.trimmed().isEmpty() ? QStringLiteral("—") : context.appName.trimmed(),
             context.windowClass.trimmed().isEmpty() ? QStringLiteral("—") : context.windowClass.trimmed(),
             context.windowId.trimmed().isEmpty() ? QStringLiteral("—") : context.windowId.trimmed(),
             context.fullscreen ? QStringLiteral("yes") : QStringLiteral("no"));
}

void ApplicationController::updateCurrentTargetUi(const WindowContext& context) {
    trayIcon_.setCurrentTargetContext(context, windowBackend_.lastError());
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

void ApplicationController::clearLayoutMemoryIfDisabled() {
    if (!config_.rememberLayoutPerWindow && !config_.rememberLayoutPerApp) {
        layoutMemory_.clear();
        persistLayoutMemory();
    }
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

std::optional<Conversion::ConversionDirection> ApplicationController::conversionDirectionForLayout(const QString& layoutId) const {
    if (layoutId == QStringLiteral("us")) {
        return Conversion::ConversionDirection::UsToRu;
    }
    if (layoutId == QStringLiteral("ru")) {
        return Conversion::ConversionDirection::RuToUs;
    }
    return std::nullopt;
}

QString ApplicationController::targetLayoutIdForDirection(const Conversion::ConversionDirection direction) const {
    return direction == Conversion::ConversionDirection::UsToRu ? QStringLiteral("ru") : QStringLiteral("us");
}

bool ApplicationController::conversionPermitted() const {
    if (!config_.enabled) {
        notifyInfo(QStringLiteral("Application is disabled"));
        return false;
    }

    const WindowContext context = windowBackend_.currentContext();
    if (!Rules::isAllowed(config_, context)) {
        notifyInfo(targetDeniedMessage(context));
        return false;
    }
    return true;
}

std::optional<Conversion::ConversionDirection> ApplicationController::resolveDirectionForCurrentLayout() const {
    const auto direction = conversionDirectionForLayout(currentLayout());
    if (!direction.has_value()) {
        notifyWarning(QStringLiteral("Current layout is not supported for RU/EN conversion"));
    }
    return direction;
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
