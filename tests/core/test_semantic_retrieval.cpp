#include "sentinel/core/SemanticRetrieval.h"

#include <QtTest>

using sentinel::core::assembleSemanticSupplements;
using sentinel::core::ContextAssemblySourceKind;
using sentinel::core::EmbeddingDocument;
using sentinel::core::EmbeddingGenerationPolicy;
using sentinel::core::EmbeddingGenerationReadiness;
using sentinel::core::EmbeddingGenerationResult;
using sentinel::core::EmbeddingIsolationPolicy;
using sentinel::core::EmbeddingRequest;
using sentinel::core::EmbeddingRuntimeHealth;
using sentinel::core::embeddingRuntimePlan;
using sentinel::core::EmbeddingRuntimeReadiness;
using sentinel::core::EmbeddingRuntimeSession;
using sentinel::core::EmbeddingRuntimeStatus;
using sentinel::core::evaluateSemanticPromptAuthority;
using sentinel::core::FakeEmbeddingProvider;
using sentinel::core::FakeVectorIndex;
using sentinel::core::generateIsolatedEmbeddings;
using sentinel::core::hybridRetrievalBridge;
using sentinel::core::HybridRetrievalBridgePolicy;
using sentinel::core::HybridRetrievalBridgeStatus;
using sentinel::core::HybridRetrievalPolicy;
using sentinel::core::hybridRetrievalReadiness;
using sentinel::core::HybridRetrievalStatus;
using sentinel::core::includeSemanticPromptSupplements;
using sentinel::core::LocalVectorPersistenceIndex;
using sentinel::core::orchestrateSemanticCandidates;
using sentinel::core::planRetrieval;
using sentinel::core::RetrievalCandidate;
using sentinel::core::RetrievalPlanningPolicy;
using sentinel::core::RetrievalPlanningResult;
using sentinel::core::RetrievalPlanningStatus;
using sentinel::core::RetrievalSourcePriority;
using sentinel::core::selectSemanticProvider;
using sentinel::core::semanticAcceptance;
using sentinel::core::SemanticAcceptancePolicy;
using sentinel::core::SemanticAcceptanceResult;
using sentinel::core::SemanticAcceptanceStatus;
using sentinel::core::semanticActivationReadiness;
using sentinel::core::SemanticArbitrationPolicy;
using sentinel::core::SemanticArbitrationStatus;
using sentinel::core::SemanticCandidate;
using sentinel::core::SemanticCandidatePolicy;
using sentinel::core::SemanticCandidateSelection;
using sentinel::core::SemanticCandidateSource;
using sentinel::core::SemanticCandidateStatus;
using sentinel::core::SemanticPromptAuthorityDecision;
using sentinel::core::SemanticPromptAuthorityPolicy;
using sentinel::core::SemanticPromptAuthorityStatus;
using sentinel::core::SemanticPromptInclusionPolicy;
using sentinel::core::SemanticPromptInclusionStatus;
using sentinel::core::SemanticProviderMode;
using sentinel::core::SemanticProviderPolicy;
using sentinel::core::SemanticProviderReadiness;
using sentinel::core::SemanticSearchPolicy;
using sentinel::core::SemanticSearchResult;
using sentinel::core::SemanticSearchSession;
using sentinel::core::SemanticSearchStatus;
using sentinel::core::SemanticSupplementAssemblyPolicy;
using sentinel::core::SemanticSupplementAssemblyStatus;
using sentinel::core::simulateSemanticArbitration;
using sentinel::core::VectorPersistenceBudget;
using sentinel::core::VectorPersistenceHealth;
using sentinel::core::VectorPersistencePolicy;
using sentinel::core::VectorPersistenceReadiness;
using sentinel::core::VectorPersistenceSession;
using sentinel::core::VectorPersistenceStatus;
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
    void isolatedEmbeddingGenerationSucceedsWithFakeProvider();
    void isolatedEmbeddingGenerationRefusesNonLocalPolicy();
    void isolatedEmbeddingGenerationTimesOutDeterministically();
    void isolatedEmbeddingGenerationRejectsStaleAndBusyRequests();
    void vectorPersistenceLifecycleIsExplicitAndEmptySafe();
    void vectorPersistenceAcceptsOnlyIsolatedEmbeddingOutputs();
    void vectorPersistenceRejectsStaleBusyAndBoundedOverflow();
    void semanticSearchOrdersCandidatesDeterministicallyAndBoundsResults();
    void semanticSearchHandlesTimeoutStaleBusyAndEmptyIndex();
    void semanticSearchEnforcesLocalOnlyNonAuthoritativePolicy();
    void hybridBridgePreservesDeterministicAuthorityAndBounds();
    void hybridBridgeFallsBackForDisabledStaleBusyAndTimeoutSemanticSources();
    void hybridBridgeRefusesPromptOrPlanningMutationAuthority();
    void semanticAcceptanceApprovesBoundedSupplementsOnly();
    void semanticAcceptanceFallsBackForDisabledErrorTimeoutStaleAndBusy();
    void semanticAcceptanceRefusesAuthorityMutationAndPreservesInputs();
    void semanticSupplementAssemblyDisabledReturnsSafeFallback();
    void semanticSupplementAssemblyBuildsBoundedSeparateMetadata();
    void semanticSupplementAssemblyOrdersAndTruncatesDeterministically();
    void semanticSupplementAssemblyRefusesPromptAuthorityAndStaleState();
    void semanticPromptAuthorityDefaultsToDeniedDisabledFallback();
    void semanticPromptAuthorityAllowsReadinessMetadataOnly();
    void semanticPromptAuthoritySafetyReportBlocksUnsafeInclusion();
    void semanticPromptInclusionDisabledLeavesPromptUnchanged();
    void semanticPromptInclusionAppendsDelimitedBlockAfterDeterministicContext();
    void semanticPromptInclusionBoundsAndTruncatesDeterministically();
    void semanticPromptInclusionFallsBackForDeniedUnsafeTimeoutStaleAndEmpty();
    void defaultSemanticProviderSelectionIsDisabled();
    void fakeProviderSelectionIsMetadataOnly();
    void localOllamaProviderIsPlannedButInactive();
    void activationReadinessRefusesByDefault();
};

