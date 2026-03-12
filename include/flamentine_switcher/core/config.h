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
    QString convertSelection = QStringLiteral("Meta+Pause");
    QString openSettings = QStringLiteral("Meta+Alt+K");
    QString toggleEnabled = QStringLiteral("Meta+Alt+S");

    static HotkeyConfig fromJsonObject(const QJsonObject& object);
    QJsonObject toJsonObject() const;
};

struct ConversionConfig {
    bool preserveCase = true;
    bool preservePunctuation = true;
    bool autoConvertEnabled = false;
    bool heuristicsEnabled = true;

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
    QStringList excludedApps;
    QStringList excludedWindowClasses;
    bool excludeTerminals = true;
    bool excludeFullscreen = true;
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

