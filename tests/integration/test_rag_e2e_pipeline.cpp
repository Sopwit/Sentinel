#include <QDir>
#include <QTemporaryDir>
#include <QtTest>
#include "sentinel/core/memory/LocalRagStore.h"

using sentinel::core::LocalRagStore;
using sentinel::core::RagDocumentRecord;

class RagE2ePipelineTest final : public QObject {
    Q_OBJECT

private slots:
    void testRagPipelineExecution();
};

void RagE2ePipelineTest::testRagPipelineExecution() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString dbPath = QDir(tempDir.path()).filePath(QStringLiteral("test_rag.sqlite3"));
    LocalRagStore ragStore(dbPath);
    QVERIFY(ragStore.isAvailable());

    RagDocumentRecord record;
    record.id = QStringLiteral("doc-1");
    record.workspaceId = QStringLiteral("ws-test");
    record.fileName = QStringLiteral("test.txt");
    record.fileType = QStringLiteral("text/plain");
    record.sizeBytes = 1024;
    record.sourceSummary = QStringLiteral("Test source summary");

    const auto addResult = ragStore.addDocument(record);
    QVERIFY2(addResult.success, qPrintable(addResult.summary));

    const auto docs = ragStore.documents(QStringLiteral("ws-test"));
    QCOMPARE(docs.size(), 1);
    QCOMPARE(docs.first().fileName, QStringLiteral("test.txt"));
}

QTEST_MAIN(RagE2ePipelineTest)

#include "test_rag_e2e_pipeline.moc"
