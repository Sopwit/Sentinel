#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <memory>
#include <optional>

namespace sentinel::core {

enum class CredentialStoreStatus : std::uint8_t {
    Disabled,
    Unavailable,
    Ready,
};

enum class CredentialStoreBackend : std::uint8_t {
    MacOSKeychain,
    WindowsCredentialManager,
    LinuxSecretService,
    InMemoryTest,
    LocalUnavailableFallback,
};

enum class CredentialStoreReadiness : std::uint8_t {
    Placeholder,
    RequiresFutureImplementation,
    TestOnlyReady,
    DisabledFallback,
};

enum class CredentialStoreCapability : std::uint8_t {
    ReadinessMetadata,
    StoreSecret,
    RemoveSecret,
    TestCredential,
};

enum class CredentialStoreAction : std::uint8_t {
    AddApiKey,
    UpdateApiKey,
    RemoveApiKey,
};

enum class CredentialBackendOperation : std::uint8_t {
    Store,
    Read,
    Delete,
    Contains,
};

struct CredentialKey {
    QString providerId;
    QString credentialName = QStringLiteral("apiKey");

    QString normalizedProviderId() const;
    QString normalizedCredentialName() const;
    QString storageKey() const;
    bool isValid() const;
};

struct CredentialStoreSafetyReport {
    bool plaintextPersistenceAllowed = false;
    bool secretLoggingAllowed = false;
    bool rawSecretExposureAllowed = false;
    bool automaticProviderCallsAllowed = false;
    bool userExplicitOnly = true;
    bool providerScopedCredentials = true;
    bool osSecretStorePreferred = true;
    bool disabledBackendFallback = true;
    QString summary;
    QStringList checks;
};

struct CredentialStoreTrace {
    CredentialStoreBackend backend = CredentialStoreBackend::LocalUnavailableFallback;
    CredentialStoreReadiness readiness = CredentialStoreReadiness::DisabledFallback;
    bool currentPlatform = false;
    bool available = false;
    QString summary;
};

struct CredentialStoreSummary {
    CredentialStoreStatus status = CredentialStoreStatus::Disabled;
    CredentialStoreBackend preferredBackend = CredentialStoreBackend::LocalUnavailableFallback;
    CredentialStoreReadiness readiness = CredentialStoreReadiness::DisabledFallback;
    bool secretPersistenceEnabled = false;
    bool plaintextPersistenceAllowed = false;
    bool providerExecutionEnabled = false;
    QString summary;
    QString backendSummary;
    QString safetySummary;
};

struct CredentialBackendResult {
    CredentialBackendOperation operation = CredentialBackendOperation::Store;
    CredentialStoreBackend backend = CredentialStoreBackend::LocalUnavailableFallback;
    bool succeeded = false;
    bool mutatedState = false;
    bool secretReturned = false;
    QString providerId;
    QString credentialName;
    QString summary;
};

struct CredentialReadResult {
    CredentialBackendResult result;
    std::optional<QString> secret;
};

struct CredentialStoreResult {
    CredentialStoreAction action = CredentialStoreAction::AddApiKey;
    bool succeeded = false;
    bool mutatedState = false;
    bool executionAttempted = false;
    QString summary;
};

QString credentialStoreStatusName(CredentialStoreStatus status);
QString credentialStoreBackendName(CredentialStoreBackend backend);
QString credentialStoreReadinessName(CredentialStoreReadiness readiness);
QString credentialStoreCapabilityName(CredentialStoreCapability capability);
QString credentialStoreActionName(CredentialStoreAction action);
QString credentialBackendOperationName(CredentialBackendOperation operation);
QString credentialStoreTraceSummary(const CredentialStoreTrace& trace);
QString credentialStoreResultSummary(const CredentialStoreResult& result);
QString credentialBackendResultSummary(const CredentialBackendResult& result);

class ICredentialBackend {
public:
    virtual ~ICredentialBackend() = default;

    virtual CredentialStoreBackend backend() const = 0;
    virtual CredentialStoreSummary summary() const = 0;
    virtual CredentialBackendResult storeCredential(const CredentialKey& key,
                                                    const QString& secret) = 0;
    virtual CredentialReadResult readCredential(const CredentialKey& key) const = 0;
    virtual CredentialBackendResult deleteCredential(const CredentialKey& key) = 0;
    virtual CredentialBackendResult containsCredential(const CredentialKey& key) const = 0;
};

class DisabledCredentialBackend final : public ICredentialBackend {
public:
    CredentialStoreBackend backend() const override;
    CredentialStoreSummary summary() const override;
    CredentialBackendResult storeCredential(const CredentialKey& key,
                                            const QString& secret) override;
    CredentialReadResult readCredential(const CredentialKey& key) const override;
    CredentialBackendResult deleteCredential(const CredentialKey& key) override;
    CredentialBackendResult containsCredential(const CredentialKey& key) const override;
};

class PlaceholderCredentialBackend final : public ICredentialBackend {
public:
    explicit PlaceholderCredentialBackend(CredentialStoreBackend backend);

    CredentialStoreBackend backend() const override;
    CredentialStoreSummary summary() const override;
    CredentialBackendResult storeCredential(const CredentialKey& key,
                                            const QString& secret) override;
    CredentialReadResult readCredential(const CredentialKey& key) const override;
    CredentialBackendResult deleteCredential(const CredentialKey& key) override;
    CredentialBackendResult containsCredential(const CredentialKey& key) const override;

private:
    CredentialStoreBackend backend_ = CredentialStoreBackend::LocalUnavailableFallback;
};

class InMemoryCredentialBackend final : public ICredentialBackend {
public:
    CredentialStoreBackend backend() const override;
    CredentialStoreSummary summary() const override;
    CredentialBackendResult storeCredential(const CredentialKey& key,
                                            const QString& secret) override;
    CredentialReadResult readCredential(const CredentialKey& key) const override;
    CredentialBackendResult deleteCredential(const CredentialKey& key) override;
    CredentialBackendResult containsCredential(const CredentialKey& key) const override;

private:
    QHash<QString, QString> credentials_;
};

class CredentialStore final {
public:
    CredentialStore();
    explicit CredentialStore(std::shared_ptr<ICredentialBackend> backend);

    CredentialStoreSummary summary() const;
    CredentialStoreSafetyReport safetyReport() const;
    QList<CredentialStoreTrace> traces() const;
    QStringList traceSummaries() const;
    QStringList capabilitySummaries() const;
    CredentialStoreResult performDisabledAction(CredentialStoreAction action) const;
    CredentialBackendResult storeCredential(const CredentialKey& key, const QString& secret);
    CredentialReadResult readCredential(const CredentialKey& key) const;
    CredentialBackendResult deleteCredential(const CredentialKey& key);
    CredentialBackendResult containsCredential(const CredentialKey& key) const;

private:
    std::shared_ptr<ICredentialBackend> backend_;
    QList<CredentialStoreTrace> traces_;
};

CredentialStore defaultCredentialStore();
CredentialStore inMemoryTestCredentialStore();

} // namespace sentinel::core
