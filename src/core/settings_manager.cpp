#include "flamentine_switcher/core/settings_manager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
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

std::optional<LayoutMemoryState> SettingsManager::loadLayoutMemoryState() const {
    const auto object = readJsonObject(stateFilePath());
    if (!object.has_value()) {
        return std::nullopt;
    }

    return LayoutMemoryState::fromJsonObject(object.value());
}

bool SettingsManager::saveLayoutMemoryState(const LayoutMemoryState& state) const {
    if (state.isEmpty()) {
        return clearLayoutMemoryState();
    }

    return writeJsonObject(stateFilePath(), state.toJsonObject());
}

bool SettingsManager::clearLayoutMemoryState() const {
    lastError_.clear();

    QFile file(stateFilePath());
    if (!file.exists()) {
        return true;
    }
    if (!file.remove()) {
        lastError_ = QStringLiteral("Failed to remove layout memory state file: %1").arg(file.errorString());
        return false;
    }
    return true;
}

QString SettingsManager::configDirectory() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString SettingsManager::configFilePath() const {
    return configDirectory() + QStringLiteral("/config.json");
}

QString SettingsManager::stateFilePath() const {
    return configDirectory() + QStringLiteral("/state.json");
}

QString SettingsManager::lastError() const {
    return lastError_;
}

bool SettingsManager::writeConfig(const QString& filePath, const AppConfig& config) const {
    return writeJsonObject(filePath, config.toJsonObject());
}

std::optional<AppConfig> SettingsManager::readConfig(const QString& filePath) const {
    const auto object = readJsonObject(filePath);
    if (!object.has_value()) {
        return std::nullopt;
    }

    return AppConfig::fromJsonObject(object.value());
}

bool SettingsManager::writeJsonObject(const QString& filePath, const QJsonObject& object) const {
    lastError_.clear();

    QFileInfo fileInfo(filePath);
    QDir().mkpath(fileInfo.dir().absolutePath());

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        lastError_ = QStringLiteral("Failed to open JSON file for writing: %1").arg(file.errorString());
        return false;
    }

    const QJsonDocument document(object);
    file.write(document.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        lastError_ = QStringLiteral("Failed to commit JSON file: %1").arg(file.errorString());
        return false;
    }

    return true;
}

std::optional<QJsonObject> SettingsManager::readJsonObject(const QString& filePath) const {
    lastError_.clear();

    QFile file(filePath);
    if (!file.exists()) {
        return std::nullopt;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        lastError_ = QStringLiteral("Failed to open JSON file: %1").arg(file.errorString());
        return std::nullopt;
    }

    QJsonParseError parseError {};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (document.isNull() || !document.isObject()) {
        lastError_ = QStringLiteral("JSON file is not a valid object: %1").arg(parseError.errorString());
        return std::nullopt;
    }
    return document.object();
}

}  // namespace FlamentineSwitcher::Core
