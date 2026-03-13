#include "flamentine_switcher/backends/text/noop_text_input_backend.h"

namespace FlamentineSwitcher::Backends::Text {

NoopTextInputBackend::NoopTextInputBackend(QObject* parent)
    : ITextInputBackend(parent)
    , lastError_(QStringLiteral("Typed-text observation is unavailable in the current session")) {
}

bool NoopTextInputBackend::isSupported() const {
    return false;
}

QString NoopTextInputBackend::backendName() const {
    return QStringLiteral("noop-text-input");
}

void NoopTextInputBackend::applyConfig(const FlamentineSwitcher::Core::AppConfig& config) {
    Q_UNUSED(config)
}

void NoopTextInputBackend::setEnabled(const bool enabled) {
    Q_UNUSED(enabled)
}

bool NoopTextInputBackend::replacePendingWord(const quint64 tokenId, const QString& replacement) {
    Q_UNUSED(tokenId)
    Q_UNUSED(replacement)
    return false;
}

QString NoopTextInputBackend::lastError() const {
    return lastError_;
}

}  // namespace FlamentineSwitcher::Backends::Text
