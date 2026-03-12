#include "flamentine_switcher/core/config.h"

#include <QJsonArray>

namespace {

QStringList jsonArrayToStringList(const QJsonValue& value) {
    QStringList result;
    for (const QJsonValue& item : value.toArray()) {
        result.append(item.toString().trimmed());
    }
    result.removeAll(QString());
    return result;
}

}  // namespace

namespace FlamentineSwitcher::Core {

HotkeyConfig HotkeyConfig::fromJsonObject(const QJsonObject& object) {
    HotkeyConfig config;
    config.toggleLayout = object.value(QStringLiteral("toggleLayout")).toString(config.toggleLayout);
    config.convertLastWord = object.value(QStringLiteral("convertLastWord")).toString(config.convertLastWord);
    config.convertSelection = object.value(QStringLiteral("convertSelection")).toString(config.convertSelection);
    config.openSettings = object.value(QStringLiteral("openSettings")).toString(config.openSettings);
    config.toggleEnabled = object.value(QStringLiteral("toggleEnabled")).toString(config.toggleEnabled);
    return config;
}

QJsonObject HotkeyConfig::toJsonObject() const {
    return {
        {QStringLiteral("toggleLayout"), toggleLayout},
        {QStringLiteral("convertLastWord"), convertLastWord},
        {QStringLiteral("convertSelection"), convertSelection},
        {QStringLiteral("openSettings"), openSettings},
        {QStringLiteral("toggleEnabled"), toggleEnabled},
    };
}

ConversionConfig ConversionConfig::fromJsonObject(const QJsonObject& object) {
    ConversionConfig config;
    config.preserveCase = object.value(QStringLiteral("preserveCase")).toBool(config.preserveCase);
    config.preservePunctuation = object.value(QStringLiteral("preservePunctuation")).toBool(config.preservePunctuation);
    config.autoConvertEnabled = object.value(QStringLiteral("autoConvertEnabled")).toBool(config.autoConvertEnabled);
    config.heuristicsEnabled = object.value(QStringLiteral("heuristicsEnabled")).toBool(config.heuristicsEnabled);
    return config;
}

QJsonObject ConversionConfig::toJsonObject() const {
    return {
        {QStringLiteral("preserveCase"), preserveCase},
        {QStringLiteral("preservePunctuation"), preservePunctuation},
        {QStringLiteral("autoConvertEnabled"), autoConvertEnabled},
        {QStringLiteral("heuristicsEnabled"), heuristicsEnabled},
    };
}

LoggingConfig LoggingConfig::fromJsonObject(const QJsonObject& object) {
    LoggingConfig config;
    config.level = logLevelFromString(object.value(QStringLiteral("level")).toString(QStringLiteral("info")));
    config.fileEnabled = object.value(QStringLiteral("fileEnabled")).toBool(config.fileEnabled);
    return config;
}

QJsonObject LoggingConfig::toJsonObject() const {
    return {
        {QStringLiteral("level"), toString(level)},
        {QStringLiteral("fileEnabled"), fileEnabled},
    };
}

AppConfig AppConfig::defaults() {
    return {};
}

AppConfig AppConfig::fromJsonObject(const QJsonObject& object) {
    AppConfig config = defaults();
    config.version = object.value(QStringLiteral("version")).toInt(config.version);
    config.enabled = object.value(QStringLiteral("enabled")).toBool(config.enabled);
    config.layouts = jsonArrayToStringList(object.value(QStringLiteral("layouts")));
    if (config.layouts.isEmpty()) {
        config.layouts = defaults().layouts;
    }
    config.defaultLayout = object.value(QStringLiteral("defaultLayout")).toString(config.defaultLayout);
    if (!config.layouts.contains(config.defaultLayout)) {
        config.defaultLayout = config.layouts.constFirst();
    }
    config.rememberLayoutPerWindow = object.value(QStringLiteral("rememberLayoutPerWindow")).toBool(config.rememberLayoutPerWindow);
    config.rememberLayoutPerApp = object.value(QStringLiteral("rememberLayoutPerApp")).toBool(config.rememberLayoutPerApp);
    config.notificationsEnabled = object.value(QStringLiteral("notificationsEnabled")).toBool(config.notificationsEnabled);
    config.autoStart = object.value(QStringLiteral("autoStart")).toBool(config.autoStart);
    config.excludedApps = jsonArrayToStringList(object.value(QStringLiteral("excludedApps")));
    config.excludedWindowClasses = jsonArrayToStringList(object.value(QStringLiteral("excludedWindowClasses")));
    config.excludeTerminals = object.value(QStringLiteral("excludeTerminals")).toBool(config.excludeTerminals);
    config.excludeFullscreen = object.value(QStringLiteral("excludeFullscreen")).toBool(config.excludeFullscreen);
    config.hotkeys = HotkeyConfig::fromJsonObject(object.value(QStringLiteral("hotkeys")).toObject());
    config.conversion = ConversionConfig::fromJsonObject(object.value(QStringLiteral("conversion")).toObject());
    config.logging = LoggingConfig::fromJsonObject(object.value(QStringLiteral("logging")).toObject());
    return config;
}

QJsonObject AppConfig::toJsonObject() const {
    return {
        {QStringLiteral("version"), version},
        {QStringLiteral("enabled"), enabled},
        {QStringLiteral("layouts"), QJsonArray::fromStringList(layouts)},
        {QStringLiteral("defaultLayout"), defaultLayout},
        {QStringLiteral("rememberLayoutPerWindow"), rememberLayoutPerWindow},
        {QStringLiteral("rememberLayoutPerApp"), rememberLayoutPerApp},
        {QStringLiteral("notificationsEnabled"), notificationsEnabled},
        {QStringLiteral("autoStart"), autoStart},
        {QStringLiteral("excludedApps"), QJsonArray::fromStringList(excludedApps)},
        {QStringLiteral("excludedWindowClasses"), QJsonArray::fromStringList(excludedWindowClasses)},
        {QStringLiteral("excludeTerminals"), excludeTerminals},
        {QStringLiteral("excludeFullscreen"), excludeFullscreen},
        {QStringLiteral("hotkeys"), hotkeys.toJsonObject()},
        {QStringLiteral("conversion"), conversion.toJsonObject()},
        {QStringLiteral("logging"), logging.toJsonObject()},
    };
}

QString toString(const LogLevel level) {
    switch (level) {
    case LogLevel::Trace:
        return QStringLiteral("trace");
    case LogLevel::Debug:
        return QStringLiteral("debug");
    case LogLevel::Info:
        return QStringLiteral("info");
    case LogLevel::Warning:
        return QStringLiteral("warning");
    case LogLevel::Error:
        return QStringLiteral("error");
    }

    return QStringLiteral("info");
}

LogLevel logLevelFromString(const QString& value) {
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("trace")) {
        return LogLevel::Trace;
    }
    if (normalized == QStringLiteral("debug")) {
        return LogLevel::Debug;
    }
    if (normalized == QStringLiteral("warning") || normalized == QStringLiteral("warn")) {
        return LogLevel::Warning;
    }
    if (normalized == QStringLiteral("error")) {
        return LogLevel::Error;
    }
    return LogLevel::Info;
}

}  // namespace FlamentineSwitcher::Core

