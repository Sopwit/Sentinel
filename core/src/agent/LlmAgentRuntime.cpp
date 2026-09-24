// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/LlmAgentRuntime.h"
#include "sentinel/core/runtime/IToolRegistry.h"
#include "sentinel/core/runtime/ToolArgumentValidator.h"

#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QSysInfo>

#include <algorithm>
#include <utility>

namespace sentinel::core {

namespace {

QString extractJsonObject(const QString& text) {
    const auto trimmed = text.trimmed();
    return trimmed.startsWith(QLatin1Char('{')) && trimmed.endsWith(QLatin1Char('}'))
               ? trimmed
               : QString{};
}

QString normalizedAnswer(QString text) {
    text = text.toCaseFolded();
    QString normalized;
    for (const auto character : text)
        normalized.append(character.isLetterOrNumber() ? character : QLatin1Char(' '));
    return normalized.simplified();
}

bool isEcho(const QString& answer, const QString& goal) {
    const auto normalizedGoal = normalizedAnswer(goal);
    const auto normalizedFinal = normalizedAnswer(answer);
    if (normalizedGoal.isEmpty() || normalizedFinal.isEmpty())
        return false;
    const int larger = qMax(normalizedGoal.size(), normalizedFinal.size());
    const int smaller = qMin(normalizedGoal.size(), normalizedFinal.size());
    return normalizedGoal == normalizedFinal ||
           (smaller >= 16 && larger <= smaller + qMax(8, smaller / 5) &&
            (normalizedGoal.contains(normalizedFinal) || normalizedFinal.contains(normalizedGoal)));
}

bool isContentFree(const QString& answer) {
    const auto normalized = normalizedAnswer(answer);
    static const QSet<QString> replies{
        QStringLiteral("ok"), QStringLiteral("okay"), QStringLiteral("sure"),
        QStringLiteral("done"), QStringLiteral("everything seems fine"),
        QStringLiteral("all good")};
    return replies.contains(normalized);
}

QString observationDomainForTool(const QString& id, ToolSource source) {
    if (source == ToolSource::MCP || source == ToolSource::Plugin)
        return QStringLiteral("external");
    if (id == QLatin1String("todo-write") || id == QLatin1String("todo-read") ||
        id == QLatin1String("ask-question") || id == QLatin1String("spawn-agent") ||
        id == QLatin1String("mcp-list") || id == QLatin1String("app-launch") ||
        id == QLatin1String("app-quit") || id == QLatin1String("open-url") ||
        id == QLatin1String("run-command"))
        return QStringLiteral("action");
    if (id == QLatin1String("list-directory") || id == QLatin1String("glob") ||
        id == QLatin1String("grep") || id == QLatin1String("read-file") ||
        id == QLatin1String("list-code-definitions"))
        return QStringLiteral("filesystem");
    if (id == QLatin1String("process-list") || id == QLatin1String("system-info") ||
        id == QLatin1String("current-time"))
        return QStringLiteral("system");
    if (id.startsWith(QLatin1String("clipboard-")))
        return QStringLiteral("clipboard");
    if (id.startsWith(QLatin1String("web-")) || id.startsWith(QLatin1String("browser-")))
        return QStringLiteral("network");
    if (id == QLatin1String("memory-search") || id == QLatin1String("history-search"))
        return QStringLiteral("memory");
    return QStringLiteral("external");
}

QString toolRiskLine(const ToolDescriptor& tool) {
    return QStringLiteral("- %1 | risk: %2 | %3 | %4")
        .arg(tool.id,
             tool.riskLevel == ToolRiskLevel::High
                 ? QStringLiteral("High")
                 : (tool.riskLevel == ToolRiskLevel::Medium ? QStringLiteral("Medium")
                                                            : QStringLiteral("Low")),
             tool.description, ToolArgumentValidator::compactContract(tool));
}

} // namespace

LlmAgentRuntime::LlmAgentRuntime(QList<ToolDescriptor> tools, IChatProvider* provider)
    : heuristic_(tools), tools_(std::move(tools)), provider_(provider) {}

void LlmAgentRuntime::bindModel(ModelBinding binding, std::shared_ptr<IChatProvider> provider) {
    boundProvider_ = std::move(provider);
    modelBinding_ = std::move(binding);
    provider_ = boundProvider_.get();
}

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
    };
}

