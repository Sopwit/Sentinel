#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

#include "sentinel/core/ContextAssembly.h"

namespace sentinel::core {

enum class EmbeddingProviderStatus : std::uint8_t {
    NotConfigured,
    Disabled,
    Ready,
    Error,
};

enum class VectorIndexStatus : std::uint8_t {
    NotConfigured,
    Disabled,
    Empty,
    Ready,
    Error,
};

enum class SemanticRetrievalStatus : std::uint8_t {
    Disabled,
    NotConfigured,
    ReadyMetadataOnly,
};

enum class SemanticCandidateSource : std::uint8_t {
    RecentConversation,
    DeterministicSummary,
    CommittedMemory,
    RuntimeMetadata,
    OrchestrationMetadata,
    FutureSemanticVector,
};

enum class SemanticCandidateSelection : std::uint8_t {
    NotEvaluated,
    Selected,
    Excluded,
    Truncated,
};

enum class SemanticCandidateStatus : std::uint8_t {
    Disabled,
    Empty,
    Ready,
    Truncated,
};

enum class HybridRetrievalStatus : std::uint8_t {
    DeterministicOnly,
    SemanticDisabled,
    ReadyMetadataOnly,
};

enum class SemanticArbitrationStatus : std::uint8_t {
    Disabled,
    Empty,
    Simulated,
};

enum class SemanticProviderMode : std::uint8_t {
    Disabled,
    FakeInMemory,
    LocalOllamaEmbeddings,
    LocalFileVectorIndex,
};

enum class SemanticProviderReadiness : std::uint8_t {
    Disabled,
    Planned,
    ReadyMetadataOnly,
    Blocked,
};

enum class SemanticProviderHealth : std::uint8_t {
    NotChecked,
    MetadataOnly,
    Blocked,
};

enum class SemanticProviderCapability : std::uint8_t {
    LocalOnly,
    FakeInMemory,
    EmbeddingGeneration,
    VectorIndexing,
    MetadataOnlyHealth,
    PromptMutationBlocked,
    VectorWritesBlocked,
};

enum class EmbeddingRuntimeReadiness : std::uint8_t {
    NotConfigured,
    Planned,
    Blocked,
};

enum class EmbeddingRuntimeStatus : std::uint8_t {
    Disabled,
    Ready,
    Running,
    Succeeded,
    Failed,
    TimedOut,
    Stale,
    Busy,
    Refused,
};

enum class EmbeddingRuntimeHealth : std::uint8_t {
    NotChecked,
    LocalOnlyReady,
    Blocked,
    Failed,
};

enum class EmbeddingGenerationReadiness : std::uint8_t {
    Refused,
    Ready,
};

enum class VectorPersistenceStatus : std::uint8_t {
    Disabled,
    Empty,
    Ready,
    Created,
    Reset,
    Cleared,
    Refused,
    Busy,
    Stale,
    LimitReached,
};

enum class VectorPersistenceHealth : std::uint8_t {
    NotChecked,
    LocalOnlyReady,
    Empty,
    Blocked,
};

enum class VectorPersistenceReadiness : std::uint8_t {
    Disabled,
    Ready,
    Refused,
};

enum class SemanticSearchStatus : std::uint8_t {
    Disabled,
    Empty,
    Ready,
    TimedOut,
    Stale,
    Busy,
    Refused,
};

enum class HybridRetrievalBridgeStatus : std::uint8_t {
    Disabled,
    DeterministicOnly,
    Ready,
    TimedOut,
    Stale,
    Busy,
    Refused,
};

enum class SemanticAcceptanceStatus : std::uint8_t {
    Disabled,
    DeterministicOnly,
    Ready,
    TimedOut,
    Stale,
    Busy,
    Refused,
};

QString embeddingProviderStatusName(EmbeddingProviderStatus status);
QString vectorIndexStatusName(VectorIndexStatus status);
QString semanticRetrievalStatusName(SemanticRetrievalStatus status);
QString semanticCandidateSourceName(SemanticCandidateSource source);
QString semanticCandidateSelectionName(SemanticCandidateSelection selection);
QString semanticCandidateStatusName(SemanticCandidateStatus status);
QString hybridRetrievalStatusName(HybridRetrievalStatus status);
QString semanticArbitrationStatusName(SemanticArbitrationStatus status);
QString semanticProviderModeName(SemanticProviderMode mode);
QString semanticProviderReadinessName(SemanticProviderReadiness readiness);
QString semanticProviderHealthName(SemanticProviderHealth health);
QString semanticProviderCapabilityName(SemanticProviderCapability capability);
QString embeddingRuntimeReadinessName(EmbeddingRuntimeReadiness readiness);
QString embeddingRuntimeStatusName(EmbeddingRuntimeStatus status);
QString embeddingRuntimeHealthName(EmbeddingRuntimeHealth health);
QString embeddingGenerationReadinessName(EmbeddingGenerationReadiness readiness);
QString vectorPersistenceStatusName(VectorPersistenceStatus status);
QString vectorPersistenceHealthName(VectorPersistenceHealth health);
QString vectorPersistenceReadinessName(VectorPersistenceReadiness readiness);
QString semanticSearchStatusName(SemanticSearchStatus status);
QString hybridRetrievalBridgeStatusName(HybridRetrievalBridgeStatus status);
QString semanticAcceptanceStatusName(SemanticAcceptanceStatus status);

struct EmbeddingVector {
    QList<double> values;
    QString modelSummary = QStringLiteral("Deterministic fake embedding vector");
};

inline bool operator==(const EmbeddingVector& left, const EmbeddingVector& right) {
    return left.values == right.values && left.modelSummary == right.modelSummary;
}

struct EmbeddingDocument {
    QString id;
    QString text;
    QString source;
    QString summary;
};

struct EmbeddingProviderPolicy {
    bool enabled = false;
    bool localOnly = true;
    bool fakeOnly = false;
    int dimensions = 8;
    QString status = QStringLiteral("Disabled");
    QString summary = QStringLiteral(
        "Embedding provider execution is disabled until a later explicit semantic phase.");
};

struct EmbeddingRequest {
    QList<EmbeddingDocument> documents;
    EmbeddingProviderPolicy policy;
};

struct EmbeddingResult {
    EmbeddingProviderStatus status = EmbeddingProviderStatus::Disabled;
    EmbeddingProviderPolicy policy;
    QList<EmbeddingDocument> documents;
    QList<EmbeddingVector> vectors;
    int documentCount = 0;
    QString summary = QStringLiteral("No embeddings generated.");
    QStringList checks;
};

struct VectorIndexPolicy {
    bool enabled = false;
    bool inMemoryOnly = true;
    bool fakeOnly = false;
    int dimensions = 8;
    QString status = QStringLiteral("Disabled");
    QString summary =
        QStringLiteral("Vector indexing is disabled until a later explicit semantic phase.");
};

struct VectorSearchQuery {
    EmbeddingVector vector;
    int maxResults = 5;
    double minimumScore = 0.0;
};

struct VectorSearchCandidate {
    EmbeddingDocument document;
    double score = 0.0;
    int rank = 0;
    QString summary;
};

struct VectorSearchResult {
    VectorIndexStatus status = VectorIndexStatus::Empty;
    VectorIndexPolicy policy;
    QList<VectorSearchCandidate> candidates;
    int indexedItemCount = 0;
    int resultCount = 0;
    QString summary = QStringLiteral("No vector search results.");
};

struct SemanticRetrievalPolicy {
    bool enabled = false;
    bool semanticRankingEnabled = false;
    bool promptInjectionEnabled = false;
    bool exposeRawVectorsToQml = false;
    QString status = QStringLiteral("Disabled");
    QString summary = QStringLiteral(
        "Semantic retrieval is disabled; embedding and vector abstractions are readiness metadata "
        "only.");
};

struct SemanticCandidatePolicy {
    bool enabled = true;
    bool deterministicOnly = true;
    bool semanticRankingEnabled = false;
    bool vectorCandidatesEnabled = false;
    bool promptMutationEnabled = false;
    bool preserveSourceIsolation = true;
    bool preserveChronology = true;
    int maxCharacters = 3200;
    QString status = QStringLiteral("Metadata Only");
    QString summary = QStringLiteral(
        "Semantic candidate orchestration is deterministic metadata only; semantic retrieval is "
        "disabled.");
};

struct SemanticCandidateBudget {
    int maxCharacters = 3200;
    int estimatedCharacters = 0;
    int allocatedCharacters = 0;
    int includedCharacters = 0;
    int remainingCharacters = 3200;
    QString summary = QStringLiteral("0 of 3200 candidate characters selected.");
};

struct SemanticCandidate {
    SemanticCandidateSource source = SemanticCandidateSource::RecentConversation;
    QString id;
    QString title;
    QString content;
    int originalSize = 0;
    int selectedSize = 0;
    SemanticCandidateSelection selection = SemanticCandidateSelection::NotEvaluated;
    bool selected = false;
    bool truncated = false;
    QString exclusionReason;
    QString summary;
};

struct SemanticCandidateWindow {
    int candidateCount = 0;
    int selectedCandidateCount = 0;
    int excludedCandidateCount = 0;
    int truncatedCandidateCount = 0;
    QString chronologySummary =
        QStringLiteral("Chronology is preserved inside deterministic conversation sources.");
};

struct SemanticCandidateArbitration {
    SemanticCandidateStatus status = SemanticCandidateStatus::Empty;
    SemanticCandidateBudget budget;
    QList<SemanticCandidate> candidates;
    QList<SemanticCandidate> selectedCandidates;
    QString orderingSummary =
        QStringLiteral("Deterministic source order: recent conversation, summaries, committed "
                       "memory, runtime metadata, orchestration metadata, future semantic "
                       "candidates.");
    QString budgetSummary = QStringLiteral("0 candidate characters selected.");
    QString exclusionSummary = QStringLiteral("No semantic candidates excluded.");
    QString summary = QStringLiteral("No semantic candidate arbitration has run.");
};

struct SemanticCandidateSummary {
    SemanticCandidateSource source = SemanticCandidateSource::RecentConversation;
    int candidateCount = 0;
    int selectedCount = 0;
    int excludedCount = 0;
    int truncatedCount = 0;
    int includedCharacters = 0;
    QString summary = QStringLiteral("No candidate participation.");
};

struct HybridRetrievalPolicy {
    bool deterministicRetrievalAuthoritative = true;
    bool semanticPathEnabled = false;
    bool semanticPromptInjectionEnabled = false;
    QString status = QStringLiteral("Deterministic Authoritative");
    QString summary = QStringLiteral(
        "Hybrid retrieval is prepared as metadata only. Deterministic retrieval remains "
        "authoritative and semantic retrieval is disabled.");
};

struct HybridRetrievalReadiness {
    HybridRetrievalStatus status = HybridRetrievalStatus::SemanticDisabled;
    HybridRetrievalPolicy policy;
    SemanticCandidateStatus candidateStatus = SemanticCandidateStatus::Empty;
    int deterministicCandidateCount = 0;
    int semanticCandidateCount = 0;
    int selectedCandidateCount = 0;
    QString summary =
        QStringLiteral("Hybrid retrieval metadata is available; semantic retrieval is disabled.");
    QStringList checks;
};

struct SemanticArbitrationPolicy {
    bool simulationEnabled = true;
    bool deterministicRetrievalAuthoritative = true;
    bool semanticRetrievalEnabled = false;
    bool promptMutationEnabled = false;
    bool realEmbeddingScoringEnabled = false;
    int maxRankedCandidates = 6;
    QString status = QStringLiteral("Simulation Only");
    QString summary = QStringLiteral(
        "Semantic arbitration is simulated deterministically; deterministic retrieval remains "
        "authoritative.");
};

struct SemanticCandidateScore {
    QString candidateId;
    SemanticCandidateSource source = SemanticCandidateSource::RecentConversation;
    int simulatedScore = 0;
    int sourceRank = 0;
    QString tieBreakKey;
    QString summary;
};

struct SemanticBudgetSummary {
    int evaluatedCandidateCount = 0;
    int rankedCandidateCount = 0;
    int deterministicSelectedCandidateCount = 0;
    int semanticSelectedCandidateCount = 0;
    int estimatedCharacters = 0;
    int selectedCharacters = 0;
    QString summary = QStringLiteral("No simulated semantic arbitration budget.");
};

struct SemanticArbitrationResult {
    SemanticArbitrationStatus status = SemanticArbitrationStatus::Empty;
    SemanticArbitrationPolicy policy;
    SemanticBudgetSummary budget;
    QList<SemanticCandidateScore> candidateScores;
    QString readiness = QStringLiteral("Semantic simulation ready metadata only.");
    QString summary = QStringLiteral("No simulated semantic arbitration has run.");
    QStringList selectionSummaries;
    QStringList checks;
};

struct EmbeddingRuntimeBudget {
    int estimatedEmbeddingJobs = 0;
    int estimatedIndexableItems = 0;
    int estimatedRuntimeMemoryMb = 0;
    int estimatedIndexStorageMb = 0;
    int estimatedStartupSeconds = 0;
    QString summary = QStringLiteral("Embedding runtime cost is not configured.");
};

struct EmbeddingRuntimePlan {
    EmbeddingRuntimeReadiness readiness = EmbeddingRuntimeReadiness::NotConfigured;
    EmbeddingRuntimeBudget budget;
    bool localOnly = true;
    bool indexingEnabled = false;
    bool filesystemIndexingEnabled = false;
    bool ollamaEmbeddingCallsEnabled = false;
    QString summary = QStringLiteral(
        "Embedding runtime planning is metadata only; no embedding runtime is configured.");
    QStringList requirements;
    QStringList constraints;
};

struct EmbeddingIsolationPolicy {
    bool localOnlyMode = true;
    bool explicitSemanticEnableReadinessSatisfied = false;
    bool noCloudProviders = true;
    bool filesystemIndexingEnabled = false;
    bool automaticPromptIntegrationEnabled = false;
    bool retrievalRankingMutationEnabled = false;
    bool automaticMemoryWritesEnabled = false;
    bool vectorPersistenceEnabled = false;
    bool backgroundIndexingEnabled = false;
    QString summary =
        QStringLiteral("Embedding generation is isolated, local-only readiness validation.");
};

struct EmbeddingGenerationPolicy {
    SemanticProviderMode providerMode = SemanticProviderMode::Disabled;
    bool allowFakeInMemoryProvider = false;
    bool allowLocalOllamaEmbeddingsProvider = false;
    bool realCloudProvidersAllowed = false;
    int timeoutMs = 1000;
    int simulatedExecutionMs = 0;
    int maxDocuments = 4;
    int maxDocumentCharacters = 2048;
    QString requestId = QStringLiteral("embedding-test-1");
    QString summary = QStringLiteral(
        "Isolated embedding test generation is disabled unless local readiness gates pass.");
};

struct EmbeddingRuntimeSession {
    QString activeRequestId;
    bool busy = false;
    int timeoutMs = 1000;
    int boundedDocumentCount = 0;
    int boundedCharacterCount = 0;
    QString summary = QStringLiteral("No isolated embedding runtime session is active.");
};

struct EmbeddingGenerationResult {
    EmbeddingRuntimeStatus status = EmbeddingRuntimeStatus::Disabled;
    EmbeddingRuntimeHealth health = EmbeddingRuntimeHealth::NotChecked;
    EmbeddingGenerationReadiness readiness = EmbeddingGenerationReadiness::Refused;
    EmbeddingRuntimeSession session;
    int generatedDocumentCount = 0;
    int generatedVectorCount = 0;
    int elapsedMs = 0;
    QString summary = QStringLiteral("No isolated embedding generation has run.");
    QString failureReason;
    QStringList checks;
};

struct VectorPersistencePolicy {
    bool enabled = false;
    bool localOnly = true;
    bool disabledByDefault = true;
    bool isolatedEmbeddingOutputsOnly = true;
    bool automaticIndexingEnabled = false;
    bool filesystemScanningEnabled = false;
    bool backgroundIngestionEnabled = false;
    bool semanticRetrievalAuthorityEnabled = false;
    bool promptMutationEnabled = false;
    bool automaticMemoryConversionEnabled = false;
    bool cloudVectorServicesAllowed = false;
    QString summary = QStringLiteral(
        "Local vector persistence is disabled by default and isolated from retrieval authority.");
};

struct VectorPersistenceSession {
    QString activeRequestId;
    QString requestId = QStringLiteral("vector-persistence-1");
    bool busy = false;
    int lifecycleRevision = 0;
    QString summary = QStringLiteral("No vector persistence session is active.");
};

struct VectorPersistenceBudget {
    int maxIndexedItems = 16;
    int indexedItemCount = 0;
    int requestedItemCount = 0;
    int acceptedItemCount = 0;
    int rejectedItemCount = 0;
    int remainingItemCount = 16;
    QString summary = QStringLiteral("0 of 16 local vector metadata items indexed.");
};

struct VectorIndexLifecycle {
    bool created = false;
    int revision = 0;
    VectorPersistenceStatus status = VectorPersistenceStatus::Disabled;
    QString lastAction = QStringLiteral("Disabled");
    QString summary =
        QStringLiteral("Vector index lifecycle metadata is disabled and not created.");
};

struct VectorIndexSnapshotSummary {
    VectorPersistenceStatus status = VectorPersistenceStatus::Disabled;
    VectorPersistenceHealth health = VectorPersistenceHealth::NotChecked;
    int indexedItemCount = 0;
    int lifecycleRevision = 0;
    QString boundedState = QStringLiteral("local-only / disabled / 0 indexed items");
    QString summary = QStringLiteral("No local vector index snapshot is active.");
    QStringList checks;
};

struct VectorPersistenceResult {
    bool accepted = false;
    VectorPersistenceStatus status = VectorPersistenceStatus::Disabled;
    VectorPersistenceHealth health = VectorPersistenceHealth::NotChecked;
    VectorPersistenceReadiness readiness = VectorPersistenceReadiness::Disabled;
    VectorPersistencePolicy policy;
    VectorPersistenceSession session;
    VectorPersistenceBudget budget;
    VectorIndexLifecycle lifecycle;
    VectorIndexSnapshotSummary snapshot;
    QString summary = QStringLiteral("Vector persistence did not run.");
    QString failureReason;
    QStringList checks;
};

struct SemanticSearchPolicy {
    bool enabled = true;
    bool localOnly = true;
    bool deterministic = true;
    bool isolatedEmbeddingOutputsOnly = true;
    bool filesystemIndexingEnabled = false;
    bool backgroundIngestionEnabled = false;
    bool cloudProvidersAllowed = false;
    bool providerDownloadsAllowed = false;
    bool authoritative = false;
    bool promptMutationEnabled = false;
    bool retrievalPlanningMutationEnabled = false;
    int maxCandidates = 4;
    double minimumSimilarity = 0.0;
    QString summary = QStringLiteral(
        "Bounded local semantic search may return non-authoritative candidate metadata only.");
};

struct SemanticSearchBudget {
    int maxCandidates = 4;
    int evaluatedItemCount = 0;
    int returnedCandidateCount = 0;
    int timeoutMs = 1000;
    int elapsedMs = 0;
    double minimumSimilarity = 0.0;
    QString summary = QStringLiteral("0 bounded semantic candidates returned.");
};

struct SemanticSearchSession {
    QString activeRequestId;
    QString requestId = QStringLiteral("semantic-search-1");
    bool busy = false;
    int timeoutMs = 1000;
    int simulatedExecutionMs = 0;
    QString summary = QStringLiteral("No bounded semantic search session is active.");
};

struct SemanticSearchReadiness {
    bool ready = false;
    SemanticSearchStatus status = SemanticSearchStatus::Disabled;
    QString summary = QStringLiteral("Bounded local semantic search is not ready.");
    QStringList checks;
};

struct SemanticSearchCandidate {
    QString id;
    QString summary;
    double similarity = 0.0;
    int rank = 0;
    QString tieBreakKey;
};

struct SemanticSearchArbitrationSummary {
    int deterministicCandidateCount = 0;
    int semanticCandidateCount = 0;
    int exposedCandidateCount = 0;
    bool deterministicAuthoritative = true;
    bool semanticPromptAuthority = false;
    QString summary = QStringLiteral(
        "Deterministic retrieval remains authoritative; semantic candidates are metadata only.");
    QStringList checks;
};

struct SemanticSearchResult {
    bool accepted = false;
    SemanticSearchStatus status = SemanticSearchStatus::Disabled;
    SemanticSearchPolicy policy;
    SemanticSearchBudget budget;
    SemanticSearchSession session;
    SemanticSearchReadiness readiness;
    SemanticSearchArbitrationSummary arbitration;
    QList<SemanticSearchCandidate> candidates;
    QString summary = QStringLiteral("Bounded local semantic search did not run.");
    QString failureReason;
    QStringList checks;
};

struct HybridRetrievalBridgePolicy {
    bool enabled = true;
    bool deterministicRetrievalAuthoritative = true;
    bool semanticCandidatesAdvisoryOnly = true;
    bool deterministicWinsTies = true;
    bool promptMutationEnabled = false;
    bool retrievalPlanningMutationEnabled = false;
    bool promptContextMutationEnabled = false;
    int maxMergedCandidates = 6;
    int timeoutMs = 1000;
    QString summary = QStringLiteral(
        "Hybrid retrieval bridge may expose bounded metadata only; deterministic retrieval "
        "remains final authority.");
};

struct HybridBridgeBudget {
    int maxMergedCandidates = 6;
    int deterministicCandidateCount = 0;
    int semanticCandidateCount = 0;
    int mergedCandidateCount = 0;
    int semanticFillCount = 0;
    int timeoutMs = 1000;
    int elapsedMs = 0;
    QString summary = QStringLiteral("0 bounded bridge candidates merged.");
};

struct HybridBridgeReadiness {
    bool ready = false;
    HybridRetrievalBridgeStatus status = HybridRetrievalBridgeStatus::Disabled;
    QString summary = QStringLiteral("Hybrid retrieval bridge is not ready.");
    QStringList checks;
};

struct HybridBridgeCandidate {
    QString id;
    QString source;
    QString title;
    QString summary;
    int rank = 0;
    bool deterministic = true;
    bool semanticAdvisory = false;
    QString arbitrationReason;
};

struct HybridBridgeSourceSummary {
    int deterministicCandidateCount = 0;
    int semanticCandidateCount = 0;
    int deterministicSelectedCount = 0;
    int semanticFilledCount = 0;
    bool semanticSourceEmpty = true;
    QString summary = QStringLiteral("No hybrid bridge source participation.");
};

struct HybridBridgeArbitration {
    QString orderingSummary = QStringLiteral(
        "Bridge ordering is deterministic-first; semantic candidates may only fill unused "
        "bounded capacity.");
    QString fallbackSummary =
        QStringLiteral("Deterministic retrieval fallback is available and authoritative.");
    QString tieSummary = QStringLiteral("Deterministic candidates win all bridge ties.");
    QString summary = QStringLiteral("No hybrid bridge arbitration has run.");
    QStringList checks;
};

struct HybridRetrievalBridgeResult {
    bool accepted = false;
    HybridRetrievalBridgeStatus status = HybridRetrievalBridgeStatus::Disabled;
    HybridRetrievalBridgePolicy policy;
    HybridBridgeBudget budget;
    HybridBridgeReadiness readiness;
    HybridBridgeArbitration arbitration;
    HybridBridgeSourceSummary sourceSummary;
    QList<HybridBridgeCandidate> candidates;
    QString summary = QStringLiteral("Hybrid retrieval bridge did not run.");
    QString fallbackSummary =
        QStringLiteral("Deterministic retrieval remains fully available without semantic search.");
    QString failureReason;
    QStringList checks;
};

struct SemanticAcceptancePolicy {
    bool enabled = true;
    bool deterministicRetrievalAuthoritative = true;
    bool semanticSupplementsOnly = true;
    bool deterministicWinsConflicts = true;
    bool promptMutationEnabled = false;
    bool retrievalPlanningMutationEnabled = false;
    bool promptContextMutationEnabled = false;
    bool retrievalSourcePriorityMutationEnabled = false;
    int maxAcceptedSupplements = 2;
    int maxSupplementCharacters = 640;
    int maxTotalRetrievalSupplements = 6;
    int timeoutMs = 1000;
    QString summary = QStringLiteral(
        "Semantic acceptance may approve bounded semantic supplements only; deterministic "
        "retrieval remains authoritative.");
};

struct SemanticAcceptanceBudget {
    int maxAcceptedSupplements = 2;
    int maxSupplementCharacters = 640;
    int maxTotalRetrievalSupplements = 6;
    int deterministicCandidateCount = 0;
    int semanticCandidateCount = 0;
    int bridgeSemanticFillCount = 0;
    int acceptedSupplementCount = 0;
    int acceptedSupplementCharacters = 0;
    int remainingSupplementSlots = 0;
    int timeoutMs = 1000;
    int elapsedMs = 0;
    QString summary = QStringLiteral("0 bounded semantic supplements accepted.");
};

struct SemanticAcceptanceReadiness {
    bool ready = false;
    SemanticAcceptanceStatus status = SemanticAcceptanceStatus::Disabled;
    QString summary = QStringLiteral("Semantic acceptance is not ready.");
    QStringList checks;
};

struct SemanticAcceptedCandidate {
    QString id;
    QString source = QStringLiteral("Semantic Supplemental");
    QString title;
    QString summary;
    int supplementRank = 0;
    int deterministicOffsetRank = 0;
    int estimatedCharacters = 0;
    bool semantic = true;
    bool supplementalOnly = true;
    QString acceptanceReason;
};

struct SemanticAcceptanceArbitration {
    QString orderingSummary = QStringLiteral(
        "Deterministic retrieval candidates remain primary; accepted semantic supplements follow "
        "deterministic ordering.");
    QString conflictSummary =
        QStringLiteral("Deterministic retrieval wins all acceptance conflicts.");
    QString tieSummary = QStringLiteral("Semantic ties are handled by bridge order, rank, and id.");
    QString summary = QStringLiteral("No semantic acceptance arbitration has run.");
    QStringList checks;
};

struct SemanticAcceptanceFallback {
    bool deterministicOnly = true;
    QString state = QStringLiteral("Deterministic Only");
    QString summary =
        QStringLiteral("Deterministic retrieval fallback is authoritative and available.");
    QStringList summaries;
};

struct SemanticAcceptanceSourceSummary {
    int deterministicCandidateCount = 0;
    int semanticCandidateCount = 0;
    int bridgeCandidateCount = 0;
    int bridgeSemanticFillCount = 0;
    int acceptedSupplementCount = 0;
    bool localOnly = true;
    bool semanticAuthoritative = false;
    QString summary = QStringLiteral("No semantic acceptance source participation.");
};

struct SemanticAcceptanceResult {
    bool accepted = false;
    SemanticAcceptanceStatus status = SemanticAcceptanceStatus::Disabled;
    SemanticAcceptancePolicy policy;
    SemanticAcceptanceBudget budget;
    SemanticAcceptanceReadiness readiness;
    SemanticAcceptanceArbitration arbitration;
    SemanticAcceptanceFallback fallback;
    SemanticAcceptanceSourceSummary sourceSummary;
    QList<SemanticAcceptedCandidate> acceptedCandidates;
    QString summary = QStringLiteral("Semantic acceptance did not run.");
    QString failureReason;
    QStringList checks;
};

struct SemanticProviderPolicy {
    bool disabledByDefault = true;
    bool allowFakeInMemoryProvider = false;
    bool allowLocalOllamaEmbeddingsProvider = false;
    bool allowLocalFileVectorIndex = false;
    bool allowRealEmbeddingCalls = false;
    bool allowVectorIndexWrites = false;
    bool allowSemanticPromptInjection = false;
    bool localOnly = true;
    QString status = QStringLiteral("Disabled By Default");
    QString summary = QStringLiteral(
        "Semantic provider selection is planning metadata only; activation is refused by "
        "default.");
};

struct SemanticProviderDescriptor {
    SemanticProviderMode mode = SemanticProviderMode::Disabled;
    QString name = QStringLiteral("Disabled");
    QString summary = QStringLiteral("Semantic retrieval provider is disabled.");
    SemanticProviderReadiness readiness = SemanticProviderReadiness::Disabled;
    SemanticProviderHealth health = SemanticProviderHealth::NotChecked;
    QList<SemanticProviderCapability> capabilities;
    QStringList capabilitySummaries;
    QStringList requiredActivationSteps;
};

struct SemanticProviderSelection {
    SemanticProviderMode mode = SemanticProviderMode::Disabled;
    SemanticProviderDescriptor descriptor;
    SemanticProviderReadiness readiness = SemanticProviderReadiness::Disabled;
    SemanticProviderHealth health = SemanticProviderHealth::NotChecked;
    bool selected = false;
    bool active = false;
    QString disabledReason = QStringLiteral("Semantic retrieval is disabled by default.");
    QString summary = QStringLiteral("Selected semantic provider: Disabled.");
    QStringList requiredActivationSteps;
    QStringList capabilitySummaries;
};

struct SemanticActivationReadiness {
    bool ready = false;
    SemanticProviderMode providerMode = SemanticProviderMode::Disabled;
    QString status = QStringLiteral("Refused");
    QString summary = QStringLiteral("Semantic activation is refused by default.");
    QStringList checks;
    QStringList requiredSteps;
};

struct SemanticActivationResult {
    bool accepted = false;
    SemanticActivationReadiness readiness;
    QString summary = QStringLiteral("Semantic activation did not run.");
};

class IEmbeddingProvider {
public:
    virtual ~IEmbeddingProvider() = default;

