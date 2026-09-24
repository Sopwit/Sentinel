// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/AgentLoop.h"

#include "sentinel/core/runtime/IToolExecutor.h"
#include "sentinel/core/runtime/IToolRegistry.h"
#include "sentinel/core/security/ExternalDirectoryGate.h"
#include "sentinel/core/security/IApprovalPolicy.h"
#include "sentinel/core/security/ISandboxPolicy.h"
#include <QDir>

#include <QTimer>
#include <QUuid>
#include <optional>
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

QStringList AgentLoop::externalPathsRequiringApproval(const ToolInvocationPlan& plan) const {
    if (!externalDirectoryGate_ || plan.invocations.isEmpty())
        return {};
    const auto& invocation = plan.invocations.first();
    static const QSet<QString> filesystem{
        QStringLiteral("list-directory"), QStringLiteral("read-file"),
        QStringLiteral("write-file"),     QStringLiteral("edit-file"),
        QStringLiteral("delete-file"),    QStringLiteral("move-file"),
        QStringLiteral("apply-patch"),    QStringLiteral("open-workspace"),
        QStringLiteral("glob"),           QStringLiteral("grep")};
    if (!filesystem.contains(invocation.toolId))
        return {};
    const bool write = invocation.toolId == QLatin1String("write-file") ||
                       invocation.toolId == QLatin1String("edit-file") ||
                       invocation.toolId == QLatin1String("delete-file") ||
                       invocation.toolId == QLatin1String("move-file") ||
                       invocation.toolId == QLatin1String("apply-patch");
    QStringList paths;
    for (const auto& argument : invocation.arguments) {
        if (argument.id != QLatin1String("path") && argument.id != QLatin1String("source") &&
            argument.id != QLatin1String("destination"))
            continue;
        const QString raw = argument.value.trimmed();
        if (raw.isEmpty())
            continue;
        if (externalDirectoryGate_->canRequestPermission(raw, QDir::currentPath()) &&
            !externalDirectoryGate_->isAccessAllowed(raw, QDir::currentPath(), write))
            paths.append(raw);
    }
    return paths;
}

void AgentLoop::grantExternalPaths(const ToolInvocationPlan& plan) {
    if (!externalDirectoryGate_)
        return;
    const auto& tool = plan.invocations.first().toolId;
    const bool write = tool == QLatin1String("write-file") || tool == QLatin1String("edit-file") ||
                       tool == QLatin1String("delete-file") || tool == QLatin1String("move-file") ||
                       tool == QLatin1String("apply-patch");
    for (const auto& path : externalPathsRequiringApproval(plan))
        externalDirectoryGate_->grantPermission(path, write);
}

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

void AgentLoop::setToolCallback(ToolCallback callback) {
    toolCallback_ = std::move(callback);
}

void AgentLoop::setPlanningCallback(PlanningCallback callback) {
    planningCallback_ = std::move(callback);
}

void AgentLoop::setOutputCallback(OutputCallback callback) {
    outputCallback_ = std::move(callback);
}

void AgentLoop::setToolCallIdProvider(ToolCallIdProvider provider) {
    toolCallIdProvider_ = std::move(provider);
}

void AgentLoop::runAsync(const QString& goal, const QString& sessionId, QObject* context,
                         CompletionCallback completion) {
    asyncContext_ = context;
    completionCallback_ = std::move(completion);
    asyncState_ = {};
    asyncState_.sessionId = sessionId;
    asyncState_.goal = goal;
    asyncState_.phase = AgentLoopPhase::Running;
    initializeObservationIntent(asyncState_);
    asyncFinished_ = false;
    if (statusCallback_)
        statusCallback_(QStringLiteral("Agent loop running for goal: %1").arg(goal));
    scheduleAsyncAdvance();
}

