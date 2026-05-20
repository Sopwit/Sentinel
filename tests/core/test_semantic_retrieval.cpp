#include "sentinel/core/SemanticRetrieval.h"

#include <QtTest>

using sentinel::core::EmbeddingDocument;
using sentinel::core::EmbeddingRequest;
using sentinel::core::embeddingRuntimePlan;
using sentinel::core::EmbeddingRuntimeReadiness;
using sentinel::core::FakeEmbeddingProvider;
using sentinel::core::FakeVectorIndex;
using sentinel::core::HybridRetrievalPolicy;
using sentinel::core::hybridRetrievalReadiness;
using sentinel::core::HybridRetrievalStatus;
using sentinel::core::orchestrateSemanticCandidates;
using sentinel::core::selectSemanticProvider;
using sentinel::core::semanticActivationReadiness;
using sentinel::core::SemanticArbitrationPolicy;
using sentinel::core::SemanticArbitrationStatus;
using sentinel::core::SemanticCandidate;
using sentinel::core::SemanticCandidatePolicy;
using sentinel::core::SemanticCandidateSelection;
using sentinel::core::SemanticCandidateSource;
using sentinel::core::SemanticCandidateStatus;
using sentinel::core::SemanticProviderMode;
using sentinel::core::SemanticProviderPolicy;
using sentinel::core::SemanticProviderReadiness;
using sentinel::core::simulateSemanticArbitration;
using sentinel::core::VectorSearchQuery;

class SemanticRetrievalTest final : public QObject {
    Q_OBJECT

private slots:
    void fakeEmbeddingGenerationIsDeterministic();
    void fakeEmbeddingUsesStableVectorsForSameInput();
    void fakeVectorIndexInsertSearchRemoveIsDeterministic();
    void fakeVectorIndexUsesStableScoringOrder();
    void candidateOrchestrationOrdersAndBudgetsDeterministically();
    void candidateOrchestrationPreservesSourceIsolationAndChronology();
    void hybridReadinessKeepsSemanticPathDisabled();
    void arbitrationSimulationScoresDeterministically();
    void arbitrationSimulationUsesStableTieHandling();
    void embeddingRuntimePlanSummarizesLocalOnlyBlockedReadiness();
    void defaultSemanticProviderSelectionIsDisabled();
    void fakeProviderSelectionIsMetadataOnly();
    void localOllamaProviderIsPlannedButInactive();
    void activationReadinessRefusesByDefault();
};

void SemanticRetrievalTest::fakeEmbeddingGenerationIsDeterministic() {
    FakeEmbeddingProvider provider{8};

    const EmbeddingRequest request{
        QList<EmbeddingDocument>{
            {QStringLiteral("doc-a"), QStringLiteral("alpha beta beta"), QStringLiteral("test"),
             QStringLiteral("A")},
            {QStringLiteral("doc-b"), QStringLiteral("gamma delta"), QStringLiteral("test"),
             QStringLiteral("B")},
        },
    };

    const auto first = provider.embed(request);
    const auto second = provider.embed(request);

    QCOMPARE(first.documentCount, 2);
    QCOMPARE(first.vectors.size(), 2);
    QCOMPARE(first.vectors.at(0).values, second.vectors.at(0).values);
    QCOMPARE(first.vectors.at(1).values, second.vectors.at(1).values);
    QCOMPARE(first.vectors.first().values.size(), 8);
    QVERIFY(first.checks.contains(QStringLiteral("Provider/model calls: disabled")));
    QVERIFY(first.checks.contains(QStringLiteral("Cloud/API keys: disabled")));
}

void SemanticRetrievalTest::fakeEmbeddingUsesStableVectorsForSameInput() {
    FakeEmbeddingProvider provider{8};
    const EmbeddingRequest request{
        QList<EmbeddingDocument>{
            {QStringLiteral("doc-a"), QStringLiteral("same stable text"), {}, {}},
            {QStringLiteral("doc-b"), QStringLiteral("same stable text"), {}, {}},
            {QStringLiteral("doc-c"), QStringLiteral("different stable text"), {}, {}},
        },
    };

    const auto result = provider.embed(request);

    QCOMPARE(result.vectors.at(0).values, result.vectors.at(1).values);
    QVERIFY(result.vectors.at(0).values != result.vectors.at(2).values);
}

