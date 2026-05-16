#pragma once

#include "sentinel/core/RuntimePermissions.h"
#include "sentinel/core/RuntimeSafety.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

enum class RuntimePipelineStage : std::uint8_t {
    RequestReceived,
    PermissionPolicy,
    SafetyPolicy,
    ExecutionBoundary,
};

inline QString runtimePipelineStageName(RuntimePipelineStage stage) {
    switch (stage) {
    case RuntimePipelineStage::RequestReceived:
        return QStringLiteral("Request Received");
    case RuntimePipelineStage::PermissionPolicy:
        return QStringLiteral("Permission Policy");
    case RuntimePipelineStage::SafetyPolicy:
        return QStringLiteral("Safety Policy");
    case RuntimePipelineStage::ExecutionBoundary:
        return QStringLiteral("Execution Boundary");
    }

    return QStringLiteral("Request Received");
}

enum class RuntimePipelineStatus : std::uint8_t {
    NotRequested,
    CompletedMetadata,
    Blocked,
};

inline QString runtimePipelineStatusName(RuntimePipelineStatus status) {
    switch (status) {
    case RuntimePipelineStatus::NotRequested:
        return QStringLiteral("Not Requested");
    case RuntimePipelineStatus::CompletedMetadata:
        return QStringLiteral("Completed Metadata");
    case RuntimePipelineStatus::Blocked:
        return QStringLiteral("Blocked");
    }

    return QStringLiteral("Not Requested");
}

struct RuntimePipelineRequest {
    QString id = QStringLiteral("runtime-pipeline-request-1");
    QString summary;
    RuntimePermissionRequest permissionRequest;
};

struct RuntimePipelineTrace {
    RuntimePipelineStage stage = RuntimePipelineStage::RequestReceived;
    QString status;
    QString summary;
};

struct RuntimePipelineResult {
    RuntimePipelineStatus status = RuntimePipelineStatus::NotRequested;
    QString summary;
    QList<RuntimePipelineTrace> traces;
    bool executionBlocked = true;
};

QString runtimePipelineTraceSummary(const RuntimePipelineTrace& trace);
QStringList runtimePipelineTraceSummaries(const QList<RuntimePipelineTrace>& traces);
QString safeRuntimePipelineSummary(const RuntimePipelineResult& result);

class IRuntimePipeline {
public:
    virtual ~IRuntimePipeline() = default;

    virtual RuntimePipelineResult evaluate(const RuntimePipelineRequest& request,
                                           const RuntimePermissionDecision& permissionDecision,
                                           const RuntimeSafetyReport& safetyReport) const = 0;
};

class StaticRuntimePipeline final : public IRuntimePipeline {
public:
    RuntimePipelineResult evaluate(const RuntimePipelineRequest& request,
                                   const RuntimePermissionDecision& permissionDecision,
                                   const RuntimeSafetyReport& safetyReport) const override;
};

} // namespace sentinel::core
