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
    QList<ToolGatewayMetadata> toolMetadata() const;
    QList<ToolGatewaySummary> toolSummaries(const QString& defaultPermissionState,
                                            const PermissionPolicyService& permissionPolicy) const;
    ToolGatewayRegistrySummary
    registrySummary(const QString& defaultPermissionState,
                    const PermissionPolicyService& permissionPolicy) const;

    ToolExecutionResult execute(const ToolExecutionRequest& request,
                                const IToolExecutor& executor) const;
};

QString toolGatewayRiskLevelName(ToolGatewayRiskLevel riskLevel);
QString toolGatewayScopeName(ToolGatewayScope scope);
QString toolExecutionAvailabilityName(ToolExecutionAvailability availability);

} // namespace sentinel::core
