#include "sentinel/core/SemanticRetrieval.h"

#include <QtTest>

using sentinel::core::EmbeddingDocument;
using sentinel::core::EmbeddingRequest;
using sentinel::core::FakeEmbeddingProvider;
using sentinel::core::FakeVectorIndex;
using sentinel::core::VectorSearchQuery;

class SemanticRetrievalTest final : public QObject {
    Q_OBJECT

private slots:
    void fakeEmbeddingGenerationIsDeterministic();
    void fakeEmbeddingUsesStableVectorsForSameInput();
    void fakeVectorIndexInsertSearchRemoveIsDeterministic();
    void fakeVectorIndexUsesStableScoringOrder();
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

QTEST_MAIN(SemanticRetrievalTest)

#include "test_semantic_retrieval.moc"
