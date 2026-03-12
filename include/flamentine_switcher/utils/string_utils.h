#pragma once

#include <QString>
#include <QStringList>

namespace FlamentineSwitcher::Utils::StringUtils {

QStringList splitCommaSeparated(const QString& value);
QStringList splitLines(const QString& value);

}  // namespace FlamentineSwitcher::Utils::StringUtils
