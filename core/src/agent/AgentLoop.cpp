// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/AgentLoop.h"
#include "sentinel/core/agent/ClaimGroundingResolver.h"
#include "sentinel/core/chat/IChatHistoryStore.h"
#include "sentinel/core/interfaces/IMemoryStore.h"

#include "sentinel/core/runtime/IToolExecutor.h"
#include "sentinel/core/runtime/IToolRegistry.h"
#include "sentinel/core/security/AuthorizationResolver.h"
#include "sentinel/core/security/ExternalDirectoryGate.h"
#include "sentinel/core/security/IApprovalPolicy.h"
#include "sentinel/core/security/ISandboxPolicy.h"
#include "sentinel/core/security/PermissionService.h"
#include <QDir>
#include <QFileInfo>

#include <QTimer>
#include <QUuid>
#include <optional>
#include <algorithm>
#include <utility>

namespace sentinel::core {

void AgentLoop::preparePlanningContext(const AgentLoopState& state) {
    AgentContextInput input;
    input.goal = state.goal;
    input.workspace = QDir::currentPath();
    input.steps = state.steps;
    input.evidence = state.evidence;
    input.intent = state.observationIntent;
    input.facts = ClaimGroundingResolver::facts(state.observationIntent, state.evidence);
    input.contextWindowTokens = contextWindowTokens_;
    input.maxOutputTokens = maxOutputTokens_;
    input.memoryStore = memoryStore_;
    input.chatHistoryStore = chatHistoryStore_;
    if (toolRegistry_) {
        for (const auto& tool : toolRegistry_->enabledTools())
            if (knownToolIds_.contains(tool.id))
                input.tools.append(tool);
    }
    const auto context = contextEngine_.build(input);
    if (contextCallback_)
        contextCallback_(static_cast<int>(state.steps.size()) + 1, context);
    planner_.setPlanningContext(context);
}

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

StructuredObservationPtr resourceFailureObservation(const ResourceAuthorizationResult& result) {
    auto observation = std::make_shared<StructuredObservation>();
    observation->kind = StructuredObservationKind::FileSystemFailure;
    observation->fileSystemFailure = result.failure;
    observation->failureResource = result.resource;
    observation->data.insert(QStringLiteral("resource"), result.resource);
    return observation;
}

} // namespace

QStringList AgentLoop::externalPathsRequiringApproval(const ToolInvocationPlan& plan,
                                                     const QString& sessionId) const {
    if (!externalDirectoryGate_ || plan.invocations.isEmpty())
        return {};
    QStringList paths;
    for (const auto& invocation : plan.invocations) {
        const auto registration = toolRegistry_
                                     ? toolRegistry_->findRegistration(invocation.toolId)
                                     : std::optional<IToolRegistry::Registration>{};
        const auto* descriptor = invocation.descriptorSnapshot
                                     ? invocation.descriptorSnapshot.get()
                                     : (registration ? &registration->descriptor : nullptr);
        if (!descriptor)
            continue;
        for (const auto& request : AuthorizationResolver::resolve(
                 *descriptor, invocation, externalDirectoryGate_, QDir::currentPath())) {
            if (request.domain != SecurityDomain::FileSystem || request.resource.isEmpty())
                continue;
            if (permissionService_ && permissionService_->evaluateAuthorization(
                                          request, sessionId) == PermissionEffect::Allow)
                continue;
            if (externalDirectoryGate_->canRequestPermission(request.resource, QDir::currentPath()))
                paths.append(request.resource);
        }
    }
    paths.removeDuplicates();
    return paths;
}

void AgentLoop::grantExternalPaths(const ToolInvocationPlan& plan, const QString& sessionId) {
    for (const auto& invocation : plan.invocations) {
        const auto registration = toolRegistry_
                                     ? toolRegistry_->findRegistration(invocation.toolId)
                                     : std::optional<IToolRegistry::Registration>{};
        const auto* descriptor = invocation.descriptorSnapshot
                                     ? invocation.descriptorSnapshot.get()
                                     : (registration ? &registration->descriptor : nullptr);
        if (!descriptor)
            continue;
        for (const auto& request : AuthorizationResolver::resolve(
                 *descriptor, invocation, externalDirectoryGate_, QDir::currentPath())) {
            if (permissionService_ && externalDirectoryGate_ &&
                request.domain == SecurityDomain::FileSystem && !request.resource.isEmpty() &&
                externalDirectoryGate_->canRequestPermission(request.resource, QDir::currentPath()))
                permissionService_->grantAuthorization(request, sessionId, false);
        }
    }
}