void AgentLoop::resumeAsync(AgentLoopState state, bool approved, QObject* context,
                            CompletionCallback completion) {
    asyncContext_ = context;
    completionCallback_ = std::move(completion);
    asyncState_ = std::move(state);
    planner_.setObservationIntent(asyncState_.observationIntent);
    asyncState_.phase = AgentLoopPhase::Running;
    asyncFinished_ = false;
    if (statusCallback_)
        statusCallback_(QStringLiteral("Agent loop running for goal: %1").arg(asyncState_.goal));
    const auto plan = asyncState_.pendingApprovalPlan;
    const auto thought = asyncState_.pendingApprovalThought;
    asyncState_.pendingApprovalPlan = {};
    asyncState_.pendingApprovalThought.clear();
    if (approved) {
        grantExternalPaths(plan);
        executeStepAsync(
            plan, thought,
            {ApprovalStatus::Approved, QStringLiteral("User approved execution in chat."), {}});
    } else {
        appendBlockedStep(asyncState_, plan, thought, QStringLiteral("Denied"),
                          QStringLiteral("User denied execution in chat."));
        scheduleAsyncAdvance();
    }
}

void AgentLoop::cancelAsync() {
    cancelled_ = true;
    if (asyncFinished_)
        return;
    if (waitingForTool_ && cancelTool_) {
        cancelTool_();
        asyncState_.phase = AgentLoopPhase::Cancelled;
        asyncState_.abortReason = QStringLiteral("Agent run cancelled by user.");
        completeAsync();
        return;
    }
    asyncState_.phase = AgentLoopPhase::Cancelled;
    asyncState_.abortReason = QStringLiteral("Agent run cancelled by user.");
    completeAsync();
}

void AgentLoop::scheduleAsyncAdvance() {
    QTimer::singleShot(0, asyncContext_, [this] {
        if (!asyncFinished_)
            advanceAsync();
    });
}

void AgentLoop::completeAsync() {
    if (asyncFinished_)
        return;
    asyncFinished_ = true;
    waitingForTool_ = false;
    cancelTool_ = {};
    if (completionCallback_)
        completionCallback_(asyncState_);
}