namespace {

RetrievalPlanningResult deterministicPlanningForBridge() {
    return planRetrieval(
        QList<RetrievalCandidate>{
            {ContextAssemblySourceKind::Conversation, RetrievalSourcePriority::RecentConversation,
             QStringLiteral("Recent Conversation"),
             QStringLiteral("deterministic recent conversation")},
            {ContextAssemblySourceKind::CommittedMemory, RetrievalSourcePriority::CommittedMemory,
             QStringLiteral("Committed Memory"), QStringLiteral("deterministic committed memory")},
        },
        RetrievalPlanningPolicy{});
}

SemanticSearchResult semanticSearchResultForBridge(SemanticSearchStatus status) {
    SemanticSearchResult result;
    result.status = status;
    result.accepted = status == SemanticSearchStatus::Ready;
    result.budget.elapsedMs = status == SemanticSearchStatus::TimedOut ? 1500 : 10;
    result.budget.returnedCandidateCount = status == SemanticSearchStatus::Ready ? 2 : 0;
    if (status == SemanticSearchStatus::Ready) {
        result.candidates = {
            {QStringLiteral("semantic-a"), QStringLiteral("semantic advisory alpha"), 0.91, 1,
             QStringLiteral("a")},
            {QStringLiteral("semantic-b"), QStringLiteral("semantic advisory beta"), 0.88, 2,
             QStringLiteral("b")},
        };
    }
    return result;
}

sentinel::core::SemanticSupplementAssemblyResult readySemanticAssemblyForInclusion() {
    const auto planning = deterministicPlanningForBridge();
    const auto search = semanticSearchResultForBridge(SemanticSearchStatus::Ready);
    const auto bridge = hybridRetrievalBridge(planning, search, HybridRetrievalBridgePolicy{});
    const auto acceptance =
        semanticAcceptance(planning, bridge, search, SemanticAcceptancePolicy{});

    SemanticSupplementAssemblyPolicy assemblyPolicy;
    assemblyPolicy.enabled = true;
    assemblyPolicy.allowTestOnlyAssembly = true;
    return assembleSemanticSupplements(acceptance, assemblyPolicy);
}

SemanticPromptAuthorityPolicy allowingAuthorityPolicy() {
    SemanticPromptAuthorityPolicy policy;
    policy.enabled = true;
    policy.promptInjectionExplicitlyEnabled = true;
    policy.semanticPromptAuthorityAllowed = true;
    policy.allowTestOnlyWouldIncludeMetadata = true;
    return policy;
}

sentinel::core::PromptContextInjectionResult deterministicPromptForInclusion() {
    return sentinel::core::injectPromptContext(
        QStringLiteral("question"),
        QList<sentinel::core::PromptContextBlock>{
            {ContextAssemblySourceKind::Conversation, QStringLiteral("Conversation"),
             QStringLiteral("deterministic conversation")},
            {ContextAssemblySourceKind::CommittedMemory, QStringLiteral("Committed Memory"),
             QStringLiteral("deterministic memory")},
        },
        sentinel::core::PromptContextInjectionPolicy{true, 2000});
}

} // namespace

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

void SemanticRetrievalTest::isolatedEmbeddingGenerationSucceedsWithFakeProvider() {
    FakeEmbeddingProvider provider{8};
    EmbeddingIsolationPolicy isolation;
    isolation.explicitSemanticEnableReadinessSatisfied = true;
    EmbeddingGenerationPolicy policy;
    policy.providerMode = SemanticProviderMode::FakeInMemory;
    policy.allowFakeInMemoryProvider = true;
    policy.requestId = QStringLiteral("request-ok");

    const auto result = generateIsolatedEmbeddings(
        provider,
        QList<EmbeddingDocument>{
            {QStringLiteral("doc-a"), QStringLiteral("local readiness text"), {}, {}},
        },
        isolation, policy, EmbeddingRuntimeSession{});

    QCOMPARE(result.readiness, EmbeddingGenerationReadiness::Ready);
    QCOMPARE(result.status, EmbeddingRuntimeStatus::Succeeded);
    QCOMPARE(result.health, EmbeddingRuntimeHealth::LocalOnlyReady);
    QCOMPARE(result.generatedVectorCount, 1);
    QCOMPARE(result.session.boundedDocumentCount, 1);
    QVERIFY(result.summary.contains(QStringLiteral("readiness validation only")));
    QVERIFY(result.checks.contains(QStringLiteral("Prompt integration: disabled")));
    QVERIFY(result.checks.contains(QStringLiteral("Vector DB persistence: disabled")));
}

void SemanticRetrievalTest::isolatedEmbeddingGenerationRefusesNonLocalPolicy() {
    FakeEmbeddingProvider provider{8};
    EmbeddingIsolationPolicy isolation;
    isolation.localOnlyMode = false;
    isolation.explicitSemanticEnableReadinessSatisfied = true;
    EmbeddingGenerationPolicy policy;
    policy.providerMode = SemanticProviderMode::FakeInMemory;
    policy.allowFakeInMemoryProvider = true;

    const auto result = generateIsolatedEmbeddings(
        provider,
        QList<EmbeddingDocument>{
            {QStringLiteral("doc-a"), QStringLiteral("local readiness text"), {}, {}},
        },
        isolation, policy, EmbeddingRuntimeSession{});

    QCOMPARE(result.readiness, EmbeddingGenerationReadiness::Refused);
    QCOMPARE(result.status, EmbeddingRuntimeStatus::Refused);
    QCOMPARE(result.health, EmbeddingRuntimeHealth::Blocked);
    QCOMPARE(result.generatedVectorCount, 0);
    QVERIFY(result.failureReason.contains(QStringLiteral("local-only policy gates")));
}

void SemanticRetrievalTest::isolatedEmbeddingGenerationTimesOutDeterministically() {
    FakeEmbeddingProvider provider{8};
    EmbeddingIsolationPolicy isolation;
    isolation.explicitSemanticEnableReadinessSatisfied = true;
    EmbeddingGenerationPolicy policy;
    policy.providerMode = SemanticProviderMode::FakeInMemory;
    policy.allowFakeInMemoryProvider = true;
    policy.timeoutMs = 5;
    policy.simulatedExecutionMs = 10;

    const auto result = generateIsolatedEmbeddings(
        provider,
        QList<EmbeddingDocument>{
            {QStringLiteral("doc-a"), QStringLiteral("local readiness text"), {}, {}},
        },
        isolation, policy, EmbeddingRuntimeSession{});

    QCOMPARE(result.status, EmbeddingRuntimeStatus::TimedOut);
    QCOMPARE(result.health, EmbeddingRuntimeHealth::Failed);
    QCOMPARE(result.generatedVectorCount, 0);
    QVERIFY(result.summary.contains(QStringLiteral("timed out")));
}

