// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/AgentLoop.h"

#include "sentinel/core/runtime/IToolExecutor.h"
#include "sentinel/core/security/IApprovalPolicy.h"
#include "sentinel/core/security/ISandboxPolicy.h"

#include <QUuid>
#include <utility>

namespace sentinel::core {

namespace {

QString decisionActionKey(const AgentStepDecision& decision) {
    QStringList argumentParts;
    for (const auto& argument : decision.arguments) {
        argumentParts.append(QStringLiteral("%1=%2").arg(argument.id, argument.value));
    }
    return decision.toolId + QLatin1Char('|') + argumentParts.join(QLatin1Char(';'));
}

void fillRecordFromPlan(AgentStepRecord& record, const ToolInvocationPlan& plan) {
    if (plan.invocations.isEmpty()) {
        return;
    }
    const auto& invocation = plan.invocations.first();
    record.toolId = invocation.toolId;
    record.toolName = invocation.toolName;
    record.arguments = invocation.arguments;
}

} // namespace

AgentLoop::AgentLoop(IAgentStepPlanner& planner, const IToolExecutor& executor,
                     const IApprovalPolicy& approvalPolicy, const ISandboxPolicy& sandboxPolicy,
                     QStringList knownToolIds, Config config)
    : planner_(planner), executor_(executor), approvalPolicy_(approvalPolicy),
      sandboxPolicy_(sandboxPolicy), knownToolIds_(std::move(knownToolIds)), config_(config) {
    TruncationConfig truncationConfig;
    truncationConfig.enabled = true;
    truncationConfig.maxLines = 400;
    truncationConfig.maxBytes = config_.observationMaxBytes;
    truncationConfig.previewLines = config_.observationPreviewLines;
    truncator_ = ToolOutputTruncator(truncationConfig);
}

void AgentLoop::setStepCallback(StepCallback callback) {
    stepCallback_ = std::move(callback);
}

void AgentLoop::setStatusCallback(StatusCallback callback) {
    statusCallback_ = std::move(callback);
}

void AgentLoop::setCancelQuery(CancelQuery query) {
    cancelQuery_ = std::move(query);
}

bool AgentLoop::cancellationRequested() const {
    return cancelled_.load() || (cancelQuery_ && cancelQuery_());
}

AgentLoopState AgentLoop::run(const QString& goal, const QString& sessionId) {
    AgentLoopState state;
    state.sessionId = sessionId.trimmed().isEmpty()
                          ? QUuid::createUuid().toString(QUuid::WithoutBraces)
                          : sessionId;
    state.goal = goal;
    state.phase = AgentLoopPhase::Running;
    return advance(std::move(state));
}

AgentLoopState AgentLoop::resume(AgentLoopState state, bool approved) {
    state.phase = AgentLoopPhase::Running;

    auto pendingPlan = state.pendingApprovalPlan;
    const QString thought = state.pendingApprovalThought;
    state.pendingApprovalPlan = ToolInvocationPlan{};
    state.pendingApprovalThought.clear();

    if (!approved) {
        appendBlockedStep(state, pendingPlan, thought, QStringLiteral("Denied"),
                          QStringLiteral("User denied execution in chat."));
        return advance(std::move(state));
    }

    executeStep(state, pendingPlan, thought,
                ApprovalDecision{
                    ApprovalStatus::Approved,
                    QStringLiteral("User approved execution in chat."),
                    {},
                });
    return advance(std::move(state));
}

AgentLoopState AgentLoop::advance(AgentLoopState state) {
    if (statusCallback_) {
        statusCallback_(QStringLiteral("Agent loop running for goal: %1").arg(state.goal));
    }

    while (true) {
        if (cancellationRequested()) {
            state.phase = AgentLoopPhase::Cancelled;
            state.abortReason = QStringLiteral("Agent run cancelled by user.");
            return state;
        }

        if (static_cast<int>(state.steps.size()) >= config_.maxIterations) {
            state.phase = AgentLoopPhase::Failed;
            state.abortReason =
                QStringLiteral("Iteration limit reached (%1 steps).").arg(config_.maxIterations);
            return state;
        }

        const auto decision = planner_.nextStep(state.goal, state.steps);

        if (decision.kind == AgentStepDecision::Kind::FinalAnswer) {
            state.finalAnswer =
                decision.answer.trimmed().isEmpty() ? decision.thought : decision.answer;
            state.phase = AgentLoopPhase::Completed;
            return state;
        }

        if (decision.kind == AgentStepDecision::Kind::GiveUp) {
            state.phase = AgentLoopPhase::Failed;
            state.abortReason =
                decision.reason.trimmed().isEmpty() ? decision.thought : decision.reason;
            return state;
        }

        if (!knownToolIds_.contains(decision.toolId)) {
            appendBlockedStep(state, planFromDecision(decision), decision.thought,
                              QStringLiteral("Unknown Tool"),
                              QStringLiteral("Unknown tool requested: %1").arg(decision.toolId));
            continue;
        }

        const auto plan = planFromDecision(decision);

        doomDetector_.recordAction(state.sessionId, decisionActionKey(decision));
        if (doomDetector_.isStuck(state.sessionId)) {
            state.phase = AgentLoopPhase::Stuck;
            state.abortReason = QStringLiteral(
                "Doom loop detected: the agent repeated the same action without progress.");
            return state;
        }

        ApprovalDecision approval;
        if (config_.autonomousMode) {
            approval = ApprovalDecision{
                ApprovalStatus::Approved,
                QStringLiteral("Autonomous Mode is enabled: per-step approval is bypassed."),
                {},
            };
        } else {
            approval = approvalPolicy_.evaluate(plan);
            if (approval.status == ApprovalStatus::RequiresApproval &&
                config_.sessionApprovedToolIds.contains(decision.toolId)) {
                approval = ApprovalDecision{
                    ApprovalStatus::Approved,
                    QStringLiteral(
                        "Session-level approval: the user already allowed this tool for this "
                        "session."),
                    {},
                };
            }
        }

        if (approval.status == ApprovalStatus::RequiresApproval) {
            state.pendingApprovalPlan = plan;
            state.pendingApprovalThought = decision.thought;
            state.phase = AgentLoopPhase::AwaitingApproval;
            if (statusCallback_) {
                statusCallback_(QStringLiteral("Agent loop paused: approval required for %1.")
                                    .arg(decision.toolId));
            }
            return state;
        }

        if (approval.status == ApprovalStatus::Denied) {
            appendBlockedStep(
                state, plan, decision.thought, QStringLiteral("Denied"),
                QStringLiteral("Denied by approval policy: %1").arg(approval.summary));
            continue;
        }

        executeStep(state, plan, decision.thought, approval);
    }
}

void AgentLoop::executeStep(AgentLoopState& state, const ToolInvocationPlan& plan,
                            const QString& thought, ApprovalDecision approval) {
    const auto sandbox = sandboxPolicy_.evaluate(plan, approval);

    const auto result = gateway_.execute(
        ToolExecutionRequest{
            plan,
            approval,
            sandbox,
            knownToolIds_,
        },
        executor_);

    AgentStepRecord record;
    record.index = static_cast<int>(state.steps.size()) + 1;
    record.thought = thought;
    fillRecordFromPlan(record, plan);
    record.succeeded = result.status == ToolExecutionStatus::Succeeded ||
                       result.status == ToolExecutionStatus::PlaceholderSucceeded;
    record.statusText = toolExecutionStatusName(result.status);
    record.observation = truncator_.truncate(result.summary.toUtf8(), record.toolId).preview;

    state.steps.append(record);
    if (stepCallback_) {
        stepCallback_(record);
    }
    if (statusCallback_) {
        statusCallback_(
            QStringLiteral("Agent loop step %1 finished: %2 (%3)")
                .arg(QString::number(record.index), record.toolName, record.statusText));
    }
}

void AgentLoop::appendBlockedStep(AgentLoopState& state, const ToolInvocationPlan& plan,
                                  const QString& thought, const QString& statusText,
                                  const QString& observation) {
    AgentStepRecord record;
    record.index = static_cast<int>(state.steps.size()) + 1;
    record.thought = thought;
    fillRecordFromPlan(record, plan);
    record.succeeded = false;
    record.statusText = statusText;
    record.observation = observation;

    state.steps.append(record);
    if (stepCallback_) {
        stepCallback_(record);
    }
}

ToolInvocationPlan AgentLoop::planFromDecision(const AgentStepDecision& decision) const {
    ToolInvocationPlan plan;
    plan.status = ToolInvocationPlanStatus::Planned;
    plan.summary = QStringLiteral("Agent loop step: %1").arg(decision.toolId);
    plan.invocations.append(PlannedToolInvocation{
        decision.toolId,
        decision.toolName.isEmpty() ? decision.toolId : decision.toolName,
        QStringLiteral("Agent loop invocation for %1").arg(decision.toolId),
        decision.thought,
        decision.riskLevel,
        decision.executionMode,
        decision.arguments,
        {},
    });
    return plan;
}

} // namespace sentinel::core
