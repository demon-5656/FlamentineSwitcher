#pragma once

#include <QHash>

namespace FlamentineSwitcher::Conversion {

struct LayoutMapSet {
    QHash<QChar, QChar> usToRu;
    QHash<QChar, QChar> ruToUs;
};

const LayoutMapSet& usRuMap();
bool isLatinLetter(QChar character);
bool isCyrillicLetter(QChar character);

}  // namespace FlamentineSwitcher::Conversion

