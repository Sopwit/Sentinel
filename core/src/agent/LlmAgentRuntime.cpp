// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/LlmAgentRuntime.h"

#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>

#include <utility>

namespace sentinel::core {

namespace {

QString extractJsonObject(const QString& text) {
    QString trimmed = text.trimmed();
    if (trimmed.startsWith(QStringLiteral("```"))) {
        const int firstNewline = trimmed.indexOf(QLatin1Char('\n'));
        if (firstNewline >= 0) {
            trimmed = trimmed.mid(firstNewline + 1);
        }
        const int fenceEnd = trimmed.lastIndexOf(QStringLiteral("```"));
        if (fenceEnd >= 0) {
            trimmed = trimmed.left(fenceEnd);
        }
        trimmed = trimmed.trimmed();
    }

    const int start = trimmed.indexOf(QLatin1Char('{'));
    if (start < 0) {
        return {};
    }

    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (int i = start; i < trimmed.size(); ++i) {
        const QChar c = trimmed.at(i);
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (c == QLatin1Char('\\')) {
                escaped = true;
            } else if (c == QLatin1Char('"')) {
                inString = false;
            }
            continue;
        }
        if (c == QLatin1Char('"')) {
            inString = true;
        } else if (c == QLatin1Char('{')) {
            ++depth;
        } else if (c == QLatin1Char('}')) {
            --depth;
            if (depth == 0) {
                return trimmed.mid(start, i - start + 1);
            }
        }
    }
    return {};
}

QString toolRiskLine(const ToolDescriptor& tool) {
    QStringList parameters;
    for (const auto& parameter : tool.parameters) {
        parameters.append(QStringLiteral("%1%2").arg(
            parameter.id,
            parameter.required ? QStringLiteral(" (required)") : QStringLiteral(" (optional)")));
    }
    return QStringLiteral("- %1 | risk: %2 | description: %3 | params: %4")
        .arg(tool.id,
             tool.riskLevel == ToolRiskLevel::High
                 ? QStringLiteral("High")
                 : (tool.riskLevel == ToolRiskLevel::Medium ? QStringLiteral("Medium")
                                                            : QStringLiteral("Low")),
             tool.description,
             parameters.isEmpty() ? QStringLiteral("none") : parameters.join(QStringLiteral(", ")));
}

} // namespace

LlmAgentRuntime::LlmAgentRuntime(QList<ToolDescriptor> tools, IChatProvider* provider)
    : heuristic_(tools), tools_(std::move(tools)), provider_(provider) {}

QString LlmAgentRuntime::name() const {
    return QStringLiteral("LlmAgentRuntime");
}

AgentStatus LlmAgentRuntime::status() const {
    return provider_ ? AgentStatus::Ready : AgentStatus::Unavailable;
}

QList<AgentCapabilityDescriptor> LlmAgentRuntime::capabilities() const {
    return {
        {QStringLiteral("llm-step-planning"),
         QStringLiteral("Plans each agent step with the configured chat provider."), true},
        {QStringLiteral("heuristic-fallback"),
         QStringLiteral("Falls back to deterministic local planning when no provider output is "
                        "available."),
         true},
    };
}

QList<ToolDescriptor> LlmAgentRuntime::availableTools() const {
    return tools_;
}

bool LlmAgentRuntime::lastDecisionUsedLlm() const {
    return lastDecisionUsedLlm_;
}

AgentStepDecision LlmAgentRuntime::nextStep(const QString& goal,
                                            const QList<AgentStepRecord>& history) const {
    lastDecisionUsedLlm_ = false;
    if (provider_ && !goal.trimmed().isEmpty()) {
        const auto reply = provider_->sendMessage(buildPlannerPrompt(goal, history));
        if (reply.success && !reply.message.trimmed().isEmpty()) {
            const auto decision = decisionFromLlmOutput(reply.message);
            if (decision.kind != AgentStepDecision::Kind::GiveUp || !decision.reason.isEmpty()) {
                lastDecisionUsedLlm_ = true;
                return decision;
            }
        }
    }
    return heuristicDecision(goal, history);
}

