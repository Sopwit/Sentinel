#include "sentinel/core/NullAgentRuntime.h"

namespace sentinel::core {

QString NullAgentRuntime::name() const {
    return QStringLiteral("NullAgentRuntime");
}

AgentStatus NullAgentRuntime::status() const {
    return AgentStatus::Ready;
}

QList<AgentCapabilityDescriptor> NullAgentRuntime::capabilities() const {
    return {
        {
            QStringLiteral("placeholder-local-response"),
            QStringLiteral("Deterministic local placeholder response with no tool execution."),
            true,
        },
    };
}

AgentResponse NullAgentRuntime::execute(const AgentRequest& request) {
    const auto trimmed = request.prompt.trimmed();
    if (trimmed.isEmpty()) {
        return {
            false,
            QStringLiteral("Agent request was empty."),
            AgentStatus::Ready,
        };
    }

    return {
        true,
        QStringLiteral("Local agent placeholder processed: %1").arg(trimmed),
        AgentStatus::Ready,
    };
}

} // namespace sentinel::core
