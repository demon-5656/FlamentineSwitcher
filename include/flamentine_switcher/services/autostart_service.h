#pragma once

#include <QString>

namespace FlamentineSwitcher::Services {

class AutostartService {
public:
    bool isEnabled() const;
    bool setEnabled(bool enabled) const;
    QString desktopFilePath() const;
    QString lastError() const;

private:
    bool writeDesktopFile() const;

    mutable QString lastError_;
};

}  // namespace FlamentineSwitcher::Services

