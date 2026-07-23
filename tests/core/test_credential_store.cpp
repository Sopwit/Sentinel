#include "sentinel/core/CredentialStore.h"

#include <QtTest>
#include <memory>

using sentinel::core::CredentialKey;
using sentinel::core::CredentialStore;
using sentinel::core::CredentialStoreAction;
using sentinel::core::CredentialStoreBackend;
using sentinel::core::CredentialStoreReadiness;
using sentinel::core::CredentialStoreStatus;
using sentinel::core::defaultCredentialStore;
using sentinel::core::inMemoryTestCredentialStore;
using sentinel::core::PlaceholderCredentialBackend;

class CredentialStoreTest final : public QObject {
    Q_OBJECT

private slots:
    void defaultStateIsDisabledAndNonPersistent();
    void backendReadinessSummaryIsDeterministic();
    void disabledActionsDoNotMutateOrExecute();
    void disabledBackendRefusesPersistence();
    void inMemoryBackendStoresReadsAndDeletesForTestsOnly();
    void placeholderBackendsDoNotExecute();
    void resultSummariesDoNotExposeRawSecrets();
    void safetyPolicyRefusesSecretExposure();
    void testPlatformCredentialBackend();
};

void CredentialStoreTest::defaultStateIsDisabledAndNonPersistent() {
    const auto store = defaultCredentialStore();
    const auto summary = store.summary();

    QCOMPARE(summary.status, CredentialStoreStatus::Disabled);
    QVERIFY(!summary.secretPersistenceEnabled);
    QVERIFY(!summary.plaintextPersistenceAllowed);
    QVERIFY(!summary.providerExecutionEnabled);
    QVERIFY(summary.summary.contains(QStringLiteral("no API key storage is active")));
}

void CredentialStoreTest::backendReadinessSummaryIsDeterministic() {
    const auto store = defaultCredentialStore();

    QCOMPARE(store.traces().size(), 5);
    QCOMPARE(store.traceSummaries().size(), 5);
    QVERIFY(store.summary().backendSummary.contains(QStringLiteral("storage unavailable")));
    QVERIFY(store.traceSummaries()
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("localUnavailableFallback")));
    QVERIFY(
        store.traceSummaries().join(QStringLiteral("\n")).contains(QStringLiteral("inMemoryTest")));
}

void CredentialStoreTest::disabledActionsDoNotMutateOrExecute() {
    const auto store = defaultCredentialStore();

    for (const auto action : {CredentialStoreAction::AddApiKey, CredentialStoreAction::RemoveApiKey,
                              CredentialStoreAction::UpdateApiKey}) {
        const auto result = store.performDisabledAction(action);
        QVERIFY(!result.succeeded);
        QVERIFY(!result.mutatedState);
        QVERIFY(!result.executionAttempted);
        QVERIFY(result.summary.contains(QStringLiteral("disabled")));
    }
}

void CredentialStoreTest::disabledBackendRefusesPersistence() {
    auto store = defaultCredentialStore();
    const CredentialKey key{QStringLiteral("openai-compatible"), QStringLiteral("apiKey")};

    const auto stored = store.storeCredential(key, QStringLiteral("sk-test-secret"));
    QVERIFY(!stored.succeeded);
    QVERIFY(!stored.mutatedState);
    QVERIFY(!stored.secretReturned);

    const auto contains = store.containsCredential(key);
    QVERIFY(!contains.succeeded);
    QVERIFY(!contains.mutatedState);

    const auto read = store.readCredential(key);
    QVERIFY(!read.result.succeeded);
    QVERIFY(!read.secret.has_value());

    const auto removed = store.deleteCredential(key);
    QVERIFY(!removed.succeeded);
    QVERIFY(!removed.mutatedState);
}

