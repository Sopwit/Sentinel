#pragma once

#include <QString>

#include <cstdint>

namespace sentinel::core {

enum class ConversationState : std::uint8_t {
    Idle,
    Listening,
    Planning,
    Routing,
    WaitingForApproval,
    ReadyToRespond,
    Responding,
    Completed,
    Error,
};

inline QString conversationStateName(ConversationState state) {
    switch (state) {
    case ConversationState::Idle:
        return QStringLiteral("Idle");
    case ConversationState::Listening:
        return QStringLiteral("Listening");
    case ConversationState::Planning:
        return QStringLiteral("Planning");
    case ConversationState::Routing:
        return QStringLiteral("Routing");
    case ConversationState::WaitingForApproval:
        return QStringLiteral("Waiting For Approval");
    case ConversationState::ReadyToRespond:
        return QStringLiteral("Ready To Respond");
    case ConversationState::Responding:
        return QStringLiteral("Responding");
    case ConversationState::Completed:
        return QStringLiteral("Completed");
    case ConversationState::Error:
        return QStringLiteral("Error");
    }

    return QStringLiteral("Idle");
}

enum class ConversationTransitionStatus : std::uint8_t {
    NotRequested,
    Accepted,
    Rejected,
};

inline QString conversationTransitionStatusName(ConversationTransitionStatus status) {
    switch (status) {
    case ConversationTransitionStatus::NotRequested:
        return QStringLiteral("Not Requested");
    case ConversationTransitionStatus::Accepted:
        return QStringLiteral("Accepted");
    case ConversationTransitionStatus::Rejected:
        return QStringLiteral("Rejected");
    }

    return QStringLiteral("Not Requested");
}

struct ConversationTransition {
    ConversationState from = ConversationState::Idle;
    ConversationState to = ConversationState::Idle;
    QString reason;
};

struct ConversationTransitionResult {
    ConversationTransition transition;
    ConversationTransitionStatus status = ConversationTransitionStatus::NotRequested;
    QString summary = QStringLiteral("No conversation transition yet.");
};

QString safeConversationTransitionSummary(const ConversationTransitionResult& result);

class IConversationStateGraph {
public:
    virtual ~IConversationStateGraph() = default;

    virtual ConversationState currentState() const = 0;
    virtual ConversationTransitionResult lastTransitionResult() const = 0;
    virtual ConversationTransitionResult transitionTo(ConversationState nextState,
                                                      const QString& reason = {}) = 0;
    virtual void reset() = 0;
};

class StaticConversationStateGraph final : public IConversationStateGraph {
public:
    ConversationState currentState() const override;
    ConversationTransitionResult lastTransitionResult() const override;
    ConversationTransitionResult transitionTo(ConversationState nextState,
                                              const QString& reason = {}) override;
    void reset() override;

private:
    ConversationState currentState_ = ConversationState::Idle;
    ConversationTransitionResult lastTransitionResult_;
};

bool isConversationTransitionAllowed(ConversationState from, ConversationState to);

} // namespace sentinel::core
