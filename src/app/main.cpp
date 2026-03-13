#include <QApplication>

#include "flamentine_switcher/backends/hotkeys/noop_hotkey_backend.h"
#include "flamentine_switcher/backends/hotkeys/x11_hotkey_backend.h"
#include "flamentine_switcher/backends/layout/wayland_layout_backend.h"
#include "flamentine_switcher/backends/layout/x11_layout_backend.h"
#include "flamentine_switcher/backends/window/fallback_window_backend.h"
#include "flamentine_switcher/backends/window/x11_window_backend.h"
#include "flamentine_switcher/core/application_controller.h"
#include "flamentine_switcher/core/settings_manager.h"
#include "flamentine_switcher/services/autostart_service.h"
#include "flamentine_switcher/services/dbus_service.h"
#include "flamentine_switcher/ui/notifications.h"
#include "flamentine_switcher/ui/settings_window.h"
#include "flamentine_switcher/ui/tray_icon.h"
#include "flamentine_switcher/utils/logging.h"
#include "flamentine_switcher/utils/process_info.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("FlamentineSwitcher"));
    QApplication::setApplicationDisplayName(QStringLiteral("FlamentineSwitcher"));
    QApplication::setOrganizationDomain(QStringLiteral("github.com/demon-5656"));
    QApplication::setQuitOnLastWindowClosed(false);

    qRegisterMetaType<FlamentineSwitcher::Core::AppConfig>();
    qRegisterMetaType<FlamentineSwitcher::Core::HotkeyAction>();

    FlamentineSwitcher::Core::SettingsManager settingsManager;
    FlamentineSwitcher::Utils::Logging::initialize(settingsManager.load().logging);

    using SessionType = FlamentineSwitcher::Utils::ProcessInfo::SessionType;

    std::unique_ptr<FlamentineSwitcher::Backends::Layout::ILayoutBackend> layoutBackend =
        std::make_unique<FlamentineSwitcher::Backends::Layout::WaylandLayoutBackend>();
    std::unique_ptr<FlamentineSwitcher::Backends::Hotkeys::IHotkeyBackend> hotkeyBackend =
        std::make_unique<FlamentineSwitcher::Backends::Hotkeys::NoopHotkeyBackend>();

    if (FlamentineSwitcher::Utils::ProcessInfo::currentSessionType() == SessionType::X11) {
        auto x11LayoutBackend = std::make_unique<FlamentineSwitcher::Backends::Layout::X11LayoutBackend>();
        if (x11LayoutBackend->isSupported()) {
            layoutBackend = std::move(x11LayoutBackend);
        }

        auto x11HotkeyBackend = std::make_unique<FlamentineSwitcher::Backends::Hotkeys::X11HotkeyBackend>();
        if (x11HotkeyBackend->isSupported()) {
            hotkeyBackend = std::move(x11HotkeyBackend);
        }
    }

    std::unique_ptr<FlamentineSwitcher::Backends::Window::IWindowBackend> windowBackend =
        std::make_unique<FlamentineSwitcher::Backends::Window::FallbackWindowBackend>();
    if (FlamentineSwitcher::Utils::ProcessInfo::currentSessionType() == SessionType::X11) {
        auto x11WindowBackend = std::make_unique<FlamentineSwitcher::Backends::Window::X11WindowBackend>();
        if (x11WindowBackend->isSupported()) {
            windowBackend = std::move(x11WindowBackend);
        }
    }
    FlamentineSwitcher::Ui::TrayIcon trayIcon;
    FlamentineSwitcher::Ui::Notifications notifications(trayIcon.systemTrayIcon());
    FlamentineSwitcher::Ui::SettingsWindow settingsWindow;
    FlamentineSwitcher::Services::AutostartService autostartService;

    FlamentineSwitcher::Core::ApplicationController controller(settingsManager,
                                                               autostartService,
                                                               *layoutBackend,
                                                               *hotkeyBackend,
                                                               *windowBackend,
                                                               trayIcon,
                                                               settingsWindow,
                                                               notifications);

    FlamentineSwitcher::Services::DbusService dbusService(controller);
    dbusService.start();

    controller.initialize();
    return app.exec();
}
