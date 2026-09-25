// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/LlmAgentRuntime.h"
#include "sentinel/core/runtime/IToolRegistry.h"
#include "sentinel/core/runtime/ToolArgumentValidator.h"
#include "sentinel/core/security/AuthorizationResolver.h"

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

QString toolRiskLine(const ToolDescriptor& tool) {
    QStringList authorization;
    for (const auto& requirement : tool.authorizationRequirements) {
        QString entry = QStringLiteral("%1/%2")
                            .arg(securityDomainName(requirement.domain),
                                 accessModeName(requirement.access));
        if (!requirement.resourceArgument.isEmpty())
            entry += QStringLiteral(" (%1)").arg(requirement.resourceArgument);
        authorization.append(std::move(entry));
    }
    if (authorization.isEmpty() &&
        (tool.source == ToolSource::MCP || tool.source == ToolSource::Plugin))
        authorization.append(QStringLiteral("External service/Invoke"));
    const QString authorizationLine = authorization.isEmpty()
                                          ? QStringLiteral("none")
                                          : authorization.join(QStringLiteral(", "));
    return QStringLiteral("- %1 | risk: %2 | authorization: %3 | %4 | %5")
        .arg(tool.id,
             tool.riskLevel == ToolRiskLevel::High
                 ? QStringLiteral("High")
                 : (tool.riskLevel == ToolRiskLevel::Medium ? QStringLiteral("Medium")
                                                            : QStringLiteral("Low")),
             authorizationLine, tool.description,
             ToolArgumentValidator::compactContract(tool));
}

} // namespace

LlmAgentRuntime::LlmAgentRuntime(QList<ToolDescriptor> tools, IChatProvider* provider)
    : tools_(std::move(tools)), provider_(provider) {}

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
    const auto enabled = registry_ ? registry_->enabledTools() : tools_;
    QList<ToolDescriptor> visible;
    for (const auto& tool : enabled)
        if (allowedToolIds_.isEmpty() || allowedToolIds_.contains(tool.id))
            visible.append(tool);
    return visible;
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
    const auto feedback = std::exchange(plannerFeedback_, QString{});
    QString repair = feedback.isEmpty() ? QString{} : prompt + QStringLiteral("\nREPAIR: %1").arg(feedback);
    for (int attempt = 0; attempt < 2; ++attempt) {
        const auto request = attempt == 0 ? (repair.isEmpty() ? prompt : repair) : repair;
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
        const auto grounding = object.value(QStringLiteral("grounding")).toString();
        if (grounding == QLatin1String("verified")) {
            decision.grounding = GroundingMode::Verified;
            decision.groundingDeclared = true;
        } else if (grounding == QLatin1String("unable_to_verify")) {
            decision.grounding = GroundingMode::UnableToVerify;
            decision.groundingDeclared = true;
        } else if (grounding == QLatin1String("context")) {
            decision.grounding = GroundingMode::Context;
            decision.groundingDeclared = true;
        }
        for (const auto& claim : object.value(QStringLiteral("claims")).toArray()) {
            const auto item = claim.toObject();
            if (item.value(QStringLiteral("assertion")).isBool())
                decision.claims.append({item.value(QStringLiteral("id")).toString(),
                                        item.value(QStringLiteral("assertion")).toBool()});
        }
        if (!decision.groundingDeclared || decision.answer.trimmed().isEmpty()) {
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
               "Final: {\"action\":\"final\",\"grounding\":\"context|verified|"
               "unable_to_verify\",\"claims\":[{\"id\":\"claim-1\",\"assertion\":true}],\"answer\":\"specific answer\"}\n"
               "Failure: {\"action\":\"giveup\",\"reason\":\"why\"}\n"
               "If the answer depends on current files, workspace, processes, clipboard, "
               "network, or external services, use an observation tool BEFORE final. Never "
               "guess that a resource exists. After a tool failure, use another tool or "
               "explain that verification failed. Do not echo the goal. Prefer list-directory, "
               "glob, read-file, and grep over shell for filesystem questions. Use only "
               "listed tools and valid JSON arguments. Use grounding=context for knowledge, "
               "verified only after successful relevant observations and with claims matching verified facts, unable_to_verify after a "
               "failed observation. Answer in the user's language.\n"
               "REQUIRED EVIDENCE: %5\nENVIRONMENT: %4\nTOOLS:\n%1\nGOAL: %2\n"
               "CURRENT TURN OBSERVATIONS:\n%3\nVERIFIED FACTS:\n%6\n"
               "NEXT JSON ACTION:")
        .arg(toolLines.join(QLatin1Char('\n')), goal,
             historyLines.isEmpty() ? QStringLiteral("(none yet)")
                                    : historyLines.join(QLatin1Char('\n')),
             environmentBlock,
             [&] {
                 QStringList requirements;
                 for (const auto& item : activeIntent_.requirements)
                     requirements.append((item.claimId.isEmpty() ? QString{} : item.claimId + QLatin1Char(' ')) +
                                         observationDomainName(item.domain) +
                                         (item.resourceHint.isEmpty()
                                              ? QString{}
                                              : QStringLiteral(" %1").arg(item.resourceHint)));
                 return activeIntent_.indeterminate
                            ? QStringLiteral("Unclear; obtain relevant current evidence before "
                                             "claiming live state")
                            : requirements.isEmpty() ? QStringLiteral("none")
                                                     : requirements.join(QStringLiteral(", "));
             }(),
             [&] {
                 QStringList facts;
                 for (const auto& fact : structuredFacts_)
                     facts.append(QStringLiteral("%1 %2 = %3 [evidence %4]")
                                      .arg(fact.id, fact.resource,
                                           fact.value ? QStringLiteral("true") : QStringLiteral("false"),
                                           fact.evidenceCallIds.join(QLatin1Char(','))));
                 return facts.isEmpty() ? QStringLiteral("(none)") : facts.join(QLatin1Char('\n'));
             }());
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
    Q_UNUSED(request)
    return {false, QStringLiteral("Agent Mode execution requires AgentRuntime."), status()};
}

} // namespace sentinel::core
