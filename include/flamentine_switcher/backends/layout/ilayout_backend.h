#pragma once

#include <QObject>

#include "flamentine_switcher/core/models.h"

namespace FlamentineSwitcher::Backends::Layout {

class ILayoutBackend : public QObject {
    Q_OBJECT

public:
    explicit ILayoutBackend(QObject* parent = nullptr)
        : QObject(parent) {
    }

    virtual ~ILayoutBackend() = default;

    virtual bool isSupported() const = 0;
    virtual QString backendName() const = 0;
    virtual QList<FlamentineSwitcher::Core::LayoutInfo> listLayouts() const = 0;
    virtual QString currentLayoutId() const = 0;
    virtual bool setLayout(const QString& layoutId) = 0;
    virtual bool toggleLayout() = 0;
    virtual QString lastError() const = 0;

signals:
    void layoutChanged(const QString& layoutId);
};

}  // namespace FlamentineSwitcher::Backends::Layout