bool AgentLoop::hasAuthorizationGrants(const ToolInvocationPlan& plan,
                                       const QString& sessionId) const {
    if (!permissionService_ || !toolRegistry_ || plan.invocations.isEmpty())
        return false;
    bool found = false;
    for (const auto& invocation : plan.invocations) {
        const auto registration = toolRegistry_->findRegistration(invocation.toolId);
        const auto* descriptor = invocation.descriptorSnapshot
                                     ? invocation.descriptorSnapshot.get()
                                     : (registration ? &registration->descriptor : nullptr);
        if (!descriptor)
            return false;
        const auto requests = AuthorizationResolver::resolve(
            *descriptor, invocation, externalDirectoryGate_, QDir::currentPath());
        for (const auto& request : requests) {
            found = true;
            if (permissionService_->evaluateAuthorization(request, sessionId) !=
                PermissionEffect::Allow)
                return false;
        }
    }
    return found;
}

bool AgentLoop::hasAuthorizationDeny(const ToolInvocationPlan& plan,
                                     const QString& sessionId) const {
    if (!permissionService_ || !toolRegistry_)
        return false;
    for (const auto& invocation : plan.invocations) {
        const auto registration = toolRegistry_->findRegistration(invocation.toolId);
        const auto* descriptor = invocation.descriptorSnapshot
                                     ? invocation.descriptorSnapshot.get()
                                     : (registration ? &registration->descriptor : nullptr);
        if (!descriptor)
            return true;
        for (const auto& request : AuthorizationResolver::resolve(
                 *descriptor, invocation, externalDirectoryGate_, QDir::currentPath())) {
            if (permissionService_->evaluateAuthorization(request, sessionId) ==
                PermissionEffect::Deny)
                return true;
        }
    }
    return false;
}

QList<AuthorizationRequest>
AgentLoop::resolveAuthorizationRequests(const ToolInvocationPlan& plan) const {
    QList<AuthorizationRequest> requests;
    for (const auto& invocation : plan.invocations) {
        const auto registration = toolRegistry_
                                     ? toolRegistry_->findRegistration(invocation.toolId)
                                     : std::optional<IToolRegistry::Registration>{};
        const auto* descriptor = invocation.descriptorSnapshot
                                     ? invocation.descriptorSnapshot.get()
                                     : (registration ? &registration->descriptor : nullptr);
        if (descriptor)
            requests.append(AuthorizationResolver::resolve(
                *descriptor, invocation, externalDirectoryGate_, QDir::currentPath()));
    }
    return requests;
}

ResourceAuthorizationResult AgentLoop::prepareResources(ToolInvocationPlan& plan) const {
    for (auto& invocation : plan.invocations) {
        if (!invocation.descriptorSnapshot)
            continue;
        auto resolved = ResourceAuthorizationResolver::resolve(
            *invocation.descriptorSnapshot, invocation, QDir::currentPath(),
            externalDirectoryGate_);
        if (!resolved.ok()) return resolved;
        invocation.resourceSnapshot =
            std::make_shared<const ResourceAuthorizationSnapshot>(std::move(resolved.snapshot));
    }
    return {};
}

ResourceAuthorizationResult AgentLoop::authorizeResources(ToolInvocationPlan& plan,
                                                          const QString& sessionId) const {
    for (auto& invocation : plan.invocations) {
        if (!invocation.resourceSnapshot)
            continue;
        auto result = ResourceAuthorizationResolver::authorize(*invocation.resourceSnapshot,
                                                               externalDirectoryGate_,
                                                               permissionService_, sessionId);
        if (!result.ok()) return result;
        invocation.resourceSnapshot =
            std::make_shared<const ResourceAuthorizationSnapshot>(std::move(result.snapshot));
    }
    return {};
}