void SemanticRetrievalTest::isolatedEmbeddingGenerationRejectsStaleAndBusyRequests() {
    FakeEmbeddingProvider provider{8};
    EmbeddingIsolationPolicy isolation;
    isolation.explicitSemanticEnableReadinessSatisfied = true;
    EmbeddingGenerationPolicy policy;
    policy.providerMode = SemanticProviderMode::FakeInMemory;
    policy.allowFakeInMemoryProvider = true;
    policy.requestId = QStringLiteral("new-request");

    EmbeddingRuntimeSession staleSession;
    staleSession.activeRequestId = QStringLiteral("old-request");
    const auto stale = generateIsolatedEmbeddings(provider, {}, isolation, policy, staleSession);
    QCOMPARE(stale.status, EmbeddingRuntimeStatus::Stale);
    QCOMPARE(stale.generatedVectorCount, 0);

    EmbeddingRuntimeSession busySession;
    busySession.busy = true;
    const auto busy = generateIsolatedEmbeddings(provider, {}, isolation, policy, busySession);
    QCOMPARE(busy.status, EmbeddingRuntimeStatus::Busy);
    QCOMPARE(busy.generatedVectorCount, 0);
}

void SemanticRetrievalTest::vectorPersistenceLifecycleIsExplicitAndEmptySafe() {
    VectorPersistencePolicy disabledPolicy;
    LocalVectorPersistenceIndex disabledIndex{disabledPolicy};
    QCOMPARE(disabledIndex.snapshot().status, VectorPersistenceStatus::Disabled);
    QCOMPARE(disabledIndex.create(VectorPersistenceSession{}).readiness,
             VectorPersistenceReadiness::Disabled);

    VectorPersistencePolicy policy;
    policy.enabled = true;
    policy.disabledByDefault = false;
    LocalVectorPersistenceIndex index{policy};

    const auto created = index.create(VectorPersistenceSession{});
    QCOMPARE(created.status, VectorPersistenceStatus::Created);
    QCOMPARE(created.health, VectorPersistenceHealth::Empty);
    QCOMPARE(index.itemCount(), 0);
    QVERIFY(created.snapshot.summary.contains(QStringLiteral("0 indexed")));

    const auto reset = index.reset(VectorPersistenceSession{});
    QCOMPARE(reset.status, VectorPersistenceStatus::Reset);
    QCOMPARE(reset.snapshot.indexedItemCount, 0);
    QVERIFY(reset.summary.contains(QStringLiteral("empty")));

    const auto cleared = index.clear(VectorPersistenceSession{});
    QCOMPARE(cleared.status, VectorPersistenceStatus::Cleared);
    QCOMPARE(cleared.snapshot.indexedItemCount, 0);
    QVERIFY(cleared.snapshot.boundedState.contains(QStringLiteral("semantic retrieval disabled")));
}

void SemanticRetrievalTest::vectorPersistenceAcceptsOnlyIsolatedEmbeddingOutputs() {
    VectorPersistencePolicy policy;
    policy.enabled = true;
    policy.disabledByDefault = false;
    LocalVectorPersistenceIndex index{policy};

    EmbeddingGenerationResult notGenerated;
    const auto refused = index.acceptIsolatedEmbeddingResult(
        notGenerated, {QStringLiteral("doc-a")}, VectorPersistenceSession{});
    QCOMPARE(refused.status, VectorPersistenceStatus::Refused);
    QCOMPARE(refused.readiness, VectorPersistenceReadiness::Refused);

    index.create(VectorPersistenceSession{});
    EmbeddingGenerationResult generated;
    generated.status = EmbeddingRuntimeStatus::Succeeded;
    generated.readiness = EmbeddingGenerationReadiness::Ready;
    generated.generatedVectorCount = 2;

    const auto accepted = index.acceptIsolatedEmbeddingResult(
        generated, {QStringLiteral("doc-b"), QStringLiteral("doc-a")}, VectorPersistenceSession{});
    QCOMPARE(accepted.status, VectorPersistenceStatus::Ready);
    QCOMPARE(accepted.snapshot.indexedItemCount, 2);
    QCOMPARE(index.itemCount(), 2);
    QVERIFY(accepted.checks.contains(QStringLiteral("Prompt mutation: disabled")));
    QVERIFY(accepted.checks.contains(QStringLiteral("Cloud/API/vector services: blocked")));
}

void SemanticRetrievalTest::vectorPersistenceRejectsStaleBusyAndBoundedOverflow() {
    VectorPersistencePolicy policy;
    policy.enabled = true;
    policy.disabledByDefault = false;
    VectorPersistenceBudget budget;
    budget.maxIndexedItems = 1;
    LocalVectorPersistenceIndex index{policy, budget};

    VectorPersistenceSession stale;
    stale.activeRequestId = QStringLiteral("old");
    stale.requestId = QStringLiteral("new");
    QCOMPARE(index.create(stale).status, VectorPersistenceStatus::Stale);

    VectorPersistenceSession busy;
    busy.busy = true;
    QCOMPARE(index.create(busy).status, VectorPersistenceStatus::Busy);

    index.create(VectorPersistenceSession{});
    EmbeddingGenerationResult generated;
    generated.status = EmbeddingRuntimeStatus::Succeeded;
    generated.readiness = EmbeddingGenerationReadiness::Ready;
    generated.generatedVectorCount = 2;

    const auto limited = index.acceptIsolatedEmbeddingResult(
        generated, {QStringLiteral("doc-a"), QStringLiteral("doc-b")}, VectorPersistenceSession{});
    QCOMPARE(limited.status, VectorPersistenceStatus::LimitReached);
    QCOMPARE(limited.snapshot.indexedItemCount, 1);
    QVERIFY(limited.summary.contains(QStringLiteral("1 rejected")));
}

void SemanticRetrievalTest::semanticSearchOrdersCandidatesDeterministicallyAndBoundsResults() {
    VectorPersistencePolicy persistencePolicy;
    persistencePolicy.enabled = true;
    persistencePolicy.disabledByDefault = false;
    LocalVectorPersistenceIndex index{persistencePolicy};
    index.create(VectorPersistenceSession{});

    EmbeddingGenerationResult generated;
    generated.status = EmbeddingRuntimeStatus::Succeeded;
    generated.readiness = EmbeddingGenerationReadiness::Ready;
    generated.generatedVectorCount = 3;
    index.acceptIsolatedEmbeddingResult(generated,
                                        {QStringLiteral("beta alpha memory"),
                                         QStringLiteral("alpha beta memory"),
                                         QStringLiteral("alpha unrelated")},
                                        VectorPersistenceSession{});

    SemanticSearchPolicy policy;
    policy.maxCandidates = 2;
    const auto first = index.searchLocalSemanticCandidates(QStringLiteral("alpha beta"), generated,
                                                           policy, SemanticSearchSession{});
    const auto second = index.searchLocalSemanticCandidates(QStringLiteral("alpha beta"), generated,
                                                            policy, SemanticSearchSession{});

    QCOMPARE(first.status, SemanticSearchStatus::Ready);
    QCOMPARE(first.candidates.size(), 2);
    QCOMPARE(first.candidates.at(0).summary, QStringLiteral("alpha beta memory"));
    QCOMPARE(first.candidates.at(1).summary, QStringLiteral("beta alpha memory"));
    QCOMPARE(first.candidates.at(0).rank, 1);
    QCOMPARE(first.candidates.at(1).rank, 2);
    QCOMPARE(first.candidates.at(0).summary, second.candidates.at(0).summary);
    QVERIFY(first.budget.summary.contains(QStringLiteral("2 bounded semantic candidates")));
    QVERIFY(first.arbitration.summary.contains(QStringLiteral("deterministic retrieval remains")));
}