    virtual EmbeddingProviderStatus status() const = 0;
    virtual EmbeddingProviderPolicy policy() const = 0;
    virtual EmbeddingResult embed(const EmbeddingRequest& request) const = 0;
};

class IVectorIndex {
public:
    virtual ~IVectorIndex() = default;

    virtual VectorIndexStatus status() const = 0;
    virtual VectorIndexPolicy policy() const = 0;
    virtual int itemCount() const = 0;
    virtual bool upsert(const EmbeddingDocument& document, const EmbeddingVector& vector) = 0;
    virtual bool remove(const QString& documentId) = 0;
    virtual void clear() = 0;
    virtual VectorSearchResult search(const VectorSearchQuery& query) const = 0;
};

class FakeEmbeddingProvider final : public IEmbeddingProvider {
public:
    explicit FakeEmbeddingProvider(int dimensions = 8);

    EmbeddingProviderStatus status() const override;
    EmbeddingProviderPolicy policy() const override;
    EmbeddingResult embed(const EmbeddingRequest& request) const override;

private:
    EmbeddingVector embedText(const QString& text) const;

    EmbeddingProviderPolicy policy_;
};

class FakeVectorIndex final : public IVectorIndex {
public:
    explicit FakeVectorIndex(int dimensions = 8);

    VectorIndexStatus status() const override;
    VectorIndexPolicy policy() const override;
    int itemCount() const override;
    bool upsert(const EmbeddingDocument& document, const EmbeddingVector& vector) override;
    bool remove(const QString& documentId) override;
    void clear() override;
    VectorSearchResult search(const VectorSearchQuery& query) const override;

private:
    struct Entry {
        EmbeddingDocument document;
        EmbeddingVector vector;
    };