void AgentLoop::applyPermissionPolicy(const ToolInvocationPlan& plan,
                                      ApprovalDecision& approval) const {
    if (!permissionPolicy_ || !toolRegistry_)
        return;
    for (const auto& invocation : plan.invocations) {
        const auto registration = toolRegistry_->findRegistration(invocation.toolId);
        const auto* descriptor = invocation.descriptorSnapshot
                                     ? invocation.descriptorSnapshot.get()
                                     : (registration ? &registration->descriptor : nullptr);
        if (!descriptor) {
            approval.status = ApprovalStatus::Denied;
            approval.summary = QStringLiteral("Tool authorization metadata is unavailable.");
            return;
        }
        const auto requests = AuthorizationResolver::resolve(
            *descriptor, invocation, externalDirectoryGate_, QDir::currentPath());
        for (const auto& request : requests) {
            const auto effect = permissionPolicy_->defaultEffect(request, defaultPermissionState_);
            if (effect == PermissionEffect::Deny) {
                approval.status = ApprovalStatus::Denied;
                approval.summary = QStringLiteral("Denied by the configured authorization policy.");
                return;
            }
            if (effect == PermissionEffect::Ask && approval.status != ApprovalStatus::Approved) {
                approval.status = ApprovalStatus::RequiresApproval;
                approval.summary = QStringLiteral("External service access requires user approval.");
            }
        }
    }
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

void AgentLoop::setCancellationToken(std::shared_ptr<std::atomic_bool> token) {
    cancellationToken_ = std::move(token);
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
    asyncState_.pendingAuthorizationRequests.clear();
    if (approved) {
        grantExternalPaths(plan, asyncState_.sessionId);
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
    if (cancellationToken_) cancellationToken_->store(true);
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
        planner_.setStructuredFacts(ClaimGroundingResolver::facts(asyncState_.observationIntent, asyncState_.evidence));
        preparePlanningContext(asyncState_);
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
    const auto resources = prepareResources(plan);
    if (!resources.ok()) {
        appendBlockedStep(asyncState_, plan, decision.thought,
                          resources.failure == FileSystemFailure::PermissionDenied
                              ? QStringLiteral("Denied") : QStringLiteral("Invalid Arguments"),
                          resources.reason, resourceFailureObservation(resources));
        if (toolCallback_)
            toolCallback_(ToolTransition::ExecutionFinished, stepIndex, plan,
                          &asyncState_.steps.last());
        scheduleAsyncAdvance();
        return;
    }
    if (externalDirectoryGate_)
        externalDirectoryGate_->setSecuritySessionId(asyncState_.sessionId);
    ApprovalDecision approval;
    if (hasAuthorizationDeny(plan, asyncState_.sessionId))
        approval = {ApprovalStatus::Denied,
                    QStringLiteral("Denied by an explicit permission grant."), {}};
    else if (config_.autonomousMode)
        approval = {ApprovalStatus::Approved,
                    QStringLiteral("Autonomous Mode is enabled: per-step approval is bypassed."),
                    {}};
    else {
        approval = hasAuthorizationGrants(plan, asyncState_.sessionId)
                       ? ApprovalDecision{ApprovalStatus::Approved,
                                          QStringLiteral("An explicit semantic permission grant "
                                                         "covers this invocation."), {}}
                       : approvalPolicy_.evaluate(plan);
    }
    const auto externalPaths = externalPathsRequiringApproval(plan, asyncState_.sessionId);
    applyPermissionPolicy(plan, approval);
    if (!externalPaths.isEmpty() && approval.status != ApprovalStatus::Denied) {
        const auto toolName = plan.invocations.isEmpty() ? QStringLiteral("tool")
                                                         : plan.invocations.first().toolName;
        approval.status = ApprovalStatus::RequiresApproval;
        const auto displayed = externalPaths.mid(0, 3).join(QStringLiteral(", "));
        approval.summary = QStringLiteral("Allow %1 to access %2%3?")
                               .arg(toolName, displayed,
                                    externalPaths.size() > 3
                                        ? QStringLiteral(" and %1 more file(s)")
                                              .arg(externalPaths.size() - 3)
                                        : QString{});
    }
    if (approval.status == ApprovalStatus::RequiresApproval) {
        if (toolCallback_)
            toolCallback_(ToolTransition::ApprovalRequired, stepIndex, plan, nullptr);
        asyncState_.pendingApprovalPlan = plan;
        asyncState_.pendingApprovalThought = decision.thought;
        asyncState_.pendingAuthorizationRequests = resolveAuthorizationRequests(plan);
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

void AgentLoop::executeStepAsync(const ToolInvocationPlan& originalPlan, const QString& thought,
                                 ApprovalDecision approval) {
    ToolInvocationPlan plan = originalPlan;
    const int index = asyncState_.steps.size() + 1;
    const auto descriptor = toolRegistry_ && !plan.invocations.isEmpty()
                                ? toolRegistry_->findToolById(plan.invocations.first().toolId)
                                : std::optional<ToolDescriptor>{};
    if (externalDirectoryGate_)
        externalDirectoryGate_->setSecuritySessionId(asyncState_.sessionId);
    const auto authorization = authorizeResources(plan, asyncState_.sessionId);
    if (!authorization.ok()) {
        appendBlockedStep(asyncState_, plan, thought, QStringLiteral("Denied"),
                          authorization.reason, resourceFailureObservation(authorization));
        if (toolCallback_)
            toolCallback_(ToolTransition::ExecutionFinished, index, plan,
                          &asyncState_.steps.last());
        scheduleAsyncAdvance();
        return;
    }
    const auto sandbox = sandboxPolicy_.evaluate(plan, approval);
    if (toolCallback_)
        toolCallback_(ToolTransition::ExecutionStarted, index, plan, nullptr);
    waitingForTool_ = true;
    ToolInvocationPlan cancellablePlan = plan;
    for (auto& invocation : cancellablePlan.invocations)
        invocation.cancellation = cancellationToken_;
    cancelTool_ = gateway_.executeAsync(
        {cancellablePlan, approval, sandbox, knownToolIds_}, executor_, asyncState_.sessionId,
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
                AgentStepRecord partial;
                partial.index = index;
                partial.thought = thought;
                fillRecordFromPlan(partial, plan);
                partial.statusText = toolExecutionStatusName(ToolExecutionStatus::Cancelled);
                partial.observation = truncator_.truncate(result.summary.toUtf8(), partial.toolId).preview;
                partial.structuredObservation = result.structuredObservation;
                asyncState_.steps.append(partial);
                if (descriptor && result.structuredObservation)
                    recordEvidence(asyncState_, *descriptor, plan, ToolExecutionStatus::Cancelled,
                                   result.summary, index, result.structuredObservation, result.mutations);
                if (toolCallback_)
                    toolCallback_(ToolTransition::ExecutionFinished, index, plan, &partial);
                if (stepCallback_)
                    stepCallback_(partial);
                asyncState_.phase = AgentLoopPhase::Cancelled;
                asyncState_.abortReason = QStringLiteral("Agent run cancelled by user.");
                completeAsync();
                return;
            }
            if (result.status == ToolExecutionStatus::Cancelled) {
                asyncState_.phase = AgentLoopPhase::Cancelled;
                asyncState_.abortReason = QStringLiteral("Active tool cancelled.");
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
            record.structuredObservation = result.structuredObservation;
            asyncState_.steps.append(record);
            if (descriptor)
                recordEvidence(asyncState_, *descriptor, plan, result.status, result.summary,
                               index, result.structuredObservation, result.mutations);
            if (toolCallback_)
                toolCallback_(ToolTransition::ExecutionFinished, index, plan, &record);
            if (stepCallback_)
                stepCallback_(record);
            if (statusCallback_)
                statusCallback_(QStringLiteral("Agent loop step %1 finished: %2 (%3)")
                                    .arg(index)
                                    .arg(record.toolName, record.statusText));
            if (result.status == ToolExecutionStatus::Cancelled)
                completeAsync();
            else
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
    state.pendingAuthorizationRequests.clear();

    if (!approved) {
        appendBlockedStep(state, pendingPlan, thought, QStringLiteral("Denied"),
                          QStringLiteral("User denied execution in chat."));
        return advance(std::move(state));
    }

    grantExternalPaths(pendingPlan, state.sessionId);
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
            planner_.setStructuredFacts(ClaimGroundingResolver::facts(state.observationIntent, state.evidence));
            preparePlanningContext(state);
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
        const auto resources = prepareResources(plan);
        if (!resources.ok()) {
            appendBlockedStep(state, plan, decision.thought,
                              resources.failure == FileSystemFailure::PermissionDenied
                                  ? QStringLiteral("Denied") : QStringLiteral("Invalid Arguments"),
                              resources.reason, resourceFailureObservation(resources));
            if (toolCallback_)
                toolCallback_(ToolTransition::ExecutionFinished, stepIndex, plan,
                              &state.steps.last());
            continue;
        }
        if (externalDirectoryGate_)
            externalDirectoryGate_->setSecuritySessionId(state.sessionId);

        ApprovalDecision approval;
        if (hasAuthorizationDeny(plan, state.sessionId)) {
            approval = {ApprovalStatus::Denied,
                        QStringLiteral("Denied by an explicit permission grant."), {}};
        } else if (config_.autonomousMode) {
            approval = ApprovalDecision{
                ApprovalStatus::Approved,
                QStringLiteral("Autonomous Mode is enabled: per-step approval is bypassed."),
                {},
            };
        } else {
            approval = hasAuthorizationGrants(plan, state.sessionId)
                           ? ApprovalDecision{ApprovalStatus::Approved,
                                              QStringLiteral("An explicit semantic permission "
                                                             "grant covers this invocation."), {}}
                           : approvalPolicy_.evaluate(plan);
        }

        const auto externalPaths = externalPathsRequiringApproval(plan, state.sessionId);
        applyPermissionPolicy(plan, approval);
        if (!externalPaths.isEmpty() && approval.status != ApprovalStatus::Denied) {
            const auto toolName = plan.invocations.isEmpty() ? QStringLiteral("tool")
                                                             : plan.invocations.first().toolName;
            approval.status = ApprovalStatus::RequiresApproval;
            const auto displayed = externalPaths.mid(0, 3).join(QStringLiteral(", "));
            approval.summary = QStringLiteral("Allow %1 to access %2%3?")
                                   .arg(toolName, displayed,
                                        externalPaths.size() > 3
                                            ? QStringLiteral(" and %1 more file(s)")
                                                  .arg(externalPaths.size() - 3)
                                            : QString{});
        }
        if (approval.status == ApprovalStatus::RequiresApproval) {
            if (toolCallback_) {
                toolCallback_(ToolTransition::ApprovalRequired, stepIndex, plan, nullptr);
            }
            state.pendingApprovalPlan = plan;
            state.pendingApprovalThought = decision.thought;
            state.pendingAuthorizationRequests = resolveAuthorizationRequests(plan);
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

void AgentLoop::executeStep(AgentLoopState& state, const ToolInvocationPlan& originalPlan,
                            const QString& thought, ApprovalDecision approval) {
    ToolInvocationPlan plan = originalPlan;
    if (externalDirectoryGate_)
        externalDirectoryGate_->setSecuritySessionId(state.sessionId);
    const auto authorization = authorizeResources(plan, state.sessionId);
    if (!authorization.ok()) {
        appendBlockedStep(state, plan, thought, QStringLiteral("Denied"),
                          authorization.reason, resourceFailureObservation(authorization));
        if (toolCallback_)
            toolCallback_(ToolTransition::ExecutionFinished, state.steps.size(), plan,
                          &state.steps.last());
        return;
    }
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

    record.structuredObservation = result.structuredObservation;
    state.steps.append(record);
    if (descriptor)
        recordEvidence(state, *descriptor, plan, result.status, result.summary, stepIndex, result.structuredObservation, result.mutations);
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
                                  const QString& observation,
                                  StructuredObservationPtr structuredObservation) {
    AgentStepRecord record;
    record.index = static_cast<int>(state.steps.size()) + 1;
    record.thought = thought;
    fillRecordFromPlan(record, plan);
    record.succeeded = false;
    record.statusText = statusText;
    record.observation = observation;
    record.structuredObservation = std::move(structuredObservation);

    state.steps.append(record);
    if (statusText == QLatin1String("Denied") && toolRegistry_ && !plan.invocations.isEmpty()) {
        const auto descriptor = toolRegistry_->findToolById(record.toolId);
        if (descriptor)
            recordEvidence(state, *descriptor, plan, ToolExecutionStatus::Blocked, observation,
                           record.index, record.structuredObservation);
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
    planner_.setStructuredFacts(ClaimGroundingResolver::facts(state.observationIntent, state.evidence));
    auto gate = EvidencePolicy::evaluate(state.observationIntent, state.evidence,
                                         decision.grounding, decision.groundingDeclared);
    if (gate.accepted && decision.grounding == GroundingMode::Verified) {
        for (const auto& assertion : decision.claims) {
            const bool known = std::any_of(state.observationIntent.requirements.cbegin(),
                                           state.observationIntent.requirements.cend(),
                [&](const ObservationRequirement& requirement) {
                    return requirement.claimType != ClaimType::None && requirement.claimId == assertion.id;
                });
            if (!known) {
                gate.accepted = false;
                gate.repair = QStringLiteral("Final references an unknown claim ID.");
                break;
            }
        }
        for (const auto& requirement : state.observationIntent.requirements) {
            if (!gate.accepted) break;
            if (requirement.claimType == ClaimType::None) continue;
            const auto assertion = std::find_if(decision.claims.cbegin(), decision.claims.cend(),
                [&](const ClaimAssertion& item) { return item.id == requirement.claimId; });
            if (assertion == decision.claims.cend()) {
                gate.accepted = false;
                gate.repair = QStringLiteral("Verified final requires a structured assertion for %1.").arg(requirement.claimId);
                break;
            }
            const auto resolved = ClaimGroundingResolver::resolve(requirement, state.evidence, *assertion);
            if (resolved.verdict != ClaimVerdict::Supported) {
                gate.accepted = false;
                gate.repair = resolved.verdict == ClaimVerdict::Contradicted
                    ? QStringLiteral("The observed result contradicts %1. Use the verified fact when answering.").arg(requirement.claimId)
                    : QStringLiteral("The observed result cannot determine %1. Observe more or report inability to verify.").arg(requirement.claimId);
                break;
            }
            gate.grounding.evidenceCallIds.append(resolved.fact.evidenceCallIds);
        }
        gate.grounding.evidenceCallIds.removeDuplicates();
    }
    if (gate.accepted && decision.observationRequirementDeclared &&
        decision.requiresObservation && state.evidence.isEmpty()) {
        gate.accepted = false;
        gate.repair = QStringLiteral("You declared that observation is needed. Use an "
                                     "appropriate tool before answering.");
    }
    if (gate.accepted) {
        state.finalGrounding = std::move(gate.grounding);
        state.finalClaims = decision.claims;
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
                               const QString& summary, int stepIndex, StructuredObservationPtr structuredObservation,
                               const QList<FileMutation>& mutations) {
    if (plan.invocations.isEmpty())
        return;
    const auto callId = toolCallIdProvider_ ? toolCallIdProvider_(stepIndex)
                                           : QStringLiteral("%1:%2").arg(state.sessionId).arg(stepIndex);
    const bool trustedMutations = descriptor.source == ToolSource::BuiltIn ||
                                  descriptor.filesystemFailureSemanticContract;
    if ((trustedMutations && !mutations.isEmpty()) ||
        (status == ToolExecutionStatus::Succeeded && descriptor.id == QLatin1String("run-command"))) {
        QStringList changed;
        for (const auto& mutation : trustedMutations ? mutations : QList<FileMutation>{})
            changed.append(normalizedObservationResource(mutation.path, ObservationDomain::FileSystem));
        if (descriptor.id == QLatin1String("run-command")) changed.append(QDir::currentPath());
        for (auto& prior : state.evidence) {
            if (prior.domain != ObservationDomain::FileSystem && prior.domain != ObservationDomain::Workspace)
                continue;
            const auto resource = normalizedObservationResource(prior.resource, ObservationDomain::FileSystem);
            for (const auto& change : changed)
                if (resource == change || resource.startsWith(change + QLatin1Char('/')) ||
                    change.startsWith(resource + QLatin1Char('/'))) {
                    prior.outcome = EvidenceOutcome::Stale;
                    break;
                }
        }
    }
    state.evidence.append(EvidencePolicy::record(descriptor, plan.invocations.first(), status,
                                                 summary, stepIndex, callId, structuredObservation));
    if (trustedMutations && !mutations.isEmpty()) {
        for (const auto& mutation : mutations) {
            EvidenceRecord fact;
            fact.toolId = descriptor.id;
            fact.toolCallId = callId;
            fact.stepIndex = stepIndex;
            fact.domain = ObservationDomain::FileSystem;
            fact.resource = normalizedObservationResource(mutation.path, ObservationDomain::FileSystem);
            fact.scope = EvidenceScope::ExactResource;
            fact.outcome = EvidenceOutcome::Verified;
            fact.observedAtUtc = QDateTime::currentDateTimeUtc();
            auto observation = std::make_shared<StructuredObservation>();
            observation->kind = StructuredObservationKind::FileSystemFact;
            const bool exists = mutation.kind != FileMutationKind::Deleted &&
                                mutation.kind != FileMutationKind::MovedFrom;
            observation->data = {{QStringLiteral("path"), fact.resource},
                                 {QStringLiteral("exists"), exists},
                                 {QStringLiteral("file"), exists},
                                 {QStringLiteral("directory"), false}};
            fact.structuredObservation = observation;
            state.evidence.append(std::move(fact));
        }
    }
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
