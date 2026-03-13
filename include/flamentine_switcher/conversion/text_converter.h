#pragma once

#include <QString>

namespace FlamentineSwitcher::Conversion {

enum class ConversionDirection {
    AutoDetect,
    UsToRu,
    RuToUs,
};

class TextConverter {
public:
    QString convertText(const QString& text, ConversionDirection direction = ConversionDirection::AutoDetect) const;
    QString convertLastWordInText(const QString& text, ConversionDirection direction = ConversionDirection::AutoDetect) const;
    ConversionDirection resolveDirection(const QString& text) const;
};

}  // namespace FlamentineSwitcher::Conversion
