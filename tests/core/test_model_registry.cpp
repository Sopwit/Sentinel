#include "sentinel/core/ModelRegistry.h"

#include <QtTest/QtTest>

using sentinel::core::ModelCapability;
using sentinel::core::ModelReadiness;
using sentinel::core::ModelRegistry;
using sentinel::core::ModelRegistryStatus;
using sentinel::core::ModelSource;
using sentinel::core::ModelStatus;
using sentinel::core::OllamaModelSummary;

class ModelRegistryTest final : public QObject {
    Q_OBJECT

private slots:
    void mapsOllamaModelsDeterministically();
    void exposesSelectedModelReadinessSummary();
    void disabledProviderPlaceholderDoesNotEnableExecutionMetadata();
    void exposesLocalAiEcosystemFoundationMetadata();
};

void ModelRegistryTest::mapsOllamaModelsDeterministically() {
    const auto models = sentinel::core::modelSummariesFromOllama({
        OllamaModelSummary{QStringLiteral("qwen2.5-coder:7b"), {}, 5LL * 1024LL * 1024LL * 1024LL},
        OllamaModelSummary{QStringLiteral("llama3.2:3b"), {}, 2LL * 1024LL * 1024LL * 1024LL},
    });

    QCOMPARE(models.size(), 2);
    QCOMPARE(models.at(0).rawName, QStringLiteral("llama3.2:3b"));
    QCOMPARE(models.at(1).rawName, QStringLiteral("qwen2.5-coder:7b"));
    QCOMPARE(models.at(0).providerId, QStringLiteral("ollama"));
    QCOMPARE(models.at(0).family, QStringLiteral("Llama"));
    QCOMPARE(models.at(0).format, QStringLiteral("Ollama"));
    QCOMPARE(models.at(0).sizeClass, QStringLiteral("3B"));
    QCOMPARE(models.at(0).readiness, ModelReadiness::Available);
    QCOMPARE(models.at(0).status, ModelStatus::Available);
    QCOMPARE(models.at(0).source, ModelSource::Local);
    QVERIFY(models.at(0).capabilities.contains(ModelCapability::Chat));
    QVERIFY(models.at(0).capabilities.contains(ModelCapability::Streaming));
    QVERIFY(!models.at(0).safetyReport.filesystemScanAttempted);
    QVERIFY(!models.at(0).safetyReport.cloudCallAttempted);
}

void ModelRegistryTest::exposesSelectedModelReadinessSummary() {
    auto models = sentinel::core::modelSummariesFromOllama({
        OllamaModelSummary{QStringLiteral("llama3.2:3b"), {}, 1024},
    });
    models.append(sentinel::core::disabledProviderModelPlaceholder(
        QStringLiteral("openai-compatible"), QStringLiteral("OpenAI-Compatible API")));
    const ModelRegistry registry{models, QStringLiteral("ollama"), QStringLiteral("llama3.2:3b")};

    const auto summary = registry.summary();
    QCOMPARE(summary.status, ModelRegistryStatus::Ready);
    QCOMPARE(summary.modelCount, 2);
    QCOMPARE(summary.availableCount, 1);
    QCOMPARE(summary.placeholderCount, 1);
    QCOMPARE(summary.selectedReadiness, QStringLiteral("available"));
    QVERIFY(registry.hasAvailableModel(QStringLiteral("ollama"), QStringLiteral("llama3.2:3b")));
    QCOMPARE(registry.modelIdsForProvider(QStringLiteral("ollama")),
             QStringList{QStringLiteral("llama3.2:3b")});
    QCOMPARE(registry.selectedModelCapabilityLabels(),
             QStringList({QStringLiteral("chat"), QStringLiteral("streaming")}));
}

void ModelRegistryTest::disabledProviderPlaceholderDoesNotEnableExecutionMetadata() {
    const ModelRegistry registry{
        {sentinel::core::disabledProviderModelPlaceholder(
            QStringLiteral("openai-compatible"), QStringLiteral("OpenAI-Compatible API"))},
        QStringLiteral("openai-compatible"), QStringLiteral("gpt-placeholder")};

    const auto summary = registry.summary();
    QCOMPARE(summary.status, ModelRegistryStatus::Disabled);
    QCOMPARE(summary.availableCount, 0);
    QCOMPARE(summary.placeholderCount, 1);
    QVERIFY(!registry.hasAvailableModel(QStringLiteral("openai-compatible"),
                                        QStringLiteral("gpt-placeholder")));
    QVERIFY(registry.selectedModelCapabilityLabels().isEmpty());
    QVERIFY(registry.selectedModelReadinessSummary().contains(QStringLiteral("missing")));
}

void ModelRegistryTest::exposesLocalAiEcosystemFoundationMetadata() {
    const ModelRegistry registry{
        sentinel::core::modelSummariesFromOllama({
            OllamaModelSummary{QStringLiteral("qwen2.5-coder:7b"), {},
                               5LL * 1024LL * 1024LL * 1024LL},
        }),
        QStringLiteral("ollama"), QStringLiteral("qwen2.5-coder:7b")};

    QVERIFY(registry.installedModelLibrarySummaries().join(QStringLiteral("\n"))
                .contains(QStringLiteral("Installed")));
    QVERIFY(registry.availableModelLibrarySummaries().join(QStringLiteral("\n"))
                .contains(QStringLiteral("LM Studio")));
    QVERIFY(registry.recommendedModelLibrarySummaries().join(QStringLiteral("\n"))
                .contains(QStringLiteral("coding")));
    QVERIFY(registry.modelDetailSummaries().join(QStringLiteral("\n"))
                .contains(QStringLiteral("quantization")));

    const auto advisor = sentinel::core::deterministicModelAdvisorRecommendations(
        sentinel::core::ModelAdvisorInput{QStringLiteral("Fedora"), QStringLiteral("x86_64"),
                                          QStringLiteral("Unknown"), QStringLiteral("Coding"),
                                          QStringLiteral("Balanced"), QStringLiteral("tr")},
        registry);
    QVERIFY(advisor.join(QStringLiteral("\n")).contains(QStringLiteral("qwen2.5-coder:7b")));
    QVERIFY(sentinel::core::downloadCenterPlaceholderSummaries(registry.models())
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("Execution Disabled")));
    QVERIFY(sentinel::core::benchmarkHubPlaceholderSummaries(registry.models())
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("tokens/sec")));
}

QTEST_MAIN(ModelRegistryTest)

#include "test_model_registry.moc"
