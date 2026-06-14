#include "sentinel/core/LocalRagStore.h"

#include <QTemporaryDir>
#include <QtTest>

using sentinel::core::LocalRagStore;
using sentinel::core::RagDocumentRecord;
using sentinel::core::RagRetrievalRecord;

class LocalRagStoreTest final : public QObject {
    Q_OBJECT

private slots:
    void persistsDocumentsPerWorkspace();
    void recordsRetrievalExplainabilityMetadata();
    void clearsWorkspaceKnowledgeBase();
};

void LocalRagStoreTest::persistsDocumentsPerWorkspace() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = dir.filePath(QStringLiteral("rag.sqlite3"));

    {
        LocalRagStore store(path);
        QVERIFY(store.isAvailable());
        const auto result = store.addDocument({QStringLiteral("doc-1"), QStringLiteral("personal"),
                                               QStringLiteral("notes.md"), QStringLiteral("Markdown"),
                                               42, QStringLiteral("Added / Not Indexed"),
                                               QStringLiteral("explicit test document")});
        QVERIFY(result.success);
    }

    LocalRagStore restored(path);
    const auto personalDocs = restored.documents(QStringLiteral("personal"));
    QCOMPARE(personalDocs.size(), 1);
    QCOMPARE(personalDocs.first().fileName, QStringLiteral("notes.md"));
    QVERIFY(restored.documents(QStringLiteral("research")).isEmpty());
}

void LocalRagStoreTest::recordsRetrievalExplainabilityMetadata() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    LocalRagStore store(dir.filePath(QStringLiteral("rag.sqlite3")));

    QVERIFY(store.addDocument({QStringLiteral("doc-1"), QStringLiteral("personal"),
                               QStringLiteral("paper.pdf"), QStringLiteral("PDF"), 100,
                               QStringLiteral("Indexed Manually"), QStringLiteral("paper")})
                .success);
    QVERIFY(store
                .recordRetrieval({QStringLiteral("ret-1"), QStringLiteral("personal"),
                                  QStringLiteral("doc-1"), QStringLiteral("chunk 2"), 0.87,
                                  QStringLiteral("matched local query terms")})
                .success);

    const auto retrievals = store.recentRetrievals(QStringLiteral("personal"));
    QCOMPARE(retrievals.size(), 1);
    QCOMPARE(retrievals.first().documentId, QStringLiteral("doc-1"));
    QCOMPARE(retrievals.first().chunkReference, QStringLiteral("chunk 2"));
    QVERIFY(retrievals.first().relevance > 0.8);
}

void LocalRagStoreTest::clearsWorkspaceKnowledgeBase() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    LocalRagStore store(dir.filePath(QStringLiteral("rag.sqlite3")));

    QVERIFY(store.addDocument({QStringLiteral("doc-1"), QStringLiteral("personal"),
                               QStringLiteral("notes.txt"), QStringLiteral("TXT"), 20,
                               QStringLiteral("Added"), QStringLiteral("notes")})
                .success);
    QVERIFY(store.clearWorkspace(QStringLiteral("personal")).success);
    QVERIFY(store.documents(QStringLiteral("personal")).isEmpty());
}

QTEST_MAIN(LocalRagStoreTest)

#include "test_local_rag_store.moc"
