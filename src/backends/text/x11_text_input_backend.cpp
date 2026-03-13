#include "flamentine_switcher/backends/text/x11_text_input_backend.h"

#include <array>

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>

#include "flamentine_switcher/backends/window/iwindow_backend.h"
#include "flamentine_switcher/conversion/layout_maps.h"
#include "flamentine_switcher/core/rules.h"

namespace {

QString translatedText(Display* display, const int keycode, const unsigned int modifiers) {
    KeySym keysym = NoSymbol;
    unsigned int consumedModifiers = 0;
    if (!XkbLookupKeySym(display, keycode, modifiers, &consumedModifiers, &keysym) || keysym == NoSymbol) {
        return {};
    }

    char buffer[32] = {};
    KeySym translatedKeysym = keysym;
    const int byteCount = XkbTranslateKeySym(display, &translatedKeysym, modifiers, buffer, sizeof(buffer), nullptr);
    if (byteCount <= 0) {
        return {};
    }

    return QString::fromLocal8Bit(buffer, byteCount);
}

KeySym eventKeySym(Display* display, const XIDeviceEvent& event) {
    const unsigned int modifiers = static_cast<unsigned int>(event.mods.effective);
    KeySym keysym = NoSymbol;
    unsigned int consumedModifiers = 0;
    if (XkbLookupKeySym(display, static_cast<KeyCode>(event.detail), modifiers, &consumedModifiers, &keysym)) {
        return keysym;
    }

    return NoSymbol;
}

bool isModifierKeySym(const KeySym keysym) {
    switch (keysym) {
    case XK_Shift_L:
    case XK_Shift_R:
    case XK_Control_L:
    case XK_Control_R:
    case XK_Alt_L:
    case XK_Alt_R:
    case XK_Meta_L:
    case XK_Meta_R:
    case XK_Super_L:
    case XK_Super_R:
    case XK_ISO_Level3_Shift:
    case XK_Caps_Lock:
    case XK_Num_Lock:
        return true;
    default:
        return false;
    }
}

bool isResetKeySym(const KeySym keysym) {
    switch (keysym) {
    case XK_Escape:
    case XK_Delete:
    case XK_Home:
    case XK_End:
    case XK_Page_Up:
    case XK_Page_Down:
    case XK_Left:
    case XK_Right:
    case XK_Up:
    case XK_Down:
    case XK_Insert:
    case XK_Menu:
        return true;
    default:
        return (keysym >= XK_F1 && keysym <= XK_F35);
    }
}

bool isBoundaryKeySym(const KeySym keysym) {
    switch (keysym) {
    case XK_space:
    case XK_Return:
    case XK_KP_Enter:
    case XK_Tab:
        return true;
    default:
        return false;
    }
}

bool isWordCharacter(const QChar character) {
    return character.isLetterOrNumber() || character == QChar('\'') || character == QChar('-') || character == QChar('_');
}

bool isBoundaryCharacter(const QChar character) {
    static const QString delimiters = QStringLiteral(" \t\r\n.,;:!?()[]{}<>/\\\\|\"“”„«»");
    return character.isSpace() || delimiters.contains(character);
}

QString usPhysicalKeyForCharacter(const QChar character) {
    static const QString usLower = QStringLiteral("`1234567890-=qwertyuiop[]\\asdfghjkl;'zxcvbnm,./");
    static const QString usUpper = QStringLiteral("~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:\"ZXCVBNM<>?");

    const auto& maps = FlamentineSwitcher::Conversion::usRuMap();
    const QChar usEquivalent = maps.ruToUs.value(character, character);

    const int upperIndex = usUpper.indexOf(usEquivalent);
    if (upperIndex >= 0) {
        return QString(usLower.at(upperIndex)) + usEquivalent;
    }

    return QString(usEquivalent);
}

QString normalizedWindowId(const QString& value) {
    return value.trimmed().toLower();
}

}  // namespace

