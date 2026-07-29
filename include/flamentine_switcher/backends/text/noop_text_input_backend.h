#pragma once

#include "flamentine_switcher/backends/text/itext_input_backend.h"

namespace FlamentineSwitcher::Backends::Text {

class NoopTextInputBackend : public ITextInputBackend {
    Q_OBJECT

public:
    explicit NoopTextInputBackend(QObject* parent = nullptr);

    bool isSupported() const override;
    QString backendName() const override;
    void applyConfig(const FlamentineSwitcher::Core::AppConfig& config) override;
    void setEnabled(bool enabled) override;
    bool copyActiveSelection() override;
    bool replaceActiveSelection() override;
    bool replacePendingWord(quint64 tokenId, const QString& replacement) override;
    QString lastError() const override;

private:
    bool injectShortcut(int keyCode) const;
    mutable QString lastError_;
};

}  // namespace FlamentineSwitcher::Backends::Text
