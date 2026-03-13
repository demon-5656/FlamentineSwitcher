#pragma once

#include <QTimer>

#include "flamentine_switcher/backends/text/itext_input_backend.h"

struct _XDisplay;

namespace FlamentineSwitcher::Backends::Window {
class IWindowBackend;
}

namespace FlamentineSwitcher::Backends::Text {

class X11TextInputBackend : public ITextInputBackend {
    Q_OBJECT

public:
    explicit X11TextInputBackend(FlamentineSwitcher::Backends::Window::IWindowBackend& windowBackend,
                                 QObject* parent = nullptr);
    ~X11TextInputBackend() override;

    bool isSupported() const override;
    QString backendName() const override;
    void applyConfig(const FlamentineSwitcher::Core::AppConfig& config) override;
    void setEnabled(bool enabled) override;
    bool replacePendingWord(quint64 tokenId, const QString& replacement) override;
    QString lastError() const override;

private slots:
    void processXEvents();
    void resumeObservation();

private:
    struct PendingWord {
        quint64 tokenId = 0;
        QString word;
        QString suffix;
        QString windowId;
        int backspaceCount = 0;
        quint64 generationAtCommit = 0;
        bool valid = false;
    };

    struct ResolvedKeystroke {
        int keycode = 0;
        bool shift = false;
    };

    bool initializeXi2();
    void resetObservationState();
    void invalidatePendingWord();
    bool currentTargetAllowed(FlamentineSwitcher::Core::WindowContext& context) const;
    void handleKeyPress(void* eventData);
    void commitCurrentToken(const FlamentineSwitcher::Core::WindowContext& context, const QString& suffix = QString());
    bool activeWindowMatches(const QString& expectedWindowId) const;
    bool resolveKeystroke(QChar character, ResolvedKeystroke& stroke) const;
    void sendKey(int keycode, bool withShift) const;

    FlamentineSwitcher::Backends::Window::IWindowBackend& windowBackend_;
    _XDisplay* display_ = nullptr;
    QTimer eventPumpTimer_;
    int xiOpcode_ = -1;
    unsigned long rootWindow_ = 0;
    FlamentineSwitcher::Core::AppConfig config_ = FlamentineSwitcher::Core::AppConfig::defaults();
    mutable QString lastError_;
    QString currentToken_;
    PendingWord pendingWord_;
    quint64 generation_ = 0;
    quint64 nextTokenId_ = 1;
    bool enabled_ = false;
    bool suppressObservation_ = false;
};

}  // namespace FlamentineSwitcher::Backends::Text
