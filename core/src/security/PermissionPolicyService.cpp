// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/security/PermissionPolicyService.h"

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
         QStringLiteral("Filesystem requests carry canonical resource paths."),
         QStringLiteral("External paths require a matching runtime grant; sensitive paths remain "
                        "blocked by the resource guard.")},
        {QStringLiteral("tool-execution"), QStringLiteral("Tool Execution"),
         QStringLiteral("Registered built-in, MCP, and plugin invocations."),
         QStringLiteral("Calls pass argument validation, authorization, approval where required, "
                        "resource checks, and sandbox evaluation.")},
        {QStringLiteral("agent-execution"), QStringLiteral("Agent Execution"),
         QStringLiteral("AgentRuntime sessions and delegated tasks."),
         QStringLiteral("Subagents use a restricted tool set and do not inherit parent session "
                        "grants.")},
        {QStringLiteral("voice-capture"), QStringLiteral("Voice Capture"),
         QStringLiteral("Audio input and transcription tools."),
         QStringLiteral("Sentinel authorization is separate from operating-system microphone "
                        "permission.")},
        {QStringLiteral("voice-playback"), QStringLiteral("Voice Playback"),
         QStringLiteral("Speech synthesis and audio output tools."),
         QStringLiteral("Sentinel authorization is separate from operating-system audio policy.")},
        {QStringLiteral("cloud-provider-access"), QStringLiteral("Cloud Provider Access"),
         QStringLiteral("ModelService resolves configured providers and credentials."),
         QStringLiteral("Provider access requires a resolved provider and user-configured "
                        "credentials.")},
        {QStringLiteral("filesystem-write"), QStringLiteral("Filesystem Write"),
         QStringLiteral("Descriptor requirements distinguish filesystem read, write, and delete."),
         QStringLiteral("PathGuard and ExternalDirectoryGate remain mandatory after approval.")},
        {QStringLiteral("subprocess-execution"), QStringLiteral("Subprocess Execution"),
         QStringLiteral("Process execution uses ProcessExecutor and command security analysis."),
         QStringLiteral("Approval and sandbox evaluation remain separate from command analysis.")},
        {QStringLiteral("memory-commit"), QStringLiteral("Memory Commit"),
         QStringLiteral("Memory search is declared as a semantic read request."),
         QStringLiteral("Memory persistence remains behind IMemoryStore.")},
        {QStringLiteral("context-injection"), QStringLiteral("Context Injection"),
         QStringLiteral("Conversation and agent operations declare their own authorization semantics."),
         QStringLiteral("Model provider identity does not alter authorization decisions.")},
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
        const bool policyApplies = domain.id == QStringLiteral("tool-execution");
        const auto grant =
            policyApplies
                ? (state == QStringLiteral("Disabled") ? QStringLiteral("Execution grant: denied")
                   : state == QStringLiteral("Ask Every Time")
                       ? QStringLiteral("Execution grant: requires approval")
                       : QStringLiteral("Execution grant: allowed"))
                : QStringLiteral("Execution decision: descriptor, risk, and resource policy");
        summaries.append({
            domain.id,
            domain.name,
            policyApplies ? state : QStringLiteral("Descriptor based"),
            domain.summary,
            domain.safetyBoundary,
            {
                QStringLiteral("Domain: %1").arg(domain.name),
                policyApplies ? QStringLiteral("Policy state: %1").arg(state)
                              : QStringLiteral("Policy state: descriptor and risk metadata"),
                grant,
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
        QStringLiteral("The default permission state governs ExternalService invocations. Other "
                       "operations use descriptor requirements, risk policy, and explicit grants."),
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

bool PermissionPolicyService::allowsAuthorization(const AuthorizationRequest& request,
                                                   const QString& defaultState,
                                                   bool explicitlyApproved) const {
    const auto effect = defaultEffect(request, defaultState);
    return effect == PermissionEffect::Allow ||
           (effect == PermissionEffect::Ask && explicitlyApproved);
}

PermissionEffect PermissionPolicyService::defaultEffect(const AuthorizationRequest& request,
                                                         const QString& defaultState) const {
    if (request.domain != SecurityDomain::ExternalService)
        return PermissionEffect::Allow;
    switch (stateFromName(defaultState)) {
    case PermissionPolicyState::Disabled: return PermissionEffect::Deny;
    case PermissionPolicyState::AskEveryTime: return PermissionEffect::Ask;
    case PermissionPolicyState::Trusted:
    case PermissionPolicyState::Enabled: return PermissionEffect::Allow;
    }
    return PermissionEffect::Deny;
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
