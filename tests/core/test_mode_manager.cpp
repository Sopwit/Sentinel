#include "sentinel/core/ModeManager.h"

#include <QSignalSpy>
#include <QtTest>

using sentinel::core::ModeManager;

class ModeManagerTest final : public QObject {
    Q_OBJECT

private slots:
    void defaultsToSentinelMode();
    void exposesNoSelectableModes();
    void ignoresModeNames();
};

void ModeManagerTest::defaultsToSentinelMode() {
    ModeManager manager;

    QCOMPARE(manager.currentMode(), ModeManager::Mode::Default);
    QCOMPARE(manager.currentModeName(), QStringLiteral("Sentinel"));
}

void ModeManagerTest::exposesNoSelectableModes() {
    ModeManager manager;

    QVERIFY(manager.availableModes().isEmpty());
}

void ModeManagerTest::ignoresModeNames() {
    ModeManager manager;
    QSignalSpy spy(&manager, &ModeManager::currentModeChanged);

    manager.setModeByName(QStringLiteral("Sentinel"));
    manager.setModeByName(QStringLiteral("Workspace"));
    manager.setModeByName(QStringLiteral("Unknown"));

    QCOMPARE(manager.currentMode(), ModeManager::Mode::Default);
    QCOMPARE(manager.currentModeName(), QStringLiteral("Sentinel"));
    QCOMPARE(spy.count(), 0);
}

QTEST_MAIN(ModeManagerTest)

#include "test_mode_manager.moc"
