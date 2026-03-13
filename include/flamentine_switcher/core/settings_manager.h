#pragma once

#include <optional>

#include <QJsonObject>
#include <QString>

#include "flamentine_switcher/core/config.h"
#include "flamentine_switcher/core/layout_memory.h"

namespace FlamentineSwitcher::Core {

class SettingsManager {
public:
    SettingsManager();

    AppConfig load() const;
    bool save(const AppConfig& config) const;
    std::optional<AppConfig> importConfig(const QString& filePath) const;
    bool exportConfig(const QString& filePath, const AppConfig& config) const;
    std::optional<LayoutMemoryState> loadLayoutMemoryState() const;
    bool saveLayoutMemoryState(const LayoutMemoryState& state) const;
    bool clearLayoutMemoryState() const;

    QString configDirectory() const;
    QString configFilePath() const;
    QString stateFilePath() const;
    QString lastError() const;

private:
    bool writeConfig(const QString& filePath, const AppConfig& config) const;
    std::optional<AppConfig> readConfig(const QString& filePath) const;
    bool writeJsonObject(const QString& filePath, const QJsonObject& object) const;
    std::optional<QJsonObject> readJsonObject(const QString& filePath) const;

    mutable QString lastError_;
};

}  // namespace FlamentineSwitcher::Core
