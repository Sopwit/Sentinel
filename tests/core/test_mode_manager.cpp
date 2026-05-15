#include "sentinel/core/ModeManager.h"

#include <QSignalSpy>
#include <QtTest>

using sentinel::core::ModeManager;

class ModeManagerTest final : public QObject {
    Q_OBJECT

private slots:
    void defaultsToCompanionMode();
    void exposesExpectedModesInUiOrder();
    void changesModeByNameAndEmitsOnce();
    void ignoresUnknownModeNames();
    void handlesRepeatedModeChanges();
};

void ModeManagerTest::defaultsToCompanionMode() {
    ModeManager manager;

    QCOMPARE(manager.currentMode(), ModeManager::Mode::Companion);
    QCOMPARE(manager.currentModeName(), QStringLiteral("Companion Mode"));
}

void ModeManagerTest::exposesExpectedModesInUiOrder() {
    ModeManager manager;

    const QStringList expected{
        QStringLiteral("Companion Mode"), QStringLiteral("Focus Mode"),
        QStringLiteral("Mission Mode"),   QStringLiteral("System Mode"),
        QStringLiteral("Minimal Mode"),   QStringLiteral("Tactical Mode"),
    };

    QCOMPARE(manager.availableModes(), expected);
}

void ModeManagerTest::changesModeByNameAndEmitsOnce() {
    ModeManager manager;
    QSignalSpy spy(&manager, &ModeManager::currentModeChanged);

    manager.setModeByName(QStringLiteral("Tactical Mode"));

    QCOMPARE(manager.currentMode(), ModeManager::Mode::Tactical);
    QCOMPARE(manager.currentModeName(), QStringLiteral("Tactical Mode"));
    QCOMPARE(spy.count(), 1);

    manager.setModeByName(QStringLiteral("Tactical Mode"));
    QCOMPARE(spy.count(), 1);
}

void ModeManagerTest::ignoresUnknownModeNames() {
    ModeManager manager;
    QSignalSpy spy(&manager, &ModeManager::currentModeChanged);

    manager.setModeByName(QStringLiteral("Unknown Mode"));

    QCOMPARE(manager.currentMode(), ModeManager::Mode::Companion);
    QCOMPARE(spy.count(), 0);
}

void ModeManagerTest::handlesRepeatedModeChanges() {
    ModeManager manager;
    QSignalSpy spy(&manager, &ModeManager::currentModeChanged);

    manager.setModeByName(QStringLiteral("Focus Mode"));
    manager.setModeByName(QStringLiteral("Mission Mode"));
    manager.setModeByName(QStringLiteral("System Mode"));
    manager.setModeByName(QStringLiteral("Minimal Mode"));
    manager.setModeByName(QStringLiteral("Tactical Mode"));

    QCOMPARE(manager.currentMode(), ModeManager::Mode::Tactical);
    QCOMPARE(manager.currentModeName(), QStringLiteral("Tactical Mode"));
    QCOMPARE(spy.count(), 5);
}

QTEST_MAIN(ModeManagerTest)

#include "test_mode_manager.moc"
