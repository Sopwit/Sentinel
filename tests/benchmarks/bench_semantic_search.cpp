// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/memory/SemanticRetrieval.h"
#include <QtTest>

using namespace sentinel::core;

namespace {
constexpr int kVectorDocumentCount = 500;
constexpr int kPersistenceItemCount = 500;

QString syntheticText(int index) {
    static const QStringList vocabulary = {
        QStringLiteral("alpha"),      QStringLiteral("beta"),       QStringLiteral("gamma"),
        QStringLiteral("delta"),      QStringLiteral("memory"),     QStringLiteral("retrieval"),
        QStringLiteral("semantic"),   QStringLiteral("assistant"),  QStringLiteral("conversation"),
        QStringLiteral("context"),    QStringLiteral("preference"), QStringLiteral("summary"),
        QStringLiteral("transcript"), QStringLiteral("metadata"),   QStringLiteral("runtime"),
        QStringLiteral("pipeline"),
    };
    QStringList tokens;
    tokens.append(vocabulary.at(index % vocabulary.size()));
    tokens.append(vocabulary.at((index / 3) % vocabulary.size()));
    tokens.append(vocabulary.at((index / 7) % vocabulary.size()));
    return tokens.join(QLatin1Char(' '));
}

FakeVectorIndex makeVectorIndex() {
    FakeVectorIndex index;
    const FakeEmbeddingProvider provider;
    QList<EmbeddingDocument> documents;
    documents.reserve(kVectorDocumentCount);
    for (int i = 0; i < kVectorDocumentCount; ++i) {
        documents.append(EmbeddingDocument{QStringLiteral("doc-%1").arg(i), syntheticText(i),
                                           QStringLiteral("bench"), QStringLiteral("synthetic")});
    }
    const auto generated = provider.embed(EmbeddingRequest{documents, {}});
    for (int i = 0; i < generated.documents.size(); ++i) {
        index.upsert(generated.documents.at(i), generated.vectors.at(i));
    }
    Q_ASSERT(index.itemCount() == kVectorDocumentCount);
    return index;
}

LocalVectorPersistenceIndex makePersistenceIndex() {
    VectorPersistencePolicy policy;
    policy.enabled = true;
    policy.disabledByDefault = false;
    VectorPersistenceBudget budget;
    budget.maxIndexedItems = kPersistenceItemCount;
    LocalVectorPersistenceIndex index{policy, budget};
    index.create(VectorPersistenceSession{});

    QStringList items;
    items.reserve(kPersistenceItemCount);
    for (int i = 0; i < kPersistenceItemCount; ++i) {
        items.append(syntheticText(i));
    }
    EmbeddingGenerationResult generated;
    generated.status = EmbeddingRuntimeStatus::Succeeded;
    generated.readiness = EmbeddingGenerationReadiness::Ready;
    generated.generatedVectorCount = kPersistenceItemCount;
    index.acceptIsolatedEmbeddingResult(generated, items, VectorPersistenceSession{});
    Q_ASSERT(index.itemCount() == kPersistenceItemCount);
    return index;
}
} // namespace

class SemanticSearchBenchmark final : public QObject {
    Q_OBJECT

private slots:
    void benchmarkVectorIndexSimilaritySearch();
    void benchmarkTokenOverlapSemanticSearch();
};

void SemanticSearchBenchmark::benchmarkVectorIndexSimilaritySearch() {
    const auto index = makeVectorIndex();
    const FakeEmbeddingProvider provider;
    const auto query = provider.embed(EmbeddingRequest{
        {EmbeddingDocument{QStringLiteral("query"), syntheticText(1), {}, {}}}, {}});
    QVERIFY(!query.vectors.isEmpty());

    const VectorSearchQuery searchQuery{query.vectors.first(), 10, 0.0};
    QBENCHMARK {
        const auto result = index.search(searchQuery);
        QVERIFY(result.candidates.size() <= 10);
    }
}

void SemanticSearchBenchmark::benchmarkTokenOverlapSemanticSearch() {
    const auto index = makePersistenceIndex();
    const QString query = QStringLiteral("alpha beta memory");

    SemanticSearchPolicy policy;
    policy.maxCandidates = 8;
    QBENCHMARK {
        const auto result =
            index.searchLocalSemanticCandidates(query, {}, policy, SemanticSearchSession{});
        QVERIFY(result.candidates.size() <= 8);
    }
}

QTEST_MAIN(SemanticSearchBenchmark)

#include "bench_semantic_search.moc"
