#include "flamentine_switcher/core/settings_manager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

namespace FlamentineSwitcher::Core {

SettingsManager::SettingsManager() = default;

AppConfig SettingsManager::load() const {
    const auto loaded = readConfig(configFilePath());
    if (loaded.has_value()) {
        return loaded.value();
    }

    const AppConfig defaults = AppConfig::defaults();
    save(defaults);
    return defaults;
}

bool SettingsManager::save(const AppConfig& config) const {
    return writeConfig(configFilePath(), config);
}

std::optional<AppConfig> SettingsManager::importConfig(const QString& filePath) const {
    return readConfig(filePath);
}

bool SettingsManager::exportConfig(const QString& filePath, const AppConfig& config) const {
    return writeConfig(filePath, config);
}

QString SettingsManager::configDirectory() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString SettingsManager::configFilePath() const {
    return configDirectory() + QStringLiteral("/config.json");
}

QString SettingsManager::lastError() const {
    return lastError_;
}

bool SettingsManager::writeConfig(const QString& filePath, const AppConfig& config) const {
    lastError_.clear();

    QFileInfo fileInfo(filePath);
    QDir().mkpath(fileInfo.dir().absolutePath());

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        lastError_ = QStringLiteral("Failed to open config for writing: %1").arg(file.errorString());
        return false;
    }

    const QJsonDocument document(config.toJsonObject());
    file.write(document.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        lastError_ = QStringLiteral("Failed to commit config file: %1").arg(file.errorString());
        return false;
    }

    return true;
}

std::optional<AppConfig> SettingsManager::readConfig(const QString& filePath) const {
    lastError_.clear();

    QFile file(filePath);
    if (!file.exists()) {
        return std::nullopt;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        lastError_ = QStringLiteral("Failed to open config: %1").arg(file.errorString());
        return std::nullopt;
    }

    QJsonParseError parseError {};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (document.isNull() || !document.isObject()) {
        lastError_ = QStringLiteral("Config file is not a valid JSON object: %1").arg(parseError.errorString());
        return std::nullopt;
    }
    return AppConfig::fromJsonObject(document.object());
}

}  // namespace FlamentineSwitcher::Core
