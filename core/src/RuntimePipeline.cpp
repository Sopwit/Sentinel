#include "sentinel/core/RuntimePipeline.h"

namespace sentinel::core {

QString runtimePipelineTraceSummary(const RuntimePipelineTrace& trace) {
    if (!trace.summary.isEmpty()) {
        return QStringLiteral("%1 [%2]: %3")
            .arg(runtimePipelineStageName(trace.stage), trace.status, trace.summary);
    }

    return QStringLiteral("%1 [%2]").arg(runtimePipelineStageName(trace.stage), trace.status);
}

QStringList runtimePipelineTraceSummaries(const QList<RuntimePipelineTrace>& traces) {
    QStringList summaries;
    for (const auto& trace : traces) {
        summaries.append(runtimePipelineTraceSummary(trace));
    }
    return summaries;
}

QString safeRuntimePipelineSummary(const RuntimePipelineResult& result) {
    if (!result.summary.isEmpty()) {
        return result.summary;
    }

    return QStringLiteral("Runtime pipeline %1 (%2 traces).")
        .arg(runtimePipelineStatusName(result.status))
        .arg(result.traces.size());
}

RuntimePipelineResult
StaticRuntimePipeline::evaluate(const RuntimePipelineRequest& request,
                                const RuntimePermissionDecision& permissionDecision,
                                const RuntimeSafetyReport& safetyReport) const {
    const RuntimePipelineTrace requestTrace{
        RuntimePipelineStage::RequestReceived,
        QStringLiteral("Metadata Received"),
        request.summary.isEmpty() ? QStringLiteral("Runtime request metadata received.")
                                  : request.summary,
    };
    const RuntimePipelineTrace permissionTrace{
        RuntimePipelineStage::PermissionPolicy,
        runtimePermissionDecisionStatusName(permissionDecision.status),
        safeRuntimePermissionDecisionSummary(permissionDecision),
    };
    const RuntimePipelineTrace safetyTrace{
        RuntimePipelineStage::SafetyPolicy,
        runtimeSafetyDecisionName(safetyReport.decision),
        safeRuntimeSafetySummary(safetyReport),
    };

    const auto permissionAllowed =
        permissionDecision.status == RuntimePermissionDecisionStatus::Allowed;
    const auto safetyCompliant = safetyReport.decision == RuntimeSafetyDecision::Compliant;

    RuntimePipelineResult result;
    result.traces = {requestTrace, permissionTrace, safetyTrace};
    result.executionBlocked = true;

    if (!permissionAllowed || !safetyCompliant) {
        result.status = RuntimePipelineStatus::Blocked;
        result.summary = QStringLiteral("Runtime request pipeline blocked execution metadata by "
                                        "permission and safety policy.");
        result.traces.append(RuntimePipelineTrace{
            RuntimePipelineStage::ExecutionBoundary,
            QStringLiteral("Blocked"),
            QStringLiteral("Execution boundary remained blocked; no runtime action was performed."),
        });
        return result;
    }

    result.status = RuntimePipelineStatus::CompletedMetadata;
    result.summary = QStringLiteral("Runtime request pipeline completed metadata evaluation only; "
                                    "execution remains disabled.");
    result.traces.append(RuntimePipelineTrace{
        RuntimePipelineStage::ExecutionBoundary,
        QStringLiteral("No Execution"),
        QStringLiteral("Execution remains disabled in metadata-only runtime boundary."),
    });
    return result;
}

} // namespace sentinel::core