void AgentLoop::advanceAsync() {
    if (cancellationRequested()) {
        asyncState_.phase = AgentLoopPhase::Cancelled;
        asyncState_.abortReason = QStringLiteral("Agent run cancelled by user.");
        completeAsync();
        return;
    }
    if (asyncState_.steps.size() >= config_.maxIterations) {
        asyncState_.phase = AgentLoopPhase::Failed;
        asyncState_.abortReason =
            QStringLiteral("Iteration limit reached (%1 steps).").arg(config_.maxIterations);
        completeAsync();
        return;
    }
    const int stepIndex = asyncState_.steps.size() + 1;
    if (planningCallback_)
        planningCallback_(stepIndex, true);
    AgentStepDecision decision;
    try {
        planner_.setObservationIntent(asyncState_.observationIntent);
        decision = planner_.nextStep(asyncState_.goal, asyncState_.steps);
    } catch (...) {
        if (planningCallback_)
            planningCallback_(stepIndex, false);
        asyncState_.phase = AgentLoopPhase::Failed;
        asyncState_.abortReason = QStringLiteral("Agent execution failed.");
        completeAsync();
        return;
    }
    if (planningCallback_)
        planningCallback_(stepIndex, false);
    if (cancellationRequested()) {
        asyncState_.phase = AgentLoopPhase::Cancelled;
        asyncState_.abortReason = QStringLiteral("Agent run cancelled by user.");
        completeAsync();
        return;
    }
    if (decision.kind == AgentStepDecision::Kind::FinalAnswer) {
        if (acceptFinalAnswer(asyncState_, decision))
            completeAsync();
        else
            scheduleAsyncAdvance();
        return;
    }
    if (decision.kind == AgentStepDecision::Kind::GiveUp) {
        asyncState_.phase = AgentLoopPhase::Failed;
        asyncState_.abortReason =
            decision.reason.trimmed().isEmpty() ? decision.thought : decision.reason;
        completeAsync();
        return;
    }
    auto plan = planFromDecision(decision);
    if (!knownToolIds_.contains(decision.toolId)) {
        appendBlockedStep(asyncState_, plan, decision.thought, QStringLiteral("Unknown Tool"),
                          QStringLiteral("Unknown tool requested: %1").arg(decision.toolId));
        scheduleAsyncAdvance();
        return;
    }
    doomDetector_.recordAction(asyncState_.sessionId, decisionActionKey(decision));
    if (doomDetector_.isStuck(asyncState_.sessionId)) {
        asyncState_.phase = AgentLoopPhase::Stuck;
        asyncState_.abortReason = QStringLiteral(
            "Doom loop detected: the agent repeated the same action without progress.");
        completeAsync();
        return;
    }
    if (toolCallback_)
        toolCallback_(ToolTransition::Requested, stepIndex, plan, nullptr);
    const auto validation = gateway_.validatePlan(plan);
    if (validation.status != ToolExecutionStatus::Succeeded) {
        appendBlockedStep(asyncState_, plan, decision.thought,
                          toolExecutionStatusName(validation.status), validation.summary);
        if (toolCallback_)
            toolCallback_(ToolTransition::ExecutionFinished, stepIndex, plan,
                          &asyncState_.steps.last());
        scheduleAsyncAdvance();
        return;
    }
    ApprovalDecision approval;
    if (config_.autonomousMode)
        approval = {ApprovalStatus::Approved,
                    QStringLiteral("Autonomous Mode is enabled: per-step approval is bypassed."),
                    {}};
    else {
        approval = approvalPolicy_.evaluate(plan);
        if (approval.status == ApprovalStatus::RequiresApproval &&
            config_.sessionApprovedToolIds.contains(decision.toolId))
            approval = {
                ApprovalStatus::Approved,
                QStringLiteral(
                    "Session-level approval: the user already allowed this tool for this session."),
                {}};
    }
    const auto externalPaths = externalPathsRequiringApproval(plan);
    if (!externalPaths.isEmpty()) {
        approval.status = ApprovalStatus::RequiresApproval;
        approval.summary = QStringLiteral("Allow %1 to access %2?")
                               .arg(decision.toolId, externalPaths.join(QStringLiteral(", ")));
    }
    if (approval.status == ApprovalStatus::RequiresApproval) {
        if (toolCallback_)
            toolCallback_(ToolTransition::ApprovalRequired, stepIndex, plan, nullptr);
        asyncState_.pendingApprovalPlan = plan;
        asyncState_.pendingApprovalThought = decision.thought;
        asyncState_.phase = AgentLoopPhase::AwaitingApproval;
        completeAsync();
        return;
    }
    if (approval.status == ApprovalStatus::Denied) {
        appendBlockedStep(asyncState_, plan, decision.thought, QStringLiteral("Denied"),
                          QStringLiteral("Denied by approval policy: %1").arg(approval.summary));
        scheduleAsyncAdvance();
        return;
    }
    executeStepAsync(plan, decision.thought, approval);
}

