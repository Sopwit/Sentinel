#include "sentinel/core/ConversationStateGraph.h"

namespace sentinel::core {

namespace {

QString transitionSummary(ConversationState from, ConversationState to, const QString& reason) {
    const auto base =
        QStringLiteral("%1 -> %2").arg(conversationStateName(from), conversationStateName(to));
    return reason.trimmed().isEmpty() ? base : QStringLiteral("%1: %2").arg(base, reason.trimmed());
}

} // namespace

QString safeConversationTransitionSummary(const ConversationTransitionResult& result) {
    if (!result.summary.isEmpty()) {
        return result.summary;
    }

    return conversationTransitionStatusName(result.status);
}

bool isConversationTransitionAllowed(ConversationState from, ConversationState to) {
    if (to == ConversationState::Error) {
        return true;
    }

    switch (from) {
    case ConversationState::Idle:
        return to == ConversationState::Listening;
    case ConversationState::Listening:
        return to == ConversationState::Planning;
    case ConversationState::Planning:
        return to == ConversationState::Routing;
    case ConversationState::Routing:
        return to == ConversationState::WaitingForApproval ||
               to == ConversationState::ReadyToRespond;
    case ConversationState::WaitingForApproval:
        return to == ConversationState::ReadyToRespond;
    case ConversationState::ReadyToRespond:
        return to == ConversationState::Responding;
    case ConversationState::Responding:
        return to == ConversationState::Completed;
    case ConversationState::Completed:
        return to == ConversationState::Idle || to == ConversationState::Listening;
    case ConversationState::Error:
        return to == ConversationState::Idle || to == ConversationState::Listening;
    }

    return false;
}

ConversationState StaticConversationStateGraph::currentState() const {
    return currentState_;
}

ConversationTransitionResult StaticConversationStateGraph::lastTransitionResult() const {
    return lastTransitionResult_;
}

ConversationTransitionResult StaticConversationStateGraph::transitionTo(ConversationState nextState,
                                                                        const QString& reason) {
    ConversationTransitionResult result;
    result.transition = ConversationTransition{currentState_, nextState, reason.trimmed()};

    if (!isConversationTransitionAllowed(currentState_, nextState)) {
        result.status = ConversationTransitionStatus::Rejected;
        result.summary = QStringLiteral("Rejected conversation transition: %1")
                             .arg(transitionSummary(currentState_, nextState, reason));
        lastTransitionResult_ = result;
        return result;
    }

    result.status = ConversationTransitionStatus::Accepted;
    result.summary = QStringLiteral("Accepted conversation transition: %1")
                         .arg(transitionSummary(currentState_, nextState, reason));
    currentState_ = nextState;
    lastTransitionResult_ = result;
    return result;
}

void StaticConversationStateGraph::reset() {
    currentState_ = ConversationState::Idle;
    lastTransitionResult_ = ConversationTransitionResult{};
}

} // namespace sentinel::core
