#include "sentinel/core/SemanticRetrieval.h"

#include <QCryptographicHash>

#include <algorithm>
#include <cmath>

namespace sentinel::core {
namespace {

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
        const auto token = tokens.at(index);
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

    result.documentCount = result.documents.size();
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
    return entries_.size();
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
    result.indexedItemCount = entries_.size();

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

    result.resultCount = result.candidates.size();
    result.summary =
        QStringLiteral("Vector search returned %1 of %2 indexed %3.")
            .arg(result.resultCount)
            .arg(result.indexedItemCount)
            .arg(result.indexedItemCount == 1 ? QStringLiteral("item") : QStringLiteral("items"));
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

} // namespace sentinel::core
