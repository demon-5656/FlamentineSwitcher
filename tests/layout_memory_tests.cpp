#include <QtTest>

#include "flamentine_switcher/core/layout_memory.h"

class LayoutMemoryTests : public QObject {
    Q_OBJECT

private slots:
    void remembersPerWindow();
    void fallsBackToPerApp();
    void prefersPerWindowOverApp();
    void roundTripsPersistentState();
};

void LayoutMemoryTests::remembersPerWindow() {
    FlamentineSwitcher::Core::LayoutMemory memory;
    FlamentineSwitcher::Core::AppConfig config = FlamentineSwitcher::Core::AppConfig::defaults();
    config.rememberLayoutPerWindow = true;
    config.rememberLayoutPerApp = false;

    FlamentineSwitcher::Core::WindowContext context;
    context.windowId = QStringLiteral("0x123");
    context.appName = QStringLiteral("code");

    memory.remember(config, context, QStringLiteral("ru"));

    const auto recalled = memory.recall(config, context);
    QVERIFY(recalled.has_value());
    QCOMPARE(recalled.value(), QStringLiteral("ru"));
}

void LayoutMemoryTests::fallsBackToPerApp() {
    FlamentineSwitcher::Core::LayoutMemory memory;
    FlamentineSwitcher::Core::AppConfig config = FlamentineSwitcher::Core::AppConfig::defaults();
    config.rememberLayoutPerWindow = false;
    config.rememberLayoutPerApp = true;

    FlamentineSwitcher::Core::WindowContext sourceContext;
    sourceContext.windowId = QStringLiteral("0x123");
    sourceContext.appName = QStringLiteral("firefox");
    memory.remember(config, sourceContext, QStringLiteral("ru"));

    FlamentineSwitcher::Core::WindowContext nextWindowContext;
    nextWindowContext.windowId = QStringLiteral("0x999");
    nextWindowContext.appName = QStringLiteral("firefox");

    const auto recalled = memory.recall(config, nextWindowContext);
    QVERIFY(recalled.has_value());
    QCOMPARE(recalled.value(), QStringLiteral("ru"));
}

void LayoutMemoryTests::prefersPerWindowOverApp() {
    FlamentineSwitcher::Core::LayoutMemory memory;
    FlamentineSwitcher::Core::AppConfig config = FlamentineSwitcher::Core::AppConfig::defaults();
    config.rememberLayoutPerWindow = true;
    config.rememberLayoutPerApp = true;

    FlamentineSwitcher::Core::WindowContext appContext;
    appContext.windowId = QStringLiteral("0x111");
    appContext.appName = QStringLiteral("code");
    memory.remember(config, appContext, QStringLiteral("ru"));

    FlamentineSwitcher::Core::WindowContext specificWindowContext;
    specificWindowContext.windowId = QStringLiteral("0x222");
    specificWindowContext.appName = QStringLiteral("code");
    memory.remember(config, specificWindowContext, QStringLiteral("us"));

    const auto recalled = memory.recall(config, specificWindowContext);
    QVERIFY(recalled.has_value());
    QCOMPARE(recalled.value(), QStringLiteral("us"));
}

void LayoutMemoryTests::roundTripsPersistentState() {
    FlamentineSwitcher::Core::LayoutMemory memory;
    FlamentineSwitcher::Core::AppConfig config = FlamentineSwitcher::Core::AppConfig::defaults();
    config.rememberLayoutPerWindow = true;
    config.rememberLayoutPerApp = true;

    FlamentineSwitcher::Core::WindowContext windowContext;
    windowContext.windowId = QStringLiteral("0xabc");
    windowContext.appName = QStringLiteral("firefox");
    memory.remember(config, windowContext, QStringLiteral("ru"));

    FlamentineSwitcher::Core::WindowContext appContext;
    appContext.windowId = QStringLiteral("0xdef");
    appContext.appName = QStringLiteral("code");
    memory.remember(config, appContext, QStringLiteral("us"));

    const auto state = memory.exportState();
    QVERIFY(!state.isEmpty());

    FlamentineSwitcher::Core::LayoutMemory restoredMemory;
    restoredMemory.restoreState(FlamentineSwitcher::Core::LayoutMemoryState::fromJsonObject(state.toJsonObject()));

    const auto restoredWindow = restoredMemory.recall(config, windowContext);
    QVERIFY(restoredWindow.has_value());
    QCOMPARE(restoredWindow.value(), QStringLiteral("ru"));

    const auto restoredApp = restoredMemory.recall(config, appContext);
    QVERIFY(restoredApp.has_value());
    QCOMPARE(restoredApp.value(), QStringLiteral("us"));
}

QTEST_APPLESS_MAIN(LayoutMemoryTests)

#include "layout_memory_tests.moc"
