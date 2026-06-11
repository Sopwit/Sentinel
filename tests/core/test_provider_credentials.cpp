#include "sentinel/core/ProviderCredentials.h"

#include <QtTest>

using sentinel::core::inMemoryTestCredentialStore;
using sentinel::core::ProviderCredentialStatus;
using sentinel::core::defaultProviderCredentialRegistry;

class ProviderCredentialsTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesMetadataOnlyDefaults();
    void keepsCloudProvidersMissingAndDisabled();
    void includesCredentialStoreReadiness();
    void providerReadinessRemainsDisabledWithoutExplicitKeyConfiguration();
    void refusesSecretPersistenceAndExecutionSafety();
};

void ProviderCredentialsTest::exposesMetadataOnlyDefaults() {
    const auto registry = defaultProviderCredentialRegistry();

    QCOMPARE(registry.requirements().size(), 4);
    QCOMPARE(registry.requirementForProvider(QStringLiteral("ollama")).status,
             ProviderCredentialStatus::NotRequired);
    QVERIFY(registry.summary().summary.contains(QStringLiteral("API key values are not stored")));
}

void ProviderCredentialsTest::keepsCloudProvidersMissingAndDisabled() {
    const auto registry = defaultProviderCredentialRegistry();

    for (const auto& providerId :
         {QStringLiteral("openai-compatible"), QStringLiteral("claude"),
          QStringLiteral("gemini")}) {
        const auto readiness = registry.readinessForProvider(providerId);
        QCOMPARE(readiness.status, ProviderCredentialStatus::Missing);
        QVERIFY(readiness.placeholderReady);
        QVERIFY(!readiness.executionAllowed);
        QVERIFY(readiness.summary.contains(QStringLiteral("not configured")));
        QVERIFY(readiness.backendSummary.contains(QStringLiteral("storage unavailable")));
    }
}

void ProviderCredentialsTest::includesCredentialStoreReadiness() {
    const auto registry = defaultProviderCredentialRegistry();

    QVERIFY(registry.summary().summary.contains(QStringLiteral("credential store is disabled")));
    QVERIFY(registry.requirementSummaries().join(QStringLiteral("\n"))
                .contains(QStringLiteral("requiresFutureImplementation"))
            || registry.requirementSummaries().join(QStringLiteral("\n"))
                   .contains(QStringLiteral("disabledFallback")));
}

void ProviderCredentialsTest::providerReadinessRemainsDisabledWithoutExplicitKeyConfiguration() {
    const auto testStore = inMemoryTestCredentialStore();
    const auto registry = defaultProviderCredentialRegistry(testStore.summary());

    const auto readiness = registry.readinessForProvider(QStringLiteral("openai-compatible"));
    QCOMPARE(readiness.status, ProviderCredentialStatus::Missing);
    QVERIFY(readiness.placeholderReady);
    QVERIFY(!readiness.configured);
    QVERIFY(!readiness.executionAllowed);
    QVERIFY(readiness.backendSummary.contains(QStringLiteral("testOnlyReady")));
}

void ProviderCredentialsTest::refusesSecretPersistenceAndExecutionSafety() {
    const auto registry = defaultProviderCredentialRegistry();
    const auto safety = registry.safetyReportForProvider(QStringLiteral("openai-compatible"));

    QVERIFY(!safety.plaintextStorageAllowed);
    QVERIFY(!safety.secretLoggingAllowed);
    QVERIFY(!safety.cloudRequestsAllowed);
    QVERIFY(!safety.backgroundDiscoveryAllowed);
    QVERIFY(!safety.fallbackRoutingAllowed);
    QVERIFY(registry.safetySummaries().join(QStringLiteral("\n"))
                .contains(QStringLiteral("plaintextStorage=refused")));
}

QTEST_MAIN(ProviderCredentialsTest)

#include "test_provider_credentials.moc"
