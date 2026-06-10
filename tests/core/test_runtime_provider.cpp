#include "sentinel/core/RuntimeProvider.h"

#include <QtTest/QtTest>

using sentinel::core::OllamaConnectionStatus;
using sentinel::core::OllamaHealthCheckResult;
using sentinel::core::OllamaHealthStatus;
using sentinel::core::OllamaModelSummary;
using sentinel::core::OllamaRuntimeProvider;
using sentinel::core::OpenAICompatibleRuntimeProvider;
using sentinel::core::RuntimeProviderRegistry;
using sentinel::core::RuntimeReadinessState;
using sentinel::core::runtimeReadinessStateName;

class RuntimeProviderTest final : public QObject {
    Q_OBJECT

private slots:
    void mapsReadyOllamaMetadata();
    void reportsReadinessTransitions();
    void keepsUnsupportedProviderDisabled();
    void fallsBackToEnabledLocalProviderForDisabledSelection();
    void exposesCapabilityMetadata();
};

static OllamaHealthCheckResult healthyHealth() {
    return OllamaHealthCheckResult{
        OllamaConnectionStatus::Connected,
        OllamaHealthStatus::Healthy,
        QStringLiteral("http://127.0.0.1:11434"),
        QStringLiteral("Ollama reachable."),
        750,
    };
}

void RuntimeProviderTest::mapsReadyOllamaMetadata() {
    const OllamaRuntimeProvider provider{
        QStringLiteral("http://127.0.0.1:11434"), healthyHealth(),
        {OllamaModelSummary{QStringLiteral("llama3.2"), {}, 1024}},
        QStringLiteral("llama3.2"), true, false};

    const auto descriptor = provider.descriptor();
    QCOMPARE(descriptor.providerId, QStringLiteral("ollama"));
    QCOMPARE(descriptor.displayName, QStringLiteral("Local Ollama"));
    QCOMPARE(descriptor.readiness, RuntimeReadinessState::Ready);
    QVERIFY(descriptor.capabilities.localOnly);
    QVERIFY(descriptor.capabilities.supportsStreaming);
    QVERIFY(!descriptor.capabilities.supportsTools);
    QCOMPARE(descriptor.modelNames, QStringList{QStringLiteral("llama3.2")});
}

void RuntimeProviderTest::reportsReadinessTransitions() {
    auto invalidHealth = healthyHealth();
    invalidHealth.connectionStatus = OllamaConnectionStatus::Blocked;
    invalidHealth.healthStatus = OllamaHealthStatus::InvalidEndpoint;

    const OllamaRuntimeProvider invalid{
        QStringLiteral("http://127.0.0.1:11434"), invalidHealth, {}, {}, true, false};
    QCOMPARE(invalid.descriptor().readiness, RuntimeReadinessState::InvalidEndpoint);

    const OllamaRuntimeProvider missingModel{
        QStringLiteral("http://127.0.0.1:11434"), healthyHealth(),
        {OllamaModelSummary{QStringLiteral("mistral"), {}, 1024}},
        QStringLiteral("llama3.2"), true, false};
    QCOMPARE(missingModel.descriptor().readiness, RuntimeReadinessState::MissingModel);

    const OllamaRuntimeProvider busy{
        QStringLiteral("http://127.0.0.1:11434"), healthyHealth(),
        {OllamaModelSummary{QStringLiteral("llama3.2"), {}, 1024}},
        QStringLiteral("llama3.2"), true, true};
    QCOMPARE(busy.descriptor().readiness, RuntimeReadinessState::Busy);
}

void RuntimeProviderTest::keepsUnsupportedProviderDisabled() {
    const OpenAICompatibleRuntimeProvider provider;
    const auto descriptor = provider.descriptor();

    QCOMPARE(descriptor.providerId, QStringLiteral("openai-compatible"));
    QCOMPARE(descriptor.readiness, RuntimeReadinessState::Disabled);
    QVERIFY(!descriptor.enabled);
    QVERIFY(descriptor.capabilities.requiresApiKey);
    QVERIFY(!descriptor.capabilities.localOnly);
}

void RuntimeProviderTest::fallsBackToEnabledLocalProviderForDisabledSelection() {
    const OllamaRuntimeProvider ollama{
        QStringLiteral("http://127.0.0.1:11434"), healthyHealth(),
        {OllamaModelSummary{QStringLiteral("llama3.2"), {}, 1024}},
        QStringLiteral("llama3.2"), true, false};
    const OpenAICompatibleRuntimeProvider openAi;
    const RuntimeProviderRegistry registry{{ollama.descriptor(), openAi.descriptor()},
                                           QStringLiteral("openai-compatible")};

    QCOMPARE(registry.selectedProviderId(), QStringLiteral("openai-compatible"));
    QCOMPARE(registry.activeProviderId(), QStringLiteral("ollama"));
    QCOMPARE(registry.activeReadinessState(), QStringLiteral("ready"));
    QCOMPARE(registry.availableLocalRuntimeSummaries().size(), 1);
}

void RuntimeProviderTest::exposesCapabilityMetadata() {
    QCOMPARE(runtimeReadinessStateName(RuntimeReadinessState::Unauthorized),
             QStringLiteral("unauthorized"));

    const OpenAICompatibleRuntimeProvider openAi;
    const RuntimeProviderRegistry registry{{openAi.descriptor()},
                                           QStringLiteral("openai-compatible")};
    const auto summaries = registry.validationTraceSummaries();
    QCOMPARE(summaries.size(), 1);
    QVERIFY(summaries.first().contains(QStringLiteral("readiness=disabled")));
    QVERIFY(sentinel::core::runtimeProviderCapabilitySummaries(registry.providers())
                .first()
                .contains(QStringLiteral("requiresApiKey: yes")));
}

QTEST_MAIN(RuntimeProviderTest)

#include "test_runtime_provider.moc"
