#include <QtTest>

#include "flamentine_switcher/core/config.h"

class ConfigTests : public QObject {
    Q_OBJECT

private slots:
    void defaultsRoundTrip();
    void loadsCustomValues();
    void legacyOptionsAreIgnoredAndMinimalConfigIsWritten();
};

void ConfigTests::defaultsRoundTrip() {
    const auto defaults = FlamentineSwitcher::Core::AppConfig::defaults();
    const auto roundTrip = FlamentineSwitcher::Core::AppConfig::fromJsonObject(defaults.toJsonObject());

    QCOMPARE(roundTrip.layouts, defaults.layouts);
    QCOMPARE(roundTrip.defaultLayout, defaults.defaultLayout);
    QCOMPARE(roundTrip.hotkeys.toggleLayout, defaults.hotkeys.toggleLayout);
    QCOMPARE(roundTrip.requireAllowedTargets, defaults.requireAllowedTargets);
    QCOMPARE(roundTrip.rememberLayoutPerWindow, defaults.rememberLayoutPerWindow);
    QCOMPARE(roundTrip.rememberLayoutPerApp, defaults.rememberLayoutPerApp);
}

void ConfigTests::loadsCustomValues() {
    QJsonObject object{
        {QStringLiteral("notificationsEnabled"), false},
        {QStringLiteral("autoStart"), true},
        {QStringLiteral("conversion"),
         QJsonObject{
             {QStringLiteral("switchLayoutAfterConversion"), false},
         }},
    };

    const auto config = FlamentineSwitcher::Core::AppConfig::fromJsonObject(object);
    QVERIFY(!config.notificationsEnabled);
    QVERIFY(config.autoStart);
    QVERIFY(!config.conversion.switchLayoutAfterConversion);
}

void ConfigTests::legacyOptionsAreIgnoredAndMinimalConfigIsWritten() {
    QJsonObject object{
        {QStringLiteral("enabled"), false},
        {QStringLiteral("rememberLayoutPerWindow"), true},
        {QStringLiteral("requireAllowedTargets"), true},
        {QStringLiteral("notificationsEnabled"), false},
        {QStringLiteral("autoStart"), true},
        {QStringLiteral("conversion"),
         QJsonObject{
             {QStringLiteral("switchLayoutAfterConversion"), false},
         }},
    };

    const auto config = FlamentineSwitcher::Core::AppConfig::fromJsonObject(object);
    QVERIFY(config.enabled);
    QVERIFY(!config.rememberLayoutPerWindow);
    QVERIFY(!config.requireAllowedTargets);
    QVERIFY(!config.notificationsEnabled);
    QVERIFY(config.autoStart);
    QVERIFY(!config.conversion.switchLayoutAfterConversion);

    const QJsonObject written = config.toJsonObject();
    QCOMPARE(written.value(QStringLiteral("version")).toInt(), 2);
    QVERIFY(!written.contains(QStringLiteral("enabled")));
    QVERIFY(!written.contains(QStringLiteral("rememberLayoutPerWindow")));
    QVERIFY(!written.contains(QStringLiteral("requireAllowedTargets")));
    QVERIFY(!written.contains(QStringLiteral("hotkeys")));
    QVERIFY(!written.contains(QStringLiteral("logging")));
    QVERIFY(written.contains(QStringLiteral("notificationsEnabled")));
    QVERIFY(written.contains(QStringLiteral("autoStart")));
    QVERIFY(written.contains(QStringLiteral("conversion")));
}

QTEST_APPLESS_MAIN(ConfigTests)

#include "config_tests.moc"
