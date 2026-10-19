#include "flamentine_switcher/conversion/text_converter.h"

#include <optional>
#include <QRegularExpression>

#include "flamentine_switcher/conversion/layout_maps.h"

namespace {

QString convertWithMap(const QString& text, const QHash<QChar, QChar>& map) {
    QString result;
    result.reserve(text.size());

    for (const QChar character : text) {
        const auto it = map.constFind(character);
        result.append(it == map.constEnd() ? character : it.value());
    }

    return result;
}


}  // namespace

namespace FlamentineSwitcher::Conversion {

QString TextConverter::convertText(const QString& text, ConversionDirection direction) const {
    if (text.isEmpty()) {
        return {};
    }

    const LayoutMapSet& maps = usRuMap();
    if (direction == ConversionDirection::AutoDetect) {
        QString result;
        result.reserve(text.size());

        std::optional<ConversionDirection> activeDirection;
        for (const QChar character : text) {
            if (isLatinLetter(character)) {
                activeDirection = ConversionDirection::UsToRu;
                break;
            }
            if (isCyrillicLetter(character)) {
                activeDirection = ConversionDirection::RuToUs;
                break;
            }
        }

        for (const QChar character : text) {
            if (isLatinLetter(character)) {
                activeDirection = ConversionDirection::UsToRu;
            } else if (isCyrillicLetter(character)) {
                activeDirection = ConversionDirection::RuToUs;
            }

            if (!activeDirection.has_value()) {
                result.append(character);
                continue;
            }

            const QHash<QChar, QChar>& map = activeDirection.value() == ConversionDirection::UsToRu
                ? maps.usToRu
                : maps.ruToUs;
            result.append(map.value(character, character));
        }
        return result;
    }

    return direction == ConversionDirection::UsToRu
        ? convertWithMap(text, maps.usToRu)
        : convertWithMap(text, maps.ruToUs);
}

QString TextConverter::convertLastWordInText(const QString& text, ConversionDirection direction) const {
    static const QRegularExpression expression(QStringLiteral("(\\S+)(\\s*)$"));

    const QRegularExpressionMatch match = expression.match(text);
    if (!match.hasMatch()) {
        return text;
    }

    const QString token = match.captured(1);
    const QString suffix = match.captured(2);

    const QString converted = convertText(token, direction);

    QString result = text;
    result.chop(token.size() + suffix.size());
    result += converted;
    result += suffix;
    return result;
}

} // namespace FlamentineSwitcher::Conversion