QList<ToolDescriptor> LlmAgentRuntime::availableTools() const {
    return registry_ ? registry_->enabledTools() : tools_;
}

bool LlmAgentRuntime::lastDecisionUsedLlm() const {
    return lastDecisionUsedLlm_;
}

void LlmAgentRuntime::setStreamObserver(std::function<void(const QString&)> onDelta,
                                        std::shared_ptr<std::atomic_bool> cancellationToken) const {
    streamObserver_ = std::move(onDelta);
    streamCancellationToken_ = std::move(cancellationToken);
}

AgentStepDecision LlmAgentRuntime::nextStep(const QString& goal,
                                            const QList<AgentStepRecord>& history) const {
    lastDecisionUsedLlm_ = false;
    if (!provider_) {
        AgentStepDecision failure;
        failure.kind = AgentStepDecision::Kind::GiveUp;
        failure.reason = QStringLiteral("No model is configured for Agent Mode.");
        return failure;
    }
    const auto prompt = buildPlannerPrompt(goal, history);
    QString repair;
    for (int attempt = 0; attempt < 2; ++attempt) {
        const auto request = attempt == 0 ? prompt : repair;
        const auto reply = attempt == 0 && provider_->supportsStreaming() && streamObserver_
                               ? provider_->sendMessageStreaming(request, streamObserver_,
                                                                 streamCancellationToken_)
                               : provider_->sendMessage(request);
        if (!reply.success) {
            AgentStepDecision failure;
            failure.kind = AgentStepDecision::Kind::GiveUp;
            failure.reason = QStringLiteral("Model planning failed: %1").arg(reply.errorMessage);
            return failure;
        }
        const auto decision = decisionFromLlmOutput(reply.message);
        bool valid = decision.kind != AgentStepDecision::Kind::GiveUp || !decision.reason.isEmpty();
        QString repairReason = QStringLiteral("Return one valid JSON action.");
        if (valid && decision.kind == AgentStepDecision::Kind::FinalAnswer) {
            if (isEcho(decision.answer, goal) || isContentFree(decision.answer)) {
                valid = false;
                repairReason = QStringLiteral(
                    "Your final answer repeated the goal or contained no useful answer. "
                    "Choose a tool or give a specific answer.");
            } else {
                QString classificationError;
                const auto domain = observationDomainForGoal(goal, &classificationError);
                if (!classificationError.isEmpty()) {
                    AgentStepDecision failure;
                    failure.kind = AgentStepDecision::Kind::GiveUp;
                    failure.reason = QStringLiteral("Model planning failed: %1")
                                         .arg(classificationError);
                    return failure;
                }
                if (!domain || (decision.observationRequirementDeclared &&
                                decision.requiresObservation && *domain == QLatin1String("none"))) {
                    valid = false;
                    repairReason = QStringLiteral(
                        "Classify whether the goal needs current external observation, then "
                        "choose a tool or a grounded final answer.");
                } else if (*domain != QLatin1String("none") &&
                           !hasRelevantObservation(*domain, history)) {
                    valid = false;
                    repairReason = QStringLiteral(
                        "The answer requires observing external state. Choose one available "
                        "observation tool with valid arguments. Do not guess.");
                }
            }
        }
        // The gateway validates the registered schema. Keep the shell-specific
        // semantic guard here because schema cannot classify user prose.
        if (valid && decision.kind == AgentStepDecision::Kind::ToolCall &&
            decision.toolId == QLatin1String("run-command")) {
            for (const auto& argument : decision.arguments) {
                if (argument.id == QLatin1String("command") &&
                    argument.value.trimmed() == goal.trimmed()) {
                    valid = false;
                    repairReason = QStringLiteral("Use a real shell command or a dedicated tool.");
                }
            }
        }
        if (valid) {
            lastDecisionUsedLlm_ = true;
            return decision;
        }
        repair = prompt + QStringLiteral("\nREPAIR: %1 Return exactly one JSON object.")
                              .arg(repairReason);
    }
    AgentStepDecision failure;
    failure.kind = AgentStepDecision::Kind::GiveUp;
    failure.reason = QStringLiteral("Agent could not determine a valid next action.");
    return failure;
}

