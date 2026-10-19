#include "flamentine_switcher/backends/text/noop_text_input_backend.h"

#include <QProcess>
#include <QStandardPaths>

namespace FlamentineSwitcher::Backends::Text {

NoopTextInputBackend::NoopTextInputBackend(QObject* parent)
    : ITextInputBackend(parent) {
}

bool NoopTextInputBackend::isSupported() const {
    return !QStandardPaths::findExecutable(QStringLiteral("ydotool")).isEmpty();
}

QString NoopTextInputBackend::backendName() const {
    return QStringLiteral("ydotool-input-injector");
}


bool NoopTextInputBackend::copyActiveSelection() {
    return injectShortcut(46); // Linux input event KEY_C
}

bool NoopTextInputBackend::replaceActiveSelection() {
    return injectShortcut(47); // Linux input event KEY_V
}

bool NoopTextInputBackend::injectShortcut(const int keyCode) const {
    const QString executable = QStandardPaths::findExecutable(QStringLiteral("ydotool"));
    if (executable.isEmpty()) {
        lastError_ = QStringLiteral("ydotool is not installed; Wayland cannot inject Ctrl+C/Ctrl+V");
        return false;
    }

    const int exitCode = QProcess::execute(executable,
                                            {QStringLiteral("key"),
                                             QStringLiteral("29:1"),
                                             QStringLiteral("%1:1").arg(keyCode),
                                             QStringLiteral("%1:0").arg(keyCode),
                                             QStringLiteral("29:0")});
    if (exitCode != 0) {
        lastError_ = QStringLiteral("ydotool failed (%1). Start: systemctl --user enable --now ydotool.service").arg(exitCode);
        return false;
    }
    lastError_.clear();
    return true;
}


QString NoopTextInputBackend::lastError() const {
    return lastError_;
}

}  // namespace FlamentineSwitcher::Backends::Text
