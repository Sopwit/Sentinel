#include "sentinel/core/NullAgentRuntime.h"

namespace sentinel::core {

NullAgentRuntime::NullAgentRuntime() {
    toolRegistry_.registerTool(ToolDescriptor{
        QStringLiteral("local-plan-summary"),
        QStringLiteral("Local Plan Summary"),
        QStringLiteral("Metadata-only placeholder tool descriptor for local planning summaries."),
        ToolRiskLevel::Low,
        ToolExecutionMode::MetadataOnly,
        {
            ToolParameterDescriptor{QStringLiteral("topic"),
                                    QStringLiteral("Short user topic for local summary."), true},
        },
    });
}

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

QList<ToolDescriptor> NullAgentRuntime::availableTools() const {
    return toolRegistry_.listTools();
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