    double similarity(const EmbeddingVector& left, const EmbeddingVector& right) const;

    VectorIndexPolicy policy_;
    QList<Entry> entries_;
};

class LocalVectorPersistenceIndex final {
public:
    explicit LocalVectorPersistenceIndex(VectorPersistencePolicy policy = {},
                                         VectorPersistenceBudget budget = {});

    VectorPersistencePolicy policy() const;
    VectorPersistenceBudget budget() const;
    VectorIndexLifecycle lifecycle() const;
    VectorIndexSnapshotSummary snapshot() const;
    int itemCount() const;

    VectorPersistenceResult create(const VectorPersistenceSession& session);
    VectorPersistenceResult reset(const VectorPersistenceSession& session);
    VectorPersistenceResult clear(const VectorPersistenceSession& session);
    VectorPersistenceResult
    acceptIsolatedEmbeddingResult(const EmbeddingGenerationResult& generationResult,
                                  const QStringList& itemSummaries,
                                  const VectorPersistenceSession& session);
    SemanticSearchResult searchLocalSemanticCandidates(
        const QString& query, const EmbeddingGenerationResult& queryEmbeddingResult,
        const SemanticSearchPolicy& policy, const SemanticSearchSession& session) const;

private:
    VectorPersistenceResult baseResult(const VectorPersistenceSession& session) const;
    VectorPersistenceResult validateSession(const VectorPersistenceSession& session) const;
    void refreshBudget();
    VectorPersistenceResult finalize(VectorPersistenceResult result) const;
    SemanticSearchReadiness semanticSearchReadiness(const QString& query,
                                                    const EmbeddingGenerationResult& queryEmbedding,
                                                    const SemanticSearchPolicy& policy,
                                                    const SemanticSearchSession& session) const;