void SemanticRetrievalTest::semanticSearchHandlesTimeoutStaleBusyAndEmptyIndex() {
    VectorPersistencePolicy persistencePolicy;
    persistencePolicy.enabled = true;
    persistencePolicy.disabledByDefault = false;
    LocalVectorPersistenceIndex emptyIndex{persistencePolicy};
    emptyIndex.create(VectorPersistenceSession{});

    EmbeddingGenerationResult generated;
    generated.status = EmbeddingRuntimeStatus::Succeeded;
    generated.readiness = EmbeddingGenerationReadiness::Ready;
    generated.generatedVectorCount = 1;

    const auto empty = emptyIndex.searchLocalSemanticCandidates(
        QStringLiteral("alpha"), generated, SemanticSearchPolicy{}, SemanticSearchSession{});
    QCOMPARE(empty.status, SemanticSearchStatus::Empty);
    QVERIFY(empty.summary.contains(QStringLiteral("empty")));

    LocalVectorPersistenceIndex index{persistencePolicy};
    index.create(VectorPersistenceSession{});
    index.acceptIsolatedEmbeddingResult(generated, {QStringLiteral("alpha beta")},
                                        VectorPersistenceSession{});

    SemanticSearchSession timeout;
    timeout.timeoutMs = 5;
    timeout.simulatedExecutionMs = 10;
    QCOMPARE(index
                 .searchLocalSemanticCandidates(QStringLiteral("alpha"), generated,
                                                SemanticSearchPolicy{}, timeout)
                 .status,
             SemanticSearchStatus::TimedOut);

    SemanticSearchSession stale;
    stale.activeRequestId = QStringLiteral("old");
    stale.requestId = QStringLiteral("new");
    QCOMPARE(index
                 .searchLocalSemanticCandidates(QStringLiteral("alpha"), generated,
                                                SemanticSearchPolicy{}, stale)
                 .status,
             SemanticSearchStatus::Stale);

    SemanticSearchSession busy;
    busy.busy = true;
    QCOMPARE(index
                 .searchLocalSemanticCandidates(QStringLiteral("alpha"), generated,
                                                SemanticSearchPolicy{}, busy)
                 .status,
             SemanticSearchStatus::Busy);
}

void SemanticRetrievalTest::semanticSearchEnforcesLocalOnlyNonAuthoritativePolicy() {
    VectorPersistencePolicy persistencePolicy;
    persistencePolicy.enabled = true;
    persistencePolicy.disabledByDefault = false;
    LocalVectorPersistenceIndex index{persistencePolicy};
    index.create(VectorPersistenceSession{});

    EmbeddingGenerationResult generated;
    generated.status = EmbeddingRuntimeStatus::Succeeded;
    generated.readiness = EmbeddingGenerationReadiness::Ready;
    generated.generatedVectorCount = 1;
    index.acceptIsolatedEmbeddingResult(generated, {QStringLiteral("alpha beta")},
                                        VectorPersistenceSession{});

    SemanticSearchPolicy policy;
    policy.cloudProvidersAllowed = true;
    auto refused = index.searchLocalSemanticCandidates(QStringLiteral("alpha"), generated, policy,
                                                       SemanticSearchSession{});
    QCOMPARE(refused.status, SemanticSearchStatus::Refused);
    QVERIFY(refused.checks.contains(QStringLiteral("Cloud/API/vector providers: blocked")));

    policy = SemanticSearchPolicy{};
    policy.authoritative = true;
    refused = index.searchLocalSemanticCandidates(QStringLiteral("alpha"), generated, policy,
                                                  SemanticSearchSession{});
    QCOMPARE(refused.status, SemanticSearchStatus::Refused);

    EmbeddingGenerationResult notGenerated;
    refused = index.searchLocalSemanticCandidates(QStringLiteral("alpha"), notGenerated,
                                                  SemanticSearchPolicy{}, SemanticSearchSession{});
    QCOMPARE(refused.status, SemanticSearchStatus::Refused);
    QVERIFY(refused.summary.contains(QStringLiteral("isolated embedding")));
}

void SemanticRetrievalTest::hybridBridgePreservesDeterministicAuthorityAndBounds() {
    HybridRetrievalBridgePolicy policy;
    policy.maxMergedCandidates = 3;
    const auto planning = deterministicPlanningForBridge();
    const auto result = hybridRetrievalBridge(
        planning, semanticSearchResultForBridge(SemanticSearchStatus::Ready), policy);

    QCOMPARE(result.status, HybridRetrievalBridgeStatus::Ready);
    QCOMPARE(result.candidates.size(), 3);
    QCOMPARE(result.budget.semanticFillCount, 1);
    QCOMPARE(result.candidates.at(0).deterministic, true);
    QCOMPARE(result.candidates.at(1).deterministic, true);
    QCOMPARE(result.candidates.at(2).semanticAdvisory, true);
    QCOMPARE(result.candidates.at(0).title, QStringLiteral("Recent Conversation"));
    QVERIFY(result.arbitration.summary.contains(QStringLiteral("Deterministic-first")));
    QVERIFY(result.checks.contains(QStringLiteral("RetrievalPlanningResult mutation: no")));
    QCOMPARE(planning.selectedCandidateCount, 2);
}