void AgentLoop::executeStepAsync(const ToolInvocationPlan& plan, const QString& thought,
                                 ApprovalDecision approval) {
    const int index = asyncState_.steps.size() + 1;
    const auto descriptor = toolRegistry_ && !plan.invocations.isEmpty()
                                ? toolRegistry_->findToolById(plan.invocations.first().toolId)
                                : std::optional<ToolDescriptor>{};
    const auto sandbox = sandboxPolicy_.evaluate(plan, approval);
    if (toolCallback_)
        toolCallback_(ToolTransition::ExecutionStarted, index, plan, nullptr);
    waitingForTool_ = true;
    cancelTool_ = gateway_.executeAsync(
        {plan, approval, sandbox, knownToolIds_}, executor_, asyncState_.sessionId,
        toolCallIdProvider_ ? toolCallIdProvider_(index) : QString::number(index),
        [this, index](const QString& processId, ProcessStream stream, const QByteArray& bytes) {
            if (!asyncFinished_ && !cancellationRequested() && outputCallback_)
                outputCallback_(index, processId, stream, bytes);
        },
        [this, index, plan, thought, descriptor](ToolExecutionResult result) {
            if (asyncFinished_)
                return;
            waitingForTool_ = false;
            cancelTool_ = {};
            if (cancellationRequested()) {
                asyncState_.phase = AgentLoopPhase::Cancelled;
                asyncState_.abortReason = QStringLiteral("Agent run cancelled by user.");
                completeAsync();
                return;
            }
            AgentStepRecord record;
            record.index = index;
            record.thought = thought;
            fillRecordFromPlan(record, plan);
            record.succeeded = result.status == ToolExecutionStatus::Succeeded ||
                               result.status == ToolExecutionStatus::PlaceholderSucceeded;
            record.statusText = toolExecutionStatusName(result.status);
            record.observation =
                truncator_.truncate(result.summary.toUtf8(), record.toolId).preview;
            asyncState_.steps.append(record);
            if (descriptor)
                recordEvidence(asyncState_, *descriptor, plan, result.status, result.summary,
                               index);
            if (toolCallback_)
                toolCallback_(ToolTransition::ExecutionFinished, index, plan, &record);
            if (stepCallback_)
                stepCallback_(record);
            if (statusCallback_)
                statusCallback_(QStringLiteral("Agent loop step %1 finished: %2 (%3)")
                                    .arg(index)
                                    .arg(record.toolName, record.statusText));
            scheduleAsyncAdvance();
        });
    if (!waitingForTool_)
        cancelTool_ = {};
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
    initializeObservationIntent(state);
    return advance(std::move(state));
}

