#pragma once

#include <QObject>
#include <QTimer>

#include "flamentine_switcher/core/config.h"
#include "flamentine_switcher/core/layout_memory.h"
#include "flamentine_switcher/core/models.h"
#include "flamentine_switcher/conversion/text_converter.h"

namespace FlamentineSwitcher::Backends::Hotkeys {
class IHotkeyBackend;
}

namespace FlamentineSwitcher::Backends::Text {
class ITextInputBackend;
}

namespace FlamentineSwitcher::Backends::Layout {
class ILayoutBackend;
}

namespace FlamentineSwitcher::Backends::Window {
class IWindowBackend;
}

namespace FlamentineSwitcher::Services {
class AutostartService;
}

namespace FlamentineSwitcher::Ui {
class Notifications;
class SettingsWindow;
class TrayIcon;
}

namespace FlamentineSwitcher::Core {

class SettingsManager;

class ApplicationController : public QObject {
    Q_OBJECT

public:
    ApplicationController(SettingsManager& settingsManager,
                          Services::AutostartService& autostartService,
                          Backends::Layout::ILayoutBackend& layoutBackend,
                          Backends::Hotkeys::IHotkeyBackend& hotkeyBackend,
                          Backends::Text::ITextInputBackend& textInputBackend,
                          Backends::Window::IWindowBackend& windowBackend,
                          Ui::TrayIcon& trayIcon,
                          Ui::SettingsWindow& settingsWindow,
                          Ui::Notifications& notifications,
                          QObject* parent = nullptr);

    void initialize();

    QString currentLayout() const;
    const AppConfig& config() const;
    QString lastError() const;

public slots:
    bool setLayout(const QString& layoutId);
    bool toggleLayout();
    QString convertLastWord();
    QString convertSelection();
    bool enable();
    bool disable();
    bool openSettings();
    bool allowCurrentTarget();
    void applyConfig(const AppConfig& config);
    void importConfig(const QString& filePath);
    void exportConfig(const QString& filePath);

signals:
    void layoutChanged(const QString& layoutId);
    void enabledChanged(bool enabled);

private slots:
    void refreshLayout();
    void handleHotkeyAction(FlamentineSwitcher::Core::HotkeyAction action);
    void handleObservedWordCommitted(quint64 tokenId, const QString& word, const FlamentineSwitcher::Core::WindowContext& context);
    void processPendingAutoConversion();

private:
    struct PendingAutoConversion {
        quint64 tokenId = 0;
        QString word;
        WindowContext context;
        bool valid = false;
    };

    bool setEnabled(bool enabled);
    QString clipboardText() const;
    void replaceClipboardText(const QString& text) const;
    void registerHotkeys();
    void rememberCurrentTargetLayout(const QString& layoutId);
    void syncRememberedLayout(const WindowContext& context, const QString& currentLayoutId);
    void updateTextInputBackendState();
    void scheduleLayoutMemoryPersist();
    void persistLayoutMemory();
    QString targetLayoutIdForDirection(Conversion::ConversionDirection direction) const;
    void notifyInfo(const QString& message) const;
    void notifyWarning(const QString& message) const;

    SettingsManager& settingsManager_;
    Services::AutostartService& autostartService_;
    Backends::Layout::ILayoutBackend& layoutBackend_;
    Backends::Hotkeys::IHotkeyBackend& hotkeyBackend_;
    Backends::Text::ITextInputBackend& textInputBackend_;
    Backends::Window::IWindowBackend& windowBackend_;
    Ui::TrayIcon& trayIcon_;
    Ui::SettingsWindow& settingsWindow_;
    Ui::Notifications& notifications_;
    Conversion::TextConverter textConverter_;
    LayoutMemory layoutMemory_;
    AppConfig config_;
    QString lastError_;
    QTimer refreshTimer_;
    QTimer autoConvertTimer_;
    QTimer layoutMemoryPersistTimer_;
    PendingAutoConversion pendingAutoConversion_;
    WindowContext lastWindowContext_;
    bool syncingRememberedLayout_ = false;
};

}  // namespace FlamentineSwitcher::Core
