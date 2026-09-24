// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/ToolExecutionGateway.h"
#include "sentinel/core/runtime/BuiltInToolProvider.h"
#include "sentinel/core/runtime/IToolHookService.h"
#include "sentinel/core/runtime/IToolRegistry.h"
#include "sentinel/core/runtime/ToolArgumentValidator.h"
#include <QTimer>

namespace sentinel::core {

namespace {

QString permissionPostureForDomain(const QString& domainId, const QString& defaultPermissionState,
                                   const PermissionPolicyService& permissionPolicy) {
    for (const auto& summary : permissionPolicy.permissionSummaries(defaultPermissionState)) {
        if (summary.domainId == domainId) {
            return summary.state;
        }
    }
    return permissionPolicy.normalizedState(defaultPermissionState);
}

QString toolSummaryLine(const ToolGatewaySummary& summary) {
    return QStringLiteral("%1 / %2 / %3 / %4")
        .arg(summary.displayName, summary.availability, summary.permissionPosture,
             summary.requiredPermissionDomain);
}

} // namespace

QList<ToolGatewayMetadata> ToolExecutionGateway::toolMetadata() const {
    const auto descriptors =
        registry_ ? registry_->listTools() : BuiltInToolProvider::descriptors();
    QList<ToolGatewayMetadata> metadata;
    for (const auto& descriptor : descriptors) {
        ToolGatewayRiskLevel risk = ToolGatewayRiskLevel::Low;
        if (descriptor.riskLevel == ToolRiskLevel::Medium)
            risk = ToolGatewayRiskLevel::Medium;
        else if (descriptor.riskLevel == ToolRiskLevel::High)
            risk = ToolGatewayRiskLevel::High;
        ToolGatewayScope scope = ToolGatewayScope::Local;
        if (descriptor.scope == ToolScope::Cloud)
            scope = ToolGatewayScope::Cloud;
        else if (descriptor.scope == ToolScope::LocalAndCloud)
            scope = ToolGatewayScope::LocalAndCloud;
        metadata.append({descriptor.id, descriptor.name, descriptor.category,
                         descriptor.description, descriptor.requiredPermissionDomain, risk, scope,
                         descriptor.enabled ? ToolExecutionAvailability::Available
                                            : ToolExecutionAvailability::Refused,
                         descriptor.enabled ? QString() : QStringLiteral("Tool is disabled.")});
    }
    return metadata;
}

QList<ToolGatewaySummary>
ToolExecutionGateway::toolSummaries(const QString& defaultPermissionState,
                                    const PermissionPolicyService& permissionPolicy) const {
    QList<ToolGatewaySummary> summaries;
    for (const auto& tool : toolMetadata()) {
        const auto permissionPosture = permissionPostureForDomain(
            tool.requiredPermissionDomain, defaultPermissionState, permissionPolicy);
        const auto risk = toolGatewayRiskLevelName(tool.riskLevel);
        const auto scope = toolGatewayScopeName(tool.scope);
        const auto availability = toolExecutionAvailabilityName(tool.availability);
        const auto summary =
            QStringLiteral("%1 requires %2 permission, risk %3, scope %4, availability %5.")
                .arg(tool.displayName, permissionPosture, risk, scope, availability);

        summaries.append({
            tool.toolId,
            tool.displayName,
            tool.category,
            tool.requiredPermissionDomain,
            permissionPosture,
            risk,
            scope,
            availability,
            tool.refusalReason,
            summary,
            {
                QStringLiteral("Tool id: %1").arg(tool.toolId),
                QStringLiteral("Category: %1").arg(tool.category),
                QStringLiteral("Permission domain: %1").arg(tool.requiredPermissionDomain),
                QStringLiteral("Permission posture: %1").arg(permissionPosture),
                QStringLiteral("Risk: %1").arg(risk),
                QStringLiteral("Scope: %1").arg(scope),
                QStringLiteral("Execution availability: %1").arg(availability),
                tool.refusalReason,
                QStringLiteral("Gateway execution grant: allowed"),
            },
        });
    }
    return summaries;
}

ToolGatewayRegistrySummary
ToolExecutionGateway::registrySummary(const QString& defaultPermissionState,
                                      const PermissionPolicyService& permissionPolicy) const {
    const auto permissionPosture = permissionPolicy.normalizedState(defaultPermissionState);
    ToolGatewayRegistrySummary registry{
        QStringLiteral("Operational"),
        QStringLiteral("Tool Execution Gateway is fully operational and processes local system and "
                       "cloud tools safely."),
        permissionPosture,
        0,
        0,
        0,
        0,
        {},
        {},
    };

    const auto summaries = toolSummaries(defaultPermissionState, permissionPolicy);
    registry.toolCount = summaries.size();
    for (const auto& summary : summaries) {
        if (summary.availability ==
                toolExecutionAvailabilityName(ToolExecutionAvailability::MetadataSafe) ||
            summary.availability ==
                toolExecutionAvailabilityName(ToolExecutionAvailability::Available)) {
            ++registry.metadataSafeCount;
        } else if (summary.availability ==
                   toolExecutionAvailabilityName(ToolExecutionAvailability::Unavailable)) {
            ++registry.unavailableCount;
        } else {
            ++registry.refusedCount;
        }
        registry.toolSummaries.append(toolSummaryLine(summary));
        registry.developerDiagnostics.append(summary.diagnostics.join(QStringLiteral(" / ")));
    }

    return registry;
}

ToolExecutionResult ToolExecutionGateway::validatePlan(ToolInvocationPlan& plan) const {
    if (!registry_)
        return {ToolExecutionStatus::Succeeded,
                QStringLiteral("Compatibility executor has no registry contract.")};
    if (plan.status != ToolInvocationPlanStatus::Planned || plan.invocations.isEmpty())
        return {ToolExecutionStatus::EmptyPlan, QStringLiteral("No tool invocation to validate.")};
    for (auto& invocation : plan.invocations) {
        // findRegistration returns a value snapshot; no registry lock is held during validation.
        const auto registration = registry_->findRegistration(invocation.toolId);
        if (!registration || !registration->handler || !registration->descriptor.enabled)
            return {ToolExecutionStatus::UnknownTool,
                    QStringLiteral("Tool unavailable: %1").arg(invocation.toolId)};
        const auto contractErrors =
            ToolArgumentValidator::validateSchema(registration->descriptor.inputSchema);
        if (!contractErrors.isEmpty())
            return {ToolExecutionStatus::InvalidToolContract,
                    QStringLiteral("Tool contract is invalid for %1.").arg(invocation.toolId)};
        const auto validation =
            ToolArgumentValidator::validate(registration->descriptor, invocation.arguments);
        if (!validation.valid) {
            QStringList lines{QStringLiteral("Tool call rejected: invalid arguments for %1.")
                                  .arg(invocation.toolId)};
            for (const auto& error : validation.errors.mid(0, 8))
                lines.append(
                    QStringLiteral("%1 [%2]: %3").arg(error.path, error.keyword, error.message));
            return {ToolExecutionStatus::InvalidArguments, lines.join(QLatin1Char('\n'))};
        }
        invocation.arguments =
            ToolArgumentValidator::toInvocationArguments(validation.normalizedArguments);
    }
    return {ToolExecutionStatus::Succeeded, QStringLiteral("Arguments validated.")};
}

ToolExecutionResult ToolExecutionGateway::execute(const ToolExecutionRequest& request,
                                                  const IToolExecutor& executor) const {
    if (registry_) {
        struct State {
            ToolExecutionResult result;
            bool completed = false;
        };
        auto state = std::make_shared<State>();
        auto cancel =
            executeAsync(request, executor, {}, {}, {}, [state](ToolExecutionResult value) {
                state->result = std::move(value);
                state->completed = true;
            });
        if (!state->completed) {
            if (cancel)
                cancel();
            return {ToolExecutionStatus::Blocked,
                    QStringLiteral("Asynchronous tool requires the AgentLoop continuation path.")};
        }
        return state->result;
    }
    QStringList knownToolIds;
    for (const ToolGatewayMetadata& metadata : toolMetadata()) {
        knownToolIds.append(metadata.toolId);
    }

    ToolExecutionRequest gatedRequest = request;
    if (gatedRequest.knownToolIds.isEmpty()) {
        gatedRequest.knownToolIds = knownToolIds;
    }
    if (gatedRequest.plan.status != ToolInvocationPlanStatus::Planned ||
        gatedRequest.plan.invocations.isEmpty()) {
        return executor.execute(gatedRequest);
    }
    if (gatedRequest.approval.status != ApprovalStatus::Approved &&
        gatedRequest.approval.status != ApprovalStatus::NotRequired) {
        return {
            ToolExecutionStatus::Blocked,
            QStringLiteral("Tool gateway blocked execution until explicit approval is granted.")};
    }
    return executor.execute(gatedRequest);
}

IToolExecutor::Cancel
ToolExecutionGateway::executeAsync(const ToolExecutionRequest& originalRequest,
                                   const IToolExecutor& executor, const QString& sessionId,
                                   const QString& toolCallId, IToolExecutor::Output output,
                                   IToolExecutor::Completion completion) const {
    ToolExecutionRequest normalizedRequest = originalRequest;
    if (registry_ && normalizedRequest.plan.status == ToolInvocationPlanStatus::Planned &&
        !normalizedRequest.plan.invocations.isEmpty()) {
        const auto validation = validatePlan(normalizedRequest.plan);
        if (validation.status != ToolExecutionStatus::Succeeded) {
            completion(validation);
            return {};
        }
    }
    const auto& request = normalizedRequest;
    if (registry_) {
        if (request.plan.status != ToolInvocationPlanStatus::Planned ||
            request.plan.invocations.isEmpty()) {
            completion(
                {ToolExecutionStatus::EmptyPlan,
                 QStringLiteral("No planned tool invocation reached the execution boundary.")});
            return {};
        }
        if (request.approval.status != ApprovalStatus::Approved &&
            request.approval.status != ApprovalStatus::NotRequired) {
            completion({ToolExecutionStatus::Blocked,
                        QStringLiteral(
                            "Tool gateway blocked execution until explicit approval is granted.")});
            return {};
        }
        if (request.sandbox.status == SandboxStatus::Denied ||
            request.sandbox.status == SandboxStatus::BlockedByApproval) {
            completion(
                {ToolExecutionStatus::Blocked,
                 QStringLiteral("Execution boundary blocked by sandbox capability metadata.")});
            return {};
        }
        if (request.plan.invocations.size() > 1) {
            struct Batch {
                ToolExecutionRequest request;
                IToolExecutor::Output output;
                IToolExecutor::Completion completion;
                IToolExecutor::Cancel cancel;
                QStringList summaries;
                std::function<void()> next;
                int index = 0;
                bool active = false;
                bool finished = false;
                bool cancelled = false;
            };
            auto batch = std::make_shared<Batch>();
            batch->request = request;
            batch->output = std::move(output);
            batch->completion = std::move(completion);
            std::weak_ptr<Batch> weak = batch;
            const auto* registry = registry_;
            const auto* execution = &executor;
            auto* hooks = hooks_;
            const auto* permissionPolicy = permissionPolicy_;
            const auto defaultPermissionState = defaultPermissionState_;
            batch->next = [weak, registry, execution, hooks, permissionPolicy,
                           defaultPermissionState, sessionId, toolCallId] {
                auto state = weak.lock();
                if (!state || state->finished)
                    return;
                if (state->cancelled || state->index >= state->request.plan.invocations.size()) {
                    state->finished = true;
                    state->completion({state->cancelled ? ToolExecutionStatus::Blocked
                                                        : ToolExecutionStatus::Succeeded,
                                       state->cancelled
                                           ? QStringLiteral("Tool execution cancelled.")
                                           : state->summaries.join(QStringLiteral("\n\n"))});
                    state->next = {};
                    return;
                }
                ToolExecutionRequest one = state->request;
                one.plan.invocations = {state->request.plan.invocations.at(state->index++)};
                state->active = true;
                ToolExecutionGateway gateway(registry);
                gateway.setHookService(hooks);
                gateway.setPermissionPolicy(permissionPolicy, defaultPermissionState);
                state->cancel = gateway.executeAsync(
                    one, *execution, sessionId, toolCallId, state->output,
                    [weak](ToolExecutionResult result) {
                        auto current = weak.lock();
                        if (!current || current->finished)
                            return;
                        current->active = false;
                        current->cancel = {};
                        if (current->cancelled ||
                            (result.status != ToolExecutionStatus::Succeeded &&
                             result.status != ToolExecutionStatus::PlaceholderSucceeded)) {
                            current->finished = true;
                            current->completion(
                                current->cancelled
                                    ? ToolExecutionResult{ToolExecutionStatus::Blocked,
                                                          QStringLiteral(
                                                              "Tool execution cancelled.")}
                                    : std::move(result));
                            current->next = {};
                            return;
                        }
                        current->summaries.append(std::move(result.summary));
                        QTimer::singleShot(0, [weak] {
                            if (auto next = weak.lock(); next && next->next)
                                next->next();
                        });
                    });
                if (!state->active)
                    state->cancel = {};
            };
            batch->next();
            return [batch] {
                if (batch->finished)
                    return;
                batch->cancelled = true;
                if (batch->active && batch->cancel)
                    batch->cancel();
                else if (batch->next)
                    batch->next();
            };
        }
        const auto registration =
            registry_->findRegistration(request.plan.invocations.first().toolId);
        if (!registration || !registration->handler) {
            completion({ToolExecutionStatus::UnknownTool,
                        QStringLiteral("Execution boundary rejected unknown tool metadata: %1")
                            .arg(request.plan.invocations.first().toolId)});
            return {};
        }
        {
            ToolExecutionRequest resolved = request;
            if (!ToolArgumentValidator::validateSchema(registration->descriptor.inputSchema)
                     .isEmpty()) {
                completion({ToolExecutionStatus::InvalidToolContract,
                            QStringLiteral("Tool contract is invalid for %1.")
                                .arg(registration->descriptor.id)});
                return {};
            }
            const auto snapshotValidation = ToolArgumentValidator::validate(
                registration->descriptor, resolved.plan.invocations.first().arguments);
            if (!snapshotValidation.valid) {
                QStringList lines{QStringLiteral("Tool call rejected: invalid arguments for %1.")
                                      .arg(registration->descriptor.id)};
                for (const auto& issue : snapshotValidation.errors.mid(0, 8))
                    lines.append(QStringLiteral("%1 [%2]: %3")
                                     .arg(issue.path, issue.keyword, issue.message));
                completion({ToolExecutionStatus::InvalidArguments, lines.join(QLatin1Char('\n'))});
                return {};
            }
            resolved.plan.invocations.first().arguments =
                ToolArgumentValidator::toInvocationArguments(
                    snapshotValidation.normalizedArguments);
            if (!registration->descriptor.enabled) {
                completion(
                    {ToolExecutionStatus::Blocked, QStringLiteral("Registered tool is disabled.")});
                return {};
            }
            if (request.approval.status != ApprovalStatus::Approved &&
                request.approval.status != ApprovalStatus::NotRequired) {
                completion(
                    {ToolExecutionStatus::Blocked,
                     QStringLiteral(
                         "Tool gateway blocked execution until explicit approval is granted.")});
                return {};
            }
            if (permissionPolicy_ &&
                (registration->descriptor.source == ToolSource::MCP ||
                 registration->descriptor.source == ToolSource::Plugin) &&
                !permissionPolicy_->allowsToolExecution(
                    registration->descriptor.requiredPermissionDomain, defaultPermissionState_,
                    request.approval.status == ApprovalStatus::Approved)) {
                completion({ToolExecutionStatus::Blocked,
                            QStringLiteral("MCP tool permission policy denied execution.")});
                return {};
            }
            if (request.sandbox.status == SandboxStatus::Denied ||
                request.sandbox.status == SandboxStatus::BlockedByApproval) {
                completion({ToolExecutionStatus::Blocked,
                            QStringLiteral("Tool gateway blocked execution by sandbox policy.")});
                return {};
            }
            const auto handler = registration->handler;
            resolved.knownToolIds = {registration->descriptor.id};
            const auto toolId = registration->descriptor.id;
            if (hooks_ && hooks_->hasHooks(toolId)) {
                QJsonObject parameters;
                for (const auto& argument : resolved.plan.invocations.first().arguments)
                    parameters.insert(argument.id, argument.value);
                hooks_->beforeToolExecution(toolId, parameters);
            }
            auto* hooks = hooks_;
            auto cancel = handler->execute(
                resolved, sessionId, toolCallId, std::move(output),
                [handler, hooks, toolId,
                 completion = std::move(completion)](ToolExecutionResult result) {
                    if (hooks && hooks->hasHooks(toolId)) {
                        if (result.status == ToolExecutionStatus::Succeeded) {
                            QJsonObject output{{QStringLiteral("summary"), result.summary}};
                            hooks->afterToolExecution(toolId, output);
                        } else {
                            hooks->onToolError(toolId, result.summary);
                        }
                    }
                    completion(std::move(result));
                });
            return [handler, cancel = std::move(cancel)] {
                if (cancel)
                    cancel();
            };
        }
    }
    QStringList knownToolIds;
    for (const ToolGatewayMetadata& metadata : toolMetadata())
        knownToolIds.append(metadata.toolId);
    ToolExecutionRequest gated = request;
    if (gated.knownToolIds.isEmpty())
        gated.knownToolIds = knownToolIds;
    if (gated.plan.status == ToolInvocationPlanStatus::Planned &&
        !gated.plan.invocations.isEmpty() && gated.approval.status != ApprovalStatus::Approved &&
        gated.approval.status != ApprovalStatus::NotRequired) {
        completion(
            {ToolExecutionStatus::Blocked,
             QStringLiteral("Tool gateway blocked execution until explicit approval is granted.")});
        return {};
    }
    return executor.executeAsync(gated, sessionId, toolCallId, std::move(output),
                                 std::move(completion));
}

QString toolGatewayRiskLevelName(ToolGatewayRiskLevel riskLevel) {
    switch (riskLevel) {
    case ToolGatewayRiskLevel::Low:
        return QStringLiteral("Low");
    case ToolGatewayRiskLevel::Medium:
        return QStringLiteral("Medium");
    case ToolGatewayRiskLevel::High:
        return QStringLiteral("High");
    case ToolGatewayRiskLevel::Critical:
        return QStringLiteral("Critical");
    }
    return QStringLiteral("Low");
}

QString toolGatewayScopeName(ToolGatewayScope scope) {
    switch (scope) {
    case ToolGatewayScope::Local:
        return QStringLiteral("Local");
    case ToolGatewayScope::Cloud:
        return QStringLiteral("Cloud");
    case ToolGatewayScope::LocalAndCloud:
        return QStringLiteral("Local + Cloud");
    }
    return QStringLiteral("Local");
}

QString toolExecutionAvailabilityName(ToolExecutionAvailability availability) {
    switch (availability) {
    case ToolExecutionAvailability::MetadataSafe:
        return QStringLiteral("Metadata safe");
    case ToolExecutionAvailability::Available:
        return QStringLiteral("Available");
    case ToolExecutionAvailability::Unavailable:
        return QStringLiteral("Unavailable");
    case ToolExecutionAvailability::Refused:
        return QStringLiteral("Refused");
    }
    return QStringLiteral("Refused");
}

} // namespace sentinel::core
