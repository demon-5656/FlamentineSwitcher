#pragma once

#include "flamentine_switcher/backends/layout/ilayout_backend.h"

struct _XDisplay;

namespace FlamentineSwitcher::Backends::Layout {

class X11LayoutBackend : public ILayoutBackend {
    Q_OBJECT

public:
    explicit X11LayoutBackend(QObject* parent = nullptr);
    ~X11LayoutBackend() override;

    bool isSupported() const override;
    QString backendName() const override;
    QList<FlamentineSwitcher::Core::LayoutInfo> listLayouts() const override;
    QString currentLayoutId() const override;
    bool setLayout(const QString& layoutId) override;
    bool toggleLayout() override;
    QString lastError() const override;

private:
    bool refreshCachedLayouts() const;

    _XDisplay* display_ = nullptr;
    mutable QList<FlamentineSwitcher::Core::LayoutInfo> cachedLayouts_;
    mutable QString lastError_;
};

}  // namespace FlamentineSwitcher::Backends::Layout

