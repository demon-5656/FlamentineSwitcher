#include <QtTest>

#include "flamentine_switcher/conversion/heuristics.h"

class HeuristicsTests : public QObject {
    Q_OBJECT

private slots:
    void detectsEnglishMistypeToRussian();
    void detectsRussianMistypeToEnglish();
    void doesNotFlagNormalEnglishWord();
};

void HeuristicsTests::detectsEnglishMistypeToRussian() {
    FlamentineSwitcher::Conversion::LayoutHeuristics heuristics;
    const auto assessment = heuristics.assessWord(QStringLiteral("ghbdtn"));

    QVERIFY(assessment.looksMistyped);
    QCOMPARE(assessment.target, FlamentineSwitcher::Conversion::SuggestedLayout::Ru);
}

void HeuristicsTests::detectsRussianMistypeToEnglish() {
    FlamentineSwitcher::Conversion::LayoutHeuristics heuristics;
    const auto assessment = heuristics.assessWord(QStringLiteral("руддщ"));

    QVERIFY(assessment.looksMistyped);
    QCOMPARE(assessment.target, FlamentineSwitcher::Conversion::SuggestedLayout::Us);
}

void HeuristicsTests::doesNotFlagNormalEnglishWord() {
    FlamentineSwitcher::Conversion::LayoutHeuristics heuristics;
    const auto assessment = heuristics.assessWord(QStringLiteral("hello"));

    QVERIFY(!assessment.looksMistyped);
}

QTEST_APPLESS_MAIN(HeuristicsTests)

#include "heuristics_tests.moc"
