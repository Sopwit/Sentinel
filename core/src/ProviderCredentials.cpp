#include "sentinel/core/ProviderCredentials.h"

namespace sentinel::core {

namespace {

ProviderCredentialRequirement unknownRequirement(const QString& providerId) {
    const auto normalized = providerId.trimmed().toLower();
    return ProviderCredentialRequirement{
        normalized.isEmpty() ? QStringLiteral("unknown-provider") : normalized,
        QStringLiteral("Unknown Provider"),
        ProviderCredentialScope::CloudApiPlaceholder,
        ProviderCredentialPolicy::Refused,
        ProviderCredentialStatus::Refused,
        false,
        false,
        QStringLiteral("Unknown provider credentials are refused and cannot enable execution."),
    };
}

ProviderCredentialSafetyReport safetyReportForRequirement(
    const ProviderCredentialRequirement& requirement) {
    return ProviderCredentialSafetyReport{
        requirement.providerId,
        false,
        false,
        false,
        false,
        false,
        QStringLiteral("%1 credential safety: no plaintext storage, no secret logging, no cloud "
                       "requests, no fallback routing, and no background discovery.")
            .arg(requirement.displayName),
        {
            QStringLiteral("API key values are not stored by this phase."),
            QStringLiteral("Credential metadata exposes only configured, missing, or refused "
                           "state."),
            QStringLiteral("Cloud/API requests remain disabled."),
            QStringLiteral("No provider fallback routing or autonomous provider switching."),
        },
    };
}

} // namespace

QString providerCredentialPolicyName(ProviderCredentialPolicy policy) {
    switch (policy) {
    case ProviderCredentialPolicy::NotRequired:
        return QStringLiteral("notRequired");
    case ProviderCredentialPolicy::RequiredPlaceholderOnly:
        return QStringLiteral("requiredPlaceholderOnly");
    case ProviderCredentialPolicy::Refused:
        return QStringLiteral("refused");
    }
    return QStringLiteral("refused");
}

QString providerCredentialStatusName(ProviderCredentialStatus status) {
    switch (status) {
    case ProviderCredentialStatus::Configured:
        return QStringLiteral("configured");
    case ProviderCredentialStatus::Missing:
        return QStringLiteral("missing");
    case ProviderCredentialStatus::Refused:
        return QStringLiteral("refused");
    case ProviderCredentialStatus::NotRequired:
        return QStringLiteral("notRequired");
    }
    return QStringLiteral("missing");
}

QString providerCredentialScopeName(ProviderCredentialScope scope) {
    switch (scope) {
    case ProviderCredentialScope::LocalRuntime:
        return QStringLiteral("localRuntime");
    case ProviderCredentialScope::CloudApiPlaceholder:
        return QStringLiteral("cloudApiPlaceholder");
    }
    return QStringLiteral("cloudApiPlaceholder");
}

QString providerCredentialRequirementSummary(
    const ProviderCredentialRequirement& requirement) {
    return QStringLiteral("%1 (%2): %3 / %4 / %5 / API keys not stored")
        .arg(requirement.displayName, requirement.providerId,
             providerCredentialScopeName(requirement.scope),
             providerCredentialStatusName(requirement.status),
             requirement.executionEnabled ? QStringLiteral("execution enabled")
                                          : QStringLiteral("execution disabled"));
}

QString providerCredentialSafetySummary(const ProviderCredentialSafetyReport& report) {
    return QStringLiteral("%1: plaintextStorage=%2 / secretLogging=%3 / cloudRequests=%4 / "
                          "fallbackRouting=%5 / backgroundDiscovery=%6")
        .arg(report.providerId,
             report.plaintextStorageAllowed ? QStringLiteral("allowed")
                                            : QStringLiteral("refused"),
             report.secretLoggingAllowed ? QStringLiteral("allowed") : QStringLiteral("refused"),
             report.cloudRequestsAllowed ? QStringLiteral("allowed") : QStringLiteral("refused"),
             report.fallbackRoutingAllowed ? QStringLiteral("allowed")
                                           : QStringLiteral("refused"),
             report.backgroundDiscoveryAllowed ? QStringLiteral("allowed")
                                               : QStringLiteral("refused"));
}

QString providerCredentialReadinessSummary(const ProviderCredentialReadiness& readiness) {
    return QStringLiteral("%1: %2 / %3 / API key values not stored")
        .arg(readiness.displayName, providerCredentialStatusName(readiness.status),
             readiness.executionAllowed ? QStringLiteral("execution allowed")
                                        : QStringLiteral("execution disabled"));
}

ProviderCredentialRegistry::ProviderCredentialRegistry(
    QList<ProviderCredentialRequirement> requirements)
    : requirements_(std::move(requirements)) {}

QList<ProviderCredentialRequirement> ProviderCredentialRegistry::requirements() const {
    return requirements_;
}

ProviderCredentialRequirement ProviderCredentialRegistry::requirementForProvider(
    const QString& providerId) const {
    const auto normalized = providerId.trimmed().toLower();
    for (const auto& requirement : requirements_) {
        if (requirement.providerId == normalized) {
            return requirement;
        }
    }
    return unknownRequirement(normalized);
}

ProviderCredentialReadiness ProviderCredentialRegistry::readinessForProvider(
    const QString& providerId) const {
    const auto requirement = requirementForProvider(providerId);
    const auto configured = requirement.status == ProviderCredentialStatus::Configured ||
                            requirement.status == ProviderCredentialStatus::NotRequired;
    return ProviderCredentialReadiness{
        requirement.providerId,
        requirement.displayName,
        requirement.status,
        configured,
        requirement.placeholderReady,
        false,
        requirement.summary.isEmpty() ? providerCredentialRequirementSummary(requirement)
                                      : requirement.summary,
    };
}

QList<ProviderCredentialReadiness> ProviderCredentialRegistry::readinessEntries() const {
    QList<ProviderCredentialReadiness> entries;
    for (const auto& requirement : requirements_) {
        entries.append(readinessForProvider(requirement.providerId));
    }
    return entries;
}

ProviderCredentialSafetyReport ProviderCredentialRegistry::safetyReportForProvider(
    const QString& providerId) const {
    return safetyReportForRequirement(requirementForProvider(providerId));
}

ProviderCredentialSummary ProviderCredentialRegistry::summary() const {
    ProviderCredentialSummary result;
    result.providerCount = static_cast<int>(requirements_.size());
    for (const auto& requirement : requirements_) {
        if (requirement.status == ProviderCredentialStatus::Configured ||
            requirement.status == ProviderCredentialStatus::NotRequired) {
            ++result.configuredCount;
        } else if (requirement.status == ProviderCredentialStatus::Missing) {
            ++result.missingCount;
        } else if (requirement.status == ProviderCredentialStatus::Refused) {
            ++result.refusedCount;
        }
        if (requirement.placeholderReady) {
            ++result.placeholderReadyCount;
        }
    }
    result.status = result.missingCount > 0 ? ProviderCredentialStatus::Missing
                                            : ProviderCredentialStatus::Configured;
    result.summary =
        QStringLiteral("%1 provider credential records: %2 configured/not-required, %3 missing, "
                       "%4 refused. API key values are not stored.")
            .arg(result.providerCount)
            .arg(result.configuredCount)
            .arg(result.missingCount)
            .arg(result.refusedCount);
    return result;
}

QStringList ProviderCredentialRegistry::requirementSummaries() const {
    QStringList summaries;
    for (const auto& requirement : requirements_) {
        summaries.append(providerCredentialRequirementSummary(requirement));
    }
    return summaries;
}

QStringList ProviderCredentialRegistry::readinessSummaries() const {
    QStringList summaries;
    for (const auto& readiness : readinessEntries()) {
        summaries.append(providerCredentialReadinessSummary(readiness));
    }
    return summaries;
}

QStringList ProviderCredentialRegistry::safetySummaries() const {
    QStringList summaries;
    for (const auto& requirement : requirements_) {
        summaries.append(providerCredentialSafetySummary(
            safetyReportForProvider(requirement.providerId)));
    }
    return summaries;
}

ProviderCredentialRegistry defaultProviderCredentialRegistry() {
    return ProviderCredentialRegistry{
        {
            ProviderCredentialRequirement{
                QStringLiteral("ollama"),
                QStringLiteral("Local Ollama"),
                ProviderCredentialScope::LocalRuntime,
                ProviderCredentialPolicy::NotRequired,
                ProviderCredentialStatus::NotRequired,
                true,
                false,
                QStringLiteral("Local Ollama does not require an API key."),
            },
            ProviderCredentialRequirement{
                QStringLiteral("openai-compatible"),
                QStringLiteral("OpenAI-Compatible API"),
                ProviderCredentialScope::CloudApiPlaceholder,
                ProviderCredentialPolicy::RequiredPlaceholderOnly,
                ProviderCredentialStatus::Missing,
                true,
                false,
                QStringLiteral("OpenAI-compatible credentials are not configured; API key values "
                               "are not stored."),
            },
            ProviderCredentialRequirement{
                QStringLiteral("claude"),
                QStringLiteral("Claude API"),
                ProviderCredentialScope::CloudApiPlaceholder,
                ProviderCredentialPolicy::RequiredPlaceholderOnly,
                ProviderCredentialStatus::Missing,
                true,
                false,
                QStringLiteral("Claude credentials are not configured; API key values are not "
                               "stored."),
            },
            ProviderCredentialRequirement{
                QStringLiteral("gemini"),
                QStringLiteral("Gemini API"),
                ProviderCredentialScope::CloudApiPlaceholder,
                ProviderCredentialPolicy::RequiredPlaceholderOnly,
                ProviderCredentialStatus::Missing,
                true,
                false,
                QStringLiteral("Gemini credentials are not configured; API key values are not "
                               "stored."),
            },
        },
    };
}

} // namespace sentinel::core
