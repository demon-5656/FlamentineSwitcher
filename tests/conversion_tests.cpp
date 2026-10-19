#include <QtTest>

#include "flamentine_switcher/conversion/text_converter.h"

namespace Conversion = FlamentineSwitcher::Conversion;

class ConversionTests : public QObject {
    Q_OBJECT

private slots:
    void convertsRussianLayoutToUs();
    void convertsUsLayoutToRussian();
    void convertsEveryLetterIndependently();
    void convertsLayoutPunctuationAndPreservesUnmappedCharacters();
    void convertsOnlyTheLastToken();
};

void ConversionTests::convertsRussianLayoutToUs() {
    const Conversion::TextConverter converter;

    QCOMPARE(converter.convertText(QStringLiteral("зачал"), Conversion::ConversionDirection::RuToUs), QStringLiteral("pfxfk"));
    QCOMPARE(converter.convertText(QStringLiteral("привет world"), Conversion::ConversionDirection::RuToUs),
             QStringLiteral("ghbdtn world"));
}

void ConversionTests::convertsUsLayoutToRussian() {
    const Conversion::TextConverter converter;

    QCOMPARE(converter.convertText(QStringLiteral("ghbdtn мир"), Conversion::ConversionDirection::UsToRu),
             QStringLiteral("привет мир"));
    QCOMPARE(converter.convertText(QStringLiteral("privet"), Conversion::ConversionDirection::UsToRu), QStringLiteral("зкшмуе"));
    QCOMPARE(converter.convertText(QStringLiteral("privet"), Conversion::ConversionDirection::RuToUs), QStringLiteral("privet"));
}


void ConversionTests::convertsLayoutPunctuationAndPreservesUnmappedCharacters() {
    const Conversion::TextConverter converter;

    QCOMPARE(converter.convertText(QStringLiteral("ПрИвЕт, 123!"), Conversion::ConversionDirection::RuToUs),
             QStringLiteral("GhBdTn? 123!"));
    QCOMPARE(converter.convertText(QStringLiteral("ёЁ!"), Conversion::ConversionDirection::RuToUs), QStringLiteral("`~!"));
    QCOMPARE(converter.convertText(QStringLiteral("q,./"), Conversion::ConversionDirection::UsToRu), QStringLiteral("йбю."));
    QCOMPARE(converter.convertText(QStringLiteral("ну;но ка,ель"), Conversion::ConversionDirection::UsToRu),
             QStringLiteral("нужно кабель"));
}

void ConversionTests::convertsEveryLetterIndependently() {
    const Conversion::TextConverter converter;

    QCOMPARE(converter.convertText(QStringLiteral("зачал"), Conversion::ConversionDirection::AutoDetect), QStringLiteral("pfxfk"));
    QCOMPARE(converter.convertText(QStringLiteral("привет world"), Conversion::ConversionDirection::AutoDetect),
             QStringLiteral("ghbdtn цщкдв"));
    QCOMPARE(converter.convertText(QStringLiteral("ghbdtn мир"), Conversion::ConversionDirection::AutoDetect),
             QStringLiteral("привет vbh"));
    QCOMPARE(converter.convertText(QStringLiteral("ПрИвЕт"), Conversion::ConversionDirection::AutoDetect), QStringLiteral("GhBdTn"));
    QCOMPARE(converter.convertText(QStringLiteral("привет, world! 123"), Conversion::ConversionDirection::AutoDetect),
             QStringLiteral("ghbdtn? цщкдв! 123"));
    QCOMPARE(converter.convertText(QStringLiteral("а b"), Conversion::ConversionDirection::AutoDetect), QStringLiteral("f и"));
    QCOMPARE(converter.convertText(QStringLiteral("123!@#"), Conversion::ConversionDirection::AutoDetect), QStringLiteral("123!@#"));
    QCOMPARE(converter.convertText(QStringLiteral(";jgf ,jkmifz .pktc!! & nfr yt gjqltn 78 Ghjdthjr yf jlye :jge 100&"),
                                   Conversion::ConversionDirection::AutoDetect),
             QStringLiteral("жопа большая юзлес!! ? так не пойдет 78 Проверок на одну Жопу 100?"));
}

void ConversionTests::convertsOnlyTheLastToken() {
    const Conversion::TextConverter converter;

    QCOMPARE(converter.convertLastWordInText(QStringLiteral("say ghbdtn"), Conversion::ConversionDirection::UsToRu),
             QStringLiteral("say привет"));
    QCOMPARE(converter.convertLastWordInText(QStringLiteral("Привет руддщ  "), Conversion::ConversionDirection::RuToUs),
             QStringLiteral("Привет hello  "));
}

QTEST_APPLESS_MAIN(ConversionTests)

#include "conversion_tests.moc"