void SemanticRetrievalTest::hybridBridgeFallsBackForDisabledStaleBusyAndTimeoutSemanticSources() {
    const auto planning = deterministicPlanningForBridge();

    auto disabled = hybridRetrievalBridge(
        planning, semanticSearchResultForBridge(SemanticSearchStatus::Disabled),
        HybridRetrievalBridgePolicy{});
    QCOMPARE(disabled.status, HybridRetrievalBridgeStatus::DeterministicOnly);
    QCOMPARE(disabled.budget.semanticFillCount, 0);
    QVERIFY(disabled.fallbackSummary.contains(QStringLiteral("Semantic source unavailable")));

    auto empty =
        hybridRetrievalBridge(planning, semanticSearchResultForBridge(SemanticSearchStatus::Empty),
                              HybridRetrievalBridgePolicy{});
    QCOMPARE(empty.status, HybridRetrievalBridgeStatus::DeterministicOnly);

    auto stale =
        hybridRetrievalBridge(planning, semanticSearchResultForBridge(SemanticSearchStatus::Stale),
                              HybridRetrievalBridgePolicy{});
    QCOMPARE(stale.status, HybridRetrievalBridgeStatus::Stale);

    auto busy =
        hybridRetrievalBridge(planning, semanticSearchResultForBridge(SemanticSearchStatus::Busy),
                              HybridRetrievalBridgePolicy{});
    QCOMPARE(busy.status, HybridRetrievalBridgeStatus::Busy);

    HybridRetrievalBridgePolicy timeoutPolicy;
    timeoutPolicy.timeoutMs = 1000;
    auto timedOut = hybridRetrievalBridge(
        planning, semanticSearchResultForBridge(SemanticSearchStatus::TimedOut), timeoutPolicy);
    QCOMPARE(timedOut.status, HybridRetrievalBridgeStatus::TimedOut);
    QVERIFY(timedOut.fallbackSummary.contains(QStringLiteral("deterministic retrieval remains")));
}

void SemanticRetrievalTest::hybridBridgeRefusesPromptOrPlanningMutationAuthority() {
    const auto planning = deterministicPlanningForBridge();
    HybridRetrievalBridgePolicy policy;
    policy.promptMutationEnabled = true;
    auto refused = hybridRetrievalBridge(
        planning, semanticSearchResultForBridge(SemanticSearchStatus::Ready), policy);
    QCOMPARE(refused.status, HybridRetrievalBridgeStatus::Refused);

    policy = HybridRetrievalBridgePolicy{};
    policy.retrievalPlanningMutationEnabled = true;
    refused = hybridRetrievalBridge(
        planning, semanticSearchResultForBridge(SemanticSearchStatus::Ready), policy);
    QCOMPARE(refused.status, HybridRetrievalBridgeStatus::Refused);

    policy = HybridRetrievalBridgePolicy{};
    policy.deterministicRetrievalAuthoritative = false;
    refused = hybridRetrievalBridge(
        planning, semanticSearchResultForBridge(SemanticSearchStatus::Ready), policy);
    QCOMPARE(refused.status, HybridRetrievalBridgeStatus::Refused);
}

void SemanticRetrievalTest::semanticAcceptanceApprovesBoundedSupplementsOnly() {
    SemanticAcceptancePolicy policy;
    policy.maxAcceptedSupplements = 1;
    policy.maxSupplementCharacters = 128;
    const auto planning = deterministicPlanningForBridge();
    const auto search = semanticSearchResultForBridge(SemanticSearchStatus::Ready);
    const auto bridge = hybridRetrievalBridge(planning, search, HybridRetrievalBridgePolicy{});
    const auto result = semanticAcceptance(planning, bridge, search, policy);

    QCOMPARE(result.status, SemanticAcceptanceStatus::Ready);
    QCOMPARE(result.acceptedCandidates.size(), 1);
    QCOMPARE(result.budget.acceptedSupplementCount, 1);
    QCOMPARE(result.acceptedCandidates.at(0).semantic, true);
    QCOMPARE(result.acceptedCandidates.at(0).supplementalOnly, true);
    QCOMPARE(result.acceptedCandidates.at(0).deterministicOffsetRank,
             planning.selectedCandidates.size() + 1);
    QVERIFY(result.summary.contains(QStringLiteral("supplemental")));
    QVERIFY(result.arbitration.summary.contains(QStringLiteral("Deterministic acceptance")));
    QVERIFY(result.checks.contains(QStringLiteral("Deterministic candidate replacement: no")));
    QCOMPARE(planning.selectedCandidateCount, 2);
    QCOMPARE(planning.selectedCandidates.at(0).title, QStringLiteral("Recent Conversation"));
}

void SemanticRetrievalTest::semanticAcceptanceFallsBackForDisabledErrorTimeoutStaleAndBusy() {
    const auto planning = deterministicPlanningForBridge();
    const auto readySearch = semanticSearchResultForBridge(SemanticSearchStatus::Ready);
    const auto readyBridge =
        hybridRetrievalBridge(planning, readySearch, HybridRetrievalBridgePolicy{});

    SemanticAcceptancePolicy disabledPolicy;
    disabledPolicy.enabled = false;
    auto result = semanticAcceptance(planning, readyBridge, readySearch, disabledPolicy);
    QCOMPARE(result.status, SemanticAcceptanceStatus::Disabled);
    QCOMPARE(result.acceptedCandidates.size(), 0);
    QVERIFY(result.fallback.summary.contains(QStringLiteral("disabled")));

    auto staleSearch = semanticSearchResultForBridge(SemanticSearchStatus::Stale);
    auto staleBridge = hybridRetrievalBridge(planning, staleSearch, HybridRetrievalBridgePolicy{});
    result = semanticAcceptance(planning, staleBridge, staleSearch, SemanticAcceptancePolicy{});
    QCOMPARE(result.status, SemanticAcceptanceStatus::Stale);

    auto busySearch = semanticSearchResultForBridge(SemanticSearchStatus::Busy);
    auto busyBridge = hybridRetrievalBridge(planning, busySearch, HybridRetrievalBridgePolicy{});
    result = semanticAcceptance(planning, busyBridge, busySearch, SemanticAcceptancePolicy{});
    QCOMPARE(result.status, SemanticAcceptanceStatus::Busy);

    auto timeoutSearch = semanticSearchResultForBridge(SemanticSearchStatus::TimedOut);
    auto timeoutBridge =
        hybridRetrievalBridge(planning, timeoutSearch, HybridRetrievalBridgePolicy{});
    result = semanticAcceptance(planning, timeoutBridge, timeoutSearch, SemanticAcceptancePolicy{});
    QCOMPARE(result.status, SemanticAcceptanceStatus::TimedOut);
    QVERIFY(result.fallback.summary.contains(QStringLiteral("timed out")));

    auto refusedSearch = semanticSearchResultForBridge(SemanticSearchStatus::Refused);
    auto refusedBridge =
        hybridRetrievalBridge(planning, refusedSearch, HybridRetrievalBridgePolicy{});
    result = semanticAcceptance(planning, refusedBridge, refusedSearch, SemanticAcceptancePolicy{});
    QCOMPARE(result.status, SemanticAcceptanceStatus::Refused);
    QVERIFY(result.fallback.summary.contains(QStringLiteral("Semantic error")));
}

