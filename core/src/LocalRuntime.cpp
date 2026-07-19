#include "sentinel/core/LocalRuntime.h"

namespace sentinel::core {

QString localRuntimeCapabilitySummary(const LocalRuntimeCapability& capability) {
    const auto state = capability.enabled ? QStringLiteral("Enabled") : QStringLiteral("Disabled");
    if (!capability.summary.isEmpty()) {
        return QStringLiteral("%1 (%2): %3").arg(capability.name, state, capability.summary);
    }

    return QStringLiteral("%1 (%2)").arg(capability.name, state);
}

QString safeLocalRuntimeSummary(const LocalRuntimeDescriptor& descriptor) {
    if (!descriptor.summary.isEmpty()) {
        return descriptor.summary;
    }

    return QStringLiteral("%1: %2 / %3")
        .arg(descriptor.name, localRuntimeStatusName(descriptor.status),
             localRuntimeHealthName(descriptor.health));
}

QStringList localRuntimeCapabilitySummaries(const QList<LocalRuntimeCapability>& capabilities) {
    QStringList summaries;
    for (const auto& capability : capabilities) {
        summaries.append(localRuntimeCapabilitySummary(capability));
    }
    return summaries;
}

QString safeLocalRuntimeResponseSummary(const LocalRuntimeResponse& response) {
    if (!response.summary.isEmpty()) {
        return response.summary;
    }

    return response.status.isEmpty() ? QStringLiteral("Local runtime request refused.")
                                     : response.status;
}

LocalRuntimeDescriptor NullLocalRuntime::descriptor() const {
    return LocalRuntimeDescriptor{
        QStringLiteral("null-local-runtime"),
        QStringLiteral("Null Local Runtime"),
        QStringLiteral("Null Local Runtime is metadata-only; local inference execution is "
                       "disabled."),
        LocalRuntimeStatus::MetadataOnly,
        LocalRuntimeHealth::NotExecutable,
        {
            LocalRuntimeCapability{
                QStringLiteral("local-runtime.metadata"),
                QStringLiteral("Metadata Reporting"),
                QStringLiteral("Reports deterministic local runtime status and health."),
                true,
            },
            LocalRuntimeCapability{
                QStringLiteral("local-runtime.inference"),
                QStringLiteral("Local Inference"),
                QStringLiteral("Inference execution is intentionally disabled."),
                false,
            },
            LocalRuntimeCapability{
                QStringLiteral("local-runtime.streaming"),
                QStringLiteral("Streaming"),
                QStringLiteral("Streaming is intentionally disabled."),
                false,
            },
        },
    };
}

LocalRuntimeResponse NullLocalRuntime::evaluate(const LocalRuntimeRequest& request) const {
    Q_UNUSED(request);
    return LocalRuntimeResponse{
        false,
        QStringLiteral("Refused"),
        QStringLiteral("Local runtime boundary is metadata-only; execution is disabled."),
    };
}

} // namespace sentinel::core
