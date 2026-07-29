#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace FlamentineSwitcher::Core {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
};

struct HotkeyConfig {
    QString toggleLayout = QStringLiteral("Meta+Space");
    QString convertLastWord = QStringLiteral("Pause");
    QString convertSelection = QStringLiteral("Meta+End");
    QString openSettings = QStringLiteral("Meta+Alt+K");
    QString toggleEnabled = QStringLiteral("Meta+Alt+S");

    static HotkeyConfig fromJsonObject(const QJsonObject& object);
    QJsonObject toJsonObject() const;
};

struct ConversionConfig {
    bool preserveCase = true;
    bool preservePunctuation = true;
    bool switchLayoutAfterConversion = true;
    bool autoConvertEnabled = false;
    bool heuristicsEnabled = true;
    int autoConvertDelayMs = 450;

    static ConversionConfig fromJsonObject(const QJsonObject& object);
    QJsonObject toJsonObject() const;
};

struct LoggingConfig {
    LogLevel level = LogLevel::Info;
    bool fileEnabled = true;

    static LoggingConfig fromJsonObject(const QJsonObject& object);
    QJsonObject toJsonObject() const;
};

struct AppConfig {
    int version = 1;
    bool enabled = true;
    QStringList layouts = {QStringLiteral("us"), QStringLiteral("ru")};
    QString defaultLayout = QStringLiteral("us");
    bool rememberLayoutPerWindow = true;
    bool rememberLayoutPerApp = false;
    bool notificationsEnabled = true;
    bool autoStart = false;
    // A manual action is safe by intent: no background interception or allowlist is
    // needed for the default, one-click workflow.
    bool requireAllowedTargets = false;
    QStringList allowedApps;
    QStringList allowedWindowClasses;
    QStringList excludedApps;
    QStringList excludedWindowClasses;
    bool excludeTerminals = false;
    bool excludeFullscreen = false;
    HotkeyConfig hotkeys;
    ConversionConfig conversion;
    LoggingConfig logging;

    static AppConfig defaults();
    static AppConfig fromJsonObject(const QJsonObject& object);
    QJsonObject toJsonObject() const;
};

QString toString(LogLevel level);
LogLevel logLevelFromString(const QString& value);

}  // namespace FlamentineSwitcher::Core

Q_DECLARE_METATYPE(FlamentineSwitcher::Core::AppConfig)
