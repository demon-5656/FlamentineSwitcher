#pragma once

#include "flamentine_switcher/backends/window/iwindow_backend.h"

namespace FlamentineSwitcher::Backends::Window {

class FallbackWindowBackend : public IWindowBackend {
    Q_OBJECT

public:
    explicit FallbackWindowBackend(QObject* parent = nullptr);

    bool isSupported() const override;
    QString backendName() const override;
    FlamentineSwitcher::Core::WindowContext currentContext() const override;
    QString lastError() const override;

private:
    QString lastError_;
};

}  // namespace FlamentineSwitcher::Backends::Window

