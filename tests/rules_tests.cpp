#include <QtTest>

#include "flamentine_switcher/core/rules.h"

class RulesTests : public QObject {
    Q_OBJECT

private slots:
    void denyByDefaultBlocksUnknownTarget();
    void allowlistAcceptsExplicitApp();
    void exclusionOverridesAllowlist();
};

void RulesTests::denyByDefaultBlocksUnknownTarget() {
    FlamentineSwitcher::Core::AppConfig config = FlamentineSwitcher::Core::AppConfig::defaults();
    config.requireAllowedTargets = true;

    const FlamentineSwitcher::Core::WindowContext context;
    QVERIFY(!FlamentineSwitcher::Core::Rules::isAllowed(config, context));
}

void RulesTests::allowlistAcceptsExplicitApp() {
    FlamentineSwitcher::Core::AppConfig config = FlamentineSwitcher::Core::AppConfig::defaults();
    config.requireAllowedTargets = true;
    config.allowedApps = {QStringLiteral("code"), QStringLiteral("firefox")};

    FlamentineSwitcher::Core::WindowContext context;
    context.appName = QStringLiteral("code");
    QVERIFY(FlamentineSwitcher::Core::Rules::isAllowed(config, context));
}

void RulesTests::exclusionOverridesAllowlist() {
    FlamentineSwitcher::Core::AppConfig config = FlamentineSwitcher::Core::AppConfig::defaults();
    config.requireAllowedTargets = true;
    config.allowedApps = {QStringLiteral("code")};
    config.excludedApps = {QStringLiteral("code")};

    FlamentineSwitcher::Core::WindowContext context;
    context.appName = QStringLiteral("code");
    QVERIFY(!FlamentineSwitcher::Core::Rules::isAllowed(config, context));
}

QTEST_APPLESS_MAIN(RulesTests)

#include "rules_tests.moc"