void SemanticRetrievalTest::semanticAcceptanceRefusesAuthorityMutationAndPreservesInputs() {
    const auto planning = deterministicPlanningForBridge();
    const auto search = semanticSearchResultForBridge(SemanticSearchStatus::Ready);
    const auto bridge = hybridRetrievalBridge(planning, search, HybridRetrievalBridgePolicy{});

    SemanticAcceptancePolicy policy;
    policy.promptMutationEnabled = true;
    auto result = semanticAcceptance(planning, bridge, search, policy);
    QCOMPARE(result.status, SemanticAcceptanceStatus::Refused);

    policy = SemanticAcceptancePolicy{};
    policy.retrievalPlanningMutationEnabled = true;
    result = semanticAcceptance(planning, bridge, search, policy);
    QCOMPARE(result.status, SemanticAcceptanceStatus::Refused);

    policy = SemanticAcceptancePolicy{};
    policy.deterministicRetrievalAuthoritative = false;
    result = semanticAcceptance(planning, bridge, search, policy);
    QCOMPARE(result.status, SemanticAcceptanceStatus::Refused);

    QCOMPARE(planning.selectedCandidateCount, 2);
    QCOMPARE(planning.selectedCandidates.at(0).title, QStringLiteral("Recent Conversation"));
    QCOMPARE(search.candidates.size(), 2);
    QCOMPARE(bridge.candidates.at(0).deterministic, true);
}

void SemanticRetrievalTest::semanticSupplementAssemblyDisabledReturnsSafeFallback() {
    const auto planning = deterministicPlanningForBridge();
    const auto search = semanticSearchResultForBridge(SemanticSearchStatus::Ready);
    const auto bridge = hybridRetrievalBridge(planning, search, HybridRetrievalBridgePolicy{});
    const auto acceptance =
        semanticAcceptance(planning, bridge, search, SemanticAcceptancePolicy{});

    const auto result = assembleSemanticSupplements(acceptance, SemanticSupplementAssemblyPolicy{});

    QCOMPARE(result.status, SemanticSupplementAssemblyStatus::Disabled);
    QCOMPARE(result.bundle.blockCount, 0);
    QCOMPARE(result.assembled, false);
    QVERIFY(result.fallbackSummary.contains(QStringLiteral("deterministic prompt behavior")));
    QVERIFY(result.checks.contains(QStringLiteral("Live prompt inclusion: blocked")));
    QVERIFY(result.checks.contains(QStringLiteral("RetrievalPlanningResult mutation: no")));
    QVERIFY(result.checks.contains(QStringLiteral("PromptContextBlock mutation: no")));
    QCOMPARE(planning.selectedCandidateCount, 2);
}

void SemanticRetrievalTest::semanticSupplementAssemblyBuildsBoundedSeparateMetadata() {
    const auto planning = deterministicPlanningForBridge();
    const auto search = semanticSearchResultForBridge(SemanticSearchStatus::Ready);
    const auto bridge = hybridRetrievalBridge(planning, search, HybridRetrievalBridgePolicy{});
    const auto acceptance =
        semanticAcceptance(planning, bridge, search, SemanticAcceptancePolicy{});

    SemanticSupplementAssemblyPolicy policy;
    policy.enabled = true;
    policy.allowTestOnlyAssembly = true;
    policy.maxSupplementBlocks = 2;
    policy.maxCharacters = 128;
    const auto result = assembleSemanticSupplements(acceptance, policy);

    QCOMPARE(result.status, SemanticSupplementAssemblyStatus::Ready);
    QCOMPARE(result.bundle.blockCount, 2);
    QCOMPARE(result.bundle.separateFromDeterministicContext, true);
    QCOMPARE(result.bundle.blocks.at(0).semantic, true);
    QCOMPARE(result.bundle.blocks.at(0).supplementalOnly, true);
    QCOMPARE(result.bundle.blocks.at(0).nonAuthoritative, true);
    QVERIFY(result.summary.contains(QStringLiteral("live prompt behavior is unchanged")));
    QVERIFY(result.safety.summary.contains(QStringLiteral("non-authoritative")));
    QCOMPARE(planning.selectedCandidateCount, 2);
}

void SemanticRetrievalTest::semanticSupplementAssemblyOrdersAndTruncatesDeterministically() {
    SemanticAcceptanceResult acceptance;
    acceptance.status = SemanticAcceptanceStatus::Ready;
    acceptance.budget.elapsedMs = 10;
    acceptance.acceptedCandidates = {
        {QStringLiteral("semantic-b"), QStringLiteral("Semantic Supplemental"),
         QStringLiteral("Beta"), QStringLiteral("bbbbbbbbbbbbbbbbbbbb"), 2, 4, 20, true, true,
         QStringLiteral("accepted")},
        {QStringLiteral("semantic-a"), QStringLiteral("Semantic Supplemental"),
         QStringLiteral("Alpha"), QStringLiteral("aaaaaaaaaaaaaaaaaaaa"), 1, 3, 20, true, true,
         QStringLiteral("accepted")},
    };

    SemanticSupplementAssemblyPolicy policy;
    policy.enabled = true;
    policy.allowTestOnlyAssembly = true;
    policy.maxSupplementBlocks = 2;
    policy.maxCharacters = 25;
    const auto result = assembleSemanticSupplements(acceptance, policy);

    QCOMPARE(result.status, SemanticSupplementAssemblyStatus::Truncated);
    QCOMPARE(result.bundle.blockCount, 2);
    QCOMPARE(result.bundle.blocks.at(0).id, QStringLiteral("semantic-a"));
    QCOMPARE(result.bundle.blocks.at(1).id, QStringLiteral("semantic-b"));
    QCOMPARE(result.bundle.truncatedBlockCount, 1);
    QCOMPARE(result.budget.includedCharacters, 25);
}

void SemanticRetrievalTest::semanticSupplementAssemblyRefusesPromptAuthorityAndStaleState() {
    const auto planning = deterministicPlanningForBridge();
    const auto search = semanticSearchResultForBridge(SemanticSearchStatus::Ready);
    const auto bridge = hybridRetrievalBridge(planning, search, HybridRetrievalBridgePolicy{});
    const auto acceptance =
        semanticAcceptance(planning, bridge, search, SemanticAcceptancePolicy{});

    SemanticSupplementAssemblyPolicy policy;
    policy.enabled = true;
    policy.allowTestOnlyAssembly = true;
    policy.includeInLivePrompt = true;
    auto result = assembleSemanticSupplements(acceptance, policy);
    QCOMPARE(result.status, SemanticSupplementAssemblyStatus::Refused);
    QCOMPARE(result.bundle.blockCount, 0);

    policy = SemanticSupplementAssemblyPolicy{};
    policy.enabled = true;
    policy.allowTestOnlyAssembly = true;
    SemanticAcceptanceResult staleAcceptance;
    staleAcceptance.status = SemanticAcceptanceStatus::Stale;
    result = assembleSemanticSupplements(staleAcceptance, policy);
    QCOMPARE(result.status, SemanticSupplementAssemblyStatus::Stale);
    QVERIFY(result.fallbackSummary.contains(QStringLiteral("deterministic prompt behavior")));
}

