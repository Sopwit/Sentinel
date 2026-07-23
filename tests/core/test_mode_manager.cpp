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

    QCOMPARE(manager.currentMode(), ModeManager::Mode::Chat);
    QCOMPARE(manager.currentModeName(), QStringLiteral("Chat"));
}

void ModeManagerTest::exposesNoSelectableModes() {
    ModeManager manager;

    QCOMPARE(manager.availableModes(),
             QStringList({QStringLiteral("Chat"), QStringLiteral("Agent")}));
}

void ModeManagerTest::ignoresModeNames() {
    ModeManager manager;
    QSignalSpy spy(&manager, &ModeManager::currentModeChanged);

    manager.setModeByName(QStringLiteral("Chat"));
    manager.setModeByName(QStringLiteral("Agent"));
    manager.setModeByName(QStringLiteral("Unknown"));

    QCOMPARE(manager.currentMode(), ModeManager::Mode::Agent);
    QCOMPARE(manager.currentModeName(), QStringLiteral("Agent"));
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(ModeManagerTest)

#include "test_mode_manager.moc"