AgentLoopState AgentLoop::resume(AgentLoopState state, bool approved) {
    planner_.setObservationIntent(state.observationIntent);
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

    grantExternalPaths(pendingPlan);
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

        const int stepIndex = static_cast<int>(state.steps.size()) + 1;
        if (planningCallback_) {
            planningCallback_(stepIndex, true);
        }
        AgentStepDecision decision;
        try {
            planner_.setObservationIntent(state.observationIntent);
            decision = planner_.nextStep(state.goal, state.steps);
        } catch (...) {
            if (planningCallback_)
                planningCallback_(stepIndex, false);
            throw;
        }
        if (planningCallback_) {
            planningCallback_(stepIndex, false);
        }
        if (cancellationRequested()) {
            state.phase = AgentLoopPhase::Cancelled;
            state.abortReason = QStringLiteral("Agent run cancelled by user.");
            return state;
        }

        if (decision.kind == AgentStepDecision::Kind::FinalAnswer) {
            if (acceptFinalAnswer(state, decision))
                return state;
            continue;
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

        auto plan = planFromDecision(decision);
        doomDetector_.recordAction(state.sessionId, decisionActionKey(decision));
        if (doomDetector_.isStuck(state.sessionId)) {
            state.phase = AgentLoopPhase::Stuck;
            state.abortReason = QStringLiteral(
                "Doom loop detected: the agent repeated the same action without progress.");
            return state;
        }
        if (toolCallback_) {
            toolCallback_(ToolTransition::Requested, stepIndex, plan, nullptr);
        }
        const auto validation = gateway_.validatePlan(plan);
        if (validation.status != ToolExecutionStatus::Succeeded) {
            appendBlockedStep(state, plan, decision.thought,
                              toolExecutionStatusName(validation.status), validation.summary);
            if (toolCallback_)
                toolCallback_(ToolTransition::ExecutionFinished, stepIndex, plan,
                              &state.steps.last());
            continue;
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

        const auto externalPaths = externalPathsRequiringApproval(plan);
        if (!externalPaths.isEmpty()) {
            approval.status = ApprovalStatus::RequiresApproval;
            approval.summary = QStringLiteral("Allow %1 to access %2?")
                                   .arg(decision.toolId, externalPaths.join(QStringLiteral(", ")));
        }
        if (approval.status == ApprovalStatus::RequiresApproval) {
            if (toolCallback_) {
                toolCallback_(ToolTransition::ApprovalRequired, stepIndex, plan, nullptr);
            }
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
    const int stepIndex = static_cast<int>(state.steps.size()) + 1;
    const auto descriptor = toolRegistry_ && !plan.invocations.isEmpty()
                                ? toolRegistry_->findToolById(plan.invocations.first().toolId)
                                : std::optional<ToolDescriptor>{};
    if (toolCallback_) {
        toolCallback_(ToolTransition::ExecutionStarted, stepIndex, plan, nullptr);
    }

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
    if (descriptor)
        recordEvidence(state, *descriptor, plan, result.status, result.summary, stepIndex);
    if (toolCallback_) {
        toolCallback_(ToolTransition::ExecutionFinished, stepIndex, plan, &record);
    }
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
    if (statusText == QLatin1String("Denied") && toolRegistry_ && !plan.invocations.isEmpty()) {
        const auto descriptor = toolRegistry_->findToolById(record.toolId);
        if (descriptor)
            recordEvidence(state, *descriptor, plan, ToolExecutionStatus::Blocked, observation,
                           record.index);
    }
    if (statusText == QLatin1String("Unknown Tool") &&
        (record.toolId.startsWith(QLatin1String("mcp.")) ||
         record.toolId.startsWith(QLatin1String("plugin.")))) {
        EvidenceRecord unavailable;
        unavailable.toolId = record.toolId;
        unavailable.toolCallId = toolCallIdProvider_
                                     ? toolCallIdProvider_(record.index)
                                     : QStringLiteral("%1:%2").arg(state.sessionId).arg(record.index);
        unavailable.stepIndex = record.index;
        unavailable.domain = ObservationDomain::ExternalService;
        unavailable.scope = EvidenceScope::Provider;
        unavailable.outcome = EvidenceOutcome::Unavailable;
        unavailable.observedAtUtc = QDateTime::currentDateTimeUtc();
        state.evidence.append(std::move(unavailable));
    }
    if (stepCallback_) {
        stepCallback_(record);
    }
}

void AgentLoop::initializeObservationIntent(AgentLoopState& state) {
    if (observationIntentPolicy_) {
        if (statusCallback_)
            statusCallback_(QStringLiteral("Determining required observations."));
        state.observationIntent = observationIntentPolicy_->classify(
            state.goal, observationContext_,
            toolRegistry_ ? toolRegistry_->enabledTools() : QList<ToolDescriptor>{});
    }
    planner_.setObservationIntent(state.observationIntent);
}

bool AgentLoop::acceptFinalAnswer(AgentLoopState& state, const AgentStepDecision& decision) {
    auto gate = EvidencePolicy::evaluate(state.observationIntent, state.evidence,
                                         decision.grounding, decision.groundingDeclared);
    if (gate.accepted && decision.observationRequirementDeclared &&
        decision.requiresObservation && state.evidence.isEmpty()) {
        gate.accepted = false;
        gate.repair = QStringLiteral("You declared that observation is needed. Use an "
                                     "appropriate tool before answering.");
    }
    if (gate.accepted) {
        state.finalGrounding = std::move(gate.grounding);
        state.finalAnswer = gate.answerOverride.isEmpty() ? decision.answer : gate.answerOverride;
        state.phase = AgentLoopPhase::Completed;
        return true;
    }
    ++state.rejectedFinalAnswers;
    if (state.rejectedFinalAnswers >= 2) {
        state.phase = AgentLoopPhase::Failed;
        state.abortReason = QStringLiteral("Agent could not determine a grounded next action.");
        return true;
    }
    planner_.setPlannerFeedback(gate.repair);
    return false;
}

void AgentLoop::recordEvidence(AgentLoopState& state, const ToolDescriptor& descriptor,
                               const ToolInvocationPlan& plan, ToolExecutionStatus status,
                               const QString& summary, int stepIndex) {
    if (plan.invocations.isEmpty())
        return;
    const auto callId = toolCallIdProvider_ ? toolCallIdProvider_(stepIndex)
                                           : QStringLiteral("%1:%2").arg(state.sessionId).arg(stepIndex);
    state.evidence.append(EvidencePolicy::record(descriptor, plan.invocations.first(), status,
                                                 summary, stepIndex, callId));
    while (state.evidence.size() > 48)
        state.evidence.removeFirst();
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
