#pragma once

#include <QString>

#include "flamentine_switcher/core/config.h"

namespace FlamentineSwitcher::Utils::Logging {

void initialize(const FlamentineSwitcher::Core::LoggingConfig& config);
QString logFilePath();

}  // namespace FlamentineSwitcher::Utils::Logging