void SemanticRetrievalTest::semanticPromptAuthorityDefaultsToDeniedDisabledFallback() {
    const auto planning = deterministicPlanningForBridge();
    const auto search = semanticSearchResultForBridge(SemanticSearchStatus::Ready);
    const auto bridge = hybridRetrievalBridge(planning, search, HybridRetrievalBridgePolicy{});
    const auto acceptance =
        semanticAcceptance(planning, bridge, search, SemanticAcceptancePolicy{});

    SemanticSupplementAssemblyPolicy assemblyPolicy;
    assemblyPolicy.enabled = true;
    assemblyPolicy.allowTestOnlyAssembly = true;
    const auto assembly = assembleSemanticSupplements(acceptance, assemblyPolicy);
    const auto result = evaluateSemanticPromptAuthority(assembly, SemanticPromptAuthorityPolicy{});

    QCOMPARE(result.status, SemanticPromptAuthorityStatus::Disabled);
    QCOMPARE(result.decision, SemanticPromptAuthorityDecision::Denied);
    QCOMPARE(result.allowed, false);
    QCOMPARE(result.wouldInclude, false);
    QCOMPARE(result.livePromptMutationAllowed, false);
    QVERIFY(result.fallback.summary.contains(QStringLiteral("deterministic prompt assembly")));
    QVERIFY(result.audit.denialReason.contains(QStringLiteral("disabled")));
    QVERIFY(result.checks.contains(QStringLiteral("Live prompt mutation: blocked")));
    QVERIFY(result.checks.contains(QStringLiteral("PromptContextBlock mutation: no")));
    QVERIFY(result.checks.contains(QStringLiteral("RetrievalPlanningResult mutation: no")));
    QCOMPARE(planning.selectedCandidateCount, 2);
}

void SemanticRetrievalTest::semanticPromptAuthorityAllowsReadinessMetadataOnly() {
    const auto planning = deterministicPlanningForBridge();
    const auto search = semanticSearchResultForBridge(SemanticSearchStatus::Ready);
    const auto bridge = hybridRetrievalBridge(planning, search, HybridRetrievalBridgePolicy{});
    const auto acceptance =
        semanticAcceptance(planning, bridge, search, SemanticAcceptancePolicy{});

    SemanticSupplementAssemblyPolicy assemblyPolicy;
    assemblyPolicy.enabled = true;
    assemblyPolicy.allowTestOnlyAssembly = true;
    const auto assembly = assembleSemanticSupplements(acceptance, assemblyPolicy);

    SemanticPromptAuthorityPolicy policy;
    policy.enabled = true;
    policy.promptInjectionExplicitlyEnabled = true;
    policy.semanticPromptAuthorityAllowed = true;
    policy.allowTestOnlyWouldIncludeMetadata = true;
    const auto result = evaluateSemanticPromptAuthority(assembly, policy);

    QCOMPARE(result.status, SemanticPromptAuthorityStatus::WouldIncludeMetadataOnly);
    QCOMPARE(result.decision, SemanticPromptAuthorityDecision::WouldIncludeMetadataOnly);
    QCOMPARE(result.allowed, true);
    QCOMPARE(result.wouldInclude, true);
    QCOMPARE(result.livePromptMutationAllowed, false);
    QCOMPARE(result.wouldIncludeBlockCount, assembly.bundle.blockCount);
    QVERIFY(result.summary.contains(QStringLiteral("test-only")));
    QVERIFY(
        result.readiness.summary.contains(QStringLiteral("live prompt inclusion remains blocked")));
    QVERIFY(result.checks.contains(
        QStringLiteral("Semantic prompt authority policy allows inclusion: yes")));
    QCOMPARE(planning.selectedCandidateCount, 2);
}

void SemanticRetrievalTest::semanticPromptAuthoritySafetyReportBlocksUnsafeInclusion() {
    const auto planning = deterministicPlanningForBridge();
    const auto search = semanticSearchResultForBridge(SemanticSearchStatus::Ready);
    const auto bridge = hybridRetrievalBridge(planning, search, HybridRetrievalBridgePolicy{});
    const auto acceptance =
        semanticAcceptance(planning, bridge, search, SemanticAcceptancePolicy{});

    SemanticSupplementAssemblyPolicy assemblyPolicy;
    assemblyPolicy.enabled = true;
    assemblyPolicy.allowTestOnlyAssembly = true;
    auto assembly = assembleSemanticSupplements(acceptance, assemblyPolicy);
    assembly.safety.safe = false;

    SemanticPromptAuthorityPolicy policy;
    policy.enabled = true;
    policy.promptInjectionExplicitlyEnabled = true;
    policy.semanticPromptAuthorityAllowed = true;
    policy.allowTestOnlyWouldIncludeMetadata = true;
    const auto result = evaluateSemanticPromptAuthority(assembly, policy);

    QCOMPARE(result.status, SemanticPromptAuthorityStatus::SafetyBlocked);
    QCOMPARE(result.decision, SemanticPromptAuthorityDecision::Denied);
    QCOMPARE(result.allowed, false);
    QCOMPARE(result.wouldInclude, false);
    QVERIFY(result.audit.denialReason.contains(QStringLiteral("safety")));
    QVERIFY(result.fallback.summary.contains(QStringLiteral("Safety-report fallback")));
    QCOMPARE(planning.selectedCandidateCount, 2);
}

void SemanticRetrievalTest::semanticPromptInclusionDisabledLeavesPromptUnchanged() {
    const auto deterministicPrompt = deterministicPromptForInclusion();
    const auto assembly = readySemanticAssemblyForInclusion();
    const auto authority = evaluateSemanticPromptAuthority(assembly, allowingAuthorityPolicy());

    const auto result = includeSemanticPromptSupplements(deterministicPrompt, assembly, authority,
                                                         SemanticPromptInclusionPolicy{});

    QCOMPARE(result.status, SemanticPromptInclusionStatus::Disabled);
    QCOMPARE(result.prompt, deterministicPrompt.prompt);
    QCOMPARE(result.included, false);
    QCOMPARE(result.budget.includedSupplementBlocks, 0);
    QVERIFY(result.fallback.summary.contains(QStringLiteral("deterministic-only")));
}