AgentStepDecision LlmAgentRuntime::decisionFromLlmOutput(const QString& output) const {
    AgentStepDecision invalid;
    invalid.reason = QString();

    const auto candidate = extractJsonObject(output);
    if (candidate.isEmpty()) {
        return invalid;
    }

    const auto document = QJsonDocument::fromJson(candidate.toUtf8());
    if (!document.isObject()) {
        return invalid;
    }
    const auto object = document.object();

    AgentStepDecision decision;
    decision.thought = object.value(QStringLiteral("thought")).toString();

    const QString action = object.value(QStringLiteral("action")).toString().toLower();
    if (action == QStringLiteral("final")) {
        decision.kind = AgentStepDecision::Kind::FinalAnswer;
        decision.answer = object.value(QStringLiteral("answer")).toString();
        if (decision.answer.trimmed().isEmpty()) {
            return invalid;
        }
        return decision;
    }

    if (action == QStringLiteral("giveup")) {
        decision.kind = AgentStepDecision::Kind::GiveUp;
        decision.reason = object.value(QStringLiteral("reason")).toString();
        if (decision.reason.trimmed().isEmpty()) {
            return invalid;
        }
        return decision;
    }

    if (action != QStringLiteral("tool")) {
        return invalid;
    }

    decision.kind = AgentStepDecision::Kind::ToolCall;
    decision.toolId = object.value(QStringLiteral("tool")).toString().trimmed();

    const ToolDescriptor* matched = nullptr;
    for (const auto& tool : tools_) {
        if (tool.id == decision.toolId) {
            matched = &tool;
            break;
        }
    }
    if (!matched) {
        return invalid;
    }

    decision.toolName = matched->name;
    decision.riskLevel = matched->riskLevel;
    decision.executionMode = matched->executionMode;

    const auto args = object.value(QStringLiteral("args")).toObject();
    for (auto it = args.begin(); it != args.end(); ++it) {
        const auto value = it.value();
        QString text;
        if (value.isString()) {
            text = value.toString();
        } else if (value.isDouble()) {
            text = QString::number(value.toDouble());
        } else if (value.isBool()) {
            text = value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
        } else {
            text = QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
        }
        decision.arguments.append(ToolInvocationArgument{it.key(), text});
    }

    return decision;
}

AgentStepDecision LlmAgentRuntime::heuristicDecision(const QString& goal,
                                                     const QList<AgentStepRecord>& history) const {
    AgentStepDecision decision;

    if (!history.isEmpty()) {
        decision.kind = AgentStepDecision::Kind::FinalAnswer;
        decision.thought = QStringLiteral("Local heuristic planner: summarizing completed steps.");
        QStringList parts;
        for (const auto& record : history) {
            const QString observation = record.observation.size() > 400
                                            ? record.observation.left(400) + QStringLiteral("…")
                                            : record.observation;
            parts.append(QStringLiteral("Step %1 (%2): %3")
                             .arg(QString::number(record.index), record.toolName, observation));
        }
        decision.answer = parts.join(QStringLiteral("\n"));
        return decision;
    }

    const auto plan = heuristic_.plan(AgentRequest{goal, QString()});
    if (plan.status != ToolInvocationPlanStatus::Planned || plan.invocations.isEmpty()) {
        decision.kind = AgentStepDecision::Kind::GiveUp;
        decision.reason = plan.summary;
        return decision;
    }

    const auto& invocation = plan.invocations.first();
    decision.kind = AgentStepDecision::Kind::ToolCall;
    decision.toolId = invocation.toolId;
    decision.toolName = invocation.toolName;
    decision.riskLevel = invocation.riskLevel;
    decision.executionMode = invocation.executionMode;
    decision.arguments = invocation.arguments;
    decision.thought = QStringLiteral("Heuristic plan: %1").arg(plan.summary);
    return decision;
}

