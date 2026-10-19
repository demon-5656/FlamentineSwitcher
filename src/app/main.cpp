#include <QApplication>
#include <QAction>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

#include <KGlobalAccel>

#include "flamentine_switcher/backends/hotkeys/noop_hotkey_backend.h"
#include "flamentine_switcher/backends/hotkeys/x11_hotkey_backend.h"
#include "flamentine_switcher/backends/layout/wayland_layout_backend.h"
#include "flamentine_switcher/backends/layout/x11_layout_backend.h"
#include "flamentine_switcher/backends/text/noop_text_input_backend.h"
#include "flamentine_switcher/backends/text/x11_text_input_backend.h"
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

namespace {

int runKdeWaylandPreflight() {
    QTextStream output(stdout);
    QTextStream errors(stderr);
    bool passed = true;

    output << "FlamentineSwitcher KDE Wayland preflight\n";
    const QString ydotool = QStandardPaths::findExecutable(QStringLiteral("ydotool"));
    if (ydotool.isEmpty()) {
        errors << "FAIL: ydotool is not installed. Install the ydotool package.\n";
        return 2;
    }
    output << "OK: ydotool found at " << ydotool << "\n";

    QProcess ydotoolDebug;
    ydotoolDebug.start(ydotool, {QStringLiteral("debug")});
    if (!ydotoolDebug.waitForStarted(1000) || !ydotoolDebug.waitForFinished(3000) || ydotoolDebug.exitCode() != 0) {
        errors << "FAIL: ydotool cannot reach its virtual input daemon. Start it with:\n"
               << "  systemctl --user enable --now ydotool.service\n";
        passed = false;
    } else {
        output << "OK: ydotool virtual input daemon is reachable\n";
    }

    QDBusInterface klipper(QStringLiteral("org.kde.klipper"),
                           QStringLiteral("/klipper"),
                           QStringLiteral("org.kde.klipper.klipper"));
    if (!klipper.isValid()) {
        errors << "FAIL: KDE Klipper D-Bus service is unavailable. Enable the Clipboard applet in Plasma.\n";
        passed = false;
    } else {
        output << "OK: KDE Klipper service is available\n";
    }

    QDBusInterface keyboardLayouts(QStringLiteral("org.kde.keyboard"),
                                   QStringLiteral("/Layouts"),
                                   QStringLiteral("org.kde.KeyboardLayouts"));
    if (!keyboardLayouts.isValid()) {
        errors << "FAIL: KDE keyboard layout service is unavailable. Enable the Keyboard Layouts Plasma widget.\n";
        passed = false;
    } else {
        output << "OK: KDE keyboard layout service is available\n";
    }

    const QString launcherPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                          QStringLiteral("applications/flamentine-transliterate-selection.desktop"),
                                                          QStandardPaths::LocateFile);
    if (launcherPath.isEmpty()) {
        errors << "FAIL: KDE shortcut launcher is not installed. Reinstall FlamentineSwitcher.\n";
        passed = false;
    } else {
        QSettings launcher(launcherPath, QSettings::IniFormat);
        launcher.beginGroup(QStringLiteral("Desktop Entry"));
        const QString command = launcher.value(QStringLiteral("Exec")).toString();
        launcher.endGroup();
        if (!command.contains(QStringLiteral("--transliterate-selection"))) {
            errors << "FAIL: KDE shortcut launcher is malformed: " << launcherPath << "\n";
            passed = false;
        } else {
            output << "OK: KDE action launcher is installed at " << launcherPath << "\n";
        }
    }

    QDBusInterface globalAccel(QStringLiteral("org.kde.kglobalaccel"),
                               QStringLiteral("/kglobalaccel"),
                               QStringLiteral("org.kde.KGlobalAccel"));
    if (!globalAccel.isValid()) {
        errors << "FAIL: KDE Global Shortcuts service is unavailable. Restart Plasma.\n";
        passed = false;
    } else {
        output << "OK: KDE Global Shortcuts service is available\n";
    }

