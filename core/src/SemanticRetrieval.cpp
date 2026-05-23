#include "sentinel/core/SemanticRetrieval.h"

#include <QCryptographicHash>

#include <algorithm>
#include <cmath>
#include <utility>

namespace sentinel::core {
namespace {

int toInt(qsizetype value) {
    return static_cast<int>(value);
}

QStringList normalizedTokens(const QString& text) {
    QString normalized;
    normalized.reserve(text.size());
    for (const auto ch : text.toLower()) {
        normalized.append(ch.isLetterOrNumber() ? ch : QLatin1Char(' '));
    }
    return normalized.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

QByteArray stableDigest(const QString& value) {
    return QCryptographicHash::hash(value.toUtf8(), QCryptographicHash::Sha256);
}

QString sanitizedSemanticPromptText(const QString& text) {
    QStringList retained;
    const auto parts = text.simplified().split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const auto& part : parts) {
        const auto normalized = part.trimmed();
        const auto lowered = normalized.toLower();
        if (lowered.contains(QStringLiteral("vector:")) ||
            lowered.contains(QStringLiteral("vector=")) ||
            lowered.contains(QStringLiteral("embedding:")) ||
            lowered.contains(QStringLiteral("embedding=")) ||
            lowered.contains(QStringLiteral("score:")) ||
            lowered.contains(QStringLiteral("score=")) ||
            lowered.contains(QStringLiteral("provider handle")) ||
            lowered.contains(QStringLiteral("filesystem path")) ||
            lowered.contains(QStringLiteral("debug payload")) ||
            lowered.contains(QStringLiteral("debug dump"))) {
            continue;
        }
        retained.append(normalized);
    }
    return retained.join(QStringLiteral("; ")).trimmed();
}

} // namespace

QString embeddingProviderStatusName(EmbeddingProviderStatus status) {
    switch (status) {
    case EmbeddingProviderStatus::NotConfigured:
        return QStringLiteral("Not Configured");
    case EmbeddingProviderStatus::Disabled:
        return QStringLiteral("Disabled");
    case EmbeddingProviderStatus::Ready:
        return QStringLiteral("Ready");
    case EmbeddingProviderStatus::Error:
        return QStringLiteral("Error");
    }

    return QStringLiteral("Not Configured");
}

QString vectorIndexStatusName(VectorIndexStatus status) {
    switch (status) {
    case VectorIndexStatus::NotConfigured:
        return QStringLiteral("Not Configured");
    case VectorIndexStatus::Disabled:
        return QStringLiteral("Disabled");
    case VectorIndexStatus::Empty:
        return QStringLiteral("Empty");
    case VectorIndexStatus::Ready:
        return QStringLiteral("Ready");
    case VectorIndexStatus::Error:
        return QStringLiteral("Error");
    }

    return QStringLiteral("Not Configured");
}

QString semanticRetrievalStatusName(SemanticRetrievalStatus status) {
    switch (status) {
    case SemanticRetrievalStatus::Disabled:
        return QStringLiteral("Disabled");
    case SemanticRetrievalStatus::NotConfigured:
        return QStringLiteral("Not Configured");
    case SemanticRetrievalStatus::ReadyMetadataOnly:
        return QStringLiteral("Ready Metadata Only");
    }

    return QStringLiteral("Disabled");
}

QString semanticCandidateSourceName(SemanticCandidateSource source) {
    switch (source) {
    case SemanticCandidateSource::RecentConversation:
        return QStringLiteral("Recent Conversation Window");
    case SemanticCandidateSource::DeterministicSummary:
        return QStringLiteral("Deterministic Summaries");
    case SemanticCandidateSource::CommittedMemory:
        return QStringLiteral("Committed Memory");
    case SemanticCandidateSource::RuntimeMetadata:
        return QStringLiteral("Runtime Metadata");
    case SemanticCandidateSource::OrchestrationMetadata:
        return QStringLiteral("Orchestration Metadata");
    case SemanticCandidateSource::FutureSemanticVector:
        return QStringLiteral("Future Semantic/Vector Candidates");
    }

    return QStringLiteral("Recent Conversation Window");
}

QString semanticCandidateSelectionName(SemanticCandidateSelection selection) {
    switch (selection) {
    case SemanticCandidateSelection::NotEvaluated:
        return QStringLiteral("Not Evaluated");
    case SemanticCandidateSelection::Selected:
        return QStringLiteral("Selected");
    case SemanticCandidateSelection::Excluded:
        return QStringLiteral("Excluded");
    case SemanticCandidateSelection::Truncated:
        return QStringLiteral("Truncated");
    }

    return QStringLiteral("Not Evaluated");
}

QString semanticCandidateStatusName(SemanticCandidateStatus status) {
    switch (status) {
    case SemanticCandidateStatus::Disabled:
        return QStringLiteral("Disabled");
    case SemanticCandidateStatus::Empty:
        return QStringLiteral("Empty");
    case SemanticCandidateStatus::Ready:
        return QStringLiteral("Ready");
    case SemanticCandidateStatus::Truncated:
        return QStringLiteral("Truncated");
    }

    return QStringLiteral("Disabled");
}

QString hybridRetrievalStatusName(HybridRetrievalStatus status) {
    switch (status) {
    case HybridRetrievalStatus::DeterministicOnly:
        return QStringLiteral("Deterministic Only");
    case HybridRetrievalStatus::SemanticDisabled:
        return QStringLiteral("Semantic Disabled");
    case HybridRetrievalStatus::ReadyMetadataOnly:
        return QStringLiteral("Ready Metadata Only");
    }

    return QStringLiteral("Semantic Disabled");
}

QString semanticArbitrationStatusName(SemanticArbitrationStatus status) {
    switch (status) {
    case SemanticArbitrationStatus::Disabled:
        return QStringLiteral("Disabled");
    case SemanticArbitrationStatus::Empty:
        return QStringLiteral("Empty");
    case SemanticArbitrationStatus::Simulated:
        return QStringLiteral("Simulated");
    }

    return QStringLiteral("Disabled");
}

QString semanticProviderModeName(SemanticProviderMode mode) {
    switch (mode) {
    case SemanticProviderMode::Disabled:
        return QStringLiteral("Disabled");
    case SemanticProviderMode::FakeInMemory:
        return QStringLiteral("Fake/InMemory");
    case SemanticProviderMode::LocalOllamaEmbeddings:
        return QStringLiteral("Local Ollama Embeddings");
    case SemanticProviderMode::LocalFileVectorIndex:
        return QStringLiteral("Local File/Vector Index");
    }

    return QStringLiteral("Disabled");
}

QString semanticProviderReadinessName(SemanticProviderReadiness readiness) {
    switch (readiness) {
    case SemanticProviderReadiness::Disabled:
        return QStringLiteral("Disabled");
    case SemanticProviderReadiness::Planned:
        return QStringLiteral("Planned");
    case SemanticProviderReadiness::ReadyMetadataOnly:
        return QStringLiteral("Ready Metadata Only");
    case SemanticProviderReadiness::Blocked:
        return QStringLiteral("Blocked");
    }

    return QStringLiteral("Disabled");
}

QString semanticProviderHealthName(SemanticProviderHealth health) {
    switch (health) {
    case SemanticProviderHealth::NotChecked:
        return QStringLiteral("Not Checked");
    case SemanticProviderHealth::MetadataOnly:
        return QStringLiteral("Metadata Only");
    case SemanticProviderHealth::Blocked:
        return QStringLiteral("Blocked");
    }

    return QStringLiteral("Not Checked");
}

QString semanticProviderCapabilityName(SemanticProviderCapability capability) {
    switch (capability) {
    case SemanticProviderCapability::LocalOnly:
        return QStringLiteral("Local only");
    case SemanticProviderCapability::FakeInMemory:
        return QStringLiteral("Fake/InMemory test provider");
    case SemanticProviderCapability::EmbeddingGeneration:
        return QStringLiteral("Embedding generation planned");
    case SemanticProviderCapability::VectorIndexing:
        return QStringLiteral("Vector indexing planned");
    case SemanticProviderCapability::MetadataOnlyHealth:
        return QStringLiteral("Metadata-only health/readiness");
    case SemanticProviderCapability::PromptMutationBlocked:
        return QStringLiteral("Prompt mutation blocked");
    case SemanticProviderCapability::VectorWritesBlocked:
        return QStringLiteral("Vector writes blocked");
    }

    return QStringLiteral("Metadata-only health/readiness");
}

QString embeddingRuntimeReadinessName(EmbeddingRuntimeReadiness readiness) {
    switch (readiness) {
    case EmbeddingRuntimeReadiness::NotConfigured:
        return QStringLiteral("Not Configured");
    case EmbeddingRuntimeReadiness::Planned:
        return QStringLiteral("Planned");
    case EmbeddingRuntimeReadiness::Blocked:
        return QStringLiteral("Blocked");
    }

    return QStringLiteral("Not Configured");
}

QString embeddingRuntimeStatusName(EmbeddingRuntimeStatus status) {
    switch (status) {
    case EmbeddingRuntimeStatus::Disabled:
        return QStringLiteral("Disabled");
    case EmbeddingRuntimeStatus::Ready:
        return QStringLiteral("Ready");
    case EmbeddingRuntimeStatus::Running:
        return QStringLiteral("Running");
    case EmbeddingRuntimeStatus::Succeeded:
        return QStringLiteral("Succeeded");
    case EmbeddingRuntimeStatus::Failed:
        return QStringLiteral("Failed");
    case EmbeddingRuntimeStatus::TimedOut:
        return QStringLiteral("Timed Out");
    case EmbeddingRuntimeStatus::Stale:
        return QStringLiteral("Stale");
    case EmbeddingRuntimeStatus::Busy:
        return QStringLiteral("Busy");
    case EmbeddingRuntimeStatus::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

QString embeddingRuntimeHealthName(EmbeddingRuntimeHealth health) {
    switch (health) {
    case EmbeddingRuntimeHealth::NotChecked:
        return QStringLiteral("Not Checked");
    case EmbeddingRuntimeHealth::LocalOnlyReady:
        return QStringLiteral("Local Only Ready");
    case EmbeddingRuntimeHealth::Blocked:
        return QStringLiteral("Blocked");
    case EmbeddingRuntimeHealth::Failed:
        return QStringLiteral("Failed");
    }

    return QStringLiteral("Not Checked");
}

QString embeddingGenerationReadinessName(EmbeddingGenerationReadiness readiness) {
    switch (readiness) {
    case EmbeddingGenerationReadiness::Refused:
        return QStringLiteral("Refused");
    case EmbeddingGenerationReadiness::Ready:
        return QStringLiteral("Ready");
    }

    return QStringLiteral("Refused");
}

QString vectorPersistenceStatusName(VectorPersistenceStatus status) {
    switch (status) {
    case VectorPersistenceStatus::Disabled:
        return QStringLiteral("Disabled");
    case VectorPersistenceStatus::Empty:
        return QStringLiteral("Empty");
    case VectorPersistenceStatus::Ready:
        return QStringLiteral("Ready");
    case VectorPersistenceStatus::Created:
        return QStringLiteral("Created");
    case VectorPersistenceStatus::Reset:
        return QStringLiteral("Reset");
    case VectorPersistenceStatus::Cleared:
        return QStringLiteral("Cleared");
    case VectorPersistenceStatus::Refused:
        return QStringLiteral("Refused");
    case VectorPersistenceStatus::Busy:
        return QStringLiteral("Busy");
    case VectorPersistenceStatus::Stale:
        return QStringLiteral("Stale");
    case VectorPersistenceStatus::LimitReached:
        return QStringLiteral("Limit Reached");
    }

    return QStringLiteral("Disabled");
}

QString vectorPersistenceHealthName(VectorPersistenceHealth health) {
    switch (health) {
    case VectorPersistenceHealth::NotChecked:
        return QStringLiteral("Not Checked");
    case VectorPersistenceHealth::LocalOnlyReady:
        return QStringLiteral("Local Only Ready");
    case VectorPersistenceHealth::Empty:
        return QStringLiteral("Empty");
    case VectorPersistenceHealth::Blocked:
        return QStringLiteral("Blocked");
    }

    return QStringLiteral("Not Checked");
}

QString vectorPersistenceReadinessName(VectorPersistenceReadiness readiness) {
    switch (readiness) {
    case VectorPersistenceReadiness::Disabled:
        return QStringLiteral("Disabled");
    case VectorPersistenceReadiness::Ready:
        return QStringLiteral("Ready");
    case VectorPersistenceReadiness::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

QString semanticSearchStatusName(SemanticSearchStatus status) {
    switch (status) {
    case SemanticSearchStatus::Disabled:
        return QStringLiteral("Disabled");
    case SemanticSearchStatus::Empty:
        return QStringLiteral("Empty");
    case SemanticSearchStatus::Ready:
        return QStringLiteral("Ready");
    case SemanticSearchStatus::TimedOut:
        return QStringLiteral("Timed Out");
    case SemanticSearchStatus::Stale:
        return QStringLiteral("Stale");
    case SemanticSearchStatus::Busy:
        return QStringLiteral("Busy");
    case SemanticSearchStatus::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

QString hybridRetrievalBridgeStatusName(HybridRetrievalBridgeStatus status) {
    switch (status) {
    case HybridRetrievalBridgeStatus::Disabled:
        return QStringLiteral("Disabled");
    case HybridRetrievalBridgeStatus::DeterministicOnly:
        return QStringLiteral("Deterministic Only");
    case HybridRetrievalBridgeStatus::Ready:
        return QStringLiteral("Ready");
    case HybridRetrievalBridgeStatus::TimedOut:
        return QStringLiteral("Timed Out");
    case HybridRetrievalBridgeStatus::Stale:
        return QStringLiteral("Stale");
    case HybridRetrievalBridgeStatus::Busy:
        return QStringLiteral("Busy");
    case HybridRetrievalBridgeStatus::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

QString semanticAcceptanceStatusName(SemanticAcceptanceStatus status) {
    switch (status) {
    case SemanticAcceptanceStatus::Disabled:
        return QStringLiteral("Disabled");
    case SemanticAcceptanceStatus::DeterministicOnly:
        return QStringLiteral("Deterministic Only");
    case SemanticAcceptanceStatus::Ready:
        return QStringLiteral("Ready");
    case SemanticAcceptanceStatus::TimedOut:
        return QStringLiteral("Timed Out");
    case SemanticAcceptanceStatus::Stale:
        return QStringLiteral("Stale");
    case SemanticAcceptanceStatus::Busy:
        return QStringLiteral("Busy");
    case SemanticAcceptanceStatus::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

QString semanticSupplementAssemblyStatusName(SemanticSupplementAssemblyStatus status) {
    switch (status) {
    case SemanticSupplementAssemblyStatus::Disabled:
        return QStringLiteral("Disabled");
    case SemanticSupplementAssemblyStatus::Empty:
        return QStringLiteral("Empty");
    case SemanticSupplementAssemblyStatus::Ready:
        return QStringLiteral("Ready");
    case SemanticSupplementAssemblyStatus::Truncated:
        return QStringLiteral("Truncated");
    case SemanticSupplementAssemblyStatus::TimedOut:
        return QStringLiteral("Timed Out");
    case SemanticSupplementAssemblyStatus::Stale:
        return QStringLiteral("Stale");
    case SemanticSupplementAssemblyStatus::Busy:
        return QStringLiteral("Busy");
    case SemanticSupplementAssemblyStatus::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

QString semanticPromptAuthorityStatusName(SemanticPromptAuthorityStatus status) {
    switch (status) {
    case SemanticPromptAuthorityStatus::Disabled:
        return QStringLiteral("Disabled");
    case SemanticPromptAuthorityStatus::Denied:
        return QStringLiteral("Denied");
    case SemanticPromptAuthorityStatus::WouldIncludeMetadataOnly:
        return QStringLiteral("Would Include Metadata Only");
    case SemanticPromptAuthorityStatus::SafetyBlocked:
        return QStringLiteral("Safety Blocked");
    case SemanticPromptAuthorityStatus::TimedOut:
        return QStringLiteral("Timed Out");
    case SemanticPromptAuthorityStatus::Stale:
        return QStringLiteral("Stale");
    case SemanticPromptAuthorityStatus::Busy:
        return QStringLiteral("Busy");
    case SemanticPromptAuthorityStatus::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

QString semanticPromptAuthorityDecisionName(SemanticPromptAuthorityDecision decision) {
    switch (decision) {
    case SemanticPromptAuthorityDecision::Denied:
        return QStringLiteral("Denied");
    case SemanticPromptAuthorityDecision::WouldIncludeMetadataOnly:
        return QStringLiteral("Would Include Metadata Only");
    }

    return QStringLiteral("Denied");
}

QString semanticPromptInclusionStatusName(SemanticPromptInclusionStatus status) {
    switch (status) {
    case SemanticPromptInclusionStatus::Disabled:
        return QStringLiteral("Disabled");
    case SemanticPromptInclusionStatus::Denied:
        return QStringLiteral("Denied");
    case SemanticPromptInclusionStatus::Included:
        return QStringLiteral("Included");
    case SemanticPromptInclusionStatus::Truncated:
        return QStringLiteral("Truncated");
    case SemanticPromptInclusionStatus::SafetyBlocked:
        return QStringLiteral("Safety Blocked");
    case SemanticPromptInclusionStatus::Empty:
        return QStringLiteral("Empty");
    case SemanticPromptInclusionStatus::TimedOut:
        return QStringLiteral("Timed Out");
    case SemanticPromptInclusionStatus::Stale:
        return QStringLiteral("Stale");
    case SemanticPromptInclusionStatus::Busy:
        return QStringLiteral("Busy");
    case SemanticPromptInclusionStatus::Refused:
        return QStringLiteral("Refused");
    }

    return QStringLiteral("Disabled");
}

SemanticCandidateSource semanticCandidateSourceForContextSource(ContextAssemblySourceKind source) {
    switch (source) {
    case ContextAssemblySourceKind::Conversation:
        return SemanticCandidateSource::RecentConversation;
    case ContextAssemblySourceKind::ConversationSummary:
        return SemanticCandidateSource::DeterministicSummary;
    case ContextAssemblySourceKind::CommittedMemory:
        return SemanticCandidateSource::CommittedMemory;
    case ContextAssemblySourceKind::RuntimeMetadata:
        return SemanticCandidateSource::RuntimeMetadata;
    case ContextAssemblySourceKind::Orchestration:
    case ContextAssemblySourceKind::SelectedConversationMetadata:
        return SemanticCandidateSource::OrchestrationMetadata;
    }

    return SemanticCandidateSource::RecentConversation;
}

FakeEmbeddingProvider::FakeEmbeddingProvider(int dimensions) {
    policy_.enabled = true;
    policy_.fakeOnly = true;
    policy_.dimensions = std::max(1, dimensions);
    policy_.status = QStringLiteral("Fake Ready");
    policy_.summary =
        QStringLiteral("Deterministic local fake embeddings for tests only; no model/provider "
                       "calls are made.");
}

EmbeddingProviderStatus FakeEmbeddingProvider::status() const {
    return EmbeddingProviderStatus::Ready;
}

EmbeddingProviderPolicy FakeEmbeddingProvider::policy() const {
    return policy_;
}

EmbeddingVector FakeEmbeddingProvider::embedText(const QString& text) const {
    EmbeddingVector vector;
    vector.values.reserve(policy_.dimensions);
    for (int i = 0; i < policy_.dimensions; ++i) {
        vector.values.append(0.0);
    }
    vector.modelSummary = QStringLiteral("Fake deterministic hash/token vector, %1 dimensions")
                              .arg(policy_.dimensions);

    const auto tokens = normalizedTokens(text);
    for (int index = 0; index < tokens.size(); ++index) {
        const auto& token = tokens.at(index);
        const auto digest = stableDigest(QStringLiteral("%1:%2").arg(token).arg(index));
        const auto bucket = static_cast<unsigned char>(digest.at(0)) % policy_.dimensions;
        const auto sign = (static_cast<unsigned char>(digest.at(1)) % 2) == 0 ? 1.0 : -1.0;
        const auto weight = static_cast<double>(token.size() + 1);
        vector.values[bucket] += sign * weight;
    }

    const auto textDigest = stableDigest(text.simplified());
    for (int i = 0; i < policy_.dimensions; ++i) {
        const auto byte = static_cast<unsigned char>(textDigest.at(i % textDigest.size()));
        vector.values[i] += static_cast<double>(byte % 17) / 17.0;
    }

    return vector;
}

EmbeddingResult FakeEmbeddingProvider::embed(const EmbeddingRequest& request) const {
    EmbeddingResult result;
    result.policy = request.policy.dimensions > 0 ? request.policy : policy_;
    result.policy.enabled = true;
    result.policy.fakeOnly = true;
    result.policy.localOnly = true;
    result.policy.dimensions = policy_.dimensions;
    result.status = EmbeddingProviderStatus::Ready;
    result.documents.reserve(request.documents.size());
    result.vectors.reserve(request.documents.size());
    result.checks.append(QStringLiteral("Provider/model calls: disabled"));
    result.checks.append(QStringLiteral("Cloud/API keys: disabled"));
    result.checks.append(QStringLiteral("Filesystem writes/downloads: disabled"));
    result.checks.append(QStringLiteral("Semantic inference: fake deterministic test layer only"));

    for (const auto& document : request.documents) {
        if (document.id.trimmed().isEmpty() || document.text.trimmed().isEmpty()) {
            continue;
        }
        result.documents.append(document);
        result.vectors.append(embedText(document.text));
    }

    result.documentCount = toInt(result.documents.size());
    result.summary = QStringLiteral("Generated %1 deterministic fake %2 with %3 dimensions.")
                         .arg(result.documentCount)
                         .arg(result.documentCount == 1 ? QStringLiteral("embedding")
                                                        : QStringLiteral("embeddings"))
                         .arg(policy_.dimensions);
    return result;
}

FakeVectorIndex::FakeVectorIndex(int dimensions) {
    policy_.enabled = true;
    policy_.fakeOnly = true;
    policy_.inMemoryOnly = true;
    policy_.dimensions = std::max(1, dimensions);
    policy_.status = QStringLiteral("Fake Ready");
    policy_.summary = QStringLiteral(
        "Deterministic in-memory fake vector index for tests only; no external database is used.");
}

VectorIndexStatus FakeVectorIndex::status() const {
    return entries_.isEmpty() ? VectorIndexStatus::Empty : VectorIndexStatus::Ready;
}

VectorIndexPolicy FakeVectorIndex::policy() const {
    return policy_;
}

int FakeVectorIndex::itemCount() const {
    return toInt(entries_.size());
}

bool FakeVectorIndex::upsert(const EmbeddingDocument& document, const EmbeddingVector& vector) {
    const auto id = document.id.trimmed();
    if (id.isEmpty() || vector.values.isEmpty()) {
        return false;
    }

    Entry entry{document, vector};
    entry.document.id = id;
    for (auto& existing : entries_) {
        if (existing.document.id == id) {
            existing = entry;
            return true;
        }
    }

    entries_.append(entry);
    std::stable_sort(entries_.begin(), entries_.end(), [](const Entry& left, const Entry& right) {
        return left.document.id < right.document.id;
    });
    return true;
}

bool FakeVectorIndex::remove(const QString& documentId) {
    const auto id = documentId.trimmed();
    for (int i = 0; i < entries_.size(); ++i) {
        if (entries_.at(i).document.id == id) {
            entries_.removeAt(i);
            return true;
        }
    }
    return false;
}

void FakeVectorIndex::clear() {
    entries_.clear();
}

double FakeVectorIndex::similarity(const EmbeddingVector& left,
                                   const EmbeddingVector& right) const {
    const auto size = std::min(left.values.size(), right.values.size());
    if (size <= 0) {
        return 0.0;
    }

    double dot = 0.0;
    double leftNorm = 0.0;
    double rightNorm = 0.0;
    for (int i = 0; i < size; ++i) {
        dot += left.values.at(i) * right.values.at(i);
        leftNorm += left.values.at(i) * left.values.at(i);
        rightNorm += right.values.at(i) * right.values.at(i);
    }
    if (leftNorm <= 0.0 || rightNorm <= 0.0) {
        return 0.0;
    }
    return dot / (std::sqrt(leftNorm) * std::sqrt(rightNorm));
}

VectorSearchResult FakeVectorIndex::search(const VectorSearchQuery& query) const {
    VectorSearchResult result;
    result.policy = policy_;
    result.status = status();
    result.indexedItemCount = toInt(entries_.size());

    QList<VectorSearchCandidate> candidates;
    candidates.reserve(entries_.size());
    for (const auto& entry : entries_) {
        const auto score = similarity(query.vector, entry.vector);
        if (score < query.minimumScore) {
            continue;
        }

        VectorSearchCandidate candidate;
        candidate.document = entry.document;
        candidate.score = score;
        candidate.summary =
            QStringLiteral("%1 / score %2").arg(entry.document.id).arg(score, 0, 'f', 6);
        candidates.append(candidate);
    }

    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const VectorSearchCandidate& left, const VectorSearchCandidate& right) {
                         if (left.score == right.score) {
                             return left.document.id < right.document.id;
                         }
                         return left.score > right.score;
                     });

    const auto limit = std::max(0, query.maxResults);
    for (int i = 0; i < candidates.size() && i < limit; ++i) {
        auto candidate = candidates.at(i);
        candidate.rank = i + 1;
        result.candidates.append(candidate);
    }

    result.resultCount = toInt(result.candidates.size());
    result.summary =
        QStringLiteral("Vector search returned %1 of %2 indexed %3.")
            .arg(result.resultCount)
            .arg(result.indexedItemCount)
            .arg(result.indexedItemCount == 1 ? QStringLiteral("item") : QStringLiteral("items"));
    return result;
}

LocalVectorPersistenceIndex::LocalVectorPersistenceIndex(VectorPersistencePolicy policy,
                                                         VectorPersistenceBudget budget)
    : policy_(std::move(policy)), budget_(std::move(budget)) {
    budget_.maxIndexedItems = std::max(0, budget_.maxIndexedItems);
    refreshBudget();
    if (!policy_.enabled) {
        lifecycle_.status = VectorPersistenceStatus::Disabled;
        lifecycle_.lastAction = QStringLiteral("Disabled");
        lifecycle_.summary =
            QStringLiteral("Local vector persistence is disabled; no index metadata is active.");
    }
}

VectorPersistencePolicy LocalVectorPersistenceIndex::policy() const {
    return policy_;
}

VectorPersistenceBudget LocalVectorPersistenceIndex::budget() const {
    return budget_;
}

VectorIndexLifecycle LocalVectorPersistenceIndex::lifecycle() const {
    return lifecycle_;
}

VectorIndexSnapshotSummary LocalVectorPersistenceIndex::snapshot() const {
    VectorIndexSnapshotSummary summary;
    summary.status = lifecycle_.status;
    summary.health = !policy_.enabled
                         ? VectorPersistenceHealth::Blocked
                         : (itemSummaries_.isEmpty() ? VectorPersistenceHealth::Empty
                                                     : VectorPersistenceHealth::LocalOnlyReady);
    summary.indexedItemCount = toInt(itemSummaries_.size());
    summary.lifecycleRevision = lifecycle_.revision;
    summary.boundedState =
        QStringLiteral("local-only / %1 / %2 of %3 indexed items / semantic retrieval disabled")
            .arg(policy_.enabled ? QStringLiteral("enabled metadata")
                                 : QStringLiteral("disabled by default"))
            .arg(summary.indexedItemCount)
            .arg(budget_.maxIndexedItems);
    summary.summary = QStringLiteral("Vector index snapshot: %1, revision %2, %3 indexed item(s).")
                          .arg(vectorPersistenceStatusName(summary.status))
                          .arg(summary.lifecycleRevision)
                          .arg(summary.indexedItemCount);
    summary.checks = {
        QStringLiteral("Local-only vector persistence: %1")
            .arg(policy_.localOnly ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Disabled by default: %1")
            .arg(policy_.disabledByDefault ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Automatic indexing: disabled"),
        QStringLiteral("Filesystem scanning: disabled"),
        QStringLiteral("Background ingestion: disabled"),
        QStringLiteral("Semantic retrieval authority: disabled"),
        QStringLiteral("Prompt mutation: disabled"),
        QStringLiteral("Cloud/API/vector services: blocked"),
    };
    return summary;
}

int LocalVectorPersistenceIndex::itemCount() const {
    return toInt(itemSummaries_.size());
}

VectorPersistenceResult
LocalVectorPersistenceIndex::baseResult(const VectorPersistenceSession& session) const {
    VectorPersistenceResult result;
    result.policy = policy_;
    result.session = session;
    result.budget = budget_;
    result.lifecycle = lifecycle_;
    result.snapshot = snapshot();
    result.checks = result.snapshot.checks;
    return result;
}

VectorPersistenceResult
LocalVectorPersistenceIndex::validateSession(const VectorPersistenceSession& session) const {
    auto result = baseResult(session);
    result.readiness = vectorPersistenceReadiness(policy_, session);
    if (!policy_.enabled) {
        result.status = VectorPersistenceStatus::Disabled;
        result.health = VectorPersistenceHealth::Blocked;
        result.failureReason = QStringLiteral("Local vector persistence is disabled by default.");
        result.summary = result.failureReason;
        return result;
    }
    if (result.readiness == VectorPersistenceReadiness::Refused) {
        result.status =
            session.busy ? VectorPersistenceStatus::Busy : VectorPersistenceStatus::Stale;
        result.health = VectorPersistenceHealth::Blocked;
        result.failureReason =
            session.busy ? QStringLiteral("Vector persistence session is busy.")
                         : QStringLiteral("Vector persistence session is stale or invalid.");
        result.summary = result.failureReason;
        return result;
    }
    return result;
}

void LocalVectorPersistenceIndex::refreshBudget() {
    budget_.indexedItemCount = toInt(itemSummaries_.size());
    budget_.remainingItemCount = std::max(0, budget_.maxIndexedItems - budget_.indexedItemCount);
    budget_.summary = QStringLiteral("%1 of %2 local vector metadata items indexed; %3 remaining.")
                          .arg(budget_.indexedItemCount)
                          .arg(budget_.maxIndexedItems)
                          .arg(budget_.remainingItemCount);
}

VectorPersistenceResult
LocalVectorPersistenceIndex::finalize(VectorPersistenceResult result) const {
    const auto requestedItemCount = result.budget.requestedItemCount;
    const auto acceptedItemCount = result.budget.acceptedItemCount;
    const auto rejectedItemCount = result.budget.rejectedItemCount;
    result.policy = policy_;
    result.budget = budget_;
    result.budget.requestedItemCount = requestedItemCount;
    result.budget.acceptedItemCount = acceptedItemCount;
    result.budget.rejectedItemCount = rejectedItemCount;
    result.lifecycle = lifecycle_;
    result.snapshot = snapshot();
    result.checks = result.snapshot.checks;
    return result;
}

VectorPersistenceResult
LocalVectorPersistenceIndex::create(const VectorPersistenceSession& session) {
    auto result = validateSession(session);
    if (result.readiness != VectorPersistenceReadiness::Ready) {
        return result;
    }

    lifecycle_.created = true;
    lifecycle_.revision += 1;
    lifecycle_.status =
        itemSummaries_.isEmpty() ? VectorPersistenceStatus::Empty : VectorPersistenceStatus::Ready;
    lifecycle_.lastAction = QStringLiteral("Create");
    lifecycle_.summary =
        QStringLiteral("Local vector index metadata created at revision %1 with %2 indexed items.")
            .arg(lifecycle_.revision)
            .arg(itemSummaries_.size());

    result.accepted = true;
    result.status = VectorPersistenceStatus::Created;
    result.health = itemSummaries_.isEmpty() ? VectorPersistenceHealth::Empty
                                             : VectorPersistenceHealth::LocalOnlyReady;
    result.readiness = VectorPersistenceReadiness::Ready;
    result.summary = lifecycle_.summary;
    return finalize(result);
}

VectorPersistenceResult
LocalVectorPersistenceIndex::reset(const VectorPersistenceSession& session) {
    auto result = validateSession(session);
    if (result.readiness != VectorPersistenceReadiness::Ready) {
        return result;
    }

    itemSummaries_.clear();
    refreshBudget();
    lifecycle_.created = true;
    lifecycle_.revision += 1;
    lifecycle_.status = VectorPersistenceStatus::Reset;
    lifecycle_.lastAction = QStringLiteral("Reset");
    lifecycle_.summary =
        QStringLiteral("Local vector index metadata reset at revision %1; index is empty.")
            .arg(lifecycle_.revision);

    result.accepted = true;
    result.status = VectorPersistenceStatus::Reset;
    result.health = VectorPersistenceHealth::Empty;
    result.readiness = VectorPersistenceReadiness::Ready;
    result.summary = lifecycle_.summary;
    return finalize(result);
}

VectorPersistenceResult
LocalVectorPersistenceIndex::clear(const VectorPersistenceSession& session) {
    auto result = validateSession(session);
    if (result.readiness != VectorPersistenceReadiness::Ready) {
        return result;
    }

    itemSummaries_.clear();
    refreshBudget();
    lifecycle_.created = false;
    lifecycle_.revision += 1;
    lifecycle_.status = VectorPersistenceStatus::Cleared;
    lifecycle_.lastAction = QStringLiteral("Clear");
    lifecycle_.summary =
        QStringLiteral("Local vector index metadata cleared at revision %1; no index is active.")
            .arg(lifecycle_.revision);

    result.accepted = true;
    result.status = VectorPersistenceStatus::Cleared;
    result.health = VectorPersistenceHealth::Empty;
    result.readiness = VectorPersistenceReadiness::Ready;
    result.summary = lifecycle_.summary;
    return finalize(result);
}

VectorPersistenceResult LocalVectorPersistenceIndex::acceptIsolatedEmbeddingResult(
    const EmbeddingGenerationResult& generationResult, const QStringList& itemSummaries,
    const VectorPersistenceSession& session) {
    auto result = validateSession(session);
    if (result.readiness != VectorPersistenceReadiness::Ready) {
        return result;
    }
    if (!lifecycle_.created) {
        result.status = VectorPersistenceStatus::Refused;
        result.health = VectorPersistenceHealth::Blocked;
        result.readiness = VectorPersistenceReadiness::Refused;
        result.failureReason =
            QStringLiteral("Vector index metadata must be explicitly created before accepting "
                           "isolated embedding output metadata.");
        result.summary = result.failureReason;
        return finalize(result);
    }
    if (generationResult.status != EmbeddingRuntimeStatus::Succeeded ||
        generationResult.readiness != EmbeddingGenerationReadiness::Ready ||
        generationResult.generatedVectorCount <= 0) {
        result.status = VectorPersistenceStatus::Refused;
        result.health = VectorPersistenceHealth::Blocked;
        result.readiness = VectorPersistenceReadiness::Refused;
        result.failureReason =
            QStringLiteral("Only successful isolated embedding runtime outputs are accepted.");
        result.summary = result.failureReason;
        return finalize(result);
    }

    result.budget.requestedItemCount = toInt(itemSummaries.size());
    for (const auto& summary : itemSummaries) {
        if (itemSummaries_.size() >= budget_.maxIndexedItems) {
            ++result.budget.rejectedItemCount;
            continue;
        }
        const auto normalized = summary.simplified();
        if (normalized.isEmpty()) {
            ++result.budget.rejectedItemCount;
            continue;
        }
        itemSummaries_.append(normalized.left(160));
        ++result.budget.acceptedItemCount;
    }

    std::stable_sort(itemSummaries_.begin(), itemSummaries_.end());
    refreshBudget();
    lifecycle_.revision += 1;
    lifecycle_.status = result.budget.rejectedItemCount > 0 ? VectorPersistenceStatus::LimitReached
                                                            : VectorPersistenceStatus::Ready;
    lifecycle_.lastAction = QStringLiteral("Accept Isolated Embedding Output");
    lifecycle_.summary =
        QStringLiteral("Accepted %1 isolated embedding output metadata item(s); %2 rejected by "
                       "bounded local vector persistence limits.")
            .arg(result.budget.acceptedItemCount)
            .arg(result.budget.rejectedItemCount);

    result.accepted = result.budget.acceptedItemCount > 0;
    result.status = lifecycle_.status;
    result.health = itemSummaries_.isEmpty() ? VectorPersistenceHealth::Empty
                                             : VectorPersistenceHealth::LocalOnlyReady;
    result.readiness = VectorPersistenceReadiness::Ready;
    result.summary = lifecycle_.summary;
    return finalize(result);
}

SemanticSearchReadiness semanticSearchReadiness(const VectorPersistencePolicy& persistencePolicy,
                                                int indexedItemCount, const QString& query,
                                                const EmbeddingGenerationResult& queryEmbedding,
                                                const SemanticSearchPolicy& policy,
                                                const SemanticSearchSession& session) {
    SemanticSearchReadiness readiness;
    readiness.checks = {
        QStringLiteral("Local-only semantic search: %1")
            .arg(policy.localOnly ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Deterministic scoring: %1")
            .arg(policy.deterministic ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Indexed local vector persistence entries: %1").arg(indexedItemCount),
        QStringLiteral("Isolated embedding output required: %1")
            .arg(policy.isolatedEmbeddingOutputsOnly ? QStringLiteral("yes")
                                                     : QStringLiteral("no")),
        QStringLiteral("Filesystem indexing: disabled"),
        QStringLiteral("Background ingestion: disabled"),
        QStringLiteral("Cloud/API/vector providers: blocked"),
        QStringLiteral("Provider downloads: blocked"),
        QStringLiteral("Semantic prompt authority: disabled"),
        QStringLiteral("Retrieval planning mutation: disabled"),
    };

    const bool stale = !session.activeRequestId.trimmed().isEmpty() &&
                       session.activeRequestId.trimmed() != session.requestId.trimmed();
    if (!policy.enabled || !persistencePolicy.enabled) {
        readiness.status = SemanticSearchStatus::Disabled;
        readiness.summary =
            QStringLiteral("Bounded local semantic search is disabled or has no active local "
                           "vector persistence lifecycle.");
        return readiness;
    }
    if (session.busy) {
        readiness.status = SemanticSearchStatus::Busy;
        readiness.summary = QStringLiteral("Semantic search session is busy.");
        return readiness;
    }
    if (stale || session.requestId.trimmed().isEmpty()) {
        readiness.status = SemanticSearchStatus::Stale;
        readiness.summary = QStringLiteral("Semantic search request is stale or invalid.");
        return readiness;
    }
    if (!policy.localOnly || !policy.deterministic || !policy.isolatedEmbeddingOutputsOnly ||
        policy.filesystemIndexingEnabled || policy.backgroundIngestionEnabled ||
        policy.cloudProvidersAllowed || policy.providerDownloadsAllowed || policy.authoritative ||
        policy.promptMutationEnabled || policy.retrievalPlanningMutationEnabled ||
        !persistencePolicy.localOnly || persistencePolicy.filesystemScanningEnabled ||
        persistencePolicy.backgroundIngestionEnabled ||
        persistencePolicy.cloudVectorServicesAllowed ||
        persistencePolicy.semanticRetrievalAuthorityEnabled ||
        persistencePolicy.promptMutationEnabled) {
        readiness.status = SemanticSearchStatus::Refused;
        readiness.summary =
            QStringLiteral("Semantic search refused by local-only non-authoritative policy gates.");
        return readiness;
    }
    if (query.trimmed().isEmpty()) {
        readiness.status = SemanticSearchStatus::Empty;
        readiness.summary = QStringLiteral("Semantic search query is empty.");
        return readiness;
    }
    if (queryEmbedding.status != EmbeddingRuntimeStatus::Succeeded ||
        queryEmbedding.readiness != EmbeddingGenerationReadiness::Ready ||
        queryEmbedding.generatedVectorCount <= 0) {
        readiness.status = SemanticSearchStatus::Refused;
        readiness.summary = QStringLiteral(
            "Semantic search requires successful isolated embedding runtime output.");
        return readiness;
    }
    if (indexedItemCount <= 0) {
        readiness.status = SemanticSearchStatus::Empty;
        readiness.summary = QStringLiteral("Local semantic search index is empty.");
        return readiness;
    }

    readiness.ready = true;
    readiness.status = SemanticSearchStatus::Ready;
    readiness.summary =
        QStringLiteral("Bounded local semantic search is ready for non-authoritative candidates.");
    return readiness;
}

SemanticSearchReadiness LocalVectorPersistenceIndex::semanticSearchReadiness(
    const QString& query, const EmbeddingGenerationResult& queryEmbedding,
    const SemanticSearchPolicy& policy, const SemanticSearchSession& session) const {
    return sentinel::core::semanticSearchReadiness(policy_, toInt(itemSummaries_.size()), query,
                                                   queryEmbedding, policy, session);
}

SemanticSearchResult LocalVectorPersistenceIndex::searchLocalSemanticCandidates(
    const QString& query, const EmbeddingGenerationResult& queryEmbeddingResult,
    const SemanticSearchPolicy& policy, const SemanticSearchSession& session) const {
    SemanticSearchResult result;
    result.policy = policy;
    result.session = session;
    result.session.timeoutMs = std::max(0, session.timeoutMs);
    result.budget.maxCandidates = std::max(0, policy.maxCandidates);
    result.budget.timeoutMs = result.session.timeoutMs;
    result.budget.elapsedMs = std::max(0, session.simulatedExecutionMs);
    result.budget.minimumSimilarity = std::clamp(policy.minimumSimilarity, 0.0, 1.0);
    result.readiness = semanticSearchReadiness(query, queryEmbeddingResult, policy, session);
    result.status = result.readiness.status;
    result.checks = result.readiness.checks;

    if (!result.readiness.ready) {
        result.failureReason = result.readiness.summary;
        result.summary = result.failureReason;
        return result;
    }
    if (result.budget.elapsedMs > result.budget.timeoutMs) {
        result.status = SemanticSearchStatus::TimedOut;
        result.failureReason = QStringLiteral("Bounded semantic search timed out after %1 ms.")
                                   .arg(result.budget.timeoutMs);
        result.summary = result.failureReason;
        return result;
    }

    const auto queryTokens = normalizedTokens(query);
    QList<SemanticSearchCandidate> candidates;
    candidates.reserve(itemSummaries_.size());
    for (int i = 0; i < itemSummaries_.size(); ++i) {
        const auto item = itemSummaries_.at(i).simplified();
        const auto itemTokens = normalizedTokens(item);
        if (itemTokens.isEmpty()) {
            continue;
        }
        int overlap = 0;
        for (const auto& token : queryTokens) {
            if (itemTokens.contains(token)) {
                ++overlap;
            }
        }
        const int denominator =
            std::max(1, static_cast<int>(queryTokens.size() + itemTokens.size()) - overlap);
        double similarity =
            std::clamp(static_cast<double>(overlap) / static_cast<double>(denominator), 0.0, 1.0);
        if (queryTokens.isEmpty()) {
            similarity = 0.0;
        }
        if (similarity < result.budget.minimumSimilarity) {
            continue;
        }

        SemanticSearchCandidate candidate;
        candidate.id = QStringLiteral("local-vector-%1").arg(i);
        candidate.summary = item.left(160);
        candidate.similarity = similarity;
        candidate.tieBreakKey = item;
        candidates.append(candidate);
    }

    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const SemanticSearchCandidate& left, const SemanticSearchCandidate& right) {
                         if (left.similarity != right.similarity) {
                             return left.similarity > right.similarity;
                         }
                         return left.tieBreakKey < right.tieBreakKey;
                     });

    result.budget.evaluatedItemCount = toInt(itemSummaries_.size());
    for (int i = 0; i < candidates.size() && i < result.budget.maxCandidates; ++i) {
        auto candidate = candidates.at(i);
        candidate.rank = i + 1;
        result.candidates.append(candidate);
    }
    result.budget.returnedCandidateCount = toInt(result.candidates.size());
    result.budget.summary =
        QStringLiteral("%1 bounded semantic candidates returned from %2 local vector persistence "
                       "entries within %3 ms budget.")
            .arg(result.budget.returnedCandidateCount)
            .arg(result.budget.evaluatedItemCount)
            .arg(result.budget.timeoutMs);
    result.arbitration.semanticCandidateCount = result.budget.returnedCandidateCount;
    result.arbitration.exposedCandidateCount = result.budget.returnedCandidateCount;
    result.arbitration.summary =
        QStringLiteral("%1 semantic metadata candidates exposed for hybrid arbitration; "
                       "deterministic retrieval remains authoritative.")
            .arg(result.budget.returnedCandidateCount);
    result.arbitration.checks = {
        QStringLiteral("Deterministic retrieval authoritative: yes"),
        QStringLiteral("Semantic candidate prompt authority: no"),
        QStringLiteral("RetrievalPlanningResult mutation: no"),
        QStringLiteral("PromptContextBlocks mutation: no"),
        QStringLiteral("Tie handling: similarity then local summary"),
    };
    result.accepted = true;
    result.status =
        result.candidates.isEmpty() ? SemanticSearchStatus::Empty : SemanticSearchStatus::Ready;
    result.summary =
        result.candidates.isEmpty()
            ? QStringLiteral("Bounded semantic search found no local candidate metadata.")
            : QStringLiteral("Bounded semantic search returned %1 non-authoritative local "
                             "candidate metadata matches.")
                  .arg(result.candidates.size());
    return result;
}

SemanticSearchArbitrationSummary
semanticSearchArbitrationSummary(const SemanticSearchResult& searchResult,
                                 const SemanticCandidateArbitration& deterministicArbitration) {
    auto summary = searchResult.arbitration;
    summary.deterministicCandidateCount = toInt(deterministicArbitration.selectedCandidates.size());
    summary.semanticCandidateCount = toInt(searchResult.candidates.size());
    summary.exposedCandidateCount = toInt(searchResult.candidates.size());
    summary.deterministicAuthoritative = true;
    summary.semanticPromptAuthority = false;
    summary.summary =
        QStringLiteral("Hybrid arbitration boundary: %1 deterministic candidates remain final "
                       "authority; %2 semantic candidates are metadata-only.")
            .arg(summary.deterministicCandidateCount)
            .arg(summary.semanticCandidateCount);
    if (summary.checks.isEmpty()) {
        summary.checks = {
            QStringLiteral("Deterministic retrieval authoritative: yes"),
            QStringLiteral("Semantic prompt authority: no"),
            QStringLiteral("RetrievalPlanningResult mutation: no"),
            QStringLiteral("PromptContextBlocks mutation: no"),
            QStringLiteral("Semantic candidates cannot override deterministic ranking"),
        };
    }
    return summary;
}

HybridRetrievalBridgeResult
hybridRetrievalBridge(const RetrievalPlanningResult& deterministicPlanning,
                      const SemanticSearchResult& semanticSearchResult,
                      const HybridRetrievalBridgePolicy& policy) {
    HybridRetrievalBridgeResult result;
    result.policy = policy;
    result.budget.maxMergedCandidates = std::max(0, policy.maxMergedCandidates);
    result.budget.deterministicCandidateCount =
        toInt(deterministicPlanning.selectedCandidates.size());
    result.budget.semanticCandidateCount = toInt(semanticSearchResult.candidates.size());
    result.budget.timeoutMs = std::max(0, policy.timeoutMs);
    result.budget.elapsedMs = semanticSearchResult.budget.elapsedMs;
    result.sourceSummary.deterministicCandidateCount = result.budget.deterministicCandidateCount;
    result.sourceSummary.semanticCandidateCount = result.budget.semanticCandidateCount;
    result.sourceSummary.semanticSourceEmpty = semanticSearchResult.candidates.isEmpty();
    result.readiness.checks = {
        QStringLiteral("Deterministic retrieval final authority: %1")
            .arg(policy.deterministicRetrievalAuthoritative ? QStringLiteral("yes")
                                                            : QStringLiteral("no")),
        QStringLiteral("Semantic candidates advisory only: %1")
            .arg(policy.semanticCandidatesAdvisoryOnly ? QStringLiteral("yes")
                                                       : QStringLiteral("no")),
        QStringLiteral("Deterministic tie wins: %1")
            .arg(policy.deterministicWinsTies ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("RetrievalPlanningResult mutation: no"),
        QStringLiteral("PromptContextBlocks mutation: no"),
        QStringLiteral("Prompt content injection: disabled"),
        QStringLiteral("Raw vectors exposed: no"),
        QStringLiteral("Provider handles exposed: no"),
    };
    result.checks = result.readiness.checks;

    if (!policy.enabled) {
        result.status = HybridRetrievalBridgeStatus::Disabled;
        result.readiness.status = result.status;
        result.failureReason = QStringLiteral("Hybrid retrieval bridge is disabled.");
        result.summary = result.failureReason;
        return result;
    }
    if (!policy.deterministicRetrievalAuthoritative || !policy.semanticCandidatesAdvisoryOnly ||
        !policy.deterministicWinsTies || policy.promptMutationEnabled ||
        policy.retrievalPlanningMutationEnabled || policy.promptContextMutationEnabled) {
        result.status = HybridRetrievalBridgeStatus::Refused;
        result.readiness.status = result.status;
        result.failureReason =
            QStringLiteral("Hybrid retrieval bridge refused by deterministic authority gates.");
        result.summary = result.failureReason;
        return result;
    }
    if (semanticSearchResult.status == SemanticSearchStatus::Busy) {
        result.status = HybridRetrievalBridgeStatus::Busy;
        result.readiness.status = result.status;
        result.failureReason = QStringLiteral("Hybrid retrieval bridge semantic source is busy.");
        result.summary = result.failureReason;
        return result;
    }
    if (semanticSearchResult.status == SemanticSearchStatus::Stale) {
        result.status = HybridRetrievalBridgeStatus::Stale;
        result.readiness.status = result.status;
        result.failureReason =
            QStringLiteral("Hybrid retrieval bridge ignored a stale semantic source.");
        result.summary = result.failureReason;
        return result;
    }
    if (semanticSearchResult.status == SemanticSearchStatus::TimedOut ||
        result.budget.elapsedMs > result.budget.timeoutMs) {
        result.status = HybridRetrievalBridgeStatus::TimedOut;
        result.readiness.status = result.status;
        result.failureReason =
            QStringLiteral("Hybrid retrieval bridge timed out and kept deterministic fallback.");
        result.summary = result.failureReason;
        result.fallbackSummary =
            QStringLiteral("Semantic bridge timed out; deterministic retrieval remains available.");
        return result;
    }

    result.readiness.ready = true;
    result.accepted = true;
    const int capacity = result.budget.maxMergedCandidates;
    int rank = 1;
    for (int i = 0;
         i < deterministicPlanning.selectedCandidates.size() && result.candidates.size() < capacity;
         ++i) {
        const auto& candidate = deterministicPlanning.selectedCandidates.at(i);
        result.candidates.append(HybridBridgeCandidate{
            QStringLiteral("deterministic-%1").arg(i),
            contextAssemblySourceKindName(candidate.source),
            candidate.title,
            candidate.truncated ? QStringLiteral("%1 (truncated)").arg(candidate.title)
                                : candidate.title,
            rank++,
            true,
            false,
            QStringLiteral("Selected by deterministic retrieval planning."),
        });
    }
    result.sourceSummary.deterministicSelectedCount = toInt(result.candidates.size());

    const bool semanticUsable = semanticSearchResult.status == SemanticSearchStatus::Ready;
    if (semanticUsable) {
        for (int i = 0;
             i < semanticSearchResult.candidates.size() && result.candidates.size() < capacity;
             ++i) {
            const auto& candidate = semanticSearchResult.candidates.at(i);
            result.candidates.append(HybridBridgeCandidate{
                candidate.id,
                QStringLiteral("Semantic Advisory"),
                QStringLiteral("Semantic Candidate %1").arg(candidate.rank),
                candidate.summary,
                rank++,
                false,
                true,
                QStringLiteral("Filled unused bridge capacity without prompt authority."),
            });
            ++result.budget.semanticFillCount;
        }
    }

    result.budget.mergedCandidateCount = toInt(result.candidates.size());
    result.sourceSummary.semanticFilledCount = result.budget.semanticFillCount;
    result.status = result.budget.semanticFillCount > 0
                        ? HybridRetrievalBridgeStatus::Ready
                        : HybridRetrievalBridgeStatus::DeterministicOnly;
    result.readiness.status = result.status;
    result.readiness.summary =
        result.status == HybridRetrievalBridgeStatus::Ready
            ? QStringLiteral("Hybrid retrieval bridge produced bounded advisory metadata.")
            : QStringLiteral("Hybrid retrieval bridge is using deterministic fallback only.");
    result.budget.summary =
        QStringLiteral("%1 bridge candidates merged: %2 deterministic, %3 semantic advisory "
                       "within %4 candidate capacity.")
            .arg(result.budget.mergedCandidateCount)
            .arg(result.sourceSummary.deterministicSelectedCount)
            .arg(result.budget.semanticFillCount)
            .arg(result.budget.maxMergedCandidates);
    result.sourceSummary.summary =
        QStringLiteral("%1 deterministic candidates participated; %2 semantic candidates "
                       "available; %3 filled unused bridge slots.")
            .arg(result.budget.deterministicCandidateCount)
            .arg(result.budget.semanticCandidateCount)
            .arg(result.budget.semanticFillCount);
    result.arbitration.summary =
        QStringLiteral("Deterministic-first bridge arbitration selected %1 metadata candidates; "
                       "semantic candidates filled %2 unused slots and remain non-authoritative.")
            .arg(result.budget.mergedCandidateCount)
            .arg(result.budget.semanticFillCount);
    result.arbitration.fallbackSummary =
        result.budget.semanticFillCount == 0
            ? QStringLiteral("Semantic source unavailable or empty; deterministic fallback used.")
            : QStringLiteral(
                  "Deterministic retrieval stayed first; semantic metadata was advisory.");
    result.arbitration.checks = result.checks;
    result.summary =
        result.status == HybridRetrievalBridgeStatus::Ready
            ? QStringLiteral("Hybrid bridge ready: deterministic authority preserved with bounded "
                             "semantic advisory fill.")
            : QStringLiteral("Hybrid bridge deterministic fallback: semantic source did not add "
                             "advisory candidates.");
    result.fallbackSummary = result.arbitration.fallbackSummary;
    return result;
}

SemanticAcceptanceResult semanticAcceptance(const RetrievalPlanningResult& deterministicPlanning,
                                            const HybridRetrievalBridgeResult& bridgeResult,
                                            const SemanticSearchResult& semanticSearchResult,
                                            const SemanticAcceptancePolicy& policy) {
    SemanticAcceptanceResult result;
    result.policy = policy;
    result.budget.maxAcceptedSupplements = std::max(0, policy.maxAcceptedSupplements);
    result.budget.maxSupplementCharacters = std::max(0, policy.maxSupplementCharacters);
    result.budget.maxTotalRetrievalSupplements = std::max(0, policy.maxTotalRetrievalSupplements);
    result.budget.deterministicCandidateCount =
        toInt(deterministicPlanning.selectedCandidates.size());
    result.budget.semanticCandidateCount = toInt(semanticSearchResult.candidates.size());
    result.budget.bridgeSemanticFillCount = bridgeResult.budget.semanticFillCount;
    result.budget.timeoutMs = std::max(0, policy.timeoutMs);
    result.budget.elapsedMs =
        std::max(semanticSearchResult.budget.elapsedMs, bridgeResult.budget.elapsedMs);
    result.budget.remainingSupplementSlots = std::max(
        0, result.budget.maxTotalRetrievalSupplements - result.budget.deterministicCandidateCount);
    result.sourceSummary.deterministicCandidateCount = result.budget.deterministicCandidateCount;
    result.sourceSummary.semanticCandidateCount = result.budget.semanticCandidateCount;
    result.sourceSummary.bridgeCandidateCount = toInt(bridgeResult.candidates.size());
    result.sourceSummary.bridgeSemanticFillCount = result.budget.bridgeSemanticFillCount;
    result.readiness.checks = {
        QStringLiteral("Deterministic retrieval authoritative: %1")
            .arg(policy.deterministicRetrievalAuthoritative ? QStringLiteral("yes")
                                                            : QStringLiteral("no")),
        QStringLiteral("Semantic supplements only: %1")
            .arg(policy.semanticSupplementsOnly ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Deterministic conflicts win: %1")
            .arg(policy.deterministicWinsConflicts ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("RetrievalPlanningResult mutation: no"),
        QStringLiteral("PromptContextBlocks mutation: no"),
        QStringLiteral("Deterministic candidate replacement: no"),
        QStringLiteral("Retrieval source priority mutation: no"),
        QStringLiteral("Prompt content injection: disabled"),
        QStringLiteral("Raw vectors exposed: no"),
        QStringLiteral("Provider handles exposed: no"),
    };
    result.checks = result.readiness.checks;
    result.arbitration.checks = result.checks;

    const auto deterministicFallback = [&](SemanticAcceptanceStatus status, const QString& reason,
                                           const QString& fallbackSummary) {
        result.status = status;
        result.readiness.status = status;
        result.failureReason = reason;
        result.summary = reason;
        result.fallback.state = semanticAcceptanceStatusName(status);
        result.fallback.summary = fallbackSummary;
        result.fallback.summaries = {
            QStringLiteral("%1 deterministic retrieval candidates remain primary.")
                .arg(result.budget.deterministicCandidateCount),
            fallbackSummary,
        };
        result.sourceSummary.summary =
            QStringLiteral("%1 deterministic candidates available; %2 semantic candidates "
                           "received; 0 accepted supplements.")
                .arg(result.budget.deterministicCandidateCount)
                .arg(result.budget.semanticCandidateCount);
    };

    if (!policy.enabled) {
        deterministicFallback(
            SemanticAcceptanceStatus::Disabled, QStringLiteral("Semantic acceptance is disabled."),
            QStringLiteral("Semantic acceptance disabled; deterministic-only fallback active."));
        return result;
    }
    if (!policy.deterministicRetrievalAuthoritative || !policy.semanticSupplementsOnly ||
        !policy.deterministicWinsConflicts || policy.promptMutationEnabled ||
        policy.retrievalPlanningMutationEnabled || policy.promptContextMutationEnabled ||
        policy.retrievalSourcePriorityMutationEnabled) {
        deterministicFallback(
            SemanticAcceptanceStatus::Refused,
            QStringLiteral("Semantic acceptance refused by deterministic authority gates."),
            QStringLiteral("Semantic acceptance refused; deterministic retrieval remains "
                           "authoritative."));
        return result;
    }
    if (semanticSearchResult.status == SemanticSearchStatus::Busy ||
        bridgeResult.status == HybridRetrievalBridgeStatus::Busy) {
        deterministicFallback(
            SemanticAcceptanceStatus::Busy,
            QStringLiteral("Semantic acceptance skipped because semantic retrieval is busy."),
            QStringLiteral("Busy semantic source ignored; deterministic-only fallback active."));
        return result;
    }
    if (semanticSearchResult.status == SemanticSearchStatus::Stale ||
        bridgeResult.status == HybridRetrievalBridgeStatus::Stale) {
        deterministicFallback(
            SemanticAcceptanceStatus::Stale,
            QStringLiteral("Semantic acceptance ignored a stale semantic source."),
            QStringLiteral("Stale semantic source ignored; deterministic-only fallback active."));
        return result;
    }
    if (semanticSearchResult.status == SemanticSearchStatus::TimedOut ||
        bridgeResult.status == HybridRetrievalBridgeStatus::TimedOut ||
        result.budget.elapsedMs > result.budget.timeoutMs) {
        deterministicFallback(
            SemanticAcceptanceStatus::TimedOut,
            QStringLiteral("Semantic acceptance timed out before approving supplements."),
            QStringLiteral("Semantic acceptance timed out; deterministic-only fallback active."));
        return result;
    }
    if (semanticSearchResult.status == SemanticSearchStatus::Refused ||
        bridgeResult.status == HybridRetrievalBridgeStatus::Refused) {
        deterministicFallback(
            SemanticAcceptanceStatus::Refused,
            QStringLiteral("Semantic acceptance refused an errored semantic source."),
            QStringLiteral("Semantic error fallback active; deterministic retrieval remains "
                           "authoritative."));
        return result;
    }
    if (semanticSearchResult.status != SemanticSearchStatus::Ready ||
        bridgeResult.status != HybridRetrievalBridgeStatus::Ready ||
        result.budget.remainingSupplementSlots == 0) {
        deterministicFallback(
            SemanticAcceptanceStatus::DeterministicOnly,
            QStringLiteral(
                "Semantic acceptance found no approved supplement capacity; deterministic "
                "retrieval remains primary."),
            QStringLiteral("Semantic source disabled, empty, or capacity exhausted; "
                           "deterministic-only fallback active."));
        result.readiness.ready = true;
        return result;
    }

    result.readiness.ready = true;
    result.status = SemanticAcceptanceStatus::Ready;
    result.readiness.status = result.status;
    result.readiness.summary =
        QStringLiteral("Semantic acceptance ready for bounded supplemental candidates.");

    const int supplementLimit =
        std::min(result.budget.maxAcceptedSupplements, result.budget.remainingSupplementSlots);
    int supplementRank = 1;
    for (const auto& candidate : bridgeResult.candidates) {
        if (!candidate.semanticAdvisory || result.acceptedCandidates.size() >= supplementLimit) {
            continue;
        }
        const int estimatedCharacters = toInt(candidate.summary.size());
        if (result.budget.acceptedSupplementCharacters + estimatedCharacters >
            result.budget.maxSupplementCharacters) {
            break;
        }
        result.acceptedCandidates.append(SemanticAcceptedCandidate{
            candidate.id,
            QStringLiteral("Semantic Supplemental"),
            candidate.title,
            candidate.summary,
            supplementRank,
            result.budget.deterministicCandidateCount + supplementRank,
            estimatedCharacters,
            true,
            true,
            QStringLiteral(
                "Approved through deterministic acceptance gates as a bounded supplement."),
        });
        ++supplementRank;
        result.budget.acceptedSupplementCharacters += estimatedCharacters;
    }

    result.budget.acceptedSupplementCount = toInt(result.acceptedCandidates.size());
    result.sourceSummary.acceptedSupplementCount = result.budget.acceptedSupplementCount;
    result.accepted = result.budget.acceptedSupplementCount > 0;
    if (!result.accepted) {
        result.status = SemanticAcceptanceStatus::DeterministicOnly;
        result.readiness.status = result.status;
        result.summary =
            QStringLiteral("Semantic acceptance deterministic fallback: supplement budget left no "
                           "accepted semantic candidates.");
        result.fallback.summary =
            QStringLiteral("Semantic supplement budget exhausted; deterministic retrieval remains "
                           "authoritative.");
    } else {
        result.summary =
            QStringLiteral("Semantic acceptance approved %1 bounded supplemental candidates while "
                           "preserving deterministic retrieval authority.")
                .arg(result.budget.acceptedSupplementCount);
        result.fallback.deterministicOnly = false;
        result.fallback.state = QStringLiteral("Bounded Semantic Supplements");
        result.fallback.summary =
            QStringLiteral("Deterministic retrieval remains primary; semantic supplements are "
                           "bounded and non-authoritative.");
    }
    result.budget.summary =
        QStringLiteral("%1 of %2 semantic supplements accepted, using %3 of %4 supplement "
                       "characters after %5 deterministic candidates.")
            .arg(result.budget.acceptedSupplementCount)
            .arg(result.budget.maxAcceptedSupplements)
            .arg(result.budget.acceptedSupplementCharacters)
            .arg(result.budget.maxSupplementCharacters)
            .arg(result.budget.deterministicCandidateCount);
    result.sourceSummary.summary =
        QStringLiteral("%1 deterministic candidates primary; %2 semantic candidates available; "
                       "%3 accepted bounded supplements; local-only and non-authoritative.")
            .arg(result.budget.deterministicCandidateCount)
            .arg(result.budget.semanticCandidateCount)
            .arg(result.budget.acceptedSupplementCount);
    result.arbitration.summary =
        QStringLiteral("Deterministic acceptance arbitration approved %1 semantic supplements "
                       "only after deterministic retrieval occupied primary order.")
            .arg(result.budget.acceptedSupplementCount);
    result.fallback.summaries = {
        QStringLiteral("Deterministic retrieval wins all conflicts."),
        QStringLiteral("Accepted semantic supplements cannot replace deterministic candidates."),
        result.fallback.summary,
    };
    return result;
}

SemanticSupplementAssemblyResult
assembleSemanticSupplements(const SemanticAcceptanceResult& acceptanceResult,
                            const SemanticSupplementAssemblyPolicy& policy) {
    SemanticSupplementAssemblyResult result;
    result.policy = policy;
    result.budget.maxSupplementBlocks = std::max(0, policy.maxSupplementBlocks);
    result.budget.maxCharacters = std::max(0, policy.maxCharacters);
    result.budget.acceptedCandidateCount = toInt(acceptanceResult.acceptedCandidates.size());
    result.budget.timeoutMs = std::max(0, policy.timeoutMs);
    result.budget.elapsedMs = acceptanceResult.budget.elapsedMs;

    result.safety.disabledByDefault = !policy.enabled;
    result.safety.deterministicRetrievalAuthoritative = policy.deterministicRetrievalAuthoritative;
    result.safety.separateFromDeterministicContext = policy.separateFromDeterministicContext;
    result.safety.promptMutationBlocked = !policy.includeInLivePrompt;
    result.safety.promptContextMutationBlocked = !policy.promptContextMutationEnabled;
    result.safety.retrievalPlanningMutationBlocked = !policy.retrievalPlanningMutationEnabled;
    result.safety.deterministicContextReplacementBlocked =
        !policy.deterministicContextReplacementEnabled;
    result.safety.deterministicContextReorderingBlocked =
        !policy.deterministicContextReorderingEnabled;
    result.safety.conversationWindowOverrideBlocked = !policy.conversationWindowOverrideEnabled;
    result.safety.summariesOverrideBlocked = !policy.summariesOverrideEnabled;
    result.safety.committedMemoryOverrideBlocked = !policy.committedMemoryOverrideEnabled;
    result.safety.runtimeMetadataOverrideBlocked = !policy.runtimeMetadataOverrideEnabled;
    result.safety.rawVectorsBlocked = !policy.exposeRawVectors;
    result.safety.scoresBlocked = !policy.exposeScores;
    result.safety.providerHandlesBlocked = !policy.exposeProviderHandles;
    result.safety.filesystemPathsBlocked = !policy.exposeFilesystemPaths;
    result.safety.debugDumpsBlocked = !policy.exposeDebugDumps;
    result.safety.safe =
        result.safety.nonAuthoritative && result.safety.deterministicRetrievalAuthoritative &&
        result.safety.separateFromDeterministicContext && result.safety.promptMutationBlocked &&
        result.safety.promptContextMutationBlocked &&
        result.safety.retrievalPlanningMutationBlocked &&
        result.safety.deterministicContextReplacementBlocked &&
        result.safety.deterministicContextReorderingBlocked &&
        result.safety.conversationWindowOverrideBlocked && result.safety.summariesOverrideBlocked &&
        result.safety.committedMemoryOverrideBlocked &&
        result.safety.runtimeMetadataOverrideBlocked && result.safety.rawVectorsBlocked &&
        result.safety.scoresBlocked && result.safety.providerHandlesBlocked &&
        result.safety.filesystemPathsBlocked && result.safety.debugDumpsBlocked;
    result.safety.checks = {
        QStringLiteral("Deterministic retrieval authoritative: %1")
            .arg(result.safety.deterministicRetrievalAuthoritative ? QStringLiteral("yes")
                                                                   : QStringLiteral("no")),
        QStringLiteral("Semantic supplements non-authoritative: yes"),
        QStringLiteral("Separate from deterministic context blocks: %1")
            .arg(result.safety.separateFromDeterministicContext ? QStringLiteral("yes")
                                                                : QStringLiteral("no")),
        QStringLiteral("Live prompt inclusion: blocked"),
        QStringLiteral("PromptContextBlock mutation: no"),
        QStringLiteral("RetrievalPlanningResult mutation: no"),
        QStringLiteral("Deterministic context replacement: no"),
        QStringLiteral("Deterministic context reordering: no"),
        QStringLiteral("Conversation window override: no"),
        QStringLiteral("Summary block override: no"),
        QStringLiteral("Committed memory override: no"),
        QStringLiteral("Runtime metadata override: no"),
        QStringLiteral("Raw vectors exposed: no"),
        QStringLiteral("Raw scores exposed: no"),
        QStringLiteral("Provider handles exposed: no"),
        QStringLiteral("Filesystem paths exposed: no"),
        QStringLiteral("Debug dumps exposed: no"),
    };
    result.checks = result.safety.checks;
    result.readiness.checks = result.checks;

    const auto fallback = [&](SemanticSupplementAssemblyStatus status, const QString& reason,
                              const QString& summary) {
        result.status = status;
        result.readiness.status = status;
        result.failureReason = reason;
        result.summary = reason;
        result.fallbackSummary = summary;
        result.readiness.summary = summary;
        result.bundle.summary = summary;
        result.budget.summary =
            QStringLiteral("0 of %1 semantic supplement characters assembled from %2 accepted "
                           "candidates.")
                .arg(result.budget.maxCharacters)
                .arg(result.budget.acceptedCandidateCount);
    };

    if (!policy.enabled) {
        fallback(SemanticSupplementAssemblyStatus::Disabled,
                 QStringLiteral("Semantic supplement assembly is disabled."),
                 QStringLiteral("Semantic supplement assembly disabled; deterministic prompt "
                                "behavior unchanged."));
        return result;
    }
    if (!policy.allowTestOnlyAssembly || policy.includeInLivePrompt ||
        !policy.deterministicRetrievalAuthoritative || !policy.semanticSupplementsOnly ||
        !policy.separateFromDeterministicContext || policy.promptContextMutationEnabled ||
        policy.retrievalPlanningMutationEnabled || policy.deterministicContextReplacementEnabled ||
        policy.deterministicContextReorderingEnabled || policy.conversationWindowOverrideEnabled ||
        policy.summariesOverrideEnabled || policy.committedMemoryOverrideEnabled ||
        policy.runtimeMetadataOverrideEnabled || policy.exposeRawVectors || policy.exposeScores ||
        policy.exposeProviderHandles || policy.exposeFilesystemPaths || policy.exposeDebugDumps ||
        !result.safety.safe) {
        fallback(SemanticSupplementAssemblyStatus::Refused,
                 QStringLiteral("Semantic supplement assembly refused by prompt authority gates."),
                 QStringLiteral("Semantic supplement assembly refused; deterministic prompt "
                                "context remains authoritative and unchanged."));
        return result;
    }
    if (acceptanceResult.status == SemanticAcceptanceStatus::Busy) {
        fallback(SemanticSupplementAssemblyStatus::Busy,
                 QStringLiteral("Semantic supplement assembly skipped because acceptance is busy."),
                 QStringLiteral("Busy semantic state ignored; deterministic prompt behavior "
                                "unchanged."));
        return result;
    }
    if (acceptanceResult.status == SemanticAcceptanceStatus::Stale) {
        fallback(SemanticSupplementAssemblyStatus::Stale,
                 QStringLiteral("Semantic supplement assembly ignored stale acceptance state."),
                 QStringLiteral("Stale semantic state ignored; deterministic prompt behavior "
                                "unchanged."));
        return result;
    }
    if (acceptanceResult.status == SemanticAcceptanceStatus::TimedOut ||
        result.budget.elapsedMs > result.budget.timeoutMs) {
        fallback(SemanticSupplementAssemblyStatus::TimedOut,
                 QStringLiteral("Semantic supplement assembly timed out."),
                 QStringLiteral("Timed-out semantic state ignored; deterministic prompt behavior "
                                "unchanged."));
        return result;
    }
    if (acceptanceResult.status == SemanticAcceptanceStatus::Refused) {
        fallback(SemanticSupplementAssemblyStatus::Refused,
                 QStringLiteral("Semantic supplement assembly refused errored acceptance state."),
                 QStringLiteral("Semantic error fallback active; deterministic prompt behavior "
                                "unchanged."));
        return result;
    }
    if (acceptanceResult.acceptedCandidates.isEmpty() ||
        acceptanceResult.status != SemanticAcceptanceStatus::Ready) {
        fallback(SemanticSupplementAssemblyStatus::Empty,
                 QStringLiteral("No accepted semantic supplements are available for assembly."),
                 QStringLiteral("Empty semantic supplement fallback; deterministic prompt behavior "
                                "unchanged."));
        result.readiness.ready = true;
        return result;
    }

    auto candidates = acceptanceResult.acceptedCandidates;
    std::sort(candidates.begin(), candidates.end(),
              [](const SemanticAcceptedCandidate& left, const SemanticAcceptedCandidate& right) {
                  if (left.deterministicOffsetRank != right.deterministicOffsetRank) {
                      return left.deterministicOffsetRank < right.deterministicOffsetRank;
                  }
                  if (left.supplementRank != right.supplementRank) {
                      return left.supplementRank < right.supplementRank;
                  }
                  return left.id < right.id;
              });

    int remainingCharacters = result.budget.maxCharacters;
    const int blockLimit = result.budget.maxSupplementBlocks;
    int rank = 1;
    for (const auto& candidate : candidates) {
        if (result.bundle.blocks.size() >= blockLimit || remainingCharacters <= 0) {
            break;
        }
        const auto normalizedSummary = candidate.summary.simplified();
        const int estimatedCharacters = toInt(normalizedSummary.size());
        const int includedCharacters = std::min(estimatedCharacters, remainingCharacters);
        const bool truncated = includedCharacters < estimatedCharacters;
        result.bundle.blocks.append(SemanticSupplementBlock{
            candidate.id,
            candidate.title,
            normalizedSummary.left(includedCharacters),
            rank,
            estimatedCharacters,
            includedCharacters,
            truncated,
            true,
            true,
            true,
        });
        result.budget.estimatedCharacters += estimatedCharacters;
        result.budget.includedCharacters += includedCharacters;
        if (truncated) {
            ++result.budget.truncatedBlockCount;
        }
        remainingCharacters -= includedCharacters;
        ++rank;
    }

    result.bundle.blockCount = toInt(result.bundle.blocks.size());
    result.bundle.estimatedCharacters = result.budget.estimatedCharacters;
    result.bundle.includedCharacters = result.budget.includedCharacters;
    result.bundle.truncatedBlockCount = result.budget.truncatedBlockCount;
    result.bundle.empty = result.bundle.blocks.isEmpty();
    result.budget.assembledBlockCount = result.bundle.blockCount;
    result.assembled = result.bundle.blockCount > 0;
    result.readiness.ready = true;
    result.status = result.budget.truncatedBlockCount > 0 ||
                            result.bundle.blockCount < acceptanceResult.acceptedCandidates.size()
                        ? SemanticSupplementAssemblyStatus::Truncated
                        : SemanticSupplementAssemblyStatus::Ready;
    result.readiness.status = result.status;
    if (!result.assembled) {
        fallback(SemanticSupplementAssemblyStatus::Empty,
                 QStringLiteral("Semantic supplement assembly produced an empty bundle."),
                 QStringLiteral("Empty semantic supplement fallback; deterministic prompt behavior "
                                "unchanged."));
        result.readiness.ready = true;
        return result;
    }

    result.budget.summary =
        QStringLiteral("%1 of %2 semantic supplement blocks assembled, using %3 of %4 characters; "
                       "%5 blocks truncated.")
            .arg(result.bundle.blockCount)
            .arg(result.budget.maxSupplementBlocks)
            .arg(result.budget.includedCharacters)
            .arg(result.budget.maxCharacters)
            .arg(result.budget.truncatedBlockCount);
    result.bundle.summary =
        QStringLiteral("%1 bounded semantic supplement metadata blocks assembled separately from "
                       "deterministic prompt context.")
            .arg(result.bundle.blockCount);
    result.readiness.summary =
        QStringLiteral("Semantic supplement assembly ready for test-only bounded metadata.");
    result.summary =
        QStringLiteral("Semantic supplement assembly prepared %1 non-authoritative blocks for a "
                       "separate test-only bundle; live prompt behavior is unchanged.")
            .arg(result.bundle.blockCount);
    result.fallbackSummary =
        QStringLiteral("Deterministic prompt context remains authoritative; semantic supplements "
                       "are separate metadata only.");
    return result;
}

SemanticPromptAuthorityResult
evaluateSemanticPromptAuthority(const SemanticSupplementAssemblyResult& assemblyResult,
                                const SemanticPromptAuthorityPolicy& policy) {
    SemanticPromptAuthorityResult result;
    result.policy = policy;
    result.wouldIncludeBlockCount = assemblyResult.bundle.blockCount;
    result.wouldIncludeCharacters = assemblyResult.bundle.includedCharacters;

    result.safety.deterministicRetrievalAuthoritative =
        policy.deterministicRetrievalAuthoritative &&
        assemblyResult.safety.deterministicRetrievalAuthoritative;
    result.safety.semanticSearchLocalOnly = policy.semanticSearchLocalOnly;
    result.safety.acceptedByDeterministicLayer =
        !assemblyResult.bundle.blocks.isEmpty() &&
        assemblyResult.status != SemanticSupplementAssemblyStatus::Disabled &&
        assemblyResult.status != SemanticSupplementAssemblyStatus::Refused;
    result.safety.boundedSupplementBundle =
        !assemblyResult.bundle.blocks.isEmpty() &&
        assemblyResult.bundle.blockCount <= assemblyResult.budget.maxSupplementBlocks &&
        assemblyResult.bundle.includedCharacters <= assemblyResult.budget.maxCharacters;
    result.safety.promptInjectionExplicitlyEnabled = policy.promptInjectionExplicitlyEnabled;
    result.safety.policyExplicitlyAllows = policy.semanticPromptAuthorityAllowed;
    result.safety.livePromptMutationBlocked = !policy.includeInLivePrompt;
    result.safety.semanticAuthorityEscalationBlocked =
        policy.semanticSupplementsOnly && assemblyResult.safety.nonAuthoritative;
    result.safety.supplementsRemainSupplemental =
        policy.semanticSupplementsOnly && assemblyResult.safety.nonAuthoritative;
    result.safety.clearlyDelimitedSupplements = policy.clearlyDelimitedSupplements;
    result.safety.rawPromptPayloadsBlocked = !policy.exposeRawPromptPayloads;
    result.safety.rawSupplementBlocksBlocked = !policy.exposeRawSupplementBlocks;
    result.safety.rawVectorsBlocked = !policy.exposeRawVectors;
    result.safety.scoresBlocked = !policy.exposeScores;
    result.safety.filesystemPathsBlocked = !policy.exposeFilesystemPaths;
    result.safety.providerHandlesBlocked = !policy.exposeProviderHandles;
    result.safety.debugDumpsBlocked = !policy.exposeDebugDumps;
    result.safety.safe =
        result.safety.deterministicRetrievalAuthoritative &&
        result.safety.semanticSearchLocalOnly && result.safety.acceptedByDeterministicLayer &&
        result.safety.boundedSupplementBundle &&
        (!policy.requireSafetyReportPass || assemblyResult.safety.safe) &&
        result.safety.promptInjectionExplicitlyEnabled && result.safety.policyExplicitlyAllows &&
        result.safety.livePromptMutationBlocked &&
        result.safety.semanticAuthorityEscalationBlocked &&
        result.safety.supplementsRemainSupplemental && result.safety.clearlyDelimitedSupplements &&
        result.safety.rawPromptPayloadsBlocked && result.safety.rawSupplementBlocksBlocked &&
        result.safety.rawVectorsBlocked && result.safety.scoresBlocked &&
        result.safety.filesystemPathsBlocked && result.safety.providerHandlesBlocked &&
        result.safety.debugDumpsBlocked;
    result.safety.summary =
        result.safety.safe
            ? QStringLiteral(
                  "Semantic prompt authority safety passes for test-only metadata; live prompt "
                  "mutation remains blocked.")
            : QStringLiteral(
                  "Semantic prompt authority safety did not pass; deterministic-only fallback is "
                  "active.");
    result.safety.checks = {
        QStringLiteral("Deterministic retrieval authoritative: %1")
            .arg(result.safety.deterministicRetrievalAuthoritative ? QStringLiteral("yes")
                                                                   : QStringLiteral("no")),
        QStringLiteral("Semantic search local-only: %1")
            .arg(result.safety.semanticSearchLocalOnly ? QStringLiteral("yes")
                                                       : QStringLiteral("no")),
        QStringLiteral("Accepted by deterministic acceptance layer: %1")
            .arg(result.safety.acceptedByDeterministicLayer ? QStringLiteral("yes")
                                                            : QStringLiteral("no")),
        QStringLiteral("Supplement bundle bounded: %1")
            .arg(result.safety.boundedSupplementBundle ? QStringLiteral("yes")
                                                       : QStringLiteral("no")),
        QStringLiteral("Prompt injection setting explicitly enabled: %1")
            .arg(result.safety.promptInjectionExplicitlyEnabled ? QStringLiteral("yes")
                                                                : QStringLiteral("no")),
        QStringLiteral("Semantic prompt authority policy allows inclusion: %1")
            .arg(result.safety.policyExplicitlyAllows ? QStringLiteral("yes")
                                                      : QStringLiteral("no")),
        QStringLiteral("Assembly safety report passes: %1")
            .arg(assemblyResult.safety.safe ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Live prompt mutation: blocked"),
        QStringLiteral("Semantic supplements remain supplemental: %1")
            .arg(result.safety.supplementsRemainSupplemental ? QStringLiteral("yes")
                                                             : QStringLiteral("no")),
        QStringLiteral("Semantic authority escalation: blocked"),
        QStringLiteral("Raw prompt payloads exposed: no"),
        QStringLiteral("Raw supplement blocks exposed: no"),
        QStringLiteral("Raw vectors exposed: no"),
        QStringLiteral("Raw scores exposed: no"),
        QStringLiteral("Filesystem paths exposed: no"),
        QStringLiteral("Provider handles exposed: no"),
        QStringLiteral("Debug dumps exposed: no"),
        QStringLiteral("PromptContextBlock mutation: no"),
        QStringLiteral("RetrievalPlanningResult mutation: no"),
    };
    result.checks = result.safety.checks;
    result.readiness.checks = result.checks;

    const auto deny = [&](SemanticPromptAuthorityStatus status, const QString& reason,
                          const QString& fallbackState) {
        result.status = status;
        result.decision = SemanticPromptAuthorityDecision::Denied;
        result.allowed = false;
        result.wouldInclude = false;
        result.livePromptMutationAllowed = false;
        result.failureReason = reason;
        result.summary = reason;
        result.readiness.status = status;
        result.readiness.ready = false;
        result.readiness.summary = fallbackState;
        result.fallback.state = semanticPromptAuthorityStatusName(status);
        result.fallback.summary = fallbackState;
        result.fallback.summaries = {
            fallbackState,
            QStringLiteral("Deterministic retrieval remains authoritative."),
            QStringLiteral("Default prompt assembly is unchanged."),
        };
        result.audit.decisionSummary =
            QStringLiteral("%1 / %2").arg(semanticPromptAuthorityDecisionName(result.decision),
                                          semanticPromptAuthorityStatusName(status));
        result.audit.denialReason = reason;
        result.audit.readinessSummary = result.readiness.summary;
        result.audit.safetySummary = result.safety.summary;
        result.audit.fallbackSummary = result.fallback.summary;
        result.audit.reasons = result.fallback.summaries;
    };

    if (!policy.enabled) {
        deny(SemanticPromptAuthorityStatus::Disabled,
             QStringLiteral("Semantic prompt authority policy is disabled."),
             QStringLiteral("Disabled fallback active; deterministic prompt assembly remains "
                            "unchanged."));
        return result;
    }
    if (assemblyResult.status == SemanticSupplementAssemblyStatus::Busy) {
        deny(SemanticPromptAuthorityStatus::Busy,
             QStringLiteral("Semantic prompt authority skipped because supplement assembly is "
                            "busy."),
             QStringLiteral("Busy semantic state ignored; deterministic-only fallback active."));
        return result;
    }
    if (assemblyResult.status == SemanticSupplementAssemblyStatus::Stale) {
        deny(SemanticPromptAuthorityStatus::Stale,
             QStringLiteral("Semantic prompt authority ignored stale supplement assembly."),
             QStringLiteral("Stale semantic state ignored; deterministic-only fallback active."));
        return result;
    }
    if (assemblyResult.status == SemanticSupplementAssemblyStatus::TimedOut ||
        assemblyResult.budget.elapsedMs > policy.timeoutMs) {
        deny(SemanticPromptAuthorityStatus::TimedOut,
             QStringLiteral("Semantic prompt authority timed out."),
             QStringLiteral(
                 "Timed-out semantic state ignored; deterministic-only fallback active."));
        return result;
    }
    if (assemblyResult.status == SemanticSupplementAssemblyStatus::Refused) {
        deny(SemanticPromptAuthorityStatus::Refused,
             QStringLiteral("Semantic prompt authority refused supplement assembly state."),
             QStringLiteral("Refused semantic state ignored; deterministic-only fallback active."));
        return result;
    }
    if (!policy.allowTestOnlyWouldIncludeMetadata || policy.includeInLivePrompt ||
        !policy.promptInjectionExplicitlyEnabled || !policy.semanticPromptAuthorityAllowed ||
        !policy.deterministicRetrievalAuthoritative || !policy.semanticSupplementsOnly ||
        !policy.semanticSearchLocalOnly || !policy.clearlyDelimitedSupplements ||
        policy.exposeRawPromptPayloads || policy.exposeRawSupplementBlocks ||
        policy.exposeRawVectors || policy.exposeScores || policy.exposeFilesystemPaths ||
        policy.exposeProviderHandles || policy.exposeDebugDumps) {
        deny(SemanticPromptAuthorityStatus::Denied,
             QStringLiteral("Semantic prompt authority denied by policy gate."),
             QStringLiteral("Policy fallback active; deterministic prompt assembly remains "
                            "unchanged."));
        return result;
    }
    if (policy.requireAcceptedSupplements &&
        (assemblyResult.bundle.blocks.isEmpty() ||
         (assemblyResult.status != SemanticSupplementAssemblyStatus::Ready &&
          assemblyResult.status != SemanticSupplementAssemblyStatus::Truncated))) {
        deny(SemanticPromptAuthorityStatus::Denied,
             QStringLiteral("Semantic prompt authority denied because no accepted bounded "
                            "supplements are ready."),
             QStringLiteral("Deterministic-only fallback active; no semantic supplement metadata "
                            "would be included."));
        return result;
    }
    if (!result.safety.safe) {
        deny(SemanticPromptAuthorityStatus::SafetyBlocked,
             QStringLiteral("Semantic prompt authority blocked by safety report."),
             QStringLiteral("Safety-report fallback active; deterministic prompt assembly remains "
                            "unchanged."));
        return result;
    }

    result.status = SemanticPromptAuthorityStatus::WouldIncludeMetadataOnly;
    result.decision = SemanticPromptAuthorityDecision::WouldIncludeMetadataOnly;
    result.allowed = true;
    result.wouldInclude = true;
    result.livePromptMutationAllowed = false;
    result.failureReason.clear();
    result.readiness.status = result.status;
    result.readiness.ready = true;
    result.readiness.summary =
        QStringLiteral("Semantic prompt authority is ready only for test-only would-include "
                       "metadata; live prompt inclusion remains blocked.");
    result.fallback.state = QStringLiteral("Deterministic Only");
    result.fallback.summary =
        QStringLiteral("Deterministic fallback remains available and authoritative.");
    result.fallback.summaries = {
        result.fallback.summary,
        QStringLiteral("Semantic supplements remain supplemental and delimited."),
        QStringLiteral("Default prompt assembly is unchanged."),
    };
    result.audit.decisionSummary =
        QStringLiteral("Would include %1 semantic supplement metadata blocks in a test-only "
                       "decision; live prompt mutation is blocked.")
            .arg(result.wouldIncludeBlockCount);
    result.audit.denialReason.clear();
    result.audit.readinessSummary = result.readiness.summary;
    result.audit.safetySummary = result.safety.summary;
    result.audit.fallbackSummary = result.fallback.summary;
    result.audit.reasons = result.fallback.summaries;
    result.summary =
        QStringLiteral("Semantic prompt authority would include %1 bounded supplemental metadata "
                       "blocks only in a test-only path; default prompt assembly is unchanged.")
            .arg(result.wouldIncludeBlockCount);
    return result;
}

SemanticPromptInclusionResult
includeSemanticPromptSupplements(const PromptContextInjectionResult& deterministicPrompt,
                                 const SemanticSupplementAssemblyResult& assemblyResult,
                                 const SemanticPromptAuthorityResult& authorityResult,
                                 const SemanticPromptInclusionPolicy& policy) {
    SemanticPromptInclusionResult result;
    result.policy = policy;
    result.enabled = policy.enabled;
    result.originalPrompt = deterministicPrompt.prompt;
    result.prompt = deterministicPrompt.prompt;
    result.budget.maxSupplementBlocks = std::max(0, policy.maxSupplementBlocks);
    result.budget.maxCharacters = std::max(0, policy.maxCharacters);
    result.budget.availableSupplementBlocks = assemblyResult.bundle.blockCount;
    result.budget.timeoutMs = std::max(0, policy.timeoutMs);
    result.budget.elapsedMs = assemblyResult.budget.elapsedMs;

    result.safety.contextInjectionEnabled = policy.contextInjectionEnabled;
    result.safety.authorityApproved = authorityResult.allowed && authorityResult.wouldInclude;
    result.safety.boundedAssembly =
        !assemblyResult.bundle.blocks.isEmpty() &&
        assemblyResult.bundle.blockCount <= assemblyResult.budget.maxSupplementBlocks &&
        assemblyResult.bundle.includedCharacters <= assemblyResult.budget.maxCharacters;
    result.safety.localOnlyMode = policy.localOnlyMode;
    result.safety.deterministicRetrievalAuthoritative =
        policy.deterministicRetrievalAuthoritative &&
        authorityResult.safety.deterministicRetrievalAuthoritative &&
        assemblyResult.safety.deterministicRetrievalAuthoritative;
    result.safety.supplementalOnly = policy.supplementalOnly &&
                                     authorityResult.safety.supplementsRemainSupplemental &&
                                     assemblyResult.safety.nonAuthoritative;
    result.safety.clearlyDelimited =
        policy.clearlyDelimitedSupplements && authorityResult.safety.clearlyDelimitedSupplements;
    result.safety.deterministicContextReplacementBlocked =
        !policy.deterministicContextReplacementEnabled;
    result.safety.deterministicContextReorderingBlocked =
        !policy.deterministicContextReorderingEnabled;
    result.safety.committedMemoryOverrideBlocked = !policy.committedMemoryOverrideEnabled;
    result.safety.summariesOverrideBlocked = !policy.summariesOverrideEnabled;
    result.safety.conversationWindowOverrideBlocked = !policy.conversationWindowOverrideEnabled;
    result.safety.runtimeMetadataOverrideBlocked = !policy.runtimeMetadataOverrideEnabled;
    result.safety.rawPromptPayloadsBlocked = !policy.exposeRawPromptPayloads;
    result.safety.rawVectorsBlocked = !policy.exposeRawVectors;
    result.safety.scoresBlocked = !policy.exposeScores;
    result.safety.providerHandlesBlocked = !policy.exposeProviderHandles;
    result.safety.filesystemPathsBlocked = !policy.exposeFilesystemPaths;
    result.safety.debugDumpsBlocked = !policy.exposeDebugDumps;
    result.safety.safe =
        result.safety.contextInjectionEnabled &&
        (!policy.requireAuthorityApproval || result.safety.authorityApproved) &&
        (!policy.requireBoundedAssembly || result.safety.boundedAssembly) &&
        (!policy.requireSafetyReportPass ||
         (assemblyResult.safety.safe && authorityResult.safety.safe)) &&
        result.safety.localOnlyMode && result.safety.deterministicRetrievalAuthoritative &&
        result.safety.supplementalOnly && result.safety.clearlyDelimited &&
        result.safety.deterministicContextReplacementBlocked &&
        result.safety.deterministicContextReorderingBlocked &&
        result.safety.committedMemoryOverrideBlocked && result.safety.summariesOverrideBlocked &&
        result.safety.conversationWindowOverrideBlocked &&
        result.safety.runtimeMetadataOverrideBlocked && result.safety.rawPromptPayloadsBlocked &&
        result.safety.rawVectorsBlocked && result.safety.scoresBlocked &&
        result.safety.providerHandlesBlocked && result.safety.filesystemPathsBlocked &&
        result.safety.debugDumpsBlocked;
    result.safety.summary =
        result.safety.safe
            ? QStringLiteral("Semantic prompt inclusion safety passed for local supplemental "
                             "context.")
            : QStringLiteral("Semantic prompt inclusion safety did not pass; deterministic-only "
                             "fallback is active.");
    result.safety.checks = {
        QStringLiteral("Context injection enabled: %1")
            .arg(result.safety.contextInjectionEnabled ? QStringLiteral("yes")
                                                       : QStringLiteral("no")),
        QStringLiteral("Semantic prompt authority approved: %1")
            .arg(result.safety.authorityApproved ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Semantic supplement assembly bounded: %1")
            .arg(result.safety.boundedAssembly ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Local-only mode: %1")
            .arg(result.safety.localOnlyMode ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Deterministic retrieval authoritative: %1")
            .arg(result.safety.deterministicRetrievalAuthoritative ? QStringLiteral("yes")
                                                                   : QStringLiteral("no")),
        QStringLiteral("Semantic supplements supplemental-only: %1")
            .arg(result.safety.supplementalOnly ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Semantic supplements clearly delimited: %1")
            .arg(result.safety.clearlyDelimited ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Deterministic context replacement: no"),
        QStringLiteral("Deterministic context reordering: no"),
        QStringLiteral("Committed memory override: no"),
        QStringLiteral("Summary block override: no"),
        QStringLiteral("Conversation window override: no"),
        QStringLiteral("Runtime metadata override: no"),
        QStringLiteral("Raw prompt payloads exposed: no"),
        QStringLiteral("Raw vectors exposed: no"),
        QStringLiteral("Raw scores exposed: no"),
        QStringLiteral("Provider handles exposed: no"),
        QStringLiteral("Filesystem paths exposed: no"),
        QStringLiteral("Debug dumps exposed: no"),
    };
    result.checks = result.safety.checks;

    const auto fallback = [&](SemanticPromptInclusionStatus status, const QString& reason,
                              const QString& summary) {
        result.status = status;
        result.included = false;
        result.failureReason = reason;
        result.summary = reason;
        result.fallback.deterministicOnly = true;
        result.fallback.state = semanticPromptInclusionStatusName(status);
        result.fallback.summary = summary;
        result.fallback.summaries = {
            summary,
            QStringLiteral("Deterministic retrieval remains final prompt authority."),
            QStringLiteral("Semantic supplements did not replace or reorder deterministic "
                           "context."),
        };
        result.audit.stateSummary = semanticPromptInclusionStatusName(status);
        result.audit.decisionSummary = reason;
        result.audit.fallbackSummary = summary;
        result.audit.budgetSummary = result.budget.summary;
        result.audit.reasons = result.fallback.summaries;
        result.semanticBlockSummary = QStringLiteral("No semantic supplemental block included.");
    };

    result.budget.summary =
        QStringLiteral("0 of %1 semantic supplement characters included from %2 available "
                       "blocks.")
            .arg(result.budget.maxCharacters)
            .arg(result.budget.availableSupplementBlocks);

    if (!policy.enabled) {
        fallback(SemanticPromptInclusionStatus::Disabled,
                 QStringLiteral("Semantic prompt inclusion is disabled."),
                 QStringLiteral("Disabled fallback active; deterministic-only prompt assembly "
                                "remains unchanged."));
        return result;
    }
    if (!policy.contextInjectionEnabled ||
        deterministicPrompt.status == PromptContextInjectionStatus::Disabled) {
        fallback(SemanticPromptInclusionStatus::Disabled,
                 QStringLiteral("Semantic prompt inclusion requires context injection to be "
                                "enabled."),
                 QStringLiteral("Context-injection fallback active; deterministic-only prompt "
                                "assembly remains unchanged."));
        return result;
    }
    if (assemblyResult.status == SemanticSupplementAssemblyStatus::Busy ||
        authorityResult.status == SemanticPromptAuthorityStatus::Busy) {
        fallback(SemanticPromptInclusionStatus::Busy,
                 QStringLiteral("Semantic prompt inclusion skipped because semantic state is "
                                "busy."),
                 QStringLiteral("Busy semantic state ignored; deterministic-only prompt active."));
        return result;
    }
    if (assemblyResult.status == SemanticSupplementAssemblyStatus::Stale ||
        authorityResult.status == SemanticPromptAuthorityStatus::Stale) {
        fallback(SemanticPromptInclusionStatus::Stale,
                 QStringLiteral("Semantic prompt inclusion ignored stale semantic state."),
                 QStringLiteral("Stale semantic state ignored; deterministic-only prompt active."));
        return result;
    }
    if (assemblyResult.status == SemanticSupplementAssemblyStatus::TimedOut ||
        authorityResult.status == SemanticPromptAuthorityStatus::TimedOut ||
        assemblyResult.budget.elapsedMs > result.budget.timeoutMs) {
        fallback(
            SemanticPromptInclusionStatus::TimedOut,
            QStringLiteral("Semantic prompt inclusion timed out."),
            QStringLiteral("Timed-out semantic state ignored; deterministic-only prompt active."));
        return result;
    }
    if (assemblyResult.status == SemanticSupplementAssemblyStatus::Refused ||
        authorityResult.status == SemanticPromptAuthorityStatus::Refused) {
        fallback(SemanticPromptInclusionStatus::Refused,
                 QStringLiteral("Semantic prompt inclusion refused semantic state."),
                 QStringLiteral("Refused semantic state ignored; deterministic-only prompt "
                                "active."));
        return result;
    }
    if (assemblyResult.bundle.blocks.isEmpty()) {
        fallback(SemanticPromptInclusionStatus::Empty,
                 QStringLiteral("Semantic prompt inclusion found no supplement blocks."),
                 QStringLiteral("Empty semantic supplement fallback; deterministic-only prompt "
                                "active."));
        return result;
    }
    if (!result.safety.safe || !assemblyResult.safety.safe || !authorityResult.safety.safe) {
        fallback(SemanticPromptInclusionStatus::SafetyBlocked,
                 QStringLiteral("Semantic prompt inclusion blocked by safety report."),
                 QStringLiteral("Safety-report fallback active; deterministic-only prompt "
                                "assembly remains unchanged."));
        return result;
    }
    if (!authorityResult.allowed || !authorityResult.wouldInclude) {
        fallback(SemanticPromptInclusionStatus::Denied,
                 QStringLiteral("Semantic prompt inclusion denied by authority policy."),
                 QStringLiteral("Authority-policy fallback active; deterministic-only prompt "
                                "assembly remains unchanged."));
        return result;
    }

    auto blocks = assemblyResult.bundle.blocks;
    std::sort(blocks.begin(), blocks.end(),
              [](const SemanticSupplementBlock& left, const SemanticSupplementBlock& right) {
                  if (left.rank != right.rank) {
                      return left.rank < right.rank;
                  }
                  return left.id < right.id;
              });

    QStringList blockTexts;
    int remainingCharacters = result.budget.maxCharacters;
    int rank = 1;
    for (const auto& block : blocks) {
        if (result.budget.includedSupplementBlocks >= result.budget.maxSupplementBlocks ||
            remainingCharacters <= 0) {
            break;
        }

        const auto sanitizedSummary = sanitizedSemanticPromptText(block.summary);
        if (sanitizedSummary.isEmpty()) {
            continue;
        }

        const int estimatedCharacters = toInt(sanitizedSummary.size());
        const int includedCharacters = std::min(estimatedCharacters, remainingCharacters);
        const auto includedSummary = sanitizedSummary.left(includedCharacters).trimmed();
        if (includedSummary.isEmpty()) {
            continue;
        }

        const bool truncated = includedCharacters < estimatedCharacters || block.truncated;
        result.budget.estimatedCharacters += estimatedCharacters;
        result.budget.includedCharacters += toInt(includedSummary.size());
        result.budget.includedSupplementBlocks += 1;
        if (truncated) {
            ++result.budget.truncatedBlockCount;
        }
        remainingCharacters -= toInt(includedSummary.size());
        blockTexts.append(QStringLiteral("--- Semantic Supplement %1: %2 ---\n%3")
                              .arg(rank)
                              .arg(block.title.trimmed().isEmpty()
                                       ? QStringLiteral("Supplemental Local Context")
                                       : block.title.trimmed(),
                                   includedSummary));
        ++rank;
    }

    if (blockTexts.isEmpty()) {
        fallback(SemanticPromptInclusionStatus::Empty,
                 QStringLiteral("Semantic prompt inclusion produced an empty supplement block."),
                 QStringLiteral("Empty semantic supplement fallback; deterministic-only prompt "
                                "active."));
        return result;
    }

    const auto semanticBlock =
        QStringLiteral("%1\n%2\n%3\n%4")
            .arg(policy.delimiterStart,
                 QStringLiteral("Supplemental only. Deterministic context above remains "
                                "authoritative."),
                 blockTexts.join(QStringLiteral("\n")), policy.delimiterEnd);
    const auto userPromptMarker = QStringLiteral("\n\nUser prompt:\n");
    const auto markerIndex = deterministicPrompt.prompt.indexOf(userPromptMarker);
    if (markerIndex < 0) {
        fallback(SemanticPromptInclusionStatus::Denied,
                 QStringLiteral("Semantic prompt inclusion requires an existing deterministic "
                                "context prompt boundary."),
                 QStringLiteral("Prompt-boundary fallback active; deterministic-only prompt "
                                "assembly remains unchanged."));
        return result;
    }

    result.prompt = QStringLiteral("%1\n\n%2%3")
                        .arg(deterministicPrompt.prompt.left(markerIndex), semanticBlock,
                             deterministicPrompt.prompt.mid(markerIndex));
    result.included = true;
    result.status = result.budget.truncatedBlockCount > 0 ||
                            result.budget.includedSupplementBlocks < blocks.size()
                        ? SemanticPromptInclusionStatus::Truncated
                        : SemanticPromptInclusionStatus::Included;
    result.failureReason.clear();
    result.fallback.deterministicOnly = false;
    result.fallback.state = QStringLiteral("Deterministic Authority Preserved");
    result.fallback.summary =
        QStringLiteral("Semantic supplements included as bounded non-authoritative context; "
                       "deterministic retrieval remains final authority.");
    result.fallback.summaries = {
        result.fallback.summary,
        QStringLiteral("Semantic supplements cannot replace deterministic context."),
        QStringLiteral("Semantic supplements cannot reorder deterministic context."),
    };
    result.budget.summary =
        QStringLiteral("%1 of %2 semantic supplement blocks included, using %3 of %4 characters; "
                       "%5 blocks truncated.")
            .arg(result.budget.includedSupplementBlocks)
            .arg(result.budget.maxSupplementBlocks)
            .arg(result.budget.includedCharacters)
            .arg(result.budget.maxCharacters)
            .arg(result.budget.truncatedBlockCount);
    result.semanticBlockSummary =
        QStringLiteral("%1 non-authoritative semantic supplement blocks included after "
                       "deterministic context.")
            .arg(result.budget.includedSupplementBlocks);
    result.summary =
        QStringLiteral("Semantic prompt inclusion appended %1 supplemental blocks after "
                       "deterministic context; deterministic retrieval remains authoritative.")
            .arg(result.budget.includedSupplementBlocks);
    result.audit.stateSummary = semanticPromptInclusionStatusName(result.status);
    result.audit.decisionSummary = result.summary;
    result.audit.fallbackSummary = result.fallback.summary;
    result.audit.budgetSummary = result.budget.summary;
    result.audit.reasons = result.fallback.summaries;
    return result;
}

QStringList semanticRetrievalReadinessChecks(const SemanticRetrievalPolicy& policy,
                                             EmbeddingProviderStatus providerStatus,
                                             VectorIndexStatus indexStatus, int indexedItemCount) {
    return {
        QStringLiteral("Semantic retrieval: %1")
            .arg(policy.enabled ? QStringLiteral("enabled") : QStringLiteral("disabled")),
        QStringLiteral("Semantic ranking: %1")
            .arg(policy.semanticRankingEnabled ? QStringLiteral("enabled")
                                               : QStringLiteral("disabled")),
        QStringLiteral("Prompt injection from semantic retrieval: %1")
            .arg(policy.promptInjectionEnabled ? QStringLiteral("enabled")
                                               : QStringLiteral("disabled")),
        QStringLiteral("Embedding provider: %1").arg(embeddingProviderStatusName(providerStatus)),
        QStringLiteral("Vector index: %1").arg(vectorIndexStatusName(indexStatus)),
        QStringLiteral("Indexed items: %1").arg(indexedItemCount),
        QStringLiteral("Raw vectors exposed to QML: %1")
            .arg(policy.exposeRawVectorsToQml ? QStringLiteral("yes") : QStringLiteral("no")),
    };
}

VectorPersistenceReadiness vectorPersistenceReadiness(const VectorPersistencePolicy& policy,
                                                      const VectorPersistenceSession& session) {
    if (!policy.enabled) {
        return VectorPersistenceReadiness::Disabled;
    }
    const bool blocked = !policy.localOnly || !policy.isolatedEmbeddingOutputsOnly ||
                         policy.automaticIndexingEnabled || policy.filesystemScanningEnabled ||
                         policy.backgroundIngestionEnabled ||
                         policy.semanticRetrievalAuthorityEnabled || policy.promptMutationEnabled ||
                         policy.automaticMemoryConversionEnabled ||
                         policy.cloudVectorServicesAllowed || session.busy ||
                         session.requestId.trimmed().isEmpty() ||
                         (!session.activeRequestId.trimmed().isEmpty() &&
                          session.activeRequestId.trimmed() != session.requestId.trimmed());
    return blocked ? VectorPersistenceReadiness::Refused : VectorPersistenceReadiness::Ready;
}

SemanticCandidateArbitration
orchestrateSemanticCandidates(const QList<SemanticCandidate>& candidates,
                              const SemanticCandidatePolicy& policy) {
    SemanticCandidateArbitration result;
    result.budget.maxCharacters = std::max(0, policy.maxCharacters);
    result.budget.remainingCharacters = result.budget.maxCharacters;

    if (!policy.enabled) {
        result.status = SemanticCandidateStatus::Disabled;
        result.summary = policy.summary;
        return result;
    }

    QList<SemanticCandidate> normalized;
    normalized.reserve(candidates.size());
    int sequence = 0;
    for (const auto& candidate : candidates) {
        SemanticCandidate item = candidate;
        item.id = item.id.trimmed().isEmpty()
                      ? QStringLiteral("semantic-candidate-%1").arg(sequence)
                      : item.id.trimmed();
        item.title = item.title.simplified();
        item.content = item.content.trimmed();
        item.originalSize = toInt(item.content.size());
        item.selectedSize = 0;
        item.selected = false;
        item.truncated = false;
        item.selection = SemanticCandidateSelection::NotEvaluated;
        item.exclusionReason.clear();

        if (item.content.isEmpty()) {
            item.exclusionReason = QStringLiteral("Empty candidate");
        }
        if (item.source == SemanticCandidateSource::FutureSemanticVector &&
            !policy.vectorCandidatesEnabled) {
            item.exclusionReason = QStringLiteral("Semantic/vector candidate path disabled");
        }

        normalized.append(item);
        ++sequence;
    }

    const auto sourceRank = [](SemanticCandidateSource source) {
        switch (source) {
        case SemanticCandidateSource::RecentConversation:
            return 1;
        case SemanticCandidateSource::DeterministicSummary:
            return 2;
        case SemanticCandidateSource::CommittedMemory:
            return 3;
        case SemanticCandidateSource::RuntimeMetadata:
            return 4;
        case SemanticCandidateSource::OrchestrationMetadata:
            return 5;
        case SemanticCandidateSource::FutureSemanticVector:
            return 6;
        }
        return 99;
    };

    std::stable_sort(normalized.begin(), normalized.end(),
                     [&](const auto& left, const auto& right) {
                         return sourceRank(left.source) < sourceRank(right.source);
                     });

    int remaining = result.budget.maxCharacters;
    for (auto item : normalized) {
        result.budget.estimatedCharacters += item.originalSize;
        if (!item.exclusionReason.isEmpty()) {
            item.selection = SemanticCandidateSelection::Excluded;
            result.candidates.append(item);
            continue;
        }

        if (remaining <= 0) {
            item.selection = SemanticCandidateSelection::Excluded;
            item.exclusionReason = QStringLiteral("Semantic candidate budget exhausted");
            result.candidates.append(item);
            continue;
        }

        result.budget.allocatedCharacters += std::min(item.originalSize, remaining);
        if (item.content.size() > remaining) {
            item.content = item.content.left(remaining).trimmed();
            item.truncated = true;
        }

        item.selectedSize = toInt(item.content.size());
        if (item.selectedSize <= 0) {
            item.selection = SemanticCandidateSelection::Excluded;
            item.exclusionReason = QStringLiteral("Semantic candidate budget exhausted");
            result.candidates.append(item);
            continue;
        }

        item.selected = true;
        item.selection = item.truncated ? SemanticCandidateSelection::Truncated
                                        : SemanticCandidateSelection::Selected;
        item.summary = QStringLiteral("%1 / %2 / %3 chars")
                           .arg(semanticCandidateSourceName(item.source),
                                semanticCandidateSelectionName(item.selection))
                           .arg(item.selectedSize);
        remaining -= item.selectedSize;
        result.budget.includedCharacters += item.selectedSize;
        result.selectedCandidates.append(item);
        result.candidates.append(item);
    }

    result.budget.remainingCharacters = std::max(0, remaining);
    result.budget.summary =
        QStringLiteral("%1 of %2 estimated candidate characters selected within %3 character "
                       "budget.")
            .arg(result.budget.includedCharacters)
            .arg(result.budget.estimatedCharacters)
            .arg(result.budget.maxCharacters);
    result.budgetSummary = result.budget.summary;

    int selectedCount = 0;
    int excludedCount = 0;
    int truncatedCount = 0;
    for (const auto& item : result.candidates) {
        if (item.selected) {
            ++selectedCount;
        } else {
            ++excludedCount;
        }
        if (item.truncated) {
            ++truncatedCount;
        }
    }

    result.exclusionSummary =
        excludedCount == 0
            ? QStringLiteral("No semantic candidates excluded.")
            : QStringLiteral("%1 semantic candidate %2 excluded or unavailable.")
                  .arg(excludedCount)
                  .arg(excludedCount == 1 ? QStringLiteral("was") : QStringLiteral("were"));

    if (result.candidates.isEmpty() || selectedCount == 0) {
        result.status = SemanticCandidateStatus::Empty;
        result.summary =
            QStringLiteral("Semantic candidate orchestration found no selectable local metadata.");
        return result;
    }

    result.status = (excludedCount > 0 || truncatedCount > 0) ? SemanticCandidateStatus::Truncated
                                                              : SemanticCandidateStatus::Ready;
    result.summary =
        QStringLiteral("Semantic candidate orchestration selected %1 of %2 deterministic metadata "
                       "%3; %4 excluded, %5 truncated. Semantic retrieval remains disabled.")
            .arg(selectedCount)
            .arg(result.candidates.size())
            .arg(result.candidates.size() == 1 ? QStringLiteral("candidate")
                                               : QStringLiteral("candidates"))
            .arg(excludedCount)
            .arg(truncatedCount);
    return result;
}

QStringList
semanticCandidateParticipationSummaries(const SemanticCandidateArbitration& arbitration) {
    const QList<SemanticCandidateSource> sourceOrder{
        SemanticCandidateSource::RecentConversation,
        SemanticCandidateSource::DeterministicSummary,
        SemanticCandidateSource::CommittedMemory,
        SemanticCandidateSource::RuntimeMetadata,
        SemanticCandidateSource::OrchestrationMetadata,
        SemanticCandidateSource::FutureSemanticVector,
    };

    QStringList summaries;
    for (const auto source : sourceOrder) {
        SemanticCandidateSummary summary;
        summary.source = source;
        for (const auto& candidate : arbitration.candidates) {
            if (candidate.source != source) {
                continue;
            }
            ++summary.candidateCount;
            if (candidate.selected) {
                ++summary.selectedCount;
                summary.includedCharacters += candidate.selectedSize;
            } else {
                ++summary.excludedCount;
            }
            if (candidate.truncated) {
                ++summary.truncatedCount;
            }
        }
        if (summary.candidateCount == 0) {
            continue;
        }
        summary.summary = QStringLiteral("%1 / %2 candidates / %3 selected / %4 excluded / %5 "
                                         "truncated / %6 chars")
                              .arg(semanticCandidateSourceName(source))
                              .arg(summary.candidateCount)
                              .arg(summary.selectedCount)
                              .arg(summary.excludedCount)
                              .arg(summary.truncatedCount)
                              .arg(summary.includedCharacters);
        summaries.append(summary.summary);
    }
    return summaries;
}

HybridRetrievalReadiness hybridRetrievalReadiness(const HybridRetrievalPolicy& policy,
                                                  const SemanticCandidateArbitration& arbitration) {
    HybridRetrievalReadiness readiness;
    readiness.policy = policy;
    readiness.candidateStatus = arbitration.status;
    readiness.deterministicCandidateCount = toInt(arbitration.selectedCandidates.size());
    readiness.semanticCandidateCount = 0;
    readiness.selectedCandidateCount = toInt(arbitration.selectedCandidates.size());
    readiness.status = policy.semanticPathEnabled ? HybridRetrievalStatus::ReadyMetadataOnly
                                                  : HybridRetrievalStatus::SemanticDisabled;
    if (policy.deterministicRetrievalAuthoritative && !policy.semanticPathEnabled) {
        readiness.status = HybridRetrievalStatus::DeterministicOnly;
    }
    readiness.summary =
        QStringLiteral("Hybrid retrieval readiness: deterministic retrieval authoritative, %1 "
                       "deterministic candidates participating, semantic path disabled.")
            .arg(readiness.deterministicCandidateCount);
    readiness.checks = {
        QStringLiteral("Deterministic retrieval authoritative: %1")
            .arg(policy.deterministicRetrievalAuthoritative ? QStringLiteral("yes")
                                                            : QStringLiteral("no")),
        QStringLiteral("Semantic path enabled: %1")
            .arg(policy.semanticPathEnabled ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Semantic prompt injection: %1")
            .arg(policy.semanticPromptInjectionEnabled ? QStringLiteral("enabled")
                                                       : QStringLiteral("disabled")),
        QStringLiteral("Provider/model calls: disabled"),
        QStringLiteral("Vector database activation: disabled"),
        QStringLiteral("Prompt mutation from semantic candidates: disabled"),
    };
    return readiness;
}

SemanticArbitrationResult
simulateSemanticArbitration(const SemanticCandidateArbitration& arbitration,
                            const SemanticArbitrationPolicy& policy) {
    SemanticArbitrationResult result;
    result.policy = policy;
    result.budget.deterministicSelectedCandidateCount =
        toInt(arbitration.selectedCandidates.size());
    result.budget.semanticSelectedCandidateCount = 0;
    result.budget.evaluatedCandidateCount = toInt(arbitration.selectedCandidates.size());
    result.budget.estimatedCharacters = arbitration.budget.estimatedCharacters;
    result.budget.selectedCharacters = arbitration.budget.includedCharacters;

    if (!policy.simulationEnabled) {
        result.status = SemanticArbitrationStatus::Disabled;
        result.summary = QStringLiteral("Semantic arbitration simulation is disabled.");
        return result;
    }

    if (arbitration.selectedCandidates.isEmpty()) {
        result.status = SemanticArbitrationStatus::Empty;
        result.summary =
            QStringLiteral("Semantic arbitration simulation found no deterministic candidates.");
        result.checks = {
            QStringLiteral("Deterministic retrieval authoritative: yes"),
            QStringLiteral("Real embeddings: disabled"),
            QStringLiteral("Prompt mutation: disabled"),
        };
        return result;
    }

    const auto sourceRank = [](SemanticCandidateSource source) {
        switch (source) {
        case SemanticCandidateSource::RecentConversation:
            return 10;
        case SemanticCandidateSource::DeterministicSummary:
            return 20;
        case SemanticCandidateSource::CommittedMemory:
            return 30;
        case SemanticCandidateSource::RuntimeMetadata:
            return 40;
        case SemanticCandidateSource::OrchestrationMetadata:
            return 50;
        case SemanticCandidateSource::FutureSemanticVector:
            return 60;
        }
        return 99;
    };

    QList<SemanticCandidateScore> scores;
    scores.reserve(arbitration.selectedCandidates.size());
    for (const auto& candidate : arbitration.selectedCandidates) {
        SemanticCandidateScore score;
        score.candidateId = candidate.id;
        score.source = candidate.source;
        score.sourceRank = sourceRank(candidate.source);
        score.tieBreakKey = candidate.id;
        const int lengthBucket = std::min(candidate.selectedSize, 500) / 10;
        const int sourceWeight = 700 - (score.sourceRank * 5);
        score.simulatedScore = sourceWeight + lengthBucket;
        score.summary = QStringLiteral("%1 / rank simulation / %2")
                            .arg(candidate.title.isEmpty() ? candidate.id : candidate.title,
                                 semanticCandidateSourceName(candidate.source));
        scores.append(score);
    }

    std::stable_sort(scores.begin(), scores.end(),
                     [](const SemanticCandidateScore& left, const SemanticCandidateScore& right) {
                         if (left.simulatedScore != right.simulatedScore) {
                             return left.simulatedScore > right.simulatedScore;
                         }
                         if (left.sourceRank != right.sourceRank) {
                             return left.sourceRank < right.sourceRank;
                         }
                         return left.tieBreakKey < right.tieBreakKey;
                     });

    const int limit = std::max(0, policy.maxRankedCandidates);
    for (int i = 0; i < scores.size() && i < limit; ++i) {
        const auto& score = scores.at(i);
        result.candidateScores.append(score);
        result.selectionSummaries.append(
            QStringLiteral("%1. %2 / simulated ranking metadata / tie %3")
                .arg(i + 1)
                .arg(semanticCandidateSourceName(score.source))
                .arg(score.tieBreakKey));
    }

    result.status = SemanticArbitrationStatus::Simulated;
    result.budget.rankedCandidateCount = toInt(result.candidateScores.size());
    result.budget.summary =
        QStringLiteral("%1 deterministic candidates evaluated; %2 simulated rankings retained; "
                       "%3 chars selected by deterministic retrieval; 0 semantic candidates "
                       "selected.")
            .arg(result.budget.evaluatedCandidateCount)
            .arg(result.budget.rankedCandidateCount)
            .arg(result.budget.selectedCharacters);
    result.readiness =
        QStringLiteral("Ready metadata only; simulated semantic ranking cannot change prompts.");
    result.summary =
        QStringLiteral("Semantic arbitration simulation ranked %1 deterministic candidates. "
                       "Deterministic retrieval remains final authority.")
            .arg(result.budget.rankedCandidateCount);
    result.checks = {
        QStringLiteral("Deterministic retrieval authoritative: %1")
            .arg(policy.deterministicRetrievalAuthoritative ? QStringLiteral("yes")
                                                            : QStringLiteral("no")),
        QStringLiteral("Semantic retrieval enabled: no"),
        QStringLiteral("Real embeddings: disabled"),
        QStringLiteral("Vector search: disabled"),
        QStringLiteral("Provider/model inference: disabled"),
        QStringLiteral("Prompt mutation: disabled"),
        QStringLiteral("Tie handling: simulated score, source rank, candidate id"),
    };
    return result;
}

EmbeddingRuntimePlan embeddingRuntimePlan(const SemanticArbitrationResult& arbitration) {
    EmbeddingRuntimePlan plan;
    plan.readiness = EmbeddingRuntimeReadiness::Blocked;
    plan.budget.estimatedEmbeddingJobs = arbitration.budget.evaluatedCandidateCount;
    plan.budget.estimatedIndexableItems = arbitration.budget.evaluatedCandidateCount;
    plan.budget.estimatedRuntimeMemoryMb =
        arbitration.budget.evaluatedCandidateCount == 0
            ? 0
            : std::max(64, arbitration.budget.evaluatedCandidateCount * 4);
    plan.budget.estimatedIndexStorageMb =
        arbitration.budget.evaluatedCandidateCount == 0
            ? 0
            : std::max(1, (arbitration.budget.evaluatedCandidateCount + 31) / 32);
    plan.budget.estimatedStartupSeconds = arbitration.budget.evaluatedCandidateCount == 0 ? 0 : 2;
    plan.budget.summary =
        QStringLiteral("%1 planned embedding jobs, %2 planned indexable items, ~%3 MB runtime "
                       "memory, ~%4 MB index storage.")
            .arg(plan.budget.estimatedEmbeddingJobs)
            .arg(plan.budget.estimatedIndexableItems)
            .arg(plan.budget.estimatedRuntimeMemoryMb)
            .arg(plan.budget.estimatedIndexStorageMb);
    plan.summary =
        QStringLiteral("Embedding runtime planning is blocked until a later local-only activation "
                       "phase; no embeddings, indexing, or Ollama calls run.");
    plan.requirements = {
        QStringLiteral("Explicit local embedding provider gate"),
        QStringLiteral("Committed-memory-only indexing policy"),
        QStringLiteral("App-owned local vector index boundary"),
        QStringLiteral("Deterministic fallback to retrieval planning"),
        QStringLiteral("Prompt-injection approval gate"),
    };
    plan.constraints = {
        QStringLiteral("Local-only semantic runtime"),
        QStringLiteral("No cloud/API keys"),
        QStringLiteral("No filesystem indexing while disabled"),
        QStringLiteral("No Ollama embedding calls while disabled"),
        QStringLiteral("No vector database writes while disabled"),
    };
    return plan;
}

EmbeddingGenerationReadiness
embeddingGenerationReadiness(const EmbeddingIsolationPolicy& isolationPolicy,
                             const EmbeddingGenerationPolicy& generationPolicy,
                             const EmbeddingRuntimeSession& session,
                             EmbeddingProviderStatus providerStatus) {
    const bool providerAllowed =
        (generationPolicy.providerMode == SemanticProviderMode::FakeInMemory &&
         generationPolicy.allowFakeInMemoryProvider) ||
        (generationPolicy.providerMode == SemanticProviderMode::LocalOllamaEmbeddings &&
         generationPolicy.allowLocalOllamaEmbeddingsProvider);
    const bool blockedBehavior =
        !isolationPolicy.localOnlyMode ||
        !isolationPolicy.explicitSemanticEnableReadinessSatisfied ||
        !isolationPolicy.noCloudProviders || isolationPolicy.filesystemIndexingEnabled ||
        isolationPolicy.automaticPromptIntegrationEnabled ||
        isolationPolicy.retrievalRankingMutationEnabled ||
        isolationPolicy.automaticMemoryWritesEnabled || isolationPolicy.vectorPersistenceEnabled ||
        isolationPolicy.backgroundIndexingEnabled || generationPolicy.realCloudProvidersAllowed ||
        generationPolicy.providerMode == SemanticProviderMode::Disabled ||
        generationPolicy.providerMode == SemanticProviderMode::LocalFileVectorIndex ||
        providerStatus != EmbeddingProviderStatus::Ready || !providerAllowed || session.busy ||
        generationPolicy.requestId.trimmed().isEmpty() ||
        (!session.activeRequestId.trimmed().isEmpty() &&
         session.activeRequestId.trimmed() != generationPolicy.requestId.trimmed());

    return blockedBehavior ? EmbeddingGenerationReadiness::Refused
                           : EmbeddingGenerationReadiness::Ready;
}

EmbeddingGenerationResult generateIsolatedEmbeddings(
    const IEmbeddingProvider& provider, const QList<EmbeddingDocument>& documents,
    const EmbeddingIsolationPolicy& isolationPolicy,
    const EmbeddingGenerationPolicy& generationPolicy, const EmbeddingRuntimeSession& session) {
    EmbeddingGenerationResult result;
    result.session = session;
    result.session.timeoutMs = std::max(0, generationPolicy.timeoutMs);
    result.session.activeRequestId = generationPolicy.requestId.trimmed();
    result.checks = {
        QStringLiteral("Local-only mode: %1")
            .arg(isolationPolicy.localOnlyMode ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Explicit semantic readiness gate: %1")
            .arg(isolationPolicy.explicitSemanticEnableReadinessSatisfied ? QStringLiteral("yes")
                                                                          : QStringLiteral("no")),
        QStringLiteral("Cloud/API providers: %1")
            .arg(isolationPolicy.noCloudProviders && !generationPolicy.realCloudProvidersAllowed
                     ? QStringLiteral("blocked")
                     : QStringLiteral("allowed")),
        QStringLiteral("Filesystem indexing: disabled"),
        QStringLiteral("Prompt integration: disabled"),
        QStringLiteral("Retrieval ranking mutation: disabled"),
        QStringLiteral("Automatic memory writes: disabled"),
        QStringLiteral("Vector DB persistence: disabled"),
        QStringLiteral("Background indexing jobs: disabled"),
    };

    const auto readiness =
        embeddingGenerationReadiness(isolationPolicy, generationPolicy, session, provider.status());
    result.readiness = readiness;

    if (session.busy) {
        result.status = EmbeddingRuntimeStatus::Busy;
        result.health = EmbeddingRuntimeHealth::Blocked;
        result.failureReason = QStringLiteral("Embedding runtime session is already busy.");
        result.summary = result.failureReason;
        return result;
    }

    if (!session.activeRequestId.trimmed().isEmpty() &&
        session.activeRequestId.trimmed() != generationPolicy.requestId.trimmed()) {
        result.status = EmbeddingRuntimeStatus::Stale;
        result.health = EmbeddingRuntimeHealth::Blocked;
        result.failureReason =
            QStringLiteral("Embedding request is stale and cannot update runtime state.");
        result.summary = result.failureReason;
        return result;
    }

    if (readiness != EmbeddingGenerationReadiness::Ready) {
        result.status = EmbeddingRuntimeStatus::Refused;
        result.health = EmbeddingRuntimeHealth::Blocked;
        result.failureReason =
            QStringLiteral("Isolated embedding generation refused by local-only policy gates.");
        result.summary = result.failureReason;
        return result;
    }

    const int timeoutMs = std::max(0, generationPolicy.timeoutMs);
    const int elapsedMs = std::max(0, generationPolicy.simulatedExecutionMs);
    result.elapsedMs = elapsedMs;
    if (elapsedMs > timeoutMs) {
        result.status = EmbeddingRuntimeStatus::TimedOut;
        result.health = EmbeddingRuntimeHealth::Failed;
        result.failureReason =
            QStringLiteral("Isolated embedding generation timed out after %1 ms.").arg(timeoutMs);
        result.summary = result.failureReason;
        return result;
    }

    QList<EmbeddingDocument> boundedDocuments;
    const int maxDocuments = std::max(0, generationPolicy.maxDocuments);
    const int maxCharacters = std::max(0, generationPolicy.maxDocumentCharacters);
    for (const auto& document : documents) {
        if (boundedDocuments.size() >= maxDocuments) {
            break;
        }
        EmbeddingDocument bounded = document;
        bounded.id = bounded.id.trimmed();
        bounded.text = bounded.text.trimmed();
        if (bounded.id.isEmpty() || bounded.text.isEmpty()) {
            continue;
        }
        if (bounded.text.size() > maxCharacters) {
            bounded.text = bounded.text.left(maxCharacters).trimmed();
        }
        result.session.boundedCharacterCount += toInt(bounded.text.size());
        boundedDocuments.append(bounded);
    }
    result.session.boundedDocumentCount = toInt(boundedDocuments.size());

    const auto embeddingResult =
        provider.embed(EmbeddingRequest{boundedDocuments, provider.policy()});
    result.generatedDocumentCount = embeddingResult.documentCount;
    result.generatedVectorCount = toInt(embeddingResult.vectors.size());
    if (embeddingResult.status != EmbeddingProviderStatus::Ready ||
        result.generatedDocumentCount != boundedDocuments.size()) {
        result.status = EmbeddingRuntimeStatus::Failed;
        result.health = EmbeddingRuntimeHealth::Failed;
        result.failureReason = QStringLiteral("Embedding provider failed isolated generation.");
        result.summary = result.failureReason;
        return result;
    }

    result.status = EmbeddingRuntimeStatus::Succeeded;
    result.health = EmbeddingRuntimeHealth::LocalOnlyReady;
    result.summary =
        QStringLiteral("Generated %1 isolated local %2 for readiness validation only; retrieval, "
                       "prompts, memory, and vector persistence were unchanged.")
            .arg(result.generatedVectorCount)
            .arg(result.generatedVectorCount == 1 ? QStringLiteral("embedding")
                                                  : QStringLiteral("embeddings"));
    return result;
}

QList<SemanticProviderDescriptor>
plannedSemanticProviderDescriptors(const SemanticProviderPolicy& policy) {
    auto makeDescriptor = [](SemanticProviderMode mode, QString name, QString summary,
                             SemanticProviderReadiness readiness,
                             QList<SemanticProviderCapability> capabilities,
                             QStringList requiredSteps) -> SemanticProviderDescriptor {
        SemanticProviderDescriptor descriptor;
        descriptor.mode = mode;
        descriptor.name = std::move(name);
        descriptor.summary = std::move(summary);
        descriptor.readiness = readiness;
        descriptor.health = readiness == SemanticProviderReadiness::ReadyMetadataOnly
                                ? SemanticProviderHealth::MetadataOnly
                                : SemanticProviderHealth::NotChecked;
        descriptor.capabilities = std::move(capabilities);
        for (const auto capability : descriptor.capabilities) {
            descriptor.capabilitySummaries.append(semanticProviderCapabilityName(capability));
        }
        descriptor.requiredActivationSteps = std::move(requiredSteps);
        return descriptor;
    };

    QList<SemanticProviderDescriptor> descriptors;
    descriptors.append(makeDescriptor(
        SemanticProviderMode::Disabled, QStringLiteral("Disabled"),
        QStringLiteral("Semantic retrieval remains disabled and deterministic retrieval is "
                       "authoritative."),
        SemanticProviderReadiness::Disabled,
        {SemanticProviderCapability::PromptMutationBlocked,
         SemanticProviderCapability::VectorWritesBlocked},
        {QStringLiteral("Choose an explicit local semantic provider in a later phase."),
         QStringLiteral("Keep deterministic retrieval as fallback and prompt authority.")}));
    descriptors.append(makeDescriptor(
        SemanticProviderMode::FakeInMemory, QStringLiteral("Fake/InMemory test provider"),
        QStringLiteral("Deterministic fake embeddings and in-memory vector index for tests only."),
        policy.allowFakeInMemoryProvider ? SemanticProviderReadiness::ReadyMetadataOnly
                                         : SemanticProviderReadiness::Planned,
        {SemanticProviderCapability::LocalOnly, SemanticProviderCapability::FakeInMemory,
         SemanticProviderCapability::EmbeddingGeneration,
         SemanticProviderCapability::VectorIndexing, SemanticProviderCapability::MetadataOnlyHealth,
         SemanticProviderCapability::PromptMutationBlocked},
        {QStringLiteral("Limit use to isolated tests."),
         QStringLiteral("Do not feed fake semantic results into prompt assembly.")}));
    descriptors.append(makeDescriptor(
        SemanticProviderMode::LocalOllamaEmbeddings,
        QStringLiteral("Local Ollama embeddings provider"),
        QStringLiteral("Planned local Ollama embeddings provider; no embedding requests are made."),
        policy.allowLocalOllamaEmbeddingsProvider ? SemanticProviderReadiness::ReadyMetadataOnly
                                                  : SemanticProviderReadiness::Planned,
        {SemanticProviderCapability::LocalOnly, SemanticProviderCapability::EmbeddingGeneration,
         SemanticProviderCapability::MetadataOnlyHealth,
         SemanticProviderCapability::PromptMutationBlocked,
         SemanticProviderCapability::VectorWritesBlocked},
        {QStringLiteral("Add an explicit local embedding model selection gate."),
         QStringLiteral("Add loopback-only embedding request tests."),
         QStringLiteral("Keep semantic prompt injection disabled until separately approved.")}));
    descriptors.append(makeDescriptor(
        SemanticProviderMode::LocalFileVectorIndex, QStringLiteral("Local file/vector index"),
        QStringLiteral("Planned local vector index boundary; no vector database writes are made."),
        policy.allowLocalFileVectorIndex ? SemanticProviderReadiness::ReadyMetadataOnly
                                         : SemanticProviderReadiness::Planned,
        {SemanticProviderCapability::LocalOnly, SemanticProviderCapability::VectorIndexing,
         SemanticProviderCapability::MetadataOnlyHealth,
         SemanticProviderCapability::PromptMutationBlocked,
         SemanticProviderCapability::VectorWritesBlocked},
        {QStringLiteral("Define an app-owned semantic index path in a later phase."),
         QStringLiteral("Add explicit indexing and refresh policy for committed memory only."),
         QStringLiteral("Prove vector writes never occur while disabled.")}));
    return descriptors;
}

SemanticProviderSelection selectSemanticProvider(SemanticProviderMode mode,
                                                 const SemanticProviderPolicy& policy) {
    SemanticProviderSelection selection;
    const auto descriptors = plannedSemanticProviderDescriptors(policy);
    for (const auto& descriptor : descriptors) {
        if (descriptor.mode == mode) {
            selection.descriptor = descriptor;
            break;
        }
    }
    if (selection.descriptor.name.isEmpty()) {
        selection.descriptor = descriptors.first();
    }

    selection.mode = selection.descriptor.mode;
    selection.readiness = selection.descriptor.readiness;
    selection.health = selection.descriptor.health;
    selection.selected = true;
    selection.active = false;
    selection.requiredActivationSteps = selection.descriptor.requiredActivationSteps;
    selection.capabilitySummaries = selection.descriptor.capabilitySummaries;

    const bool fakeAllowed =
        selection.mode == SemanticProviderMode::FakeInMemory && policy.allowFakeInMemoryProvider;
    const bool ollamaAllowed = selection.mode == SemanticProviderMode::LocalOllamaEmbeddings &&
                               policy.allowLocalOllamaEmbeddingsProvider;
    const bool fileIndexAllowed = selection.mode == SemanticProviderMode::LocalFileVectorIndex &&
                                  policy.allowLocalFileVectorIndex;
    if (selection.mode == SemanticProviderMode::Disabled || policy.disabledByDefault) {
        selection.disabledReason = QStringLiteral("Semantic retrieval is disabled by default.");
    } else if (!(fakeAllowed || ollamaAllowed || fileIndexAllowed)) {
        selection.disabledReason =
            QStringLiteral("Selected semantic provider is planned but not activation-approved.");
    } else if (!policy.allowRealEmbeddingCalls &&
               selection.mode != SemanticProviderMode::FakeInMemory) {
        selection.disabledReason =
            QStringLiteral("Real embedding calls are disabled by semantic provider policy.");
    } else if (!policy.allowVectorIndexWrites &&
               selection.mode == SemanticProviderMode::LocalFileVectorIndex) {
        selection.disabledReason =
            QStringLiteral("Vector index writes are disabled by semantic provider policy.");
    } else {
        selection.disabledReason =
            QStringLiteral("Provider readiness is metadata-only; semantic ranking remains off.");
    }

    selection.summary =
        QStringLiteral("Selected semantic provider: %1 / %2 / %3.")
            .arg(selection.descriptor.name, semanticProviderReadinessName(selection.readiness),
                 selection.disabledReason);
    return selection;
}

SemanticActivationReadiness semanticActivationReadiness(const SemanticProviderSelection& selection,
                                                        const SemanticProviderPolicy& policy) {
    SemanticActivationReadiness readiness;
    readiness.providerMode = selection.mode;
    readiness.requiredSteps = selection.requiredActivationSteps;
    readiness.checks = {
        QStringLiteral("Disabled by default: %1")
            .arg(policy.disabledByDefault ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Selected provider: %1").arg(semanticProviderModeName(selection.mode)),
        QStringLiteral("Provider readiness: %1")
            .arg(semanticProviderReadinessName(selection.readiness)),
        QStringLiteral("Provider health: %1").arg(semanticProviderHealthName(selection.health)),
        QStringLiteral("Real embedding calls allowed: %1")
            .arg(policy.allowRealEmbeddingCalls ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Vector index writes allowed: %1")
            .arg(policy.allowVectorIndexWrites ? QStringLiteral("yes") : QStringLiteral("no")),
        QStringLiteral("Semantic prompt injection allowed: %1")
            .arg(policy.allowSemanticPromptInjection ? QStringLiteral("yes")
                                                     : QStringLiteral("no")),
        QStringLiteral("Deterministic retrieval authoritative: yes"),
    };

    readiness.ready = !policy.disabledByDefault &&
                      selection.readiness == SemanticProviderReadiness::ReadyMetadataOnly &&
                      selection.mode == SemanticProviderMode::FakeInMemory &&
                      policy.allowFakeInMemoryProvider && !policy.allowSemanticPromptInjection;
    readiness.status =
        readiness.ready ? QStringLiteral("Metadata Ready") : QStringLiteral("Refused");
    readiness.summary =
        readiness.ready
            ? QStringLiteral("Semantic provider metadata is ready for isolated fake tests only; "
                             "semantic ranking and prompt injection remain disabled.")
            : QStringLiteral("Semantic activation refused: %1").arg(selection.disabledReason);
    return readiness;
}

SemanticActivationResult semanticActivationResult(const SemanticProviderSelection& selection,
                                                  const SemanticProviderPolicy& policy) {
    SemanticActivationResult result;
    result.readiness = semanticActivationReadiness(selection, policy);
    result.accepted = false;
    result.summary =
        QStringLiteral("%1 No semantic ranking, embedding calls, vector writes, or prompt mutation "
                       "were started.")
            .arg(result.readiness.summary);
    return result;
}

} // namespace sentinel::core
