#include "flamentine_switcher/conversion/text_converter.h"

#include <QRegularExpression>

#include "flamentine_switcher/conversion/heuristics.h"
#include "flamentine_switcher/conversion/layout_maps.h"

namespace {

FlamentineSwitcher::Conversion::ConversionDirection autoDirection(const QString& text) {
    using namespace FlamentineSwitcher::Conversion;

    const LayoutHeuristics heuristics;
    const LayoutAssessment assessment = heuristics.assessWord(text);
    if (assessment.looksMistyped) {
        return assessment.target == SuggestedLayout::Ru ? ConversionDirection::UsToRu : ConversionDirection::RuToUs;
    }

    int latin = 0;
    int cyrillic = 0;
    for (const QChar character : text) {
        if (isLatinLetter(character)) {
            ++latin;
        } else if (isCyrillicLetter(character)) {
            ++cyrillic;
        }
    }

    return latin >= cyrillic ? ConversionDirection::UsToRu : ConversionDirection::RuToUs;
}

QString convertWithMap(const QString& text, const QHash<QChar, QChar>& map) {
    QString result;
    result.reserve(text.size());

    for (const QChar character : text) {
        const auto it = map.constFind(character);
        result.append(it != map.constEnd() ? it.value() : character);
    }

    return result;
}

}  // namespace

namespace FlamentineSwitcher::Conversion {

QString TextConverter::convertText(const QString& text, ConversionDirection direction) const {
    if (text.isEmpty()) {
        return {};
    }

    const ConversionDirection resolvedDirection =
        direction == ConversionDirection::AutoDetect ? autoDirection(text) : direction;
    const LayoutMapSet& maps = usRuMap();

    return resolvedDirection == ConversionDirection::UsToRu
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
    const QString converted = convertText(token, direction == ConversionDirection::AutoDetect ? autoDirection(token) : direction);

    QString result = text;
    result.chop(token.size() + suffix.size());
    result += converted;
    result += suffix;
    return result;
}

}  // namespace FlamentineSwitcher::Conversion

