#include "flamentine_switcher/conversion/layout_maps.h"

#include <QChar>

namespace {

FlamentineSwitcher::Conversion::LayoutMapSet buildUsRuMap() {
    using FlamentineSwitcher::Conversion::LayoutMapSet;

    const QString usLower = QStringLiteral("`1234567890-=qwertyuiop[]\\asdfghjkl;'zxcvbnm,./");
    const QString ruLower = QStringLiteral("ё1234567890-=йцукенгшщзхъ\\фывапролджэячсмитьбю.");
    const QString usUpper = QStringLiteral("~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:\"ZXCVBNM<>?");
    const QString ruUpper = QStringLiteral("Ё!\"№;%:?*()_+ЙЦУКЕНГШЩЗХЪ/ФЫВАПРОЛДЖЭЯЧСМИТЬБЮ,");

    Q_ASSERT(usLower.size() == ruLower.size());
    Q_ASSERT(usUpper.size() == ruUpper.size());

    LayoutMapSet maps;
    for (int index = 0; index < usLower.size(); ++index) {
        maps.usToRu.insert(usLower.at(index), ruLower.at(index));
        maps.ruToUs.insert(ruLower.at(index), usLower.at(index));
    }
    for (int index = 0; index < usUpper.size(); ++index) {
        maps.usToRu.insert(usUpper.at(index), ruUpper.at(index));
        maps.ruToUs.insert(ruUpper.at(index), usUpper.at(index));
    }

    return maps;
}

}  // namespace

namespace FlamentineSwitcher::Conversion {

const LayoutMapSet& usRuMap() {
    static const LayoutMapSet mapSet = buildUsRuMap();
    return mapSet;
}

bool isLatinLetter(const QChar character) {
    return (character >= QLatin1Char('a') && character <= QLatin1Char('z'))
        || (character >= QLatin1Char('A') && character <= QLatin1Char('Z'));
}

bool isCyrillicLetter(const QChar character) {
    const ushort code = character.unicode();
    return (code >= 0x0400 && code <= 0x04FF);
}

}  // namespace FlamentineSwitcher::Conversion

