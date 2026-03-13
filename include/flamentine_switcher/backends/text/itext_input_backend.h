#pragma once

#include <QObject>
#include <QString>

#include "flamentine_switcher/core/config.h"
#include "flamentine_switcher/core/models.h"

namespace FlamentineSwitcher::Backends::Text {

class ITextInputBackend : public QObject {
    Q_OBJECT

public:
    explicit ITextInputBackend(QObject* parent = nullptr)
        : QObject(parent) {
    }

    ~ITextInputBackend() override = default;

    virtual bool isSupported() const = 0;
    virtual QString backendName() const = 0;
    virtual void applyConfig(const FlamentineSwitcher::Core::AppConfig& config) = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual bool replacePendingWord(quint64 tokenId, const QString& replacement) = 0;
    virtual QString lastError() const = 0;

signals:
    void wordCommitted(quint64 tokenId, const QString& word, const FlamentineSwitcher::Core::WindowContext& context);
};

}  // namespace FlamentineSwitcher::Backends::Text
