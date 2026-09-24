// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/runtime/BuiltInToolProvider.h"

#include <QRegularExpression>

#include <utility>

namespace sentinel::core {

namespace {

bool isAutomaticWebSearchIntent(const QString& prompt) {
    const auto normalized = prompt.toLower().simplified();
    static const QStringList intentPhrases{QStringLiteral("en son"),
                                           QStringLiteral("en güncel"),
                                           QStringLiteral("en guncel"),
                                           QStringLiteral("bugün"),
                                           QStringLiteral("bugun"),
                                           QStringLiteral("son dakika"),
                                           QStringLiteral("hava durumu"),
                                           QStringLiteral("hava nasıl"),
                                           QStringLiteral("hava nasil"),
                                           QStringLiteral("weather"),
                                           QStringLiteral("forecast"),
                                           QStringLiteral("temperature"),
                                           QStringLiteral("haber"),
                                           QStringLiteral("internette"),
                                           QStringLiteral("webde"),
                                           QStringLiteral("web'de"),
                                           QStringLiteral("araştır"),
                                           QStringLiteral("arastir"),
                                           QStringLiteral("online araştır"),
                                           QStringLiteral("online arastir"),
                                           QStringLiteral("latest"),
                                           QStringLiteral("current"),
                                           QStringLiteral("today"),
                                           QStringLiteral("breaking news"),
                                           QStringLiteral("news"),
                                           QStringLiteral("recent"),
                                           QStringLiteral("research online"),
                                           QStringLiteral("search online"),
                                           QStringLiteral("look up"),
                                           QStringLiteral("on the web")};
    for (const auto& phrase : intentPhrases) {
        if (normalized.contains(phrase)) {
            return true;
        }
    }
    return false;
}

} // namespace

namespace {

// Returns a cleaned URL for domain-like targets ("sahibinden.com",
// "www.x.com", "https://x.com", "sahibinden.com'u") or an empty string when
// the text is not a website address. Used to route "X aç" requests to
// open-url instead of app-launch.
QString urlTargetFromName(const QString& text) {
    QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || trimmed.contains(QLatin1Char(' '))) {
        return QString();
    }

    const QString lowered = trimmed.toLower();
    if (lowered.startsWith(QStringLiteral("http://")) ||
        lowered.startsWith(QStringLiteral("https://")) ||
        lowered.startsWith(QStringLiteral("www."))) {
        return trimmed;
    }

    // Strip a trailing Turkish possessive suffix: "sahibinden.com'u".
    const int apos = trimmed.lastIndexOf(QLatin1Char('\''));
    if (apos > 0) {
        trimmed = trimmed.left(apos);
    }

    const int dot = trimmed.lastIndexOf(QLatin1Char('.'));
    if (dot < 1 || dot >= trimmed.size() - 2) {
        return QString();
    }

    const QString suffix = trimmed.mid(dot + 1);
    if (suffix.size() > 10) {
        return QString();
    }
    for (const QChar c : suffix) {
        if (!c.isLetter()) {
            return QString();
        }
    }

    for (const QChar c : trimmed.left(dot)) {
        if (!c.isLetterOrNumber() && c != QLatin1Char('-') && c != QLatin1Char('.')) {
            return QString();
        }
    }
    return trimmed;
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
    return BuiltInToolProvider::descriptors();
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
                   word == QStringLiteral("open") || word == QStringLiteral("başlat") ||
                   word == QStringLiteral("baslat") || word == QStringLiteral("launch") ||
                   word == QStringLiteral("start");
        };
        auto isQuitVerb = [](const QString& word) {
            return word == QStringLiteral("kapat") || word == QStringLiteral("kapat.") ||
                   word == QStringLiteral("quit") || word == QStringLiteral("close") ||
                   word == QStringLiteral("exit") || word == QStringLiteral("sonlandır") ||
                   word == QStringLiteral("durdur");
        };

        QString programName;
        bool wantsQuit = false;
        QStringList words = trimmed.split(QChar(' '), Qt::SkipEmptyParts);
        if (words.size() >= 2) {
            if (isLaunchVerb(words.first().toLower())) {
                programName = words.mid(1).join(QChar(' '));
            } else if (isLaunchVerb(words.last().toLower())) {
                programName = words.mid(0, words.size() - 1).join(QChar(' '));
            } else if (isQuitVerb(words.first().toLower())) {
                programName = words.mid(1).join(QChar(' '));
                wantsQuit = true;
            } else if (isQuitVerb(words.last().toLower())) {
                programName = words.mid(0, words.size() - 1).join(QChar(' '));
                wantsQuit = true;
            }
        }