    VectorPersistencePolicy policy_;
    VectorPersistenceBudget budget_;
    VectorIndexLifecycle lifecycle_;
    QStringList itemSummaries_;
};

QStringList semanticRetrievalReadinessChecks(const SemanticRetrievalPolicy& policy,
                                             EmbeddingProviderStatus providerStatus,
                                             VectorIndexStatus indexStatus, int indexedItemCount);
VectorPersistenceReadiness vectorPersistenceReadiness(const VectorPersistencePolicy& policy,
                                                      const VectorPersistenceSession& session);
SemanticSearchReadiness semanticSearchReadiness(const VectorPersistencePolicy& persistencePolicy,
                                                int indexedItemCount, const QString& query,
                                                const EmbeddingGenerationResult& queryEmbedding,
                                                const SemanticSearchPolicy& policy,
                                                const SemanticSearchSession& session);
SemanticSearchArbitrationSummary
semanticSearchArbitrationSummary(const SemanticSearchResult& searchResult,
                                 const SemanticCandidateArbitration& deterministicArbitration);
HybridRetrievalBridgeResult
hybridRetrievalBridge(const RetrievalPlanningResult& deterministicPlanning,
                      const SemanticSearchResult& semanticSearchResult,
                      const HybridRetrievalBridgePolicy& policy);
SemanticAcceptanceResult semanticAcceptance(const RetrievalPlanningResult& deterministicPlanning,
                                            const HybridRetrievalBridgeResult& bridgeResult,
                                            const SemanticSearchResult& semanticSearchResult,
                                            const SemanticAcceptancePolicy& policy);
SemanticCandidateSource semanticCandidateSourceForContextSource(ContextAssemblySourceKind source);
SemanticCandidateArbitration
orchestrateSemanticCandidates(const QList<SemanticCandidate>& candidates,
                              const SemanticCandidatePolicy& policy);
QStringList
semanticCandidateParticipationSummaries(const SemanticCandidateArbitration& arbitration);
HybridRetrievalReadiness hybridRetrievalReadiness(const HybridRetrievalPolicy& policy,
                                                  const SemanticCandidateArbitration& arbitration);
SemanticArbitrationResult
simulateSemanticArbitration(const SemanticCandidateArbitration& arbitration,
                            const SemanticArbitrationPolicy& policy);
EmbeddingRuntimePlan embeddingRuntimePlan(const SemanticArbitrationResult& arbitration);
EmbeddingGenerationReadiness
embeddingGenerationReadiness(const EmbeddingIsolationPolicy& isolationPolicy,
                             const EmbeddingGenerationPolicy& generationPolicy,
                             const EmbeddingRuntimeSession& session,
                             EmbeddingProviderStatus providerStatus);
EmbeddingGenerationResult generateIsolatedEmbeddings(
    const IEmbeddingProvider& provider, const QList<EmbeddingDocument>& documents,
    const EmbeddingIsolationPolicy& isolationPolicy,
    const EmbeddingGenerationPolicy& generationPolicy, const EmbeddingRuntimeSession& session);
QList<SemanticProviderDescriptor>
plannedSemanticProviderDescriptors(const SemanticProviderPolicy& policy);
SemanticProviderSelection selectSemanticProvider(SemanticProviderMode mode,
                                                 const SemanticProviderPolicy& policy);
SemanticActivationReadiness semanticActivationReadiness(const SemanticProviderSelection& selection,
                                                        const SemanticProviderPolicy& policy);
SemanticActivationResult semanticActivationResult(const SemanticProviderSelection& selection,
                                                  const SemanticProviderPolicy& policy);

} // namespace sentinel::core
