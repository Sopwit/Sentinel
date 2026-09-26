// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/ObservationEvidence.h"

#include <QJsonObject>
#include <QList>
#include <QString>

namespace sentinel::core {

enum class ToolRiskLevel {
    Low,
    Medium,
    High,
};

enum class SecurityDomain {
    FileSystem,
    Process,
    Network,
    Clipboard,
    Application,
    System,
    Memory,
    Conversation,
    Audio,
    Browser,
    Agent,
    ExternalService,
};

enum class AccessMode { Read, Write, Delete, Execute, Control, Invoke };
enum class AuthorizationResourceKind { None, Argument, ArgumentDigest, FileSystemPath, Host, Provider };

struct ToolAuthorizationRequirement {
    SecurityDomain domain = SecurityDomain::ExternalService;
    AccessMode access = AccessMode::Invoke;
    AuthorizationResourceKind resourceKind = AuthorizationResourceKind::None;
    QString resourceArgument;
    QString staticResource;
    bool operator==(const ToolAuthorizationRequirement&) const = default;
};

struct AuthorizationRequest {
    SecurityDomain domain = SecurityDomain::ExternalService;
    AccessMode access = AccessMode::Invoke;
    QString resource;
    ToolRiskLevel risk = ToolRiskLevel::Low;
    QString providerId;
    QString toolId;
    AuthorizationResourceKind resourceKind = AuthorizationResourceKind::None;
};

enum class ToolExecutionMode {
    MetadataOnly,
    Local,
};

enum class ToolSource { BuiltIn, MCP, Plugin, Internal };
enum class ToolScope { Local, Cloud, LocalAndCloud };

struct ToolParameterDescriptor {
    QString id;
    QString description;
    bool required = false;
};

struct ToolDescriptor {
    QString id;
    QString name;
    QString description;
    ToolRiskLevel riskLevel = ToolRiskLevel::Low;
    ToolExecutionMode executionMode = ToolExecutionMode::MetadataOnly;
    QList<ToolParameterDescriptor> parameters;
    QString category;
    ToolSource source = ToolSource::BuiltIn;
    bool enabled = true;
    bool exposedToModel = true;
    // Runtime-enforced JSON Schema subset; also rendered for planner tool discovery.
    QJsonObject inputSchema;
    QString providerId;
    QString version;
    QString requiredPermissionDomain;
    QList<ToolAuthorizationRequirement> authorizationRequirements;
    ToolScope scope = ToolScope::Local;
    // Declares what a successful call can verify; never grants permissions.
    QList<ToolEvidenceDescriptor> evidenceProduced;
    StructuredObservationKind structuredObservationKind = StructuredObservationKind::None;
    bool filesystemFailureSemanticContract = false;
    bool parallelSafe = false;
};

} // namespace sentinel::core
