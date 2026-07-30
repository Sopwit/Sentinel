#pragma once

#include <QString>

#include <cstdint>

namespace sentinel::core {

enum class ConversationSessionStatus : std::uint8_t {
    Active,
    Suspended,
    Closed,
};

inline QString conversationSessionStatusName(ConversationSessionStatus status) {
    switch (status) {
    case ConversationSessionStatus::Active:
        return QStringLiteral("Active");
    case ConversationSessionStatus::Suspended:
        return QStringLiteral("Suspended");
    case ConversationSessionStatus::Closed:
        return QStringLiteral("Closed");
    }

    return QStringLiteral("Active");
}

enum class InteractionMode : std::uint8_t {
    Companion,
    Focus,
    Planning,
    Review,
};

inline QString interactionModeName(InteractionMode mode) {
    switch (mode) {
    case InteractionMode::Companion:
        return QStringLiteral("Companion");
    case InteractionMode::Focus:
        return QStringLiteral("Focus");
    case InteractionMode::Planning:
        return QStringLiteral("Planning");
    case InteractionMode::Review:
        return QStringLiteral("Review");
    }

    return QStringLiteral("Companion");
}

enum class AttentionState : std::uint8_t {
    Observing,
    Focused,
    Idle,
};

inline QString attentionStateName(AttentionState state) {
    switch (state) {
    case AttentionState::Observing:
        return QStringLiteral("Observing");
    case AttentionState::Focused:
        return QStringLiteral("Focused");
    case AttentionState::Idle:
        return QStringLiteral("Idle");
    }

    return QStringLiteral("Observing");
}

enum class ContextScope : std::uint8_t {
    Workspace,
    Task,
    Runtime,
};

inline QString contextScopeName(ContextScope scope) {
    switch (scope) {
    case ContextScope::Workspace:
        return QStringLiteral("Workspace");
    case ContextScope::Task:
        return QStringLiteral("Task");
    case ContextScope::Runtime:
        return QStringLiteral("Runtime");
    }

    return QStringLiteral("Workspace");
}

struct ConversationSessionId {
    QString value = QStringLiteral("conversation-session-1");
};

struct RuntimeContextWindow {
    ContextScope scope = ContextScope::Workspace;
    QString currentRoutingMode;
    QString preferredAgentSummary;
    QString memoryAffinitySummary;
    QString latestOrchestrationSnapshotSummary;
    QString summary;
};

struct ConversationSession {
    ConversationSessionId id;
    ConversationSessionStatus status = ConversationSessionStatus::Active;
    InteractionMode interactionMode = InteractionMode::Companion;
    AttentionState attentionState = AttentionState::Observing;
    int revision = 0;
    RuntimeContextWindow contextWindow;

    QString statusName() const {
        return conversationSessionStatusName(status);
    }

    QString interactionModeName() const {
        return sentinel::core::interactionModeName(interactionMode);
    }

    QString attentionStateName() const {
        return sentinel::core::attentionStateName(attentionState);
    }
};

QString safeRuntimeContextWindowSummary(const RuntimeContextWindow& contextWindow);

class ConversationSessionContextBuilder final {
public:
    RuntimeContextWindow build(QString routingMode, QString preferredAgentSummary,
                               QString memoryAffinitySummary, QString orchestrationSnapshotSummary,
                               ContextScope scope = ContextScope::Workspace) const;
};

class ConversationSessionStore final {
public:
    ConversationSessionStore() = default;
    explicit ConversationSessionStore(ConversationSessionId id);

    const ConversationSession& session() const;
    void refreshContext(RuntimeContextWindow contextWindow,
                        InteractionMode interactionMode = InteractionMode::Companion,
                        AttentionState attentionState = AttentionState::Observing);

private:
    ConversationSession session_;
};

} // namespace sentinel::core
