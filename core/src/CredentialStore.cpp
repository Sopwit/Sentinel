#include "sentinel/core/CredentialStore.h"

#include <QCoreApplication>
#include <QProcess>
#include <QtGlobal>

#if defined(Q_OS_WIN)
// clang-format off
#include <windows.h>
#include <wincred.h>
// clang-format on
#endif

#include <utility>

namespace sentinel::core {

namespace {

CredentialStoreBackend preferredBackendForPlatform() {
#if defined(Q_OS_MACOS)
    return CredentialStoreBackend::MacOSKeychain;
#elif defined(Q_OS_WIN)
    return CredentialStoreBackend::WindowsCredentialManager;
#elif defined(Q_OS_LINUX)
    return CredentialStoreBackend::LinuxSecretService;
#else
    return CredentialStoreBackend::LocalUnavailableFallback;
#endif
}

bool isCurrentPlatformBackend(CredentialStoreBackend backend) {
    return backend == preferredBackendForPlatform();
}

QString backendDisplayName(CredentialStoreBackend backend) {
    switch (backend) {
    case CredentialStoreBackend::MacOSKeychain:
        return QStringLiteral("macOS Keychain");
    case CredentialStoreBackend::WindowsCredentialManager:
        return QStringLiteral("Windows Credential Manager");
    case CredentialStoreBackend::LinuxSecretService:
        return QStringLiteral("Linux Secret Service");
    case CredentialStoreBackend::InMemoryTest:
        return QStringLiteral("In-memory test backend");
    case CredentialStoreBackend::LocalUnavailableFallback:
        return QStringLiteral("Local unavailable fallback");
    }
    return QStringLiteral("Local unavailable fallback");
}

CredentialStoreTrace traceForBackend(CredentialStoreBackend backend) {
    const auto currentPlatform = isCurrentPlatformBackend(backend);
    const auto fallback = backend == CredentialStoreBackend::LocalUnavailableFallback;
    const auto testOnly = backend == CredentialStoreBackend::InMemoryTest;
    const auto readiness = fallback   ? CredentialStoreReadiness::DisabledFallback
                           : testOnly ? CredentialStoreReadiness::TestOnlyReady
                                      : CredentialStoreReadiness::RequiresFutureImplementation;
    const auto available = testOnly;
    return CredentialStoreTrace{
        backend,
        readiness,
        currentPlatform && !testOnly,
        available,
        QStringLiteral("%1: %2 / %3 / non-persistent readiness metadata only")
            .arg(backendDisplayName(backend), credentialStoreReadinessName(readiness),
                 currentPlatform && !testOnly ? QStringLiteral("current platform")
                                              : QStringLiteral("not current platform")),
    };
}

CredentialBackendResult refusedBackendResult(CredentialBackendOperation operation,
                                             CredentialStoreBackend backend,
                                             const CredentialKey& key, QString reason) {
    return CredentialBackendResult{
        operation,
        backend,
        false,
        false,
        false,
        key.normalizedProviderId(),
        key.normalizedCredentialName(),
        std::move(reason),
    };
}

CredentialBackendResult invalidKeyResult(CredentialBackendOperation operation,
                                         CredentialStoreBackend backend, const CredentialKey& key) {
    return refusedBackendResult(
        operation, backend, key,
        QStringLiteral("%1 refused: provider id and credential name are required; no credential "
                       "state changed.")
            .arg(credentialBackendOperationName(operation)));
}

CredentialStoreSummary summaryForBackend(CredentialStoreBackend backend,
                                         CredentialStoreStatus status,
                                         CredentialStoreReadiness readiness,
                                         bool secretPersistenceEnabled, QString summary,
                                         QString backendSummary) {
    return CredentialStoreSummary{
        status,
        backend,
        readiness,
        secretPersistenceEnabled,
        false,
        false,
        std::move(summary),
        std::move(backendSummary),
        QStringLiteral("Credential policy: no plaintext persistence, no logs, no raw secret "
                       "exposure, no automatic provider calls, user-explicit only, "
                       "provider-scoped, OS secret-store preferred, disabled fallback."),
    };
}

} // namespace

QString CredentialKey::normalizedProviderId() const {
    return providerId.trimmed().toLower();
}

QString CredentialKey::normalizedCredentialName() const {
    const auto normalized = credentialName.trimmed();
    return normalized.isEmpty() ? QStringLiteral("apiKey") : normalized;
}

QString CredentialKey::storageKey() const {
    return normalizedProviderId() + QStringLiteral(":") + normalizedCredentialName();
}

bool CredentialKey::isValid() const {
    return !normalizedProviderId().isEmpty() && !normalizedCredentialName().isEmpty();
}

QString credentialStoreStatusName(CredentialStoreStatus status) {
    switch (status) {
    case CredentialStoreStatus::Disabled:
        return QStringLiteral("disabled");
    case CredentialStoreStatus::Unavailable:
        return QStringLiteral("unavailable");
    case CredentialStoreStatus::Ready:
        return QStringLiteral("ready");
    }
    return QStringLiteral("disabled");
}

QString credentialStoreBackendName(CredentialStoreBackend backend) {
    switch (backend) {
    case CredentialStoreBackend::MacOSKeychain:
        return QStringLiteral("macosKeychain");
    case CredentialStoreBackend::WindowsCredentialManager:
        return QStringLiteral("windowsCredentialManager");
    case CredentialStoreBackend::LinuxSecretService:
        return QStringLiteral("linuxSecretService");
    case CredentialStoreBackend::InMemoryTest:
        return QStringLiteral("inMemoryTest");
    case CredentialStoreBackend::LocalUnavailableFallback:
        return QStringLiteral("localUnavailableFallback");
    }
    return QStringLiteral("localUnavailableFallback");
}

QString credentialStoreReadinessName(CredentialStoreReadiness readiness) {
    switch (readiness) {
    case CredentialStoreReadiness::Placeholder:
        return QStringLiteral("placeholder");
    case CredentialStoreReadiness::RequiresFutureImplementation:
        return QStringLiteral("requiresFutureImplementation");
    case CredentialStoreReadiness::TestOnlyReady:
        return QStringLiteral("testOnlyReady");
    case CredentialStoreReadiness::DisabledFallback:
        return QStringLiteral("disabledFallback");
    }
    return QStringLiteral("disabledFallback");
}

QString credentialStoreCapabilityName(CredentialStoreCapability capability) {
    switch (capability) {
    case CredentialStoreCapability::ReadinessMetadata:
        return QStringLiteral("readinessMetadata");
    case CredentialStoreCapability::StoreSecret:
        return QStringLiteral("storeSecret");
    case CredentialStoreCapability::RemoveSecret:
        return QStringLiteral("removeSecret");
    case CredentialStoreCapability::TestCredential:
        return QStringLiteral("testCredential");
    }
    return QStringLiteral("readinessMetadata");
}

QString credentialStoreActionName(CredentialStoreAction action) {
    switch (action) {
    case CredentialStoreAction::AddApiKey:
        return QStringLiteral("addApiKey");
    case CredentialStoreAction::UpdateApiKey:
        return QStringLiteral("updateApiKey");
    case CredentialStoreAction::RemoveApiKey:
        return QStringLiteral("removeApiKey");
    }
    return QStringLiteral("addApiKey");
}

QString credentialBackendOperationName(CredentialBackendOperation operation) {
    switch (operation) {
    case CredentialBackendOperation::Store:
        return QStringLiteral("store");
    case CredentialBackendOperation::Read:
        return QStringLiteral("read");
    case CredentialBackendOperation::Delete:
        return QStringLiteral("delete");
    case CredentialBackendOperation::Contains:
        return QStringLiteral("contains");
    }
    return QStringLiteral("store");
}

QString credentialStoreTraceSummary(const CredentialStoreTrace& trace) {
    return QStringLiteral("%1: readiness=%2 / available=%3 / platform=%4")
        .arg(credentialStoreBackendName(trace.backend),
             credentialStoreReadinessName(trace.readiness),
             trace.available ? QStringLiteral("yes") : QStringLiteral("no"),
             trace.currentPlatform ? QStringLiteral("current") : QStringLiteral("other"));
}

QString credentialStoreResultSummary(const CredentialStoreResult& result) {
    return QStringLiteral("%1: %2 / mutated=%3 / executionAttempted=%4")
        .arg(credentialStoreActionName(result.action),
             result.succeeded ? QStringLiteral("succeeded") : QStringLiteral("disabled"),
             result.mutatedState ? QStringLiteral("yes") : QStringLiteral("no"),
             result.executionAttempted ? QStringLiteral("yes") : QStringLiteral("no"));
}

QString credentialBackendResultSummary(const CredentialBackendResult& result) {
    return QStringLiteral("%1/%2/%3: %4 / mutated=%5 / secretReturned=%6")
        .arg(credentialStoreBackendName(result.backend),
             credentialBackendOperationName(result.operation),
             result.providerId.isEmpty() ? QStringLiteral("unknown-provider") : result.providerId,
             result.succeeded ? QStringLiteral("succeeded") : QStringLiteral("refused"),
             result.mutatedState ? QStringLiteral("yes") : QStringLiteral("no"),
             result.secretReturned ? QStringLiteral("yes") : QStringLiteral("no"));
}

CredentialStoreBackend DisabledCredentialBackend::backend() const {
    return CredentialStoreBackend::LocalUnavailableFallback;
}

CredentialStoreSummary DisabledCredentialBackend::summary() const {
    const auto preferredBackend = preferredBackendForPlatform();
    const auto readiness = preferredBackend == CredentialStoreBackend::LocalUnavailableFallback
                               ? CredentialStoreReadiness::DisabledFallback
                               : CredentialStoreReadiness::RequiresFutureImplementation;
    return summaryForBackend(
        preferredBackend, CredentialStoreStatus::Disabled, readiness, false,
        QStringLiteral("Credential store disabled: no API key storage is active, no plaintext "
                       "persistence is allowed, and provider execution remains disabled."),
        QStringLiteral("%1 preferred / %2 / storage unavailable until a future implementation.")
            .arg(backendDisplayName(preferredBackend), credentialStoreReadinessName(readiness)));
}

CredentialBackendResult DisabledCredentialBackend::storeCredential(const CredentialKey& key,
                                                                   const QString& secret) {
    Q_UNUSED(secret)
    return refusedBackendResult(
        CredentialBackendOperation::Store, backend(), key,
        QStringLiteral("store refused: disabled credential backend does not persist API keys."));
}

CredentialReadResult DisabledCredentialBackend::readCredential(const CredentialKey& key) const {
    return CredentialReadResult{
        refusedBackendResult(CredentialBackendOperation::Read, backend(), key,
                             QStringLiteral("read refused: disabled credential backend has no "
                                            "stored API key value.")),
        std::nullopt,
    };
}

CredentialBackendResult DisabledCredentialBackend::deleteCredential(const CredentialKey& key) {
    return refusedBackendResult(
        CredentialBackendOperation::Delete, backend(), key,
        QStringLiteral("delete refused: disabled credential backend has no credential state."));
}

CredentialBackendResult
DisabledCredentialBackend::containsCredential(const CredentialKey& key) const {
    return refusedBackendResult(
        CredentialBackendOperation::Contains, backend(), key,
        QStringLiteral("contains refused: disabled credential backend is not configured."));
}

PlaceholderCredentialBackend::PlaceholderCredentialBackend(CredentialStoreBackend backend)
    : backend_(backend) {}

CredentialStoreBackend PlaceholderCredentialBackend::backend() const {
    return backend_;
}

CredentialStoreSummary PlaceholderCredentialBackend::summary() const {
    return summaryForBackend(
        backend_, CredentialStoreStatus::Unavailable,
        CredentialStoreReadiness::RequiresFutureImplementation, false,
        QStringLiteral("%1 placeholder unavailable: OS secret-store integration is not active.")
            .arg(backendDisplayName(backend_)),
        QStringLiteral("%1 / requiresFutureImplementation / no OS credential calls are made.")
            .arg(backendDisplayName(backend_)));
}

CredentialBackendResult PlaceholderCredentialBackend::storeCredential(const CredentialKey& key,
                                                                      const QString& secret) {
    Q_UNUSED(secret)
    return refusedBackendResult(
        CredentialBackendOperation::Store, backend_, key,
        QStringLiteral("store refused: OS credential backend is a non-executing placeholder."));
}

CredentialReadResult PlaceholderCredentialBackend::readCredential(const CredentialKey& key) const {
    return CredentialReadResult{
        refusedBackendResult(CredentialBackendOperation::Read, backend_, key,
                             QStringLiteral("read refused: OS credential backend is a "
                                            "non-executing placeholder.")),
        std::nullopt,
    };
}

CredentialBackendResult PlaceholderCredentialBackend::deleteCredential(const CredentialKey& key) {
    return refusedBackendResult(
        CredentialBackendOperation::Delete, backend_, key,
        QStringLiteral("delete refused: OS credential backend is a non-executing placeholder."));
}

CredentialBackendResult
PlaceholderCredentialBackend::containsCredential(const CredentialKey& key) const {
    return refusedBackendResult(
        CredentialBackendOperation::Contains, backend_, key,
        QStringLiteral("contains refused: OS credential backend is a non-executing placeholder."));
}

CredentialStoreBackend InMemoryCredentialBackend::backend() const {
    return CredentialStoreBackend::InMemoryTest;
}

CredentialStoreSummary InMemoryCredentialBackend::summary() const {
    return summaryForBackend(
        CredentialStoreBackend::InMemoryTest, CredentialStoreStatus::Ready,
        CredentialStoreReadiness::TestOnlyReady, true,
        QStringLiteral("Credential test backend ready: secrets are stored in memory for tests "
                       "only, never persisted, logged, or exposed to QML."),
        QStringLiteral("In-memory test backend / testOnlyReady / non-persistent test storage."));
}

CredentialBackendResult InMemoryCredentialBackend::storeCredential(const CredentialKey& key,
                                                                   const QString& secret) {
    if (!key.isValid()) {
        return invalidKeyResult(CredentialBackendOperation::Store, backend(), key);
    }
    if (secret.isEmpty()) {
        return refusedBackendResult(
            CredentialBackendOperation::Store, backend(), key,
            QStringLiteral("store refused: empty credential value was not accepted."));
    }
    credentials_.insert(key.storageKey(), secret);
    return CredentialBackendResult{
        CredentialBackendOperation::Store,
        backend(),
        true,
        true,
        false,
        key.normalizedProviderId(),
        key.normalizedCredentialName(),
        QStringLiteral("store succeeded for provider credential metadata; secret value omitted."),
    };
}

CredentialReadResult InMemoryCredentialBackend::readCredential(const CredentialKey& key) const {
    if (!key.isValid()) {
        return CredentialReadResult{
            invalidKeyResult(CredentialBackendOperation::Read, backend(), key),
            std::nullopt,
        };
    }
    const auto storageKey = key.storageKey();
    if (!credentials_.contains(storageKey)) {
        return CredentialReadResult{
            refusedBackendResult(CredentialBackendOperation::Read, backend(), key,
                                 QStringLiteral("read refused: no credential is configured for "
                                                "this provider in the test backend.")),
            std::nullopt,
        };
    }
    return CredentialReadResult{
        CredentialBackendResult{
            CredentialBackendOperation::Read,
            backend(),
            true,
            false,
            true,
            key.normalizedProviderId(),
            key.normalizedCredentialName(),
            QStringLiteral("read succeeded for provider credential metadata; secret value "
                           "returned only to core caller."),
        },
        credentials_.value(storageKey),
    };
}

CredentialBackendResult InMemoryCredentialBackend::deleteCredential(const CredentialKey& key) {
    if (!key.isValid()) {
        return invalidKeyResult(CredentialBackendOperation::Delete, backend(), key);
    }
    const auto removed = credentials_.remove(key.storageKey()) > 0;
    return CredentialBackendResult{
        CredentialBackendOperation::Delete,
        backend(),
        removed,
        removed,
        false,
        key.normalizedProviderId(),
        key.normalizedCredentialName(),
        removed ? QStringLiteral("delete succeeded for provider credential metadata.")
                : QStringLiteral("delete refused: no credential was configured."),
    };
}

CredentialBackendResult
InMemoryCredentialBackend::containsCredential(const CredentialKey& key) const {
    if (!key.isValid()) {
        return invalidKeyResult(CredentialBackendOperation::Contains, backend(), key);
    }
    const auto contains = credentials_.contains(key.storageKey());
    return CredentialBackendResult{
        CredentialBackendOperation::Contains,
        backend(),
        contains,
        false,
        false,
        key.normalizedProviderId(),
        key.normalizedCredentialName(),
        contains ? QStringLiteral("credential metadata is configured in the test backend.")
                 : QStringLiteral("credential metadata is not configured in the test backend."),
    };
}

class MacKeychainCredentialBackend final : public ICredentialBackend {
public:
    CredentialStoreBackend backend() const override {
        return CredentialStoreBackend::MacOSKeychain;
    }
    CredentialStoreSummary summary() const override {
        return summaryForBackend(
            CredentialStoreBackend::MacOSKeychain, CredentialStoreStatus::Ready,
            CredentialStoreReadiness::TestOnlyReady, true,
            QStringLiteral("macOS Keychain backend ready: credentials are saved securely in macOS "
                           "Keychain system."),
            QStringLiteral("macOS Keychain / Ready / secure persistent storage."));
    }
    CredentialBackendResult storeCredential(const CredentialKey& key,
                                            const QString& secret) override {
        if (!key.isValid()) {
            return invalidKeyResult(CredentialBackendOperation::Store, backend(), key);
        }
        QProcess process;
        QStringList arguments;
        arguments << "add-generic-password" << "-a" << "Sentinel" << "-s" << key.storageKey()
                  << "-w" << secret << "-U";
        process.start("security", arguments);
        if (!process.waitForFinished(3000) || process.exitCode() != 0) {
            return refusedBackendResult(CredentialBackendOperation::Store, backend(), key,
                                        "macOS Keychain store failed");
        }
        return CredentialBackendResult{CredentialBackendOperation::Store,
                                       backend(),
                                       true,
                                       true,
                                       false,
                                       key.normalizedProviderId(),
                                       key.normalizedCredentialName(),
                                       "Saved to macOS Keychain successfully"};
    }
    CredentialReadResult readCredential(const CredentialKey& key) const override {
        if (!key.isValid()) {
            return {invalidKeyResult(CredentialBackendOperation::Read, backend(), key),
                    std::nullopt};
        }
        QProcess process;
        QStringList arguments;
        arguments << "find-generic-password" << "-a" << "Sentinel" << "-s" << key.storageKey()
                  << "-w";
        process.start("security", arguments);
        if (!process.waitForFinished(3000) || process.exitCode() != 0) {
            return {refusedBackendResult(CredentialBackendOperation::Read, backend(), key,
                                         "macOS Keychain read failed or credential not found"),
                    std::nullopt};
        }
        QString secret = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        return {CredentialBackendResult{CredentialBackendOperation::Read, backend(), true, false,
                                        true, key.normalizedProviderId(),
                                        key.normalizedCredentialName(),
                                        "Read from macOS Keychain successfully"},
                secret};
    }
    CredentialBackendResult deleteCredential(const CredentialKey& key) override {
        if (!key.isValid()) {
            return invalidKeyResult(CredentialBackendOperation::Delete, backend(), key);
        }
        QProcess process;
        QStringList arguments;
        arguments << "delete-generic-password" << "-a" << "Sentinel" << "-s" << key.storageKey();
        process.start("security", arguments);
        if (!process.waitForFinished(3000) || process.exitCode() != 0) {
            return refusedBackendResult(CredentialBackendOperation::Delete, backend(), key,
                                        "macOS Keychain delete failed");
        }
        return CredentialBackendResult{CredentialBackendOperation::Delete,
                                       backend(),
                                       true,
                                       true,
                                       false,
                                       key.normalizedProviderId(),
                                       key.normalizedCredentialName(),
                                       "Deleted from macOS Keychain successfully"};
    }
    CredentialBackendResult containsCredential(const CredentialKey& key) const override {
        if (!key.isValid()) {
            return invalidKeyResult(CredentialBackendOperation::Contains, backend(), key);
        }
        const auto readRes = readCredential(key);
        if (readRes.result.succeeded && readRes.secret.has_value()) {
            return CredentialBackendResult{CredentialBackendOperation::Contains,
                                           backend(),
                                           true,
                                           false,
                                           false,
                                           key.normalizedProviderId(),
                                           key.normalizedCredentialName(),
                                           "Credential exists in macOS Keychain"};
        }
        return refusedBackendResult(CredentialBackendOperation::Contains, backend(), key,
                                    "Credential not found in macOS Keychain");
    }
};

class SecretServiceCredentialBackend final : public ICredentialBackend {
public:
    CredentialStoreBackend backend() const override {
        return CredentialStoreBackend::LinuxSecretService;
    }
    CredentialStoreSummary summary() const override {
        return summaryForBackend(
            CredentialStoreBackend::LinuxSecretService, CredentialStoreStatus::Ready,
            CredentialStoreReadiness::TestOnlyReady, true,
            QStringLiteral("Linux Secret Service backend ready: credentials are saved securely in "
                           "Linux Secret Service."),
            QStringLiteral("Linux Secret Service / Ready / secure persistent storage."));
    }
    CredentialBackendResult storeCredential(const CredentialKey& key,
                                            const QString& secret) override {
        if (!key.isValid()) {
            return invalidKeyResult(CredentialBackendOperation::Store, backend(), key);
        }
        QProcess process;
        QStringList arguments;
        arguments << "store" << "--label=Sentinel Credential" << "service" << "sentinel" << "key"
                  << key.storageKey();
        process.start("secret-tool", arguments);
        if (!process.waitForStarted()) {
            return refusedBackendResult(CredentialBackendOperation::Store, backend(), key,
                                        "Linux Secret Service (secret-tool) failed to start");
        }
        process.write(secret.toUtf8());
        process.closeWriteChannel();
        if (!process.waitForFinished(3000) || process.exitCode() != 0) {
            return refusedBackendResult(CredentialBackendOperation::Store, backend(), key,
                                        "Linux Secret Service store failed");
        }
        return CredentialBackendResult{CredentialBackendOperation::Store,
                                       backend(),
                                       true,
                                       true,
                                       false,
                                       key.normalizedProviderId(),
                                       key.normalizedCredentialName(),
                                       "Saved to Linux Secret Service successfully"};
    }
    CredentialReadResult readCredential(const CredentialKey& key) const override {
        if (!key.isValid()) {
            return {invalidKeyResult(CredentialBackendOperation::Read, backend(), key),
                    std::nullopt};
        }
        QProcess process;
        QStringList arguments;
        arguments << "lookup" << "service" << "sentinel" << "key" << key.storageKey();
        process.start("secret-tool", arguments);
        if (!process.waitForFinished(3000) || process.exitCode() != 0) {
            return {refusedBackendResult(CredentialBackendOperation::Read, backend(), key,
                                         "Linux Secret Service read failed"),
                    std::nullopt};
        }
        QString secret = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        return {CredentialBackendResult{CredentialBackendOperation::Read, backend(), true, false,
                                        true, key.normalizedProviderId(),
                                        key.normalizedCredentialName(),
                                        "Read from Linux Secret Service successfully"},
                secret};
    }
    CredentialBackendResult deleteCredential(const CredentialKey& key) override {
        if (!key.isValid()) {
            return invalidKeyResult(CredentialBackendOperation::Delete, backend(), key);
        }
        QProcess process;
        QStringList arguments;
        arguments << "clear" << "service" << "sentinel" << "key" << key.storageKey();
        process.start("secret-tool", arguments);
        if (!process.waitForFinished(3000) || process.exitCode() != 0) {
            return refusedBackendResult(CredentialBackendOperation::Delete, backend(), key,
                                        "Linux Secret Service delete failed");
        }
        return CredentialBackendResult{CredentialBackendOperation::Delete,
                                       backend(),
                                       true,
                                       true,
                                       false,
                                       key.normalizedProviderId(),
                                       key.normalizedCredentialName(),
                                       "Deleted from Linux Secret Service successfully"};
    }
    CredentialBackendResult containsCredential(const CredentialKey& key) const override {
        if (!key.isValid()) {
            return invalidKeyResult(CredentialBackendOperation::Contains, backend(), key);
        }
        const auto readRes = readCredential(key);
        if (readRes.result.succeeded && readRes.secret.has_value()) {
            return CredentialBackendResult{CredentialBackendOperation::Contains,
                                           backend(),
                                           true,
                                           false,
                                           false,
                                           key.normalizedProviderId(),
                                           key.normalizedCredentialName(),
                                           "Credential exists in Linux Secret Service"};
        }
        return refusedBackendResult(CredentialBackendOperation::Contains, backend(), key,
                                    "Credential not found in Linux Secret Service");
    }
};

class WinCredentialBackend final : public ICredentialBackend {
public:
    CredentialStoreBackend backend() const override {
        return CredentialStoreBackend::WindowsCredentialManager;
    }
    CredentialStoreSummary summary() const override {
        return summaryForBackend(
            CredentialStoreBackend::WindowsCredentialManager, CredentialStoreStatus::Ready,
            CredentialStoreReadiness::TestOnlyReady, true,
            QStringLiteral("Windows Credential Manager backend ready: credentials are saved "
                           "securely in Windows Credential Manager."),
            QStringLiteral("Windows Credential Manager / Ready / secure persistent storage."));
    }
    CredentialBackendResult storeCredential(const CredentialKey& key,
                                            const QString& secret) override {
        if (!key.isValid()) {
            return invalidKeyResult(CredentialBackendOperation::Store, backend(), key);
        }
#if defined(Q_OS_WIN)
        CREDENTIALW cred;
        memset(&cred, 0, sizeof(cred));
        cred.Type = CRED_TYPE_GENERIC;

        std::wstring targetName = (QStringLiteral("Sentinel:") + key.storageKey()).toStdWString();
        cred.TargetName = const_cast<LPWSTR>(targetName.c_str());

        std::wstring userName = L"Sentinel";
        cred.UserName = const_cast<LPWSTR>(userName.c_str());

        QByteArray secretData = secret.toUtf8();
        cred.CredentialBlobSize = secretData.size();
        cred.CredentialBlob = reinterpret_cast<LPBYTE>(secretData.data());
        cred.Persist = CRED_PERSIST_LOCAL_MACHINE;

        if (!CredWriteW(&cred, 0)) {
            return refusedBackendResult(CredentialBackendOperation::Store, backend(), key,
                                        "Windows Credential Manager write failed");
        }
        return CredentialBackendResult{CredentialBackendOperation::Store,
                                       backend(),
                                       true,
                                       true,
                                       false,
                                       key.normalizedProviderId(),
                                       key.normalizedCredentialName(),
                                       "Saved to Windows Credential Manager successfully"};
#else
        return refusedBackendResult(CredentialBackendOperation::Store, backend(), key,
                                    "Windows Credential Manager unavailable on this platform");
#endif
    }
    CredentialReadResult readCredential(const CredentialKey& key) const override {
        if (!key.isValid()) {
            return {invalidKeyResult(CredentialBackendOperation::Read, backend(), key),
                    std::nullopt};
        }
#if defined(Q_OS_WIN)
        PCREDENTIALW cred = nullptr;
        std::wstring targetName = (QStringLiteral("Sentinel:") + key.storageKey()).toStdWString();
        if (!CredReadW(targetName.c_str(), CRED_TYPE_GENERIC, 0, &cred)) {
            return {refusedBackendResult(CredentialBackendOperation::Read, backend(), key,
                                         "Windows Credential Manager read failed"),
                    std::nullopt};
        }
        QString secret = QString::fromUtf8(reinterpret_cast<char*>(cred->CredentialBlob),
                                           cred->CredentialBlobSize);
        CredFree(cred);
        return {CredentialBackendResult{CredentialBackendOperation::Read, backend(), true, false,
                                        true, key.normalizedProviderId(),
                                        key.normalizedCredentialName(),
                                        "Read from Windows Credential Manager successfully"},
                secret};
#else
        return {refusedBackendResult(CredentialBackendOperation::Read, backend(), key,
                                     "Windows Credential Manager unavailable on this platform"),
                std::nullopt};
#endif
    }
    CredentialBackendResult deleteCredential(const CredentialKey& key) override {
        if (!key.isValid()) {
            return invalidKeyResult(CredentialBackendOperation::Delete, backend(), key);
        }
#if defined(Q_OS_WIN)
        std::wstring targetName = (QStringLiteral("Sentinel:") + key.storageKey()).toStdWString();
        if (!CredDeleteW(targetName.c_str(), CRED_TYPE_GENERIC, 0)) {
            return refusedBackendResult(CredentialBackendOperation::Delete, backend(), key,
                                        "Windows Credential Manager delete failed");
        }
        return CredentialBackendResult{CredentialBackendOperation::Delete,
                                       backend(),
                                       true,
                                       true,
                                       false,
                                       key.normalizedProviderId(),
                                       key.normalizedCredentialName(),
                                       "Deleted from Windows Credential Manager successfully"};
#else
        return refusedBackendResult(CredentialBackendOperation::Delete, backend(), key,
                                    "Windows Credential Manager unavailable on this platform");
#endif
    }
    CredentialBackendResult containsCredential(const CredentialKey& key) const override {
        if (!key.isValid()) {
            return invalidKeyResult(CredentialBackendOperation::Contains, backend(), key);
        }
#if defined(Q_OS_WIN)
        const auto readRes = readCredential(key);
        if (readRes.result.succeeded && readRes.secret.has_value()) {
            return CredentialBackendResult{CredentialBackendOperation::Contains,
                                           backend(),
                                           true,
                                           false,
                                           false,
                                           key.normalizedProviderId(),
                                           key.normalizedCredentialName(),
                                           "Credential exists in Windows Credential Manager"};
        }
        return refusedBackendResult(CredentialBackendOperation::Contains, backend(), key,
                                    "Credential not found in Windows Credential Manager");
#else
        return refusedBackendResult(CredentialBackendOperation::Contains, backend(), key,
                                    "Windows Credential Manager unavailable on this platform");
#endif
    }
};

std::shared_ptr<ICredentialBackend> createPlatformCredentialBackend() {
#if defined(Q_OS_MACOS)
    return std::make_shared<MacKeychainCredentialBackend>();
#elif defined(Q_OS_WIN)
    return std::make_shared<WinCredentialBackend>();
#elif defined(Q_OS_LINUX)
    return std::make_shared<SecretServiceCredentialBackend>();
#else
    return std::make_shared<DisabledCredentialBackend>();
#endif
}

CredentialStore::CredentialStore()
    : CredentialStore(QCoreApplication::instance() &&
                              QCoreApplication::applicationName() == QStringLiteral("Sentinel")
                          ? createPlatformCredentialBackend()
                          : std::make_shared<DisabledCredentialBackend>()) {}

CredentialStore::CredentialStore(std::shared_ptr<ICredentialBackend> backend)
    : backend_(std::move(backend)),
      traces_{
          traceForBackend(CredentialStoreBackend::MacOSKeychain),
          traceForBackend(CredentialStoreBackend::WindowsCredentialManager),
          traceForBackend(CredentialStoreBackend::LinuxSecretService),
          traceForBackend(CredentialStoreBackend::InMemoryTest),
          traceForBackend(CredentialStoreBackend::LocalUnavailableFallback),
      } {}

CredentialStoreSummary CredentialStore::summary() const {
    return backend_ ? backend_->summary() : DisabledCredentialBackend{}.summary();
}

CredentialStoreSafetyReport CredentialStore::safetyReport() const {
    return CredentialStoreSafetyReport{
        false,
        false,
        false,
        false,
        true,
        true,
        true,
        true,
        QStringLiteral("Credential policy: no plaintext persistence, no logs, no raw secret "
                       "exposure, no automatic provider calls, user-explicit only, "
                       "provider-scoped, OS secret-store preferred, disabled fallback."),
        {
            QStringLiteral("No plaintext API-key persistence is permitted."),
            QStringLiteral("Secrets are never logged or exposed through view models."),
            QStringLiteral("Provider calls and credential tests remain disabled."),
            QStringLiteral("Future credentials must be provider-scoped and user-explicit."),
        },
    };
}

QList<CredentialStoreTrace> CredentialStore::traces() const {
    return traces_;
}

QStringList CredentialStore::traceSummaries() const {
    QStringList summaries;
    for (const auto& trace : traces_) {
        summaries.append(credentialStoreTraceSummary(trace));
    }
    return summaries;
}

QStringList CredentialStore::capabilitySummaries() const {
    return {
        QStringLiteral("%1: available")
            .arg(credentialStoreCapabilityName(CredentialStoreCapability::ReadinessMetadata)),
        QStringLiteral("%1: disabled")
            .arg(credentialStoreCapabilityName(CredentialStoreCapability::StoreSecret)),
        QStringLiteral("%1: disabled")
            .arg(credentialStoreCapabilityName(CredentialStoreCapability::RemoveSecret)),
        QStringLiteral("%1: disabled")
            .arg(credentialStoreCapabilityName(CredentialStoreCapability::TestCredential)),
    };
}

CredentialStoreResult CredentialStore::performDisabledAction(CredentialStoreAction action) const {
    return CredentialStoreResult{
        action,
        false,
        false,
        false,
        QStringLiteral("%1 is disabled: credential storage, update, removal, and cloud "
                       "execution require a future explicit implementation.")
            .arg(credentialStoreActionName(action)),
    };
}

CredentialBackendResult CredentialStore::storeCredential(const CredentialKey& key,
                                                         const QString& secret) {
    return backend_ ? backend_->storeCredential(key, secret)
                    : DisabledCredentialBackend{}.storeCredential(key, secret);
}

CredentialReadResult CredentialStore::readCredential(const CredentialKey& key) const {
    return backend_ ? backend_->readCredential(key)
                    : DisabledCredentialBackend{}.readCredential(key);
}

CredentialBackendResult CredentialStore::deleteCredential(const CredentialKey& key) {
    return backend_ ? backend_->deleteCredential(key)
                    : DisabledCredentialBackend{}.deleteCredential(key);
}

CredentialBackendResult CredentialStore::containsCredential(const CredentialKey& key) const {
    return backend_ ? backend_->containsCredential(key)
                    : DisabledCredentialBackend{}.containsCredential(key);
}

CredentialStore defaultCredentialStore() {
    return CredentialStore{};
}

CredentialStore inMemoryTestCredentialStore() {
    return CredentialStore{std::make_shared<InMemoryCredentialBackend>()};
}

CredentialStore platformCredentialStore() {
    return CredentialStore{createPlatformCredentialBackend()};
}

} // namespace sentinel::core
