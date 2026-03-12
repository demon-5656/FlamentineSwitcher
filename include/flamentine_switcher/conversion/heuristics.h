#pragma once

#include <QString>

namespace FlamentineSwitcher::Conversion {

enum class SuggestedLayout {
    Unknown,
    Us,
    Ru,
};

struct LayoutAssessment {
    SuggestedLayout source = SuggestedLayout::Unknown;
    SuggestedLayout target = SuggestedLayout::Unknown;
    int confidence = 0;
    bool looksMistyped = false;
};

class LayoutHeuristics {
public:
    LayoutAssessment assessWord(const QString& text) const;
};

}  // namespace FlamentineSwitcher::Conversion

