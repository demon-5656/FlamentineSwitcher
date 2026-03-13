#pragma once

#include "flamentine_switcher/backends/window/iwindow_backend.h"

struct _XDisplay;

namespace FlamentineSwitcher::Backends::Window {

class X11WindowBackend : public IWindowBackend {
    Q_OBJECT

public:
    explicit X11WindowBackend(QObject* parent = nullptr);
    ~X11WindowBackend() override;

    bool isSupported() const override;
    QString backendName() const override;
    FlamentineSwitcher::Core::WindowContext currentContext() const override;
    QString lastError() const override;

private:
    mutable QString lastError_;
    mutable unsigned long cachedWindow_ = 0;
    mutable FlamentineSwitcher::Core::WindowContext cachedContext_;
    _XDisplay* display_ = nullptr;
};

}  // namespace FlamentineSwitcher::Backends::Window
