#include "sentinel/core/PermissionPolicyService.h"

namespace sentinel::core {

namespace {

struct PermissionPolicyDomain {
    QString id;
    QString name;
    QString summary;
    QString safetyBoundary;
};

QList<PermissionPolicyDomain> permissionDomains() {
    return {
        {QStringLiteral("workspace-access"), QStringLiteral("Workspace Access"),
         QStringLiteral("Future named workspace/path scopes."),
         QStringLiteral("No folder selection, folder reading, scanning, indexing, or workspace "
                        "prompt context is enabled.")},
        {QStringLiteral("tool-execution"), QStringLiteral("Tool Execution"),
         QStringLiteral("Future gateway-mediated tool calls."),
         QStringLiteral("No tool, plugin, MCP, or external command execution is enabled.")},
        {QStringLiteral("agent-execution"), QStringLiteral("Agent Execution"),
         QStringLiteral("Future foreground agent runtime authority."),
         QStringLiteral("No autonomous agent runtime, background loop, or delegated action is "
                        "enabled.")},
        {QStringLiteral("voice-capture"), QStringLiteral("Voice Capture"),
         QStringLiteral("Future microphone and STT authority."),
         QStringLiteral("No microphone capture, recording session, or STT activation is enabled.")},
        {QStringLiteral("voice-playback"), QStringLiteral("Voice Playback"),
         QStringLiteral("Future playback and TTS authority."),
         QStringLiteral("No audio playback, speaker output, or TTS activation is enabled.")},
        {QStringLiteral("cloud-provider-access"), QStringLiteral("Cloud Provider Access"),
         QStringLiteral("Future opt-in cloud/API provider authority."),
         QStringLiteral("No cloud request, API-key use, provider test call, or remote model lookup "
                        "is enabled.")},
        {QStringLiteral("filesystem-write"), QStringLiteral("Filesystem Write"),
         QStringLiteral("Future explicit local write authority."),
         QStringLiteral("No filesystem mutation, export write, patch write, or generated file "
                        "write is enabled by policy.")},
        {QStringLiteral("subprocess-execution"), QStringLiteral("Subprocess Execution"),
         QStringLiteral("Future process-launch authority."),
         QStringLiteral("No subprocess launch, shell command, binary execution, or runtime process "
                        "startup is enabled.")},
        {QStringLiteral("memory-commit"), QStringLiteral("Memory Commit"),
         QStringLiteral("Future committed-memory authority."),
         QStringLiteral("No automatic committed-memory write is enabled; existing explicit user "
                        "memory actions remain unchanged.")},
        {QStringLiteral("context-injection"), QStringLiteral("Context Injection"),
         QStringLiteral("Future prompt-context authority."),
         QStringLiteral("No hidden prompt mutation, workspace-derived prompt block, or automatic "
                        "context injection is enabled by this policy.")},
    };
}

PermissionPolicyState stateFromName(const QString& state) {
    const auto normalized = state.trimmed().toLower();
    if (normalized == QStringLiteral("ask-every-time") ||
        normalized == QStringLiteral("ask every time")) {
        return PermissionPolicyState::AskEveryTime;
    }
    if (normalized == QStringLiteral("trusted")) {
        return PermissionPolicyState::Trusted;
    }
    if (normalized == QStringLiteral("enabled")) {
        return PermissionPolicyState::Enabled;
    }
    return PermissionPolicyState::Disabled;
}

} // namespace

QList<PermissionPolicySummary>
PermissionPolicyService::permissionSummaries(const QString& defaultState) const {
    const auto state = permissionPolicyStateName(stateFromName(defaultState));
    QList<PermissionPolicySummary> summaries;
    for (const auto& domain : permissionDomains()) {
        summaries.append({
            domain.id,
            domain.name,
            state,
            domain.summary,
            domain.safetyBoundary,
            {
                QStringLiteral("Domain: %1").arg(domain.name),
                QStringLiteral("Policy state: %1").arg(state),
                QStringLiteral("Execution grant: allowed"),
                domain.safetyBoundary,
            },
        });
    }
    return summaries;
}

PermissionPolicyRegistrySummary
PermissionPolicyService::registrySummary(const QString& defaultState) const {
    const auto state = permissionPolicyStateName(stateFromName(defaultState));
    QStringList domainSummaries;
    QStringList developerDiagnostics;
    for (const auto& summary : permissionSummaries(state)) {
        domainSummaries.append(permissionPolicySummaryLine(summary));
        developerDiagnostics.append(
            QStringLiteral("%1 [%2]: %3")
                .arg(summary.domainName, summary.state, summary.safetyBoundary));
    }

    return {
        QStringLiteral("Operational"),
        QStringLiteral("Permission policy registry is operational; user-controlled authority "
                       "states govern active tool execution and environment access."),
        state,
        permissionStateLabels(),
        domainSummaries,
        developerDiagnostics,
    };
}

QStringList PermissionPolicyService::permissionDomainIds() const {
    QStringList ids;
    for (const auto& domain : permissionDomains()) {
        ids.append(domain.id);
    }
    return ids;
}

QStringList PermissionPolicyService::permissionDomainNames() const {
    QStringList names;
    for (const auto& domain : permissionDomains()) {
        names.append(domain.name);
    }
    return names;
}

QStringList PermissionPolicyService::permissionStateLabels() const {
    return {
        permissionPolicyStateName(PermissionPolicyState::Disabled),
        permissionPolicyStateName(PermissionPolicyState::AskEveryTime),
        permissionPolicyStateName(PermissionPolicyState::Trusted),
        permissionPolicyStateName(PermissionPolicyState::Enabled),
    };
}

QString PermissionPolicyService::normalizedState(const QString& state) const {
    return permissionPolicyStateName(stateFromName(state));
}

QString permissionPolicyStateName(PermissionPolicyState state) {
    switch (state) {
    case PermissionPolicyState::Disabled:
        return QStringLiteral("Disabled");
    case PermissionPolicyState::AskEveryTime:
        return QStringLiteral("Ask Every Time");
    case PermissionPolicyState::Trusted:
        return QStringLiteral("Trusted");
    case PermissionPolicyState::Enabled:
        return QStringLiteral("Enabled");
    }
    return QStringLiteral("Disabled");
}

QString permissionPolicySummaryLine(const PermissionPolicySummary& summary) {
    return QStringLiteral("%1 / %2 / %3").arg(summary.domainName, summary.state, summary.summary);
}

} // namespace sentinel::core
