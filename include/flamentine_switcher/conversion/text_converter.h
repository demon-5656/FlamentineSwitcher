#pragma once

#include <QString>

namespace FlamentineSwitcher::Conversion {

enum class ConversionDirection {
    // Convert every individual Latin or Cyrillic letter to its counterpart.
    AutoDetect,
    UsToRu,
    RuToUs,
};

class TextConverter {
public:
    QString convertText(const QString& text, ConversionDirection direction) const;
    QString convertLastWordInText(const QString& text, ConversionDirection direction) const;
};

}  // namespace FlamentineSwitcher::Conversion