void SemanticRetrievalTest::fakeVectorIndexInsertSearchRemoveIsDeterministic() {
    FakeEmbeddingProvider provider{8};
    FakeVectorIndex index{8};
    const QList<EmbeddingDocument> documents{
        {QStringLiteral("doc-a"), QStringLiteral("local memory alpha"), {}, {}},
        {QStringLiteral("doc-b"), QStringLiteral("local memory beta"), {}, {}},
    };
    const auto embeddings = provider.embed(EmbeddingRequest{documents});

    QVERIFY(index.upsert(documents.at(0), embeddings.vectors.at(0)));
    QVERIFY(index.upsert(documents.at(1), embeddings.vectors.at(1)));
    QCOMPARE(index.itemCount(), 2);

    const auto first = index.search(VectorSearchQuery{embeddings.vectors.at(0), 5, -1.0});
    const auto second = index.search(VectorSearchQuery{embeddings.vectors.at(0), 5, -1.0});
    QCOMPARE(first.resultCount, second.resultCount);
    QCOMPARE(first.candidates.first().document.id, QStringLiteral("doc-a"));
    QCOMPARE(first.candidates.first().document.id, second.candidates.first().document.id);
    QCOMPARE(first.candidates.first().score, second.candidates.first().score);

    QVERIFY(index.remove(QStringLiteral("doc-a")));
    QCOMPARE(index.itemCount(), 1);
    const auto afterRemove = index.search(VectorSearchQuery{embeddings.vectors.at(0), 5, -1.0});
    QCOMPARE(afterRemove.resultCount, 1);
    QCOMPARE(afterRemove.candidates.first().document.id, QStringLiteral("doc-b"));
}

void SemanticRetrievalTest::fakeVectorIndexUsesStableScoringOrder() {
    FakeEmbeddingProvider provider{8};
    FakeVectorIndex index{8};
    const QList<EmbeddingDocument> documents{
        {QStringLiteral("doc-c"), QStringLiteral("unrelated runtime metadata"), {}, {}},
        {QStringLiteral("doc-a"), QStringLiteral("alpha beta search target"), {}, {}},
        {QStringLiteral("doc-b"), QStringLiteral("alpha beta search target"), {}, {}},
    };
    const auto embeddings = provider.embed(EmbeddingRequest{documents});
    for (int i = 0; i < documents.size(); ++i) {
        QVERIFY(index.upsert(documents.at(i), embeddings.vectors.at(i)));
    }

    const auto query = provider.embed(EmbeddingRequest{
        QList<EmbeddingDocument>{
            {QStringLiteral("query"), QStringLiteral("alpha beta search target"), {}, {}}},
    });
    const auto result = index.search(VectorSearchQuery{query.vectors.first(), 3, -1.0});

    QCOMPARE(result.resultCount, 3);
    QCOMPARE(result.candidates.at(0).document.id, QStringLiteral("doc-a"));
    QCOMPARE(result.candidates.at(1).document.id, QStringLiteral("doc-b"));
    QVERIFY(result.candidates.at(0).score >= result.candidates.at(2).score);
    QCOMPARE(result.candidates.at(0).rank, 1);
    QCOMPARE(result.candidates.at(1).rank, 2);
}

void SemanticRetrievalTest::candidateOrchestrationOrdersAndBudgetsDeterministically() {
    SemanticCandidatePolicy policy;
    policy.maxCharacters = 21;

    const auto result = orchestrateSemanticCandidates(
        {
            SemanticCandidate{SemanticCandidateSource::CommittedMemory, QStringLiteral("memory"),
                              QStringLiteral("Memory"), QStringLiteral("memory-value")},
            SemanticCandidate{SemanticCandidateSource::RecentConversation, QStringLiteral("recent"),
                              QStringLiteral("Recent"), QStringLiteral("recent")},
            SemanticCandidate{SemanticCandidateSource::RuntimeMetadata, QStringLiteral("runtime"),
                              QStringLiteral("Runtime"), QStringLiteral("runtime-metadata")},
        },
        policy);

    QCOMPARE(result.status, SemanticCandidateStatus::Truncated);
    QCOMPARE(result.selectedCandidates.size(), 3);
    QCOMPARE(result.selectedCandidates.at(0).source, SemanticCandidateSource::RecentConversation);
    QCOMPARE(result.selectedCandidates.at(1).source, SemanticCandidateSource::CommittedMemory);
    QCOMPARE(result.selectedCandidates.at(2).source, SemanticCandidateSource::RuntimeMetadata);
    QCOMPARE(result.selectedCandidates.at(2).selection, SemanticCandidateSelection::Truncated);
    QCOMPARE(result.budget.includedCharacters, 21);
    QVERIFY(result.budgetSummary.contains(QStringLiteral("21")));
}

