#include "flamentine_switcher/utils/string_utils.h"

namespace FlamentineSwitcher::Utils::StringUtils {

QStringList splitCommaSeparated(const QString& value) {
    QStringList items = value.split(',', Qt::SkipEmptyParts);
    for (QString& item : items) {
        item = item.trimmed();
    }
    items.removeAll(QString());
    return items;
}

QStringList splitLines(const QString& value) {
    QStringList items = value.split('\n', Qt::SkipEmptyParts);
    for (QString& item : items) {
        item = item.trimmed();
    }
    items.removeAll(QString());
    return items;
}

}  // namespace FlamentineSwitcher::Utils::StringUtils

