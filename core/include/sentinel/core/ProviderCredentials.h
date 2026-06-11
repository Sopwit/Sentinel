#pragma once

#include "sentinel/core/CredentialStore.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <utility>

namespace sentinel::core {

enum class ProviderCredentialPolicy : std::uint8_t {
    NotRequired,
    RequiredPlaceholderOnly,
    Refused,
};

enum class ProviderCredentialStatus : std::uint8_t {
    Configured,
    Missing,
    Refused,
    NotRequired,
};

enum class ProviderCredentialScope : std::uint8_t {
    LocalRuntime,
    CloudApiPlaceholder,
};

QString providerCredentialPolicyName(ProviderCredentialPolicy policy);
QString providerCredentialStatusName(ProviderCredentialStatus status);
QString providerCredentialScopeName(ProviderCredentialScope scope);

struct ProviderCredentialRequirement {
    QString providerId;
    QString displayName;
    ProviderCredentialScope scope = ProviderCredentialScope::CloudApiPlaceholder;
    ProviderCredentialPolicy policy = ProviderCredentialPolicy::RequiredPlaceholderOnly;
    ProviderCredentialStatus status = ProviderCredentialStatus::Missing;
    bool placeholderReady = false;
    bool executionEnabled = false;
    CredentialStoreReadiness storeReadiness = CredentialStoreReadiness::DisabledFallback;
    QString backendSummary;
    QString summary;
};

struct ProviderCredentialSafetyReport {
    QString providerId;
    bool plaintextStorageAllowed = false;
    bool secretLoggingAllowed = false;
    bool cloudRequestsAllowed = false;
    bool backgroundDiscoveryAllowed = false;
    bool fallbackRoutingAllowed = false;
    QString summary;
    QStringList checks;
};

struct ProviderCredentialReadiness {
    QString providerId;
    QString displayName;
    ProviderCredentialStatus status = ProviderCredentialStatus::Missing;
    bool configured = false;
    bool placeholderReady = false;
    bool executionAllowed = false;
    QString backendSummary;
    QString summary;
};

struct ProviderCredentialSummary {
    ProviderCredentialStatus status = ProviderCredentialStatus::Missing;
    int providerCount = 0;
    int configuredCount = 0;
    int missingCount = 0;
    int refusedCount = 0;
    int placeholderReadyCount = 0;
    QString summary;
};

QString providerCredentialRequirementSummary(const ProviderCredentialRequirement& requirement);
QString providerCredentialSafetySummary(const ProviderCredentialSafetyReport& report);
QString providerCredentialReadinessSummary(const ProviderCredentialReadiness& readiness);

class ProviderCredentialRegistry final {
public:
    explicit ProviderCredentialRegistry(
        QList<ProviderCredentialRequirement> requirements,
        CredentialStoreSummary credentialStoreSummary = defaultCredentialStore().summary());

    QList<ProviderCredentialRequirement> requirements() const;
    ProviderCredentialRequirement requirementForProvider(const QString& providerId) const;
    ProviderCredentialReadiness readinessForProvider(const QString& providerId) const;
    QList<ProviderCredentialReadiness> readinessEntries() const;
    ProviderCredentialSafetyReport safetyReportForProvider(const QString& providerId) const;
    ProviderCredentialSummary summary() const;
    QStringList requirementSummaries() const;
    QStringList readinessSummaries() const;
    QStringList safetySummaries() const;

private:
    QList<ProviderCredentialRequirement> requirements_;
    CredentialStoreSummary credentialStoreSummary_;
};

ProviderCredentialRegistry defaultProviderCredentialRegistry(
    CredentialStoreSummary credentialStoreSummary = defaultCredentialStore().summary());

} // namespace sentinel::core
