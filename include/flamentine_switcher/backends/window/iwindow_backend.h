#pragma once

#include <QObject>

#include "flamentine_switcher/core/models.h"

namespace FlamentineSwitcher::Backends::Window {

class IWindowBackend : public QObject {
    Q_OBJECT

public:
    explicit IWindowBackend(QObject* parent = nullptr)
        : QObject(parent) {
    }

    virtual ~IWindowBackend() = default;

    virtual bool isSupported() const = 0;
    virtual QString backendName() const = 0;
    virtual FlamentineSwitcher::Core::WindowContext currentContext() const = 0;
    virtual QString lastError() const = 0;
};

}  // namespace FlamentineSwitcher::Backends::Window