void CredentialStoreTest::inMemoryBackendStoresReadsAndDeletesForTestsOnly() {
    auto store = inMemoryTestCredentialStore();
    const CredentialKey key{QStringLiteral("OpenAI-Compatible"), QStringLiteral("apiKey")};

    QCOMPARE(store.summary().status, CredentialStoreStatus::Ready);
    QCOMPARE(store.summary().readiness, CredentialStoreReadiness::TestOnlyReady);
    QVERIFY(store.summary().summary.contains(QStringLiteral("tests only")));

    const auto stored = store.storeCredential(key, QStringLiteral("sk-test-secret"));
    QVERIFY(stored.succeeded);
    QVERIFY(stored.mutatedState);
    QVERIFY(!stored.secretReturned);

    const auto contains = store.containsCredential(key);
    QVERIFY(contains.succeeded);
    QVERIFY(!contains.mutatedState);

    const auto read = store.readCredential(key);
    QVERIFY(read.result.succeeded);
    QVERIFY(read.result.secretReturned);
    QVERIFY(read.secret.has_value());
    QCOMPARE(read.secret.value(), QStringLiteral("sk-test-secret"));

    const auto removed = store.deleteCredential(key);
    QVERIFY(removed.succeeded);
    QVERIFY(removed.mutatedState);
    QVERIFY(!store.containsCredential(key).succeeded);
}

void CredentialStoreTest::placeholderBackendsDoNotExecute() {
    CredentialStore store{
        std::make_shared<PlaceholderCredentialBackend>(CredentialStoreBackend::LinuxSecretService)};
    const CredentialKey key{QStringLiteral("claude"), QStringLiteral("apiKey")};

    QCOMPARE(store.summary().status, CredentialStoreStatus::Unavailable);
    QVERIFY(store.summary().backendSummary.contains(QStringLiteral("no OS credential calls")));
    QVERIFY(!store.storeCredential(key, QStringLiteral("secret")).succeeded);
    QVERIFY(!store.readCredential(key).secret.has_value());
    QVERIFY(!store.deleteCredential(key).mutatedState);
}

void CredentialStoreTest::resultSummariesDoNotExposeRawSecrets() {
    auto store = inMemoryTestCredentialStore();
    const CredentialKey key{QStringLiteral("gemini"), QStringLiteral("apiKey")};
    const auto stored = store.storeCredential(key, QStringLiteral("sk-test-secret"));
    const auto read = store.readCredential(key);

    const auto safeText = stored.summary + read.result.summary +
                          sentinel::core::credentialBackendResultSummary(stored) +
                          sentinel::core::credentialBackendResultSummary(read.result) +
                          store.summary().summary + store.summary().backendSummary;
    QVERIFY(!safeText.contains(QStringLiteral("sk-test-secret")));
    QVERIFY(!safeText.contains(QStringLiteral("secret"), Qt::CaseInsensitive) ||
            safeText.contains(QStringLiteral("secret value omitted")) ||
            safeText.contains(QStringLiteral("secrets are stored in memory for tests only")));
}

void CredentialStoreTest::safetyPolicyRefusesSecretExposure() {
    const auto safety = defaultCredentialStore().safetyReport();

    QVERIFY(!safety.plaintextPersistenceAllowed);
    QVERIFY(!safety.secretLoggingAllowed);
    QVERIFY(!safety.rawSecretExposureAllowed);
    QVERIFY(!safety.automaticProviderCallsAllowed);
    QVERIFY(safety.userExplicitOnly);
    QVERIFY(safety.providerScopedCredentials);
    QVERIFY(safety.osSecretStorePreferred);
    QVERIFY(safety.disabledBackendFallback);
}

void CredentialStoreTest::testPlatformCredentialBackend() {
    auto store = sentinel::core::platformCredentialStore();
    const CredentialKey key{QStringLiteral("open-ai-test"), QStringLiteral("apiKey")};

    const auto summary = store.summary();
    if (summary.status == sentinel::core::CredentialStoreStatus::Ready) {
        // Test storing a credential
        const auto stored = store.storeCredential(key, QStringLiteral("sk-sentinel-test-value"));
        if (stored.succeeded) {
            QVERIFY(store.containsCredential(key).succeeded);

            const auto read = store.readCredential(key);
            QVERIFY(read.result.succeeded);
            QVERIFY(read.secret.has_value());
            QCOMPARE(*read.secret, QStringLiteral("sk-sentinel-test-value"));

            QVERIFY(store.deleteCredential(key).succeeded);
            QVERIFY(!store.containsCredential(key).succeeded);
        }
    }
}

QTEST_MAIN(CredentialStoreTest)

#include "test_credential_store.moc"