void SemanticRetrievalTest::
    semanticPromptInclusionAppendsDelimitedBlockAfterDeterministicContext() {
    const auto deterministicPrompt = deterministicPromptForInclusion();
    auto assembly = readySemanticAssemblyForInclusion();
    assembly.bundle.blocks[0].summary.append(
        QStringLiteral("; score: 0.91; vector: [1, 2]; debug payload: hidden"));
    const auto authority = evaluateSemanticPromptAuthority(assembly, allowingAuthorityPolicy());

    SemanticPromptInclusionPolicy policy;
    policy.enabled = true;
    policy.contextInjectionEnabled = true;
    const auto result =
        includeSemanticPromptSupplements(deterministicPrompt, assembly, authority, policy);

    QCOMPARE(result.status, SemanticPromptInclusionStatus::Included);
    QCOMPARE(result.included, true);
    QCOMPARE(result.budget.includedSupplementBlocks, assembly.bundle.blockCount);
    QVERIFY(result.prompt.contains(
        QStringLiteral("[Semantic Supplemental Context - Non-Authoritative]")));
    QVERIFY(result.prompt.contains(QStringLiteral("Supplemental only.")));
    QVERIFY(result.prompt.indexOf(QStringLiteral("--- Conversation ---")) <
            result.prompt.indexOf(QStringLiteral("[Semantic Supplemental Context")));
    QVERIFY(result.prompt.indexOf(QStringLiteral("[/Semantic Supplemental Context]")) <
            result.prompt.indexOf(QStringLiteral("User prompt:")));
    QVERIFY(result.prompt.indexOf(QStringLiteral("--- Conversation ---")) <
            result.prompt.indexOf(QStringLiteral("--- Committed Memory ---")));
    QVERIFY(result.prompt.contains(QStringLiteral("deterministic memory")));
    QVERIFY(!result.prompt.contains(QStringLiteral("score:")));
    QVERIFY(!result.prompt.contains(QStringLiteral("vector:")));
    QVERIFY(!result.prompt.contains(QStringLiteral("debug payload")));
    QVERIFY(
        result.summary.contains(QStringLiteral("deterministic retrieval remains authoritative")));
}

void SemanticRetrievalTest::semanticPromptInclusionBoundsAndTruncatesDeterministically() {
    const auto deterministicPrompt = deterministicPromptForInclusion();
    const auto assembly = readySemanticAssemblyForInclusion();
    const auto authority = evaluateSemanticPromptAuthority(assembly, allowingAuthorityPolicy());

    SemanticPromptInclusionPolicy policy;
    policy.enabled = true;
    policy.contextInjectionEnabled = true;
    policy.maxSupplementBlocks = 1;
    policy.maxCharacters = 10;

    const auto result =
        includeSemanticPromptSupplements(deterministicPrompt, assembly, authority, policy);

    QCOMPARE(result.status, SemanticPromptInclusionStatus::Truncated);
    QCOMPARE(result.budget.includedSupplementBlocks, 1);
    QCOMPARE(result.budget.includedCharacters, 10);
    QCOMPARE(result.budget.truncatedBlockCount, 1);
    QVERIFY(result.prompt.contains(QStringLiteral("semantic a")));
    QVERIFY(!result.prompt.contains(QStringLiteral("semantic advisory beta")));
}

void SemanticRetrievalTest::semanticPromptInclusionFallsBackForDeniedUnsafeTimeoutStaleAndEmpty() {
    const auto deterministicPrompt = deterministicPromptForInclusion();
    const auto assembly = readySemanticAssemblyForInclusion();
    const auto authority = evaluateSemanticPromptAuthority(assembly, allowingAuthorityPolicy());

    SemanticPromptInclusionPolicy policy;
    policy.enabled = true;
    policy.contextInjectionEnabled = true;

    auto deniedAuthority = authority;
    deniedAuthority.allowed = false;
    deniedAuthority.wouldInclude = false;
    auto result =
        includeSemanticPromptSupplements(deterministicPrompt, assembly, deniedAuthority, policy);
    QCOMPARE(result.status, SemanticPromptInclusionStatus::SafetyBlocked);
    QCOMPARE(result.prompt, deterministicPrompt.prompt);

    auto unsafeAssembly = assembly;
    unsafeAssembly.safety.safe = false;
    result =
        includeSemanticPromptSupplements(deterministicPrompt, unsafeAssembly, authority, policy);
    QCOMPARE(result.status, SemanticPromptInclusionStatus::SafetyBlocked);
    QCOMPARE(result.prompt, deterministicPrompt.prompt);

    auto timedOutAssembly = assembly;
    timedOutAssembly.budget.elapsedMs = 2000;
    result =
        includeSemanticPromptSupplements(deterministicPrompt, timedOutAssembly, authority, policy);
    QCOMPARE(result.status, SemanticPromptInclusionStatus::TimedOut);
    QCOMPARE(result.prompt, deterministicPrompt.prompt);

    auto staleAssembly = assembly;
    staleAssembly.status = SemanticSupplementAssemblyStatus::Stale;
    result =
        includeSemanticPromptSupplements(deterministicPrompt, staleAssembly, authority, policy);
    QCOMPARE(result.status, SemanticPromptInclusionStatus::Stale);
    QCOMPARE(result.prompt, deterministicPrompt.prompt);

    auto busyAssembly = assembly;
    busyAssembly.status = SemanticSupplementAssemblyStatus::Busy;
    result = includeSemanticPromptSupplements(deterministicPrompt, busyAssembly, authority, policy);
    QCOMPARE(result.status, SemanticPromptInclusionStatus::Busy);
    QCOMPARE(result.prompt, deterministicPrompt.prompt);

    auto refusedAssembly = assembly;
    refusedAssembly.status = SemanticSupplementAssemblyStatus::Refused;
    result =
        includeSemanticPromptSupplements(deterministicPrompt, refusedAssembly, authority, policy);
    QCOMPARE(result.status, SemanticPromptInclusionStatus::Refused);
    QCOMPARE(result.prompt, deterministicPrompt.prompt);

    auto emptyAssembly = assembly;
    emptyAssembly.bundle.blocks.clear();
    emptyAssembly.bundle.blockCount = 0;
    result =
        includeSemanticPromptSupplements(deterministicPrompt, emptyAssembly, authority, policy);
    QCOMPARE(result.status, SemanticPromptInclusionStatus::Empty);
    QCOMPARE(result.prompt, deterministicPrompt.prompt);
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
