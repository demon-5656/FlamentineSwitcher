#pragma once

#include <QList>

#include "flamentine_switcher/backends/layout/ilayout_backend.h"

namespace FlamentineSwitcher::Backends::Layout {

class WaylandLayoutBackend : public ILayoutBackend {
    Q_OBJECT

public:
    explicit WaylandLayoutBackend(QObject* parent = nullptr);

    bool isSupported() const override;
    QString backendName() const override;
    QList<FlamentineSwitcher::Core::LayoutInfo> listLayouts() const override;
    QString currentLayoutId() const override;
    bool setLayout(const QString& layoutId) override;
    bool toggleLayout() override;
    QString lastError() const override;

private slots:
    void handleKdeLayoutChanged(uint index);
    void handleKdeLayoutListChanged();

private:
    bool refreshLayouts() const;
    int currentLayoutIndex() const;

    mutable QList<FlamentineSwitcher::Core::LayoutInfo> cachedLayouts_;
    mutable QString lastError_;
};

}  // namespace FlamentineSwitcher::Backends::Layout
