#include <QtTest>

#include "flamentine_switcher/core/config.h"

class ConfigTests : public QObject {
    Q_OBJECT

private slots:
    void defaultsRoundTrip();
    void loadsCustomValues();
};

void ConfigTests::defaultsRoundTrip() {
    const auto defaults = FlamentineSwitcher::Core::AppConfig::defaults();
    const auto roundTrip = FlamentineSwitcher::Core::AppConfig::fromJsonObject(defaults.toJsonObject());

    QCOMPARE(roundTrip.layouts, defaults.layouts);
    QCOMPARE(roundTrip.defaultLayout, defaults.defaultLayout);
    QCOMPARE(roundTrip.hotkeys.toggleLayout, defaults.hotkeys.toggleLayout);
}

void ConfigTests::loadsCustomValues() {
    QJsonObject object{
        {QStringLiteral("enabled"), false},
        {QStringLiteral("layouts"), QJsonArray{QStringLiteral("us"), QStringLiteral("de")}},
        {QStringLiteral("defaultLayout"), QStringLiteral("de")},
        {QStringLiteral("hotkeys"),
         QJsonObject{
             {QStringLiteral("toggleLayout"), QStringLiteral("Ctrl+Space")},
         }},
    };

    const auto config = FlamentineSwitcher::Core::AppConfig::fromJsonObject(object);
    QVERIFY(!config.enabled);
    QCOMPARE(config.layouts, QStringList({QStringLiteral("us"), QStringLiteral("de")}));
    QCOMPARE(config.defaultLayout, QStringLiteral("de"));
    QCOMPARE(config.hotkeys.toggleLayout, QStringLiteral("Ctrl+Space"));
}

QTEST_APPLESS_MAIN(ConfigTests)

#include "config_tests.moc"