void SemanticRetrievalTest::candidateOrchestrationPreservesSourceIsolationAndChronology() {
    SemanticCandidatePolicy policy;
    policy.maxCharacters = 100;

    const auto result = orchestrateSemanticCandidates(
        {
            SemanticCandidate{SemanticCandidateSource::RecentConversation,
                              QStringLiteral("recent-1"), QStringLiteral("First"),
                              QStringLiteral("#1 user: old")},
            SemanticCandidate{SemanticCandidateSource::RecentConversation,
                              QStringLiteral("recent-2"), QStringLiteral("Second"),
                              QStringLiteral("#2 assistant: new")},
            SemanticCandidate{SemanticCandidateSource::DeterministicSummary,
                              QStringLiteral("summary"), QStringLiteral("Summary"),
                              QStringLiteral("older summary")},
            SemanticCandidate{SemanticCandidateSource::FutureSemanticVector,
                              QStringLiteral("future"), QStringLiteral("Future"),
                              QStringLiteral("vector payload disabled")},
        },
        policy);

    QCOMPARE(result.selectedCandidates.at(0).id, QStringLiteral("recent-1"));
    QCOMPARE(result.selectedCandidates.at(1).id, QStringLiteral("recent-2"));
    QCOMPARE(result.selectedCandidates.at(2).source, SemanticCandidateSource::DeterministicSummary);
    QVERIFY(result.exclusionSummary.contains(QStringLiteral("1 semantic candidate")));
    QVERIFY(!result.selectedCandidates.last().content.contains(QStringLiteral("vector payload")));
}

void SemanticRetrievalTest::hybridReadinessKeepsSemanticPathDisabled() {
    SemanticCandidatePolicy candidatePolicy;
    const auto arbitration = orchestrateSemanticCandidates(
        {
            SemanticCandidate{SemanticCandidateSource::RecentConversation, QStringLiteral("recent"),
                              QStringLiteral("Recent"), QStringLiteral("local only")},
        },
        candidatePolicy);

    const auto readiness = hybridRetrievalReadiness(HybridRetrievalPolicy{}, arbitration);

    QCOMPARE(readiness.status, HybridRetrievalStatus::DeterministicOnly);
    QCOMPARE(readiness.semanticCandidateCount, 0);
    QCOMPARE(readiness.deterministicCandidateCount, 1);
    QVERIFY(readiness.checks.contains(QStringLiteral("Semantic path enabled: no")));
    QVERIFY(readiness.checks.contains(QStringLiteral("Prompt mutation from semantic candidates: "
                                                     "disabled")));
}

void SemanticRetrievalTest::arbitrationSimulationScoresDeterministically() {
    SemanticCandidatePolicy candidatePolicy;
    candidatePolicy.maxCharacters = 300;
    const auto arbitration = orchestrateSemanticCandidates(
        {
            SemanticCandidate{SemanticCandidateSource::CommittedMemory, QStringLiteral("memory"),
                              QStringLiteral("Memory"), QStringLiteral("memory value")},
            SemanticCandidate{SemanticCandidateSource::RecentConversation, QStringLiteral("recent"),
                              QStringLiteral("Recent"), QStringLiteral("recent value")},
            SemanticCandidate{SemanticCandidateSource::RuntimeMetadata, QStringLiteral("runtime"),
                              QStringLiteral("Runtime"), QStringLiteral("runtime value")},
        },
        candidatePolicy);

    const auto first = simulateSemanticArbitration(arbitration, SemanticArbitrationPolicy{});
    const auto second = simulateSemanticArbitration(arbitration, SemanticArbitrationPolicy{});

    QCOMPARE(first.status, SemanticArbitrationStatus::Simulated);
    QCOMPARE(first.candidateScores.size(), second.candidateScores.size());
    QCOMPARE(first.candidateScores.first().candidateId, QStringLiteral("recent"));
    QCOMPARE(first.candidateScores.first().simulatedScore,
             second.candidateScores.first().simulatedScore);
    QVERIFY(first.summary.contains(QStringLiteral("Deterministic retrieval remains final "
                                                  "authority")));
    QVERIFY(first.checks.contains(QStringLiteral("Real embeddings: disabled")));
    QVERIFY(first.checks.contains(QStringLiteral("Prompt mutation: disabled")));
}

void SemanticRetrievalTest::arbitrationSimulationUsesStableTieHandling() {
    SemanticCandidatePolicy candidatePolicy;
    candidatePolicy.maxCharacters = 300;
    const auto arbitration = orchestrateSemanticCandidates(
        {
            SemanticCandidate{SemanticCandidateSource::CommittedMemory,
                              QStringLiteral("candidate-b"), QStringLiteral("B"),
                              QStringLiteral("same length text")},
            SemanticCandidate{SemanticCandidateSource::CommittedMemory,
                              QStringLiteral("candidate-a"), QStringLiteral("A"),
                              QStringLiteral("same length text")},
        },
        candidatePolicy);

    const auto result = simulateSemanticArbitration(arbitration, SemanticArbitrationPolicy{});

    QCOMPARE(result.candidateScores.size(), 2);
    QCOMPARE(result.candidateScores.at(0).candidateId, QStringLiteral("candidate-a"));
    QCOMPARE(result.candidateScores.at(1).candidateId, QStringLiteral("candidate-b"));
    QVERIFY(result.checks.contains(
        QStringLiteral("Tie handling: simulated score, source rank, candidate id")));
}