namespace FlamentineSwitcher::Backends::Text {

X11TextInputBackend::X11TextInputBackend(FlamentineSwitcher::Backends::Window::IWindowBackend& windowBackend, QObject* parent)
    : ITextInputBackend(parent)
    , windowBackend_(windowBackend) {
    display_ = XOpenDisplay(nullptr);
    if (display_ == nullptr) {
        lastError_ = QStringLiteral("Unable to connect to the X11 display for typed-text observation");
        return;
    }

    if (!initializeXi2()) {
        XCloseDisplay(display_);
        display_ = nullptr;
        return;
    }

    rootWindow_ = DefaultRootWindow(display_);
    // Secondary Xlib connections were unreliable with QSocketNotifier on nested X11 servers,
    // so the observer uses a lightweight timer pump while auto-conversion is enabled.
    eventPumpTimer_.setInterval(16);
    connect(&eventPumpTimer_, &QTimer::timeout, this, &X11TextInputBackend::processXEvents);
}

X11TextInputBackend::~X11TextInputBackend() {
    if (display_ != nullptr) {
        XCloseDisplay(display_);
    }
}

bool X11TextInputBackend::isSupported() const {
    return display_ != nullptr;
}

QString X11TextInputBackend::backendName() const {
    return QStringLiteral("x11-xi2-text-input");
}

void X11TextInputBackend::applyConfig(const FlamentineSwitcher::Core::AppConfig& config) {
    config_ = config;
    if (!config_.conversion.autoConvertEnabled) {
        resetObservationState();
    }
}

void X11TextInputBackend::setEnabled(const bool enabled) {
    enabled_ = enabled;
    if (enabled_ && !suppressObservation_ && display_ != nullptr) {
        eventPumpTimer_.start();
    } else {
        eventPumpTimer_.stop();
    }
    if (!enabled_) {
        resetObservationState();
    }
}

bool X11TextInputBackend::replacePendingWord(const quint64 tokenId, const QString& replacement) {
    if (display_ == nullptr) {
        lastError_ = QStringLiteral("X11 text input backend is unavailable");
        return false;
    }

    if (!pendingWord_.valid || pendingWord_.tokenId != tokenId) {
        lastError_ = QStringLiteral("No matching delayed replacement is pending");
        return false;
    }

    if (pendingWord_.generationAtCommit != generation_) {
        lastError_ = QStringLiteral("Typing continued before delayed replacement");
        invalidatePendingWord();
        return false;
    }

    if (!activeWindowMatches(pendingWord_.windowId)) {
        lastError_ = QStringLiteral("Active window changed before delayed replacement");
        invalidatePendingWord();
        return false;
    }

    const QString finalText = replacement + pendingWord_.suffix;

    QVector<ResolvedKeystroke> strokes;
    strokes.reserve(finalText.size());
    for (const QChar character : finalText) {
        ResolvedKeystroke stroke;
        if (!resolveKeystroke(character, stroke)) {
            lastError_ = QStringLiteral("Unable to resolve a keystroke for '%1' in the active layout").arg(character);
            return false;
        }
        strokes.append(stroke);
    }

    suppressObservation_ = true;
    eventPumpTimer_.stop();

    const KeyCode backspaceKeycode = XKeysymToKeycode(display_, XK_BackSpace);
    for (int index = 0; index < pendingWord_.backspaceCount; ++index) {
        sendKey(backspaceKeycode, false);
    }

    for (const ResolvedKeystroke& stroke : strokes) {
        sendKey(stroke.keycode, stroke.shift);
    }

    XFlush(display_);
    generation_++;
    resetObservationState();
    QTimer::singleShot(120, this, &X11TextInputBackend::resumeObservation);
    lastError_.clear();
    return true;
}

QString X11TextInputBackend::lastError() const {
    return lastError_;
}

void X11TextInputBackend::processXEvents() {
    if (display_ == nullptr) {
        return;
    }

    while (XPending(display_) > 0) {
        XEvent event {};
        XNextEvent(display_, &event);
        if (event.type != GenericEvent || event.xcookie.extension != xiOpcode_) {
            continue;
        }

        if (!XGetEventData(display_, &event.xcookie)) {
            continue;
        }

        if (event.xcookie.evtype == XI_KeyPress) {
            handleKeyPress(event.xcookie.data);
        }

        XFreeEventData(display_, &event.xcookie);
    }
}

void X11TextInputBackend::resumeObservation() {
    suppressObservation_ = false;
    if (enabled_ && display_ != nullptr) {
        eventPumpTimer_.start();
    }
}

bool X11TextInputBackend::initializeXi2() {
    int event = 0;
    int error = 0;
    if (!XQueryExtension(display_, "XInputExtension", &xiOpcode_, &event, &error)) {
        lastError_ = QStringLiteral("XInput2 extension is not available on this X11 server");
        return false;
    }

    int major = 2;
    int minor = 0;
    if (XIQueryVersion(display_, &major, &minor) != Success) {
        lastError_ = QStringLiteral("Failed to negotiate the XInput2 version with the X11 server");
        return false;
    }

    std::array<unsigned char, XIMaskLen(XI_LASTEVENT)> mask {};
    XIEventMask eventMask {};
    XISetMask(mask.data(), XI_KeyPress);
    eventMask.deviceid = XIAllDevices;
    eventMask.mask_len = static_cast<int>(mask.size());
    eventMask.mask = mask.data();

    if (XISelectEvents(display_, DefaultRootWindow(display_), &eventMask, 1) != Success) {
        lastError_ = QStringLiteral("Failed to subscribe to X11 key events through XInput2");
        return false;
    }

    XFlush(display_);
    lastError_.clear();
    return true;
}

void X11TextInputBackend::resetObservationState() {
    currentToken_.clear();
    invalidatePendingWord();
}

void X11TextInputBackend::invalidatePendingWord() {
    pendingWord_ = {};
}

bool X11TextInputBackend::currentTargetAllowed(FlamentineSwitcher::Core::WindowContext& context) const {
    context = windowBackend_.currentContext();
    return FlamentineSwitcher::Core::Rules::isAllowed(config_, context);
}

void X11TextInputBackend::handleKeyPress(void* eventData) {
    if (!enabled_ || suppressObservation_ || eventData == nullptr) {
        return;
    }

    const auto* keyEvent = static_cast<const XIDeviceEvent*>(eventData);
    if (keyEvent->detail == 0) {
        return;
    }

    FlamentineSwitcher::Core::WindowContext context;
    if (!currentTargetAllowed(context)) {
        resetObservationState();
        return;
    }

    generation_++;

    const KeySym keysym = eventKeySym(display_, *keyEvent);
    const unsigned int modifiers = static_cast<unsigned int>(keyEvent->mods.effective);
    const bool commandCombination = (modifiers & (ControlMask | Mod1Mask | Mod4Mask)) != 0;

    if (commandCombination || isModifierKeySym(keysym) || isResetKeySym(keysym)) {
        resetObservationState();
        return;
    }

    if (keysym == XK_BackSpace) {
        if (!currentToken_.isEmpty()) {
            currentToken_.chop(1);
        }
        invalidatePendingWord();
        return;
    }

    const QString text = translatedText(display_, static_cast<int>(keyEvent->detail), modifiers);
    if (text.isEmpty()) {
        if (isBoundaryKeySym(keysym)) {
            commitCurrentToken(context);
            currentToken_.clear();
        }
        return;
    }

    for (const QChar character : text) {
        if (isWordCharacter(character)) {
            currentToken_.append(character);
            invalidatePendingWord();
            continue;
        }

        if (isBoundaryCharacter(character)) {
            commitCurrentToken(context, QString(character));
            currentToken_.clear();
            continue;
        }

        resetObservationState();
        return;
    }
}

void X11TextInputBackend::commitCurrentToken(const FlamentineSwitcher::Core::WindowContext& context, const QString& suffix) {
    if (currentToken_.isEmpty()) {
        return;
    }

    pendingWord_.tokenId = nextTokenId_++;
    pendingWord_.word = currentToken_;
    pendingWord_.suffix = suffix;
    pendingWord_.windowId = context.windowId;
    pendingWord_.backspaceCount = currentToken_.size() + suffix.size();
    pendingWord_.generationAtCommit = generation_;
    pendingWord_.valid = true;
    emit wordCommitted(pendingWord_.tokenId, pendingWord_.word, context);
}

bool X11TextInputBackend::activeWindowMatches(const QString& expectedWindowId) const {
    if (expectedWindowId.trimmed().isEmpty()) {
        return false;
    }

    const QString activeWindowId = normalizedWindowId(windowBackend_.currentContext().windowId);
    return !activeWindowId.isEmpty() && activeWindowId == normalizedWindowId(expectedWindowId);
}

bool X11TextInputBackend::resolveKeystroke(const QChar character, ResolvedKeystroke& stroke) const {
    if (display_ == nullptr) {
        return false;
    }

    const QString usKey = usPhysicalKeyForCharacter(character);
    if (usKey.isEmpty()) {
        return false;
    }

    const QChar baseCharacter = usKey.at(0);
    const KeySym baseKeysym = static_cast<KeySym>(baseCharacter.unicode());
    const KeyCode keycode = XKeysymToKeycode(display_, baseKeysym);
    if (keycode == 0) {
        return false;
    }

    stroke.keycode = keycode;
    stroke.shift = usKey.size() > 1;
    return true;
}

void X11TextInputBackend::sendKey(const int keycode, const bool withShift) const {
    if (display_ == nullptr || keycode == 0) {
        return;
    }

    const KeyCode shiftKeycode = XKeysymToKeycode(display_, XK_Shift_L);
    if (withShift && shiftKeycode != 0) {
        XTestFakeKeyEvent(display_, shiftKeycode, True, CurrentTime);
    }

    XTestFakeKeyEvent(display_, keycode, True, CurrentTime);
    XTestFakeKeyEvent(display_, keycode, False, CurrentTime);

    if (withShift && shiftKeycode != 0) {
        XTestFakeKeyEvent(display_, shiftKeycode, False, CurrentTime);
    }
}

}  // namespace FlamentineSwitcher::Backends::Text
