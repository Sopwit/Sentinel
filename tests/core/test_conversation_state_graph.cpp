#include "sentinel/core/ConversationStateGraph.h"

#include <QtTest>

using sentinel::core::ConversationState;
using sentinel::core::conversationStateName;
using sentinel::core::ConversationTransitionStatus;
using sentinel::core::conversationTransitionStatusName;
using sentinel::core::isConversationTransitionAllowed;
using sentinel::core::safeConversationTransitionSummary;
using sentinel::core::StaticConversationStateGraph;

class ConversationStateGraphTest final : public QObject {
    Q_OBJECT

private slots:
    void namesStateAndTransitionStatus();
    void acceptsDeterministicValidTransitions();
    void rejectsInvalidTransitionsWithoutChangingState();
    void acceptsErrorTransitionsFromAnyState();
    void resetsToIdleMetadata();
};

void ConversationStateGraphTest::namesStateAndTransitionStatus() {
    QCOMPARE(conversationStateName(ConversationState::Idle), QStringLiteral("Idle"));
    QCOMPARE(conversationStateName(ConversationState::Listening), QStringLiteral("Listening"));
    QCOMPARE(conversationStateName(ConversationState::Planning), QStringLiteral("Planning"));
    QCOMPARE(conversationStateName(ConversationState::Routing), QStringLiteral("Routing"));
    QCOMPARE(conversationStateName(ConversationState::WaitingForApproval),
             QStringLiteral("Waiting For Approval"));
    QCOMPARE(conversationStateName(ConversationState::ReadyToRespond),
             QStringLiteral("Ready To Respond"));
    QCOMPARE(conversationStateName(ConversationState::Responding), QStringLiteral("Responding"));
    QCOMPARE(conversationStateName(ConversationState::Completed), QStringLiteral("Completed"));
    QCOMPARE(conversationStateName(ConversationState::Error), QStringLiteral("Error"));
    QCOMPARE(conversationTransitionStatusName(ConversationTransitionStatus::NotRequested),
             QStringLiteral("Not Requested"));
    QCOMPARE(conversationTransitionStatusName(ConversationTransitionStatus::Accepted),
             QStringLiteral("Accepted"));
    QCOMPARE(conversationTransitionStatusName(ConversationTransitionStatus::Rejected),
             QStringLiteral("Rejected"));
}

void ConversationStateGraphTest::acceptsDeterministicValidTransitions() {
    StaticConversationStateGraph graph;

    QCOMPARE(graph.currentState(), ConversationState::Idle);
    QCOMPARE(safeConversationTransitionSummary(graph.lastTransitionResult()),
             QStringLiteral("No conversation transition yet."));

    QCOMPARE(graph.transitionTo(ConversationState::Listening, QStringLiteral("input")).status,
             ConversationTransitionStatus::Accepted);
    QCOMPARE(graph.transitionTo(ConversationState::Planning).status,
             ConversationTransitionStatus::Accepted);
    QCOMPARE(graph.transitionTo(ConversationState::Routing).status,
             ConversationTransitionStatus::Accepted);
    QCOMPARE(graph.transitionTo(ConversationState::ReadyToRespond).status,
             ConversationTransitionStatus::Accepted);
    QCOMPARE(graph.transitionTo(ConversationState::Responding).status,
             ConversationTransitionStatus::Accepted);
    const auto completed = graph.transitionTo(ConversationState::Completed);

    QCOMPARE(completed.status, ConversationTransitionStatus::Accepted);
    QCOMPARE(graph.currentState(), ConversationState::Completed);
    QCOMPARE(completed.summary,
             QStringLiteral("Accepted conversation transition: Responding -> Completed"));
}

void ConversationStateGraphTest::rejectsInvalidTransitionsWithoutChangingState() {
    StaticConversationStateGraph graph;

    const auto result = graph.transitionTo(ConversationState::Responding);

    QCOMPARE(result.status, ConversationTransitionStatus::Rejected);
    QCOMPARE(graph.currentState(), ConversationState::Idle);
    QCOMPARE(result.summary,
             QStringLiteral("Rejected conversation transition: Idle -> Responding"));
}

void ConversationStateGraphTest::acceptsErrorTransitionsFromAnyState() {
    StaticConversationStateGraph graph;

    QVERIFY(isConversationTransitionAllowed(ConversationState::Idle, ConversationState::Error));
    QVERIFY(isConversationTransitionAllowed(ConversationState::Routing, ConversationState::Error));
    QVERIFY(
        isConversationTransitionAllowed(ConversationState::Responding, ConversationState::Error));

    const auto result = graph.transitionTo(ConversationState::Error, QStringLiteral("blocked"));

    QCOMPARE(result.status, ConversationTransitionStatus::Accepted);
    QCOMPARE(graph.currentState(), ConversationState::Error);
    QCOMPARE(result.summary,
             QStringLiteral("Accepted conversation transition: Idle -> Error: blocked"));
}

void ConversationStateGraphTest::resetsToIdleMetadata() {
    StaticConversationStateGraph graph;

    graph.transitionTo(ConversationState::Error);
    graph.reset();

    QCOMPARE(graph.currentState(), ConversationState::Idle);
    QCOMPARE(graph.lastTransitionResult().status, ConversationTransitionStatus::NotRequested);
    QCOMPARE(safeConversationTransitionSummary(graph.lastTransitionResult()),
             QStringLiteral("No conversation transition yet."));
}

QTEST_MAIN(ConversationStateGraphTest)

#include "test_conversation_state_graph.moc"
