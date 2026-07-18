#include "sentinel/core/NullAgentRuntime.h"

#include <utility>

namespace sentinel::core {

NullAgentRuntime::NullAgentRuntime()
    : NullAgentRuntime(QList<ToolDescriptor>{
          ToolDescriptor{
              QStringLiteral("local-plan-summary"),
              QStringLiteral("Local Plan Summary"),
              QStringLiteral("Local planning summary tool."),
              ToolRiskLevel::Low,
              ToolExecutionMode::MetadataOnly,
              {
                  ToolParameterDescriptor{QStringLiteral("topic"), QStringLiteral("Short user topic for local summary."), true},
              },
          },
          ToolDescriptor{
              QStringLiteral("open-workspace"),
              QStringLiteral("Open Workspace"),
              QStringLiteral("Open workspace folder path."),
              ToolRiskLevel::Medium,
              ToolExecutionMode::Local,
              {
                  ToolParameterDescriptor{QStringLiteral("path"), QStringLiteral("Absolute path to workspace folder."), true},
              },
          },
          ToolDescriptor{
              QStringLiteral("read-file"),
              QStringLiteral("Read File"),
              QStringLiteral("Read the contents of a file."),
              ToolRiskLevel::High,
              ToolExecutionMode::Local,
              {
                  ToolParameterDescriptor{QStringLiteral("path"), QStringLiteral("Absolute path to the target file."), true},
              },
          },
          ToolDescriptor{
              QStringLiteral("write-file"),
              QStringLiteral("Write File"),
              QStringLiteral("Write content to a file."),
              ToolRiskLevel::High,
              ToolExecutionMode::Local,
              {
                  ToolParameterDescriptor{QStringLiteral("path"), QStringLiteral("Absolute path to the target file."), true},
                  ToolParameterDescriptor{QStringLiteral("content"), QStringLiteral("Content to write into the file."), true},
              },
          },
          ToolDescriptor{
              QStringLiteral("run-command"),
              QStringLiteral("Run Command"),
              QStringLiteral("Run a shell command on the host machine."),
              ToolRiskLevel::High,
              ToolExecutionMode::Local,
              {
                  ToolParameterDescriptor{QStringLiteral("command"), QStringLiteral("Shell command to run."), true},
              },
          },
          ToolDescriptor{
              QStringLiteral("voice-transcribe"),
              QStringLiteral("Voice Transcribe"),
              QStringLiteral("Transcribe an audio file using Whisper."),
              ToolRiskLevel::Medium,
              ToolExecutionMode::Local,
              {
                  ToolParameterDescriptor{QStringLiteral("path"), QStringLiteral("Path to the input audio file."), true},
              },
          },
          ToolDescriptor{
              QStringLiteral("voice-speak"),
              QStringLiteral("Voice Speak"),
              QStringLiteral("Synthesize speech audio from text using Piper."),
              ToolRiskLevel::Medium,
              ToolExecutionMode::Local,
              {
                  ToolParameterDescriptor{QStringLiteral("text"), QStringLiteral("Text to synthesize into audio."), true},
              },
          },
          ToolDescriptor{
              QStringLiteral("web-search"),
              QStringLiteral("Web Search"),
              QStringLiteral("Perform a search query on the web."),
              ToolRiskLevel::Medium,
              ToolExecutionMode::Local,
              {
                  ToolParameterDescriptor{QStringLiteral("query"), QStringLiteral("Search query terms."), true},
              },
          },
          ToolDescriptor{
              QStringLiteral("export-conversation"),
              QStringLiteral("Export Conversation"),
              QStringLiteral("Export active chat transcript."),
              ToolRiskLevel::Low,
              ToolExecutionMode::Local,
              {
                  ToolParameterDescriptor{QStringLiteral("path"), QStringLiteral("Target export path."), false},
              },
          },
          ToolDescriptor{
              QStringLiteral("summarize-current-conversation"),
              QStringLiteral("Summarize Current Conversation"),
              QStringLiteral("Generate current conversation summary."),
              ToolRiskLevel::Low,
              ToolExecutionMode::Local,
              {}
          },
          ToolDescriptor{
              QStringLiteral("provider-test-call"),
              QStringLiteral("Provider Test Call"),
              QStringLiteral("Trigger provider endpoint health check."),
              ToolRiskLevel::Low,
              ToolExecutionMode::Local,
              {}
          }
      }) {}

NullAgentRuntime::NullAgentRuntime(QList<ToolDescriptor> tools) {
    for (auto& tool : tools) {
        toolRegistry_.registerTool(std::move(tool));
    }
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

ToolInvocationPlan NullAgentRuntime::plan(const AgentRequest& request) const {
    const auto trimmed = request.prompt.trimmed();
    if (trimmed.isEmpty()) {
        return {
            ToolInvocationPlanStatus::EmptyRequest,
            QStringLiteral("Agent request was empty."),
            {},
        };
    }

    const auto tools = toolRegistry_.listTools();
    if (tools.isEmpty()) {
        return {
            ToolInvocationPlanStatus::NoToolsAvailable,
            QStringLiteral("No tool metadata is available for planning."),
            {},
        };
    }

    const auto requestedToolId = request.requestedToolId.trimmed();
    QList<ToolDescriptor> selectedTools;
    if (!requestedToolId.isEmpty()) {
        const auto requestedTool = toolRegistry_.findToolById(requestedToolId);
        if (!requestedTool.has_value()) {
            return {
                ToolInvocationPlanStatus::UnknownTool,
                QStringLiteral("Requested tool metadata was not found: %1").arg(requestedToolId),
                {},
            };
        }
        selectedTools.append(*requestedTool);
    } else {
        selectedTools = tools;
    }

    QList<PlannedToolInvocation> invocations;
    for (const auto& tool : selectedTools) {
        QList<ToolInvocationArgument> arguments;
        for (const auto& parameter : tool.parameters) {
            arguments.append(ToolInvocationArgument{
                parameter.id,
                parameter.required ? trimmed : QString(),
            });
        }

        invocations.append(PlannedToolInvocation{
            tool.id,
            tool.name,
            QStringLiteral("Plan metadata for %1").arg(tool.name),
            QStringLiteral("Deterministic metadata-only plan for: %1").arg(trimmed),
            tool.riskLevel,
            tool.executionMode,
            arguments,
        });
    }

    return {
        ToolInvocationPlanStatus::Planned,
        QStringLiteral("Metadata-only tool plan prepared."),
        invocations,
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
