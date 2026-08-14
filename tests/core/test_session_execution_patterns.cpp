#include "sentinel/core/event/DurableEventLog.h"
#include "sentinel/core/session/ContextEpoch.h"
#include "sentinel/core/session/ProviderPromptTemplates.h"
#include "sentinel/core/session/SessionInputQueue.h"
#include "sentinel/core/session/SessionProjector.h"
#include "sentinel/core/session/SessionRunnerState.h"

#include <QSqlDatabase>
#include <QtTest>

using namespace sentinel::core;

class SessionExecutionPatternsTest final : public QObject {
    Q_OBJECT
private slots:
    void promptTemplates();
    void epochQueueAndState();
    void durableEventsAndProjector();
};

void SessionExecutionPatternsTest::promptTemplates() {
    QVERIFY(ProviderPromptTemplates::systemPrompt(QStringLiteral("openai"), QStringLiteral("base")).contains(QStringLiteral("base")));
    QCOMPARE(ProviderPromptTemplates::systemPrompt(QStringLiteral("unknown"), QStringLiteral("base")), QStringLiteral("base"));
}

void SessionExecutionPatternsTest::epochQueueAndState() {
    ContextEpoch epoch(QStringLiteral("session"));
    QCOMPARE(epoch.advance(), qint64(1));
    epoch.replace(5);
    QCOMPARE(epoch.value(), qint64(5));

    SessionInputQueue queue;
    queue.promote({QStringLiteral("1"), QStringLiteral("hello")});
    SessionInput admitted;
    QVERIFY(queue.admitNext(&admitted));
    QVERIFY(admitted.promoted && admitted.admitted);

    SessionRunnerStateMachine state;
    QVERIFY(state.beginRun());
    QVERIFY(!state.beginShell(false));
    QVERIFY(state.finish());
}

void SessionExecutionPatternsTest::durableEventsAndProjector() {
    const QString connection = QStringLiteral("session-patterns");
    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
    database.setDatabaseName(QStringLiteral(":memory:"));
    QVERIFY(database.open());
    DurableEventLog log(database);
    QVERIFY(log.initialize());
    const auto event = log.append(QStringLiteral("s1"), QStringLiteral("message.created"), QJsonObject{{"text", "hello"}});
    QVERIFY(event.has_value());
    QCOMPARE(event->sequence, qint64(1));
    SessionProjector projector;
    projector.apply(*event);
    QCOMPARE(projector.messageCount(QStringLiteral("s1")), 1);
    QCOMPARE(log.replay(QStringLiteral("s1")).size(), 1);
    database.close();
    QSqlDatabase::removeDatabase(connection);
}

QTEST_MAIN(SessionExecutionPatternsTest)
#include "test_session_execution_patterns.moc"
