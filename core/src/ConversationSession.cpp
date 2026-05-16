#include "sentinel/core/ConversationSession.h"

#include <utility>

namespace sentinel::core {

QString safeRuntimeContextWindowSummary(const RuntimeContextWindow& contextWindow) {
    if (!contextWindow.summary.isEmpty()) {
        return contextWindow.summary;
    }

    return QStringLiteral("No conversation context window yet.");
}

RuntimeContextWindow ConversationSessionContextBuilder::build(QString routingMode,
                                                              QString preferredAgentSummary,
                                                              QString memoryAffinitySummary,
                                                              QString orchestrationSnapshotSummary,
                                                              ContextScope scope) const {
    RuntimeContextWindow contextWindow;
    contextWindow.scope = scope;
    contextWindow.currentRoutingMode = std::move(routingMode);
    contextWindow.preferredAgentSummary = std::move(preferredAgentSummary);
    contextWindow.memoryAffinitySummary = std::move(memoryAffinitySummary);
    contextWindow.latestOrchestrationSnapshotSummary = std::move(orchestrationSnapshotSummary);
    contextWindow.summary =
        QStringLiteral("%1 context window: %2 route, %3, %4.")
            .arg(contextScopeName(contextWindow.scope), contextWindow.currentRoutingMode,
                 contextWindow.preferredAgentSummary, contextWindow.memoryAffinitySummary);
    return contextWindow;
}

ConversationSessionStore::ConversationSessionStore(ConversationSessionId id) {
    session_.id = std::move(id);
}

const ConversationSession& ConversationSessionStore::session() const {
    return session_;
}

void ConversationSessionStore::refreshContext(RuntimeContextWindow contextWindow,
                                              InteractionMode interactionMode,
                                              AttentionState attentionState) {
    session_.status = ConversationSessionStatus::Active;
    session_.interactionMode = interactionMode;
    session_.attentionState = attentionState;
    session_.contextWindow = std::move(contextWindow);
    session_.revision += 1;
}

} // namespace sentinel::core