void SemanticRetrievalTest::embeddingRuntimePlanSummarizesLocalOnlyBlockedReadiness() {
    SemanticCandidatePolicy candidatePolicy;
    const auto arbitration = orchestrateSemanticCandidates(
        {
            SemanticCandidate{SemanticCandidateSource::RecentConversation, QStringLiteral("recent"),
                              QStringLiteral("Recent"), QStringLiteral("recent value")},
        },
        candidatePolicy);
    const auto simulated = simulateSemanticArbitration(arbitration, SemanticArbitrationPolicy{});
    const auto plan = embeddingRuntimePlan(simulated);

    QCOMPARE(plan.readiness, EmbeddingRuntimeReadiness::Blocked);
    QCOMPARE(plan.localOnly, true);
    QCOMPARE(plan.indexingEnabled, false);
    QCOMPARE(plan.filesystemIndexingEnabled, false);
    QCOMPARE(plan.ollamaEmbeddingCallsEnabled, false);
    QCOMPARE(plan.budget.estimatedEmbeddingJobs, 1);
    QVERIFY(plan.requirements.contains(QStringLiteral("Explicit local embedding provider gate")));
    QVERIFY(plan.constraints.contains(QStringLiteral("No cloud/API keys")));
    QVERIFY(plan.constraints.contains(QStringLiteral("No vector database writes while disabled")));
}

void SemanticRetrievalTest::defaultSemanticProviderSelectionIsDisabled() {
    const SemanticProviderPolicy policy;
    const auto selection = selectSemanticProvider(SemanticProviderMode::Disabled, policy);

    QCOMPARE(selection.mode, SemanticProviderMode::Disabled);
    QCOMPARE(selection.readiness, SemanticProviderReadiness::Disabled);
    QVERIFY(!selection.active);
    QVERIFY(selection.summary.contains(QStringLiteral("disabled by default")));
    QVERIFY(selection.capabilitySummaries.contains(QStringLiteral("Prompt mutation blocked")));
}

void SemanticRetrievalTest::fakeProviderSelectionIsMetadataOnly() {
    SemanticProviderPolicy policy;
    policy.disabledByDefault = false;
    policy.allowFakeInMemoryProvider = true;

    const auto selection = selectSemanticProvider(SemanticProviderMode::FakeInMemory, policy);
    const auto readiness = semanticActivationReadiness(selection, policy);

    QCOMPARE(selection.mode, SemanticProviderMode::FakeInMemory);
    QCOMPARE(selection.readiness, SemanticProviderReadiness::ReadyMetadataOnly);
    QVERIFY(!selection.active);
    QVERIFY(selection.capabilitySummaries.contains(QStringLiteral("Fake/InMemory test provider")));
    QVERIFY(readiness.summary.contains(QStringLiteral("isolated fake tests only")));
    QVERIFY(readiness.checks.contains(QStringLiteral("Semantic prompt injection allowed: no")));
}

void SemanticRetrievalTest::localOllamaProviderIsPlannedButInactive() {
    const SemanticProviderPolicy policy;
    const auto selection =
        selectSemanticProvider(SemanticProviderMode::LocalOllamaEmbeddings, policy);
    const auto readiness = semanticActivationReadiness(selection, policy);

    QCOMPARE(selection.mode, SemanticProviderMode::LocalOllamaEmbeddings);
    QCOMPARE(selection.readiness, SemanticProviderReadiness::Planned);
    QVERIFY(!selection.active);
    QVERIFY(selection.summary.contains(QStringLiteral("disabled by default")));
    QVERIFY(readiness.requiredSteps.join(QStringLiteral("\n"))
                .contains(QStringLiteral("embedding model selection gate")));
    QVERIFY(readiness.checks.contains(QStringLiteral("Real embedding calls allowed: no")));
}

void SemanticRetrievalTest::activationReadinessRefusesByDefault() {
    const SemanticProviderPolicy policy;
    const auto selection =
        selectSemanticProvider(SemanticProviderMode::LocalFileVectorIndex, policy);
    const auto readiness = semanticActivationReadiness(selection, policy);

    QVERIFY(!readiness.ready);
    QCOMPARE(readiness.status, QStringLiteral("Refused"));
    QVERIFY(readiness.summary.contains(QStringLiteral("Semantic activation refused")));
    QVERIFY(readiness.checks.contains(QStringLiteral("Vector index writes allowed: no")));
    QVERIFY(
        readiness.checks.contains(QStringLiteral("Deterministic retrieval authoritative: yes")));
}

QTEST_MAIN(SemanticRetrievalTest)

#include "test_semantic_retrieval.moc"
