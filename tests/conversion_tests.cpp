#include <QtTest>

#include "flamentine_switcher/conversion/text_converter.h"

class ConversionTests : public QObject {
    Q_OBJECT

private slots:
    void convertsEnglishLayoutMistype();
    void convertsRussianLayoutMistype();
    void convertsLastWord();
};

void ConversionTests::convertsEnglishLayoutMistype() {
    FlamentineSwitcher::Conversion::TextConverter converter;
    QCOMPARE(converter.convertText(QStringLiteral("ghbdtn")), QStringLiteral("привет"));
}

void ConversionTests::convertsRussianLayoutMistype() {
    FlamentineSwitcher::Conversion::TextConverter converter;
    QCOMPARE(converter.convertText(QStringLiteral("руддщ")), QStringLiteral("hello"));
}

void ConversionTests::convertsLastWord() {
    FlamentineSwitcher::Conversion::TextConverter converter;
    QCOMPARE(converter.convertLastWordInText(QStringLiteral("say ghbdtn")), QStringLiteral("say привет"));
}

QTEST_APPLESS_MAIN(ConversionTests)

#include "conversion_tests.moc"

