// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/IToolExecutor.h"
#include "sentinel/core/security/PermissionPolicyService.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace sentinel::core {

class IToolRegistry;
class IToolHookService;

enum class ToolGatewayRiskLevel {
    Low,
    Medium,
    High,
    Critical,
};

enum class ToolGatewayScope {
    Local,
    Cloud,
    LocalAndCloud,
};

enum class ToolExecutionAvailability {
    MetadataSafe,
    Available,
    Unavailable,
    Refused,
};

struct ToolGatewayMetadata {
    QString toolId;
    QString displayName;
    QString category;
    QString description;
    QString requiredPermissionDomain;
    ToolGatewayRiskLevel riskLevel = ToolGatewayRiskLevel::Low;
    ToolGatewayScope scope = ToolGatewayScope::Local;
    ToolExecutionAvailability availability = ToolExecutionAvailability::Refused;
    QString refusalReason;
};

struct ToolGatewaySummary {
    QString toolId;
    QString displayName;
    QString category;
    QString requiredPermissionDomain;
    QString permissionPosture;
    QString riskLevel;
    QString scope;
    QString availability;
    QString refusalReason;
    QString summary;
    QStringList diagnostics;
};

struct ToolGatewayRegistrySummary {
    QString status;
    QString summary;
    QString permissionPosture;
    int toolCount = 0;
    int metadataSafeCount = 0;
    int unavailableCount = 0;
    int refusedCount = 0;
    QStringList toolSummaries;
    QStringList developerDiagnostics;
};

class ToolExecutionGateway final {
public:
    explicit ToolExecutionGateway(const IToolRegistry* registry = nullptr) : registry_(registry) {}
    void setRegistry(const IToolRegistry* registry) {
        registry_ = registry;
    }
    void setHookService(IToolHookService* hooks) {
        hooks_ = hooks;
    }
    void setPermissionPolicy(const PermissionPolicyService* policy, QString defaultState) {
        permissionPolicy_ = policy;
        defaultPermissionState_ = std::move(defaultState);
    }
    QList<ToolGatewayMetadata> toolMetadata() const;
    QList<ToolGatewaySummary> toolSummaries(const QString& defaultPermissionState,
                                            const PermissionPolicyService& permissionPolicy) const;
    ToolGatewayRegistrySummary
    registrySummary(const QString& defaultPermissionState,
                    const PermissionPolicyService& permissionPolicy) const;

    // Validates against a registration snapshot and replaces raw arguments with normalized values.
    ToolExecutionResult validatePlan(ToolInvocationPlan& plan) const;
    ToolExecutionResult execute(const ToolExecutionRequest& request,
                                const IToolExecutor& executor) const;
    IToolExecutor::Cancel executeAsync(const ToolExecutionRequest& request,
                                       const IToolExecutor& executor, const QString& sessionId,
                                       const QString& toolCallId, IToolExecutor::Output output,
                                       IToolExecutor::Completion completion) const;

private:
    const IToolRegistry* registry_ = nullptr;
    IToolHookService* hooks_ = nullptr;
    const PermissionPolicyService* permissionPolicy_ = nullptr;
    QString defaultPermissionState_;
};

QString toolGatewayRiskLevelName(ToolGatewayRiskLevel riskLevel);
QString toolGatewayScopeName(ToolGatewayScope scope);
QString toolExecutionAvailabilityName(ToolExecutionAvailability availability);

} // namespace sentinel::core
