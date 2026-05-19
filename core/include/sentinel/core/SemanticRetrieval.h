#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

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

QString embeddingProviderStatusName(EmbeddingProviderStatus status);
QString vectorIndexStatusName(VectorIndexStatus status);
QString semanticRetrievalStatusName(SemanticRetrievalStatus status);

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

} // namespace sentinel::core
