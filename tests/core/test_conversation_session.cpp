#include "sentinel/core/ConversationSession.h"

#include <QtTest>

using sentinel::core::AttentionState;
using sentinel::core::attentionStateName;
using sentinel::core::ContextScope;
using sentinel::core::contextScopeName;
using sentinel::core::ConversationSessionContextBuilder;
using sentinel::core::ConversationSessionStatus;
using sentinel::core::conversationSessionStatusName;
using sentinel::core::ConversationSessionStore;
using sentinel::core::InteractionMode;
using sentinel::core::interactionModeName;
using sentinel::core::safeRuntimeContextWindowSummary;

class ConversationSessionTest final : public QObject {
    Q_OBJECT

private slots:
    void namesSessionMetadata();
    void createsDeterministicSessionDefaults();
    void buildsContextWindowMetadata();
    void refreshesSessionContextDeterministically();
};

void ConversationSessionTest::namesSessionMetadata() {
    QCOMPARE(conversationSessionStatusName(ConversationSessionStatus::Active),
             QStringLiteral("Active"));
    QCOMPARE(conversationSessionStatusName(ConversationSessionStatus::Suspended),
             QStringLiteral("Suspended"));
    QCOMPARE(conversationSessionStatusName(ConversationSessionStatus::Closed),
             QStringLiteral("Closed"));
    QCOMPARE(interactionModeName(InteractionMode::Companion), QStringLiteral("Companion"));
    QCOMPARE(interactionModeName(InteractionMode::Focus), QStringLiteral("Focus"));
    QCOMPARE(interactionModeName(InteractionMode::Planning), QStringLiteral("Planning"));
    QCOMPARE(interactionModeName(InteractionMode::Review), QStringLiteral("Review"));
    QCOMPARE(attentionStateName(AttentionState::Observing), QStringLiteral("Observing"));
    QCOMPARE(attentionStateName(AttentionState::Focused), QStringLiteral("Focused"));
    QCOMPARE(attentionStateName(AttentionState::Idle), QStringLiteral("Idle"));
    QCOMPARE(contextScopeName(ContextScope::Workspace), QStringLiteral("Workspace"));
    QCOMPARE(contextScopeName(ContextScope::Task), QStringLiteral("Task"));
    QCOMPARE(contextScopeName(ContextScope::Runtime), QStringLiteral("Runtime"));
}

void ConversationSessionTest::createsDeterministicSessionDefaults() {
    const ConversationSessionStore sessionStore;

    QCOMPARE(sessionStore.session().id.value, QStringLiteral("conversation-session-1"));
    QCOMPARE(sessionStore.session().statusName(), QStringLiteral("Active"));
    QCOMPARE(sessionStore.session().interactionModeName(), QStringLiteral("Companion"));
    QCOMPARE(sessionStore.session().attentionStateName(), QStringLiteral("Observing"));
    QCOMPARE(sessionStore.session().revision, 0);
    QCOMPARE(safeRuntimeContextWindowSummary(sessionStore.session().contextWindow),
             QStringLiteral("No conversation context window yet."));
}

void ConversationSessionTest::buildsContextWindowMetadata() {
    const auto contextWindow = ConversationSessionContextBuilder{}.build(
        QStringLiteral("Local Only"), QStringLiteral("Atlas (Coordinator, Available, Local)"),
        QStringLiteral("Ambient (Available, Public Metadata, Session)"),
        QStringLiteral("Ready orchestration snapshot."));

    QCOMPARE(contextWindow.scope, ContextScope::Workspace);
    QCOMPARE(contextWindow.currentRoutingMode, QStringLiteral("Local Only"));
    QCOMPARE(contextWindow.preferredAgentSummary,
             QStringLiteral("Atlas (Coordinator, Available, Local)"));
    QCOMPARE(contextWindow.memoryAffinitySummary,
             QStringLiteral("Ambient (Available, Public Metadata, Session)"));
    QCOMPARE(contextWindow.latestOrchestrationSnapshotSummary,
             QStringLiteral("Ready orchestration snapshot."));
    QCOMPARE(contextWindow.summary,
             QStringLiteral("Workspace context window: Local Only route, Atlas (Coordinator, "
                            "Available, Local), Ambient (Available, Public Metadata, Session)."));
}

void ConversationSessionTest::refreshesSessionContextDeterministically() {
    ConversationSessionStore sessionStore;

    sessionStore.refreshContext(ConversationSessionContextBuilder{}.build(
                                    QStringLiteral("Balanced"),
                                    QStringLiteral("Orin (Planner, Available, Local)"),
                                    QStringLiteral("Procedural (Available, Local Only, Durable)"),
                                    QStringLiteral("Ready orchestration snapshot.")),
                                InteractionMode::Planning, AttentionState::Focused);

    QCOMPARE(sessionStore.session().id.value, QStringLiteral("conversation-session-1"));
    QCOMPARE(sessionStore.session().statusName(), QStringLiteral("Active"));
    QCOMPARE(sessionStore.session().interactionModeName(), QStringLiteral("Planning"));
    QCOMPARE(sessionStore.session().attentionStateName(), QStringLiteral("Focused"));
    QCOMPARE(sessionStore.session().revision, 1);
    QCOMPARE(safeRuntimeContextWindowSummary(sessionStore.session().contextWindow),
             QStringLiteral("Workspace context window: Balanced route, Orin (Planner, Available, "
                            "Local), Procedural (Available, Local Only, Durable)."));
}

QTEST_MAIN(ConversationSessionTest)

#include "test_conversation_session.moc"