        auto stripSuffix = [](QString& name) {
            if (name.endsWith(QChar('\'')) || name.endsWith(QStringLiteral("'ı")) ||
                name.endsWith(QStringLiteral("'i")) || name.endsWith(QStringLiteral("'u")) ||
                name.endsWith(QStringLiteral("'ü")) || name.endsWith(QChar('.')) ||
                name.endsWith(QChar('!'))) {
                int index = name.indexOf(QChar('\''));
                if (index > 0) {
                    name = name.left(index);
                }
                while (!name.isEmpty() && !name.at(name.size() - 1).isLetterOrNumber()) {
                    name.chop(1);
                }
            }
            if (!name.isEmpty() && name.at(0).isLower()) {
                name[0] = name[0].toUpper();
            }
        };

        QString urlTarget;
        if (!programName.isEmpty()) {
            // 'open -a X' is an explicit shell pattern; leave it to run-command.
            if (programName.startsWith(QLatin1Char('-')) ||
                trimmed.contains(QStringLiteral("open -a"))) {
                programName.clear();
            } else {
                // Website targets keep their original form; app names get the
                // Turkish suffix stripping and capitalization treatment.
                urlTarget = urlTargetFromName(programName);
                if (urlTarget.isEmpty()) {
                    stripSuffix(programName);
                }
            }
        }

        const bool hasAppTools = toolRegistry_.findToolById(QStringLiteral("app-quit")).has_value();

        if (!programName.isEmpty() && wantsQuit && hasAppTools) {
            selectedToolId = QStringLiteral("app-quit");
            arguments.append(ToolInvocationArgument{QStringLiteral("app"), programName});
        } else if (!urlTarget.isEmpty() && !wantsQuit &&
                   toolRegistry_.findToolById(QStringLiteral("open-url")).has_value()) {
            // Website requests like 'sahibinden.com aç' open in the browser,
            // never as desktop applications.
            selectedToolId = QStringLiteral("open-url");
            arguments.append(ToolInvocationArgument{QStringLiteral("url"), urlTarget});
        } else if (!programName.isEmpty() && !wantsQuit &&
                   toolRegistry_.findToolById(QStringLiteral("app-launch")).has_value()) {
            selectedToolId = QStringLiteral("app-launch");
            arguments.append(ToolInvocationArgument{QStringLiteral("app"), programName});
        } else if (!programName.isEmpty() && !wantsQuit) {
            selectedToolId = QStringLiteral("run-command");
            arguments.append(ToolInvocationArgument{
                QStringLiteral("command"), QStringLiteral("open -a \"%1\"").arg(programName)});
        } else if ((trimmed.contains(QStringLiteral("alarm")) ||
                    trimmed.contains(QStringLiteral("hatırlat")) ||
                    trimmed.contains(QStringLiteral("hatirlat")) ||
                    trimmed.contains(QStringLiteral("reminder")) ||
                    trimmed.contains(QStringLiteral("remind me")) ||
                    trimmed.contains(QStringLiteral("uyandır")) ||
                    trimmed.contains(QStringLiteral("uyandir"))) &&
                   toolRegistry_.findToolById(QStringLiteral("set-alarm")).has_value()) {
            static const QRegularExpression timePattern(
                QStringLiteral("\\b(\\d{1,2}):(\\d{2})\\b"));
            const auto timeMatch = timePattern.match(trimmed);
            if (timeMatch.hasMatch()) {
                selectedToolId = QStringLiteral("set-alarm");
                arguments.append(
                    ToolInvocationArgument{QStringLiteral("time"), timeMatch.captured(0)});
                arguments.append(ToolInvocationArgument{QStringLiteral("label"), trimmed});
            } else {
                selectedToolId = QStringLiteral("set-alarm");
                arguments.append(ToolInvocationArgument{QStringLiteral("time"), trimmed});
                arguments.append(ToolInvocationArgument{QStringLiteral("label"), trimmed});
            }
        } else if ((trimmed.startsWith(QStringLiteral("bildir ")) ||
                    trimmed.startsWith(QStringLiteral("notify ")) ||
                    trimmed.contains(QStringLiteral("bildirim göster")) ||
                    trimmed.contains(QStringLiteral("notification göster"))) &&
                   toolRegistry_.findToolById(QStringLiteral("system-notify")).has_value()) {
            selectedToolId = QStringLiteral("system-notify");
            QString message = trimmed;
            for (const auto& prefix :
                 {QStringLiteral("bildir "), QStringLiteral("notify "),
                  QStringLiteral("bildirim göster"), QStringLiteral("notification göster")}) {
                if (message.startsWith(prefix)) {
                    message = message.mid(prefix.size()).trimmed();
                    break;
                }
            }
            arguments.append(
                ToolInvocationArgument{QStringLiteral("title"), QStringLiteral("Sentinel")});
            arguments.append(ToolInvocationArgument{QStringLiteral("message"), message});
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
            return {ToolInvocationPlanStatus::NotRequested,
                    QStringLiteral("No safe heuristic decision for this request."),
                    {}};
        }
    }

    const auto toolOpt = toolRegistry_.findToolById(selectedToolId);
    if (!toolOpt.has_value()) {
        return {ToolInvocationPlanStatus::UnknownTool,
                QStringLiteral("Planned tool was not found in registry: %1").arg(selectedToolId),
                {}};
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
