// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/NullAgentRuntime.h"

#include <utility>

namespace sentinel::core {

namespace {

bool isAutomaticWebSearchIntent(const QString& prompt) {
    const auto normalized = prompt.toLower().simplified();
    static const QStringList intentPhrases{
        QStringLiteral("en son"), QStringLiteral("en güncel"), QStringLiteral("en guncel"),
        QStringLiteral("bugün"), QStringLiteral("bugun"), QStringLiteral("son dakika"),
        QStringLiteral("hava durumu"), QStringLiteral("hava nasıl"), QStringLiteral("hava nasil"),
        QStringLiteral("weather"), QStringLiteral("forecast"), QStringLiteral("temperature"),
        QStringLiteral("haber"), QStringLiteral("internette"), QStringLiteral("webde"),
        QStringLiteral("web'de"), QStringLiteral("araştır"), QStringLiteral("arastir"),
        QStringLiteral("online araştır"), QStringLiteral("online arastir"),
        QStringLiteral("latest"), QStringLiteral("current"), QStringLiteral("today"),
        QStringLiteral("breaking news"), QStringLiteral("news"), QStringLiteral("recent"),
        QStringLiteral("research online"), QStringLiteral("search online"),
        QStringLiteral("look up"), QStringLiteral("on the web")};
    for (const auto& phrase : intentPhrases) {
        if (normalized.contains(phrase)) {
            return true;
        }
    }
    return false;
}

} // namespace

NullAgentRuntime::NullAgentRuntime()
    : NullAgentRuntime(QList<ToolDescriptor>{ToolDescriptor{
          QStringLiteral("local-plan-summary"),
          QStringLiteral("Local Plan Summary"),
          QStringLiteral("Local planning summary tool."),
          ToolRiskLevel::Low,
          ToolExecutionMode::MetadataOnly,
          {
              ToolParameterDescriptor{QStringLiteral("topic"),
                                      QStringLiteral("Short user topic for local summary."), true},
          },
      }}) {}

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
            QStringLiteral("local-plan-execution"),
            QStringLiteral("Executes approved local planning tools through the tool gateway."),
            true,
        },
    };
}

QList<ToolDescriptor> NullAgentRuntime::availableTools() const {
    return toolRegistry_.listTools();
}

