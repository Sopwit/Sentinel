#include "sentinel/core/AgentActivityLog.h"

#include <QtTest>

class AgentActivityLogTest final : public QObject {
    Q_OBJECT

private slots:
    void startsEmpty();
    void appendsDeterministicEntriesInOrder();
    void clearsEntriesAndResetsSequence();
};

void AgentActivityLogTest::startsEmpty() {
    const sentinel::core::AgentActivityLog log;

    QCOMPARE(log.count(), 0);
    QVERIFY(log.entries().isEmpty());
    QCOMPARE(log.latestSummary(), QStringLiteral("No agent activity yet."));
}

void AgentActivityLogTest::appendsDeterministicEntriesInOrder() {
    sentinel::core::AgentActivityLog log;

    const auto first = log.append(sentinel::core::AgentActivityType::RequestReceived,
                                  sentinel::core::AgentActivityStatus::Recorded,
                                  QStringLiteral("Agent request received."));
    const auto second = log.append(sentinel::core::AgentActivityType::PlanCreated,
                                   sentinel::core::AgentActivityStatus::Completed,
                                   QStringLiteral("Tool planning evaluated: Planned"));

    QCOMPARE(first.sequenceId, 1);
    QCOMPARE(second.sequenceId, 2);
    QCOMPARE(first.typeName(), QStringLiteral("Request Received"));
    QCOMPARE(second.typeName(), QStringLiteral("Plan Created"));
    QCOMPARE(first.statusName(), QStringLiteral("Recorded"));
    QCOMPARE(second.statusName(), QStringLiteral("Completed"));
    QCOMPARE(log.count(), 2);
    QCOMPARE(log.entries().at(0).summary, QStringLiteral("Agent request received."));
    QCOMPARE(log.entries().at(1).summary, QStringLiteral("Tool planning evaluated: Planned"));
    QCOMPARE(log.latestSummary(), QStringLiteral("Tool planning evaluated: Planned"));
}

void AgentActivityLogTest::clearsEntriesAndResetsSequence() {
    sentinel::core::AgentActivityLog log;
    log.append(sentinel::core::AgentActivityType::RequestReceived,
               sentinel::core::AgentActivityStatus::Recorded,
               QStringLiteral("Agent request received."));

    log.clear();
    QCOMPARE(log.count(), 0);
    QVERIFY(log.entries().isEmpty());
    QCOMPARE(log.latestSummary(), QStringLiteral("No agent activity yet."));

    const auto next = log.append(sentinel::core::AgentActivityType::PipelineCompleted,
                                 sentinel::core::AgentActivityStatus::Blocked,
                                 QStringLiteral("Agent pipeline finished: Blocked"));

    QCOMPARE(log.count(), 1);
    QCOMPARE(next.sequenceId, 1);
    QCOMPARE(log.latestSummary(), QStringLiteral("Agent pipeline finished: Blocked"));
}

QTEST_MAIN(AgentActivityLogTest)
#include "test_agent_activity_log.moc"
