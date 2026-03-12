#pragma once

#include <optional>

#include <QString>

#include "flamentine_switcher/core/config.h"

namespace FlamentineSwitcher::Core {

class SettingsManager {
public:
    SettingsManager();

    AppConfig load() const;
    bool save(const AppConfig& config) const;
    std::optional<AppConfig> importConfig(const QString& filePath) const;
    bool exportConfig(const QString& filePath, const AppConfig& config) const;

    QString configDirectory() const;
    QString configFilePath() const;
    QString lastError() const;

private:
    bool writeConfig(const QString& filePath, const AppConfig& config) const;
    std::optional<AppConfig> readConfig(const QString& filePath) const;

    mutable QString lastError_;
};

}  // namespace FlamentineSwitcher::Core