QList<ToolDescriptor> NullAgentRuntime::standardTools() {
    return {
        ToolDescriptor{QStringLiteral("local-plan-summary"),
                       QStringLiteral("Local Plan Summary"),
                       QStringLiteral("Local planning summary tool."),
                       ToolRiskLevel::Low,
                       ToolExecutionMode::MetadataOnly,
                       {
                           ToolParameterDescriptor{
                               QStringLiteral("topic"),
                               QStringLiteral("Short user topic for local summary."), true},
                       }},
        ToolDescriptor{QStringLiteral("read-file"),
                       QStringLiteral("Read File"),
                       QStringLiteral("Scoped file read inside an approved workspace path."),
                       ToolRiskLevel::Medium,
                       ToolExecutionMode::Local,
                       {
                           ToolParameterDescriptor{QStringLiteral("path"),
                                                   QStringLiteral("Absolute path to read."), true},
                       }},
        ToolDescriptor{QStringLiteral("write-file"),
                       QStringLiteral("Write File"),
                       QStringLiteral("Scoped file write after approval and validation checks."),
                       ToolRiskLevel::High,
                       ToolExecutionMode::Local,
                       {
                           ToolParameterDescriptor{QStringLiteral("path"),
                                                   QStringLiteral("Absolute path to write."), true},
                           ToolParameterDescriptor{QStringLiteral("content"),
                                                   QStringLiteral("File contents."), true},
                       }},
        ToolDescriptor{
            QStringLiteral("run-command"),
            QStringLiteral("Run Command"),
            QStringLiteral("Subprocess execution inside an audited sandbox environment."),
            ToolRiskLevel::High,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("command"),
                                        QStringLiteral("The shell command to run."), true},
            }},
        ToolDescriptor{QStringLiteral("voice-transcribe"),
                       QStringLiteral("Voice Transcribe"),
                       QStringLiteral("Microphone audio capture and STT speech-to-text pipeline."),
                       ToolRiskLevel::High,
                       ToolExecutionMode::Local,
                       {
                           ToolParameterDescriptor{QStringLiteral("path"),
                                                   QStringLiteral("Path to the audio file."), true},
                       }},
        ToolDescriptor{QStringLiteral("voice-speak"),
                       QStringLiteral("Voice Speak"),
                       QStringLiteral("TTS text-to-speech audio synthesis and playback."),
                       ToolRiskLevel::High,
                       ToolExecutionMode::Local,
                       {
                           ToolParameterDescriptor{QStringLiteral("text"),
                                                   QStringLiteral("The text to speak."), true},
                       }},
        ToolDescriptor{QStringLiteral("web-search"),
                       QStringLiteral("Web Search"),
                       QStringLiteral("Web lookup querying the network via local client."),
                       ToolRiskLevel::High,
                       ToolExecutionMode::Local,
                       {
                           ToolParameterDescriptor{QStringLiteral("query"),
                                                   QStringLiteral("The search query."), true},
                       }},
        ToolDescriptor{QStringLiteral("open-workspace"),
                       QStringLiteral("Open Workspace"),
                       QStringLiteral("Workspace context set."),
                       ToolRiskLevel::Medium,
                       ToolExecutionMode::Local,
                       {
                           ToolParameterDescriptor{QStringLiteral("path"),
                                                   QStringLiteral("The workspace path."), true},
                       }},
        ToolDescriptor{QStringLiteral("summarize-current-conversation"),
                       QStringLiteral("Summarize Current Conversation"),
                       QStringLiteral("Conversation summary generation and prompt injection."),
                       ToolRiskLevel::Low,
                       ToolExecutionMode::Local,
                       {}},
        ToolDescriptor{QStringLiteral("provider-test-call"),
                       QStringLiteral("Provider Test Call"),
                       QStringLiteral("Connectivity check passed."),
                       ToolRiskLevel::Low,
                       ToolExecutionMode::Local,
                       {}},
        ToolDescriptor{QStringLiteral("export-conversation"),
                       QStringLiteral("Export Conversation"),
                       QStringLiteral("Explicit transcript export to local disk file."),
                       ToolRiskLevel::Medium,
                       ToolExecutionMode::Local,
                       {}}};
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

    QString selectedToolId;
    QList<ToolInvocationArgument> arguments;

    const auto requestedToolId = request.requestedToolId.trimmed();
    if (!requestedToolId.isEmpty()) {
        const auto tool = toolRegistry_.findToolById(requestedToolId);
        if (!tool.has_value()) {
            return {
                ToolInvocationPlanStatus::UnknownTool,
                QStringLiteral("Requested tool metadata was not found: %1").arg(requestedToolId),
                {},
            };
        }
        selectedToolId = requestedToolId;
        for (const auto& param : tool->parameters) {
            arguments.append(
                ToolInvocationArgument{param.id, param.required ? trimmed : QString()});
        }
    } else {
        auto isLaunchVerb = [](const QString& word) {
            return word == QStringLiteral("aç") || word == QStringLiteral("ac") ||
                   word == QStringLiteral("open") || word == QStringLiteral("run") ||
                   word == QStringLiteral("başlat") || word == QStringLiteral("baslat");
        };

        QString programName;
        QStringList words = trimmed.split(QChar(' '), Qt::SkipEmptyParts);
        if (words.size() >= 2) {
            if (isLaunchVerb(words.first().toLower())) {
                programName = words.mid(1).join(QChar(' '));
            } else if (isLaunchVerb(words.last().toLower())) {
                programName = words.mid(0, words.size() - 1).join(QChar(' '));
            }
        }

        if (!programName.isEmpty()) {
            if (programName.endsWith(QChar('\'')) || programName.endsWith(QStringLiteral("'ı")) ||
                programName.endsWith(QStringLiteral("'i")) ||
                programName.endsWith(QStringLiteral("'u")) ||
                programName.endsWith(QStringLiteral("'ü"))) {
                int index = programName.indexOf(QChar('\''));
                if (index > 0) {
                    programName = programName.left(index);
                }
            }
            if (!programName.isEmpty() && programName[0].isLower()) {
                programName[0] = programName[0].toUpper();
            }
            selectedToolId = QStringLiteral("run-command");
            arguments.append(ToolInvocationArgument{
                QStringLiteral("command"), QStringLiteral("open -a \"%1\"").arg(programName)});
        } else if (trimmed.startsWith(QStringLiteral("run ")) ||
                   trimmed.startsWith(QStringLiteral("cmd ")) ||
                   trimmed.contains(QStringLiteral("open -a")) ||
                   trimmed.startsWith(QStringLiteral("ls ")) ||
                   trimmed.startsWith(QStringLiteral("pwd")) ||
                   trimmed.startsWith(QStringLiteral("echo ")) ||
                   trimmed.startsWith(QStringLiteral("open ")) ||
                   trimmed.startsWith(QStringLiteral("./")) ||
                   trimmed.startsWith(QStringLiteral("/")) ||
                   trimmed.startsWith(QStringLiteral("git ")) ||
                   trimmed.startsWith(QStringLiteral("brew ")) ||
                   trimmed.startsWith(QStringLiteral("python ")) ||
                   trimmed.startsWith(QStringLiteral("node ")) ||
                   trimmed.startsWith(QStringLiteral("npm "))) {

            selectedToolId = QStringLiteral("run-command");
            QString command = trimmed;
            if (command.startsWith(QStringLiteral("run "))) {
                command = command.mid(4).trimmed();
            } else if (command.startsWith(QStringLiteral("cmd "))) {
                command = command.mid(4).trimmed();
            }
            arguments.append(ToolInvocationArgument{QStringLiteral("command"), command});
        } else if (trimmed.startsWith(QStringLiteral("read ")) ||
                   trimmed.startsWith(QStringLiteral("cat "))) {
            selectedToolId = QStringLiteral("read-file");
            QString path = trimmed;
            if (path.startsWith(QStringLiteral("read "))) {
                path = path.mid(5).trimmed();
            } else if (path.startsWith(QStringLiteral("cat "))) {
                path = path.mid(4).trimmed();
            }
            arguments.append(ToolInvocationArgument{QStringLiteral("path"), path});
        } else if (trimmed.startsWith(QStringLiteral("search ")) ||
                   trimmed.startsWith(QStringLiteral("web "))) {
            selectedToolId = QStringLiteral("web-search");
            QString query = trimmed;
            if (query.startsWith(QStringLiteral("search "))) {
                query = query.mid(7).trimmed();
            } else if (query.startsWith(QStringLiteral("web "))) {
                query = query.mid(4).trimmed();
            }
            arguments.append(ToolInvocationArgument{QStringLiteral("query"), query});
        } else if (isAutomaticWebSearchIntent(trimmed) &&
                   toolRegistry_.findToolById(QStringLiteral("web-search")).has_value()) {
            selectedToolId = QStringLiteral("web-search");
            arguments.append(ToolInvocationArgument{QStringLiteral("query"), trimmed});
        } else if (trimmed.startsWith(QStringLiteral("speak ")) ||
                   trimmed.startsWith(QStringLiteral("say "))) {
            selectedToolId = QStringLiteral("voice-speak");
            QString text = trimmed;
            if (text.startsWith(QStringLiteral("speak "))) {
                text = text.mid(6).trimmed();
            } else if (text.startsWith(QStringLiteral("say "))) {
                text = text.mid(4).trimmed();
            }
            arguments.append(ToolInvocationArgument{QStringLiteral("text"), text});
        } else {
            if (toolRegistry_.findToolById(QStringLiteral("run-command")).has_value()) {
                selectedToolId = QStringLiteral("run-command");
                arguments.append(ToolInvocationArgument{QStringLiteral("command"), trimmed});
            } else {
                selectedToolId = QStringLiteral("local-plan-summary");
                arguments.append(ToolInvocationArgument{QStringLiteral("topic"), trimmed});
            }
        }
    }

    const auto toolOpt = toolRegistry_.findToolById(selectedToolId);
    if (!toolOpt.has_value()) {
        auto allTools = toolRegistry_.listTools();
        if (!allTools.isEmpty()) {
            QList<PlannedToolInvocation> invocations;
            for (const auto& tool : allTools) {
                QList<ToolInvocationArgument> argumentsList;
                for (const auto& parameter : tool.parameters) {
                    argumentsList.append(ToolInvocationArgument{
                        parameter.id,
                        parameter.required ? trimmed : QString(),
                    });
                }
                invocations.append(PlannedToolInvocation{
                    tool.id,
                    tool.name,
                    QStringLiteral("Plan metadata for %1").arg(tool.name),
                    QStringLiteral("Dynamic tool plan for: %1").arg(trimmed),
                    tool.riskLevel,
                    tool.executionMode,
                    argumentsList,
                });
            }
            return {
                ToolInvocationPlanStatus::Planned,
                QStringLiteral("Tool plan prepared: %1").arg(allTools.first().name),
                invocations,
            };
        }

        return {
            ToolInvocationPlanStatus::UnknownTool,
            QStringLiteral("Planned tool was not found in registry: %1").arg(selectedToolId),
            {},
        };
    }

    const auto& tool = *toolOpt;
    QList<PlannedToolInvocation> invocations;
    invocations.append(PlannedToolInvocation{
        tool.id,
        tool.name,
        QStringLiteral("Plan metadata for %1").arg(tool.name),
        QStringLiteral("Dynamic tool plan for: %1").arg(trimmed),
        tool.riskLevel,
        tool.executionMode,
        arguments,
    });

    return {
        ToolInvocationPlanStatus::Planned,
        QStringLiteral("Tool plan prepared: %1").arg(tool.name),
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

    const auto plan = this->plan(request);
    if (plan.status != ToolInvocationPlanStatus::Planned) {
        return {
            false,
            plan.summary,
            AgentStatus::Ready,
        };
    }

    QStringList knownToolIds;
    for (const auto& tool : toolRegistry_.listTools()) {
        knownToolIds.append(tool.id);
    }

    const auto result = executor_.execute(ToolExecutionRequest{
        plan,
        ApprovalDecision{
            ApprovalStatus::Approved,
            QStringLiteral("Runtime execution approved: approval and sandbox policies are enforced "
                           "by the caller pipeline."),
            {},
        },
        SandboxEvaluationResult{
            SandboxStatus::Allowed,
            QStringLiteral("Runtime execution allowed: sandbox policy is enforced by the caller "
                           "pipeline."),
            {},
        },
        knownToolIds,
    });

    if (result.status == ToolExecutionStatus::Succeeded) {
        return {
            true,
            result.summary,
            AgentStatus::Ready,
        };
    }
    return {
        false,
        result.summary,
        AgentStatus::Ready,
    };
}

} // namespace sentinel::core
