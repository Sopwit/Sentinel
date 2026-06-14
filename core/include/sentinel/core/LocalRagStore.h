#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace sentinel::core {

struct RagDocumentRecord {
    QString id;
    QString workspaceId;
    QString fileName;
    QString fileType;
    qint64 sizeBytes = 0;
    QString status = QStringLiteral("Attached");
    QString sourceSummary;
};

struct RagRetrievalRecord {
    QString id;
    QString workspaceId;
    QString documentId;
    QString chunkReference;
    double relevance = 0.0;
    QString summary;
};

struct RagStoreResult {
    bool success = false;
    QString status = QStringLiteral("Failed");
    QString summary;
};

class LocalRagStore final {
public:
    explicit LocalRagStore(QString databasePath);

    bool isAvailable() const;
    QString statusSummary() const;
    RagStoreResult addDocument(const RagDocumentRecord& record);
    RagStoreResult removeDocument(const QString& workspaceId, const QString& documentId);
    RagStoreResult markReindexed(const QString& workspaceId);
    RagStoreResult clearWorkspace(const QString& workspaceId);
    QList<RagDocumentRecord> documents(const QString& workspaceId) const;
    QList<RagRetrievalRecord> recentRetrievals(const QString& workspaceId) const;
    RagStoreResult recordRetrieval(const RagRetrievalRecord& record);

private:
    bool ensureSchema() const;
    QString connectionName() const;
    QString databasePath_;
};

QString ragDocumentSummary(const RagDocumentRecord& record);
QString ragRetrievalSummary(const RagRetrievalRecord& record);

} // namespace sentinel::core
