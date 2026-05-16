#include "sentinel/core/RuntimeCapabilities.h"

#include <algorithm>
#include <utility>

namespace sentinel::core {

namespace {

QList<RuntimeCapabilityDescriptor> defaultCapabilities() {
    return {
        RuntimeCapabilityDescriptor{
            QStringLiteral("cloud-relay-support"),
            QStringLiteral("Cloud Relay Support"),
            RuntimeCapabilityGroup::Integration,
            RuntimeCapabilityState::Unavailable,
            QStringLiteral("Cloud relay metadata is unavailable and cannot execute."),
        },
        RuntimeCapabilityDescriptor{
            QStringLiteral("embeddings"),
            QStringLiteral("Embeddings"),
            RuntimeCapabilityGroup::Memory,
            RuntimeCapabilityState::Unavailable,
            QStringLiteral("Embedding generation is not available."),
        },
        RuntimeCapabilityDescriptor{
            QStringLiteral("external-process-execution"),
            QStringLiteral("External Process Execution"),
            RuntimeCapabilityGroup::Platform,
            RuntimeCapabilityState::Unavailable,
            QStringLiteral("External process launch is unavailable."),
        },
        RuntimeCapabilityDescriptor{
            QStringLiteral("filesystem-access"),
            QStringLiteral("Filesystem Access"),
            RuntimeCapabilityGroup::Platform,
            RuntimeCapabilityState::Unavailable,
            QStringLiteral("Filesystem access is unavailable to runtime negotiation."),
        },
        RuntimeCapabilityDescriptor{
            QStringLiteral("local-inference"),
            QStringLiteral("Local Inference"),
            RuntimeCapabilityGroup::Inference,
            RuntimeCapabilityState::Disabled,
            QStringLiteral("Local model inference remains disabled."),
        },
        RuntimeCapabilityDescriptor{
            QStringLiteral("local-only-enforcement"),
            QStringLiteral("Local-Only Enforcement"),
            RuntimeCapabilityGroup::Security,
            RuntimeCapabilityState::Enabled,
            QStringLiteral("Negotiation metadata enforces a local-only posture."),
        },
        RuntimeCapabilityDescriptor{
            QStringLiteral("memory-binding"),
            QStringLiteral("Memory Binding"),
            RuntimeCapabilityGroup::Memory,
            RuntimeCapabilityState::Disabled,
            QStringLiteral("Runtime memory binding is descriptive only."),
        },
        RuntimeCapabilityDescriptor{
            QStringLiteral("multimodal"),
            QStringLiteral("Multimodal"),
            RuntimeCapabilityGroup::Inference,
            RuntimeCapabilityState::Disabled,
            QStringLiteral("Multimodal runtime support is disabled."),
        },
        RuntimeCapabilityDescriptor{
            QStringLiteral("plugin-bridge"),
            QStringLiteral("Plugin Bridge"),
            RuntimeCapabilityGroup::Integration,
            RuntimeCapabilityState::Unavailable,
            QStringLiteral("Plugin bridge loading is unavailable."),
        },
        RuntimeCapabilityDescriptor{
            QStringLiteral("privacy-safe-mode"),
            QStringLiteral("Privacy-Safe Mode"),
            RuntimeCapabilityGroup::Security,
            RuntimeCapabilityState::Enabled,
            QStringLiteral("Privacy-safe runtime metadata mode is active."),
        },
        RuntimeCapabilityDescriptor{
            QStringLiteral("semantic-memory"),
            QStringLiteral("Semantic Memory"),
            RuntimeCapabilityGroup::Memory,
            RuntimeCapabilityState::Disabled,
            QStringLiteral("Semantic memory execution is disabled."),
        },
        RuntimeCapabilityDescriptor{
            QStringLiteral("streaming"),
            QStringLiteral("Streaming"),
            RuntimeCapabilityGroup::Inference,
            RuntimeCapabilityState::Disabled,
            QStringLiteral("Token streaming is disabled."),
        },
        RuntimeCapabilityDescriptor{
            QStringLiteral("tool-bridge"),
            QStringLiteral("Tool Bridge"),
            RuntimeCapabilityGroup::Integration,
            RuntimeCapabilityState::Unavailable,
            QStringLiteral("Tool bridge execution is unavailable."),
        },
    };
}

void sortCapabilities(QList<RuntimeCapabilityDescriptor>& capabilities) {
    std::sort(capabilities.begin(), capabilities.end(),
              [](const RuntimeCapabilityDescriptor& left,
                 const RuntimeCapabilityDescriptor& right) { return left.id < right.id; });
}

} // namespace

QString runtimeCapabilitySummary(const RuntimeCapabilityDescriptor& capability) {
    const auto prefix = QStringLiteral("%1 (%2, %3)")
                            .arg(capability.name, runtimeCapabilityGroupName(capability.group),
                                 runtimeCapabilityStateName(capability.state));
    return capability.summary.isEmpty() ? prefix
                                        : QStringLiteral("%1: %2").arg(prefix, capability.summary);
}

QString safeRuntimeNegotiationProfileSummary(const RuntimeNegotiationProfile& profile) {
    if (!profile.summary.isEmpty()) {
        return profile.summary;
    }

    return QStringLiteral("%1: %2").arg(
        profile.name, profile.localOnlyEnforced ? QStringLiteral("local-only enforced")
                                                : QStringLiteral("local-only not enforced"));
}

QString safeRuntimeNegotiationSummary(const RuntimeNegotiationResult& result) {
    if (!result.summary.isEmpty()) {
        return result.summary;
    }

    return QStringLiteral("%1 negotiated %2 metadata capabilities.")
        .arg(result.profile.name)
        .arg(result.capabilities.size());
}

QString localOnlyRuntimeEnforcementSummary(const RuntimeNegotiationResult& result) {
    return result.profile.localOnlyEnforced
               ? QStringLiteral("Local-only enforcement is active; cloud relay and external "
                                "runtime execution remain unavailable.")
               : QStringLiteral("Local-only enforcement is not active.");
}

QStringList runtimeCapabilitySummaries(const QList<RuntimeCapabilityDescriptor>& capabilities) {
    QStringList summaries;
    for (const auto& capability : capabilities) {
        summaries.append(runtimeCapabilitySummary(capability));
    }
    return summaries;
}

QStringList
enabledRuntimeCapabilitySummaries(const QList<RuntimeCapabilityDescriptor>& capabilities) {
    QStringList summaries;
    for (const auto& capability : capabilities) {
        if (capability.state == RuntimeCapabilityState::Enabled) {
            summaries.append(runtimeCapabilitySummary(capability));
        }
    }
    return summaries;
}

QStringList
disabledRuntimeCapabilitySummaries(const QList<RuntimeCapabilityDescriptor>& capabilities) {
    QStringList summaries;
    for (const auto& capability : capabilities) {
        if (capability.state != RuntimeCapabilityState::Enabled) {
            summaries.append(runtimeCapabilitySummary(capability));
        }
    }
    return summaries;
}

StaticRuntimeCapabilityRegistry::StaticRuntimeCapabilityRegistry()
    : StaticRuntimeCapabilityRegistry(defaultCapabilities()) {}

StaticRuntimeCapabilityRegistry::StaticRuntimeCapabilityRegistry(
    QList<RuntimeCapabilityDescriptor> capabilities)
    : capabilities_(std::move(capabilities)) {
    sortCapabilities(capabilities_);
}

QList<RuntimeCapabilityDescriptor> StaticRuntimeCapabilityRegistry::capabilities() const {
    return capabilities_;
}

RuntimeNegotiationResult StaticRuntimeCapabilityRegistry::negotiate() const {
    return RuntimeNegotiationResult{
        RuntimeNegotiationProfile{
            QStringLiteral("runtime-negotiation-profile-1"),
            QStringLiteral("Metadata-Only Runtime Negotiation"),
            true,
            QStringLiteral(
                "Metadata-only negotiation profile; no runtime capability is activated."),
        },
        capabilities_,
        QStringLiteral("Metadata-only runtime negotiation: %1 capabilities described, %2 enabled "
                       "as safety metadata.")
            .arg(capabilities_.size())
            .arg(enabledRuntimeCapabilitySummaries(capabilities_).size()),
    };
}

} // namespace sentinel::core