std::optional<QString> LlmAgentRuntime::observationDomainForGoal(const QString& goal,
                                                                 QString* error) const {
    QStringList domains;
    for (const auto& tool : availableTools()) {
        const auto domain = observationDomainForTool(tool.id, tool.source);
        if (domain != QLatin1String("action") && !domains.contains(domain))
            domains.append(domain);
    }
    const auto request = QStringLiteral(
        "Decide whether answering this goal requires a FRESH observation of current external "
        "state. This applies to files, workspace, processes, clipboard, network, and external "
        "services, in any language. General knowledge and conversation-only questions do not "
        "require it. Return ONLY JSON: {\"domain\":\"none|filesystem|system|clipboard|"
        "network|memory|external\"}. Use 'none' only when no live tool observation is needed. "
        "Available domains: %1. Goal: %2")
                             .arg(domains.join(QLatin1Char(',')), goal);
    const auto reply = provider_->sendMessage(request);
    if (!reply.success) {
        if (error)
            *error = reply.errorMessage;
        return std::nullopt;
    }
    const auto candidate = extractJsonObject(reply.message);
    if (candidate.isEmpty())
        return std::nullopt;
    const auto document = QJsonDocument::fromJson(candidate.toUtf8());
    if (!document.isObject())
        return std::nullopt;
    const auto domain = document.object().value(QStringLiteral("domain")).toString();
    static const QSet<QString> allowed{
        QStringLiteral("none"),       QStringLiteral("filesystem"),
        QStringLiteral("system"),     QStringLiteral("clipboard"),
        QStringLiteral("network"),    QStringLiteral("memory"),
        QStringLiteral("external")};
    return allowed.contains(domain) ? std::optional<QString>(domain) : std::nullopt;
}

bool LlmAgentRuntime::hasRelevantObservation(const QString& domain,
                                             const QList<AgentStepRecord>& history) const {
    for (const auto& step : history) {
        if (step.toolId.isEmpty() || step.statusText == QLatin1String("Invalid Arguments") ||
            step.statusText == QLatin1String("Unknown Tool"))
            continue;
        for (const auto& tool : availableTools()) {
            if (tool.id != step.toolId)
                continue;
            const auto toolDomain = observationDomainForTool(tool.id, tool.source);
            if (toolDomain == domain)
                return true;
        }
    }
    return false;
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
    if (object.value(QStringLiteral("requiresObservation")).isBool()) {
        decision.requiresObservation = object.value(QStringLiteral("requiresObservation")).toBool();
        decision.observationRequirementDeclared = true;
    }

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
    for (const auto& tool : availableTools()) {
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

    if (!object.value(QStringLiteral("args")).isObject())
        return invalid;
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
        } else if (value.isArray()) {
            text = QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
        } else if (value.isObject()) {
            text = QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
        } else {
            text = QStringLiteral("null");
        }
        decision.arguments.append(ToolInvocationArgument{it.key(), text, value});
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
    for (const auto& tool : availableTools()) {
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
        historyLines.append(QStringLiteral("%1. %2(%3) [%4]: %5")
                                .arg(QString::number(record.index), record.toolId,
                                     argumentParts.join(QStringLiteral(", ")),
                                     record.statusText, observation));
    }

    const QString environmentBlock =
        QStringLiteral("PLATFORM: %1 (%2)\nWORKSPACE: %3\nNOW: %4")
            .arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture(),
                 QDir::currentPath(),
                 QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")));

    return QStringLiteral(
               "You are Sentinel's Agent Mode planner. Choose ONE next action. Reply with "
               "exactly one JSON object, no prose or markdown.\n"
               "Tool: {\"action\":\"tool\",\"tool\":\"id\",\"args\":{}}\n"
               "Final: {\"action\":\"final\",\"requiresObservation\":false,"
               "\"answer\":\"specific answer\"}\n"
               "Failure: {\"action\":\"giveup\",\"reason\":\"why\"}\n"
               "If the answer depends on current files, workspace, processes, clipboard, "
               "network, or external services, use an observation tool BEFORE final. Never "
               "guess that a resource exists. After a tool failure, use another tool or "
               "explain that verification failed. Do not echo the goal. Prefer list-directory, "
               "glob, read-file, and grep over shell for filesystem questions. Use only "
               "listed tools and valid JSON arguments. Answer in the user's language.\n"
               "ENVIRONMENT: %4\nTOOLS:\n%1\nGOAL: %2\nCURRENT TURN OBSERVATIONS:\n%3\n"
               "NEXT JSON ACTION:")
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
