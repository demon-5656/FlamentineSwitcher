#pragma once

#include <QObject>
#include <QString>

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
    virtual bool copyActiveSelection() = 0;
    // Replaces the active X11 selection by pasting the already prepared clipboard text.
    // Backends that cannot inject input must return false and explain why in lastError().
    virtual bool replaceActiveSelection() = 0;
    virtual QString lastError() const = 0;
};

}  // namespace FlamentineSwitcher::Backends::Text