QString LlmAgentRuntime::buildPlannerPrompt(const QString& goal,
                                            const QList<AgentStepRecord>& history) const {
    QStringList toolLines;
    for (const auto& tool : tools_) {
        toolLines.append(toolRiskLine(tool));
    }

    QStringList historyLines;
    for (const auto& record : history) {
        QStringList argumentParts;
        for (const auto& argument : record.arguments) {
            argumentParts.append(
                QStringLiteral("%1=%2").arg(argument.id, argument.value.left(500)));
        }
        const QString observation =
            record.observation.size() > 1200
                ? record.observation.left(1200) + QStringLiteral("… [truncated]")
                : record.observation;
        historyLines.append(
            QStringLiteral("%1. thought=%2 | action=%3(%4) -> %5\n   observation: %6")
                .arg(QString::number(record.index), record.thought.left(500), record.toolId,
                     argumentParts.join(QStringLiteral(", ")), record.statusText, observation));
    }

    const QString environmentBlock =
        QStringLiteral("PLATFORM: %1 (%2)\nWORKSPACE: %3\nNOW: %4")
            .arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture(),
                 QDir::currentPath(),
                 QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")));

    return QStringLiteral(
               "You are Sentinel's autonomous agent planner. Decide the SINGLE next action toward "
               "the "
               "user's goal.\n\n"
               "ENVIRONMENT:\n%7\n\n"
               "AVAILABLE TOOLS:\n%1\n\n"
               "RULES:\n"
               "- Reply with exactly ONE JSON object and nothing else. No markdown fences, no "
               "prose.\n"
               "- To call a tool: {\"thought\":\"why\",\"action\":\"tool\",\"tool\":\"<tool id>\","
               "\"args\":{\"<param>\":\"<value>\"}}\n"
               "- When the goal is fully achieved: "
               "{\"thought\":\"...\",\"action\":\"final\",\"answer\":"
               "\"<final answer for the user>\"}\n"
               "- When the goal is impossible after the steps so far: {\"thought\":\"...\","
               "\"action\":\"giveup\",\"reason\":\"<why>\"}\n"
               "- Only call tools from the list. Use every required parameter.\n"
               "- Learn from observations: do not repeat a failed identical action; change the "
               "approach "
               "instead.\n"
               "- Prefer the fewest steps. Chain tools when one step's output is needed by the "
               "next.\n"
               "- Use open-workspace before file tools when the goal refers to a specific "
               "folder.\n"
                "- Prefer dedicated tools over shell: read-file instead of cat, grep/glob instead "
                "of grep/find in run-command, edit-file instead of sed. app-launch/app-quit open "
                "and close desktop apps (e.g. 'spotify aç' -> app-launch with app=Spotify). "
                "set-alarm schedules alarms/reminders. web-fetch reads a page after web-search.\n"
                "- open-url opens a URL in the default browser when the user wants to see a "
                "page. clipboard-read/clipboard-write access the system clipboard. "
                "system-info/process-list/current-time report machine state read-only. "
                "memory-search looks up facts already stored in long-term memory; try it "
                "before asking the user again. delete-file/move-file reorganize workspace "
                "files (delete is permanent).\n"
               "- For goals with 3+ steps, start with todo-write to record the checklist, keep "
               "exactly one item in_progress, and mark items completed only after verifying.\n"
               "- The user speaks Turkish or English; answer in the user's language.\n\n"
               "GOAL: %2\n\n"
               "STEPS SO FAR:\n%3\n\n"
               "NEXT ACTION (one JSON object only):")
        .arg(toolLines.join(QLatin1Char('\n')), goal,
             historyLines.isEmpty() ? QStringLiteral("(none yet)")
                                    : historyLines.join(QLatin1Char('\n')),
             environmentBlock);
}

ToolInvocationPlan LlmAgentRuntime::plan(const AgentRequest& request) const {
    const auto trimmed = request.prompt.trimmed();
    if (trimmed.isEmpty()) {
        return {
            ToolInvocationPlanStatus::EmptyRequest,
            QStringLiteral("Agent request was empty."),
            {},
        };
    }

    const auto decision = nextStep(trimmed, {});

    if (decision.kind == AgentStepDecision::Kind::ToolCall) {
        ToolInvocationPlan plan;
        plan.status = ToolInvocationPlanStatus::Planned;
        plan.summary = QStringLiteral("LLM tool plan prepared: %1").arg(decision.toolName);
        plan.invocations.append(PlannedToolInvocation{
            decision.toolId,
            decision.toolName,
            QStringLiteral("LLM plan for %1").arg(decision.toolId),
            decision.thought,
            decision.riskLevel,
            decision.executionMode,
            decision.arguments,
            {},
        });
        return plan;
    }

    if (decision.kind == AgentStepDecision::Kind::FinalAnswer) {
        return {
            ToolInvocationPlanStatus::NotRequested,
            QStringLiteral("LLM planner decided no tool is required: %1").arg(decision.answer),
            {},
        };
    }

    return {
        ToolInvocationPlanStatus::NotRequested,
        QStringLiteral("LLM planner gave up: %1").arg(decision.reason),
        {},
    };
}

AgentResponse LlmAgentRuntime::execute(const AgentRequest& request) {
    return heuristic_.execute(request);
}

} // namespace sentinel::core
