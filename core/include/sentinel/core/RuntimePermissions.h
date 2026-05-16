#pragma once

#include <QString>
#include <cstdint>

namespace sentinel::core {

enum class RuntimePermission : std::uint8_t {
    LocalInference,
    ProviderInvocation,
    ToolInvocation,
    ExternalProcess,
    FilesystemAccess,
    NetworkAccess,
    PluginInvocation,
};

inline QString runtimePermissionName(RuntimePermission permission) {
    switch (permission) {
    case RuntimePermission::LocalInference:
        return QStringLiteral("Local Inference");
    case RuntimePermission::ProviderInvocation:
        return QStringLiteral("Provider Invocation");
    case RuntimePermission::ToolInvocation:
        return QStringLiteral("Tool Invocation");
    case RuntimePermission::ExternalProcess:
        return QStringLiteral("External Process");
    case RuntimePermission::FilesystemAccess:
        return QStringLiteral("Filesystem Access");
    case RuntimePermission::NetworkAccess:
        return QStringLiteral("Network Access");
    case RuntimePermission::PluginInvocation:
        return QStringLiteral("Plugin Invocation");
    }

    return QStringLiteral("Local Inference");
}

enum class RuntimePermissionLevel : std::uint8_t {
    None,
    ReadOnly,
    Execute,
};

inline QString runtimePermissionLevelName(RuntimePermissionLevel level) {
    switch (level) {
    case RuntimePermissionLevel::None:
        return QStringLiteral("None");
    case RuntimePermissionLevel::ReadOnly:
        return QStringLiteral("Read Only");
    case RuntimePermissionLevel::Execute:
        return QStringLiteral("Execute");
    }

    return QStringLiteral("None");
}

enum class RuntimePermissionDecisionStatus : std::uint8_t {
    NotRequested,
    Allowed,
    Denied,
};

inline QString runtimePermissionDecisionStatusName(RuntimePermissionDecisionStatus status) {
    switch (status) {
    case RuntimePermissionDecisionStatus::NotRequested:
        return QStringLiteral("Not Requested");
    case RuntimePermissionDecisionStatus::Allowed:
        return QStringLiteral("Allowed");
    case RuntimePermissionDecisionStatus::Denied:
        return QStringLiteral("Denied");
    }

    return QStringLiteral("Not Requested");
}

struct RuntimePermissionRequest {
    RuntimePermission permission = RuntimePermission::LocalInference;
    RuntimePermissionLevel level = RuntimePermissionLevel::None;
    QString source;
    QString summary;
};

struct RuntimePermissionDecision {
    RuntimePermissionDecisionStatus status = RuntimePermissionDecisionStatus::NotRequested;
    RuntimePermissionRequest request;
    QString summary;
};

QString safeRuntimePermissionRequestSummary(const RuntimePermissionRequest& request);
QString safeRuntimePermissionDecisionSummary(const RuntimePermissionDecision& decision);

class IRuntimePermissionPolicy {
public:
    virtual ~IRuntimePermissionPolicy() = default;

    virtual RuntimePermissionDecision evaluate(const RuntimePermissionRequest& request) const = 0;
};

class StaticRuntimePermissionPolicy final : public IRuntimePermissionPolicy {
public:
    RuntimePermissionDecision evaluate(const RuntimePermissionRequest& request) const override;
};

} // namespace sentinel::core