    QDBusInterface control(QStringLiteral("org.flamentineswitcher.Control"),
                           QStringLiteral("/org/flamentineswitcher/Control"),
                           QStringLiteral("org.flamentineswitcher.Control"));
    if (!control.isValid()) {
        errors << "FAIL: FlamentineSwitcher is not running. Start it from the Plasma menu, then run this check again.\n";
        passed = false;
    } else {
        output << "OK: FlamentineSwitcher D-Bus control service is available\n";
    }

    output << (passed ? "PASS: KDE Wayland prerequisites are ready.\n" : "FAIL: fix the checks above and run again.\n");
    return passed ? 0 : 2;
}

}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("FlamentineSwitcher"));
    QApplication::setApplicationDisplayName(QStringLiteral("FlamentineSwitcher"));
    QApplication::setDesktopFileName(QStringLiteral("flamentine-switcher"));
    QApplication::setOrganizationDomain(QStringLiteral("github.com/demon-5656"));
    QApplication::setQuitOnLastWindowClosed(false);

    if (app.arguments().contains(QStringLiteral("--check-wayland"))) {
        return runKdeWaylandPreflight();
    }

    // This tiny client mode is intended for KDE's Custom Shortcuts.  It does not
    // start a second resident process; it asks the existing daemon to process the
    // current selection and exits immediately.
    if (app.arguments().contains(QStringLiteral("--transliterate-selection"))) {
        QDBusInterface control(QStringLiteral("org.flamentineswitcher.Control"),
                               QStringLiteral("/org/flamentineswitcher/Control"),
                               QStringLiteral("org.flamentineswitcher.Control"));
        if (!control.isValid()) {
            qWarning() << "FlamentineSwitcher is not running";
            return 1;
        }

        const QDBusReply<QString> result = control.call(QStringLiteral("ConvertSelection"));
        if (!result.isValid()) {
            qWarning().noquote() << result.error().message();
            return 1;
        }
        return 0;
    }

    qRegisterMetaType<FlamentineSwitcher::Core::AppConfig>();
    qRegisterMetaType<FlamentineSwitcher::Core::HotkeyAction>();
    qRegisterMetaType<FlamentineSwitcher::Core::WindowContext>();

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
    std::unique_ptr<FlamentineSwitcher::Backends::Text::ITextInputBackend> textInputBackend =
        std::make_unique<FlamentineSwitcher::Backends::Text::NoopTextInputBackend>();
    if (FlamentineSwitcher::Utils::ProcessInfo::currentSessionType() == SessionType::X11) {
        auto x11TextInputBackend = std::make_unique<FlamentineSwitcher::Backends::Text::X11TextInputBackend>();
        if (x11TextInputBackend->isSupported()) {
            textInputBackend = std::move(x11TextInputBackend);
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
                                                               *textInputBackend,
                                                               *windowBackend,
                                                               trayIcon,
                                                               settingsWindow,
                                                               notifications);

    FlamentineSwitcher::Services::DbusService dbusService(controller);
    dbusService.start();

    QAction convertSelectionShortcut(&app);
    if (FlamentineSwitcher::Utils::ProcessInfo::currentSessionType() == SessionType::Wayland) {
        convertSelectionShortcut.setObjectName(QStringLiteral("convert-selection"));
        convertSelectionShortcut.setText(QStringLiteral("Convert selected text"));
        if (!KGlobalAccel::setGlobalShortcut(&convertSelectionShortcut,
                                             QKeySequence::fromString(QStringLiteral("Meta+End"), QKeySequence::PortableText))) {
            qWarning() << "Unable to register KDE global shortcut Meta+End";
        }
        QObject::connect(&convertSelectionShortcut,
                         &QAction::triggered,
                         &controller,
                         &FlamentineSwitcher::Core::ApplicationController::convertSelection);
    }

    controller.initialize();
    return app.exec();
}
