#pragma once

#include "flamentine_switcher/backends/text/itext_input_backend.h"

struct _XDisplay;

namespace FlamentineSwitcher::Backends::Text {

class X11TextInputBackend : public ITextInputBackend {
    Q_OBJECT

public:
    explicit X11TextInputBackend(QObject* parent = nullptr);
    ~X11TextInputBackend() override;

    bool isSupported() const override;
    QString backendName() const override;
    bool copyActiveSelection() override;
    bool replaceActiveSelection() override;
    QString lastError() const override;

private:
    _XDisplay* display_ = nullptr;
    mutable QString lastError_;
};

}  // namespace FlamentineSwitcher::Backends::Text
