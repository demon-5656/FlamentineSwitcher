#include "flamentine_switcher/conversion/heuristics.h"

#include "flamentine_switcher/conversion/layout_maps.h"
#include "flamentine_switcher/conversion/text_converter.h"

namespace {

double vowelRatio(const QString& text, const QString& vowels) {
    int letters = 0;
    int vowelCount = 0;

    for (const QChar character : text) {
        if (!character.isLetter()) {
            continue;
        }

        ++letters;
        if (vowels.contains(character)) {
            ++vowelCount;
        }
    }

    if (letters == 0) {
        return 0.0;
    }

    return static_cast<double>(vowelCount) / static_cast<double>(letters);
}

int letterCount(const QString& text) {
    int count = 0;
    for (const QChar character : text) {
        if (character.isLetter()) {
            ++count;
        }
    }
    return count;
}

int latinCount(const QString& text) {
    int count = 0;
    for (const QChar character : text) {
        if (FlamentineSwitcher::Conversion::isLatinLetter(character)) {
            ++count;
        }
    }
    return count;
}

int cyrillicCount(const QString& text) {
    int count = 0;
    for (const QChar character : text) {
        if (FlamentineSwitcher::Conversion::isCyrillicLetter(character)) {
            ++count;
        }
    }
    return count;
}

}  // namespace

namespace FlamentineSwitcher::Conversion {

LayoutAssessment LayoutHeuristics::assessWord(const QString& text) const {
    const QString trimmed = text.trimmed().toLower();
    LayoutAssessment assessment;

    const int totalLetters = letterCount(trimmed);
    if (totalLetters < 3) {
        return assessment;
    }

    const TextConverter converter;

    if (latinCount(trimmed) == totalLetters) {
        const QString converted = converter.convertText(trimmed, ConversionDirection::UsToRu).toLower();
        const double originalVowels = vowelRatio(trimmed, QStringLiteral("aeiouy"));
        const double convertedVowels = vowelRatio(converted, QStringLiteral("аеёиоуыэюя"));
        const double delta = convertedVowels - originalVowels;

        if (delta > 0.15) {
            assessment.source = SuggestedLayout::Us;
            assessment.target = SuggestedLayout::Ru;
            assessment.confidence = qBound(40, static_cast<int>(delta * 220.0), 100);
            assessment.looksMistyped = true;
        }
    } else if (cyrillicCount(trimmed) == totalLetters) {
        const QString converted = converter.convertText(trimmed, ConversionDirection::RuToUs).toLower();
        const double originalVowels = vowelRatio(trimmed, QStringLiteral("аеёиоуыэюя"));
        const double convertedVowels = vowelRatio(converted, QStringLiteral("aeiouy"));
        const double delta = convertedVowels - originalVowels;

        if (delta > 0.15) {
            assessment.source = SuggestedLayout::Ru;
            assessment.target = SuggestedLayout::Us;
            assessment.confidence = qBound(40, static_cast<int>(delta * 220.0), 100);
            assessment.looksMistyped = true;
        }
    }

    return assessment;
}

}  // namespace FlamentineSwitcher::Conversion

