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

QStringList semanticRetrievalReadinessChecks(const SemanticRetrievalPolicy& policy,
                                             EmbeddingProviderStatus providerStatus,
                                             VectorIndexStatus indexStatus, int indexedItemCount);
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
QList<SemanticProviderDescriptor>
plannedSemanticProviderDescriptors(const SemanticProviderPolicy& policy);
SemanticProviderSelection selectSemanticProvider(SemanticProviderMode mode,
                                                 const SemanticProviderPolicy& policy);
SemanticActivationReadiness semanticActivationReadiness(const SemanticProviderSelection& selection,
                                                        const SemanticProviderPolicy& policy);
SemanticActivationResult semanticActivationResult(const SemanticProviderSelection& selection,
                                                  const SemanticProviderPolicy& policy);

} // namespace sentinel::core
