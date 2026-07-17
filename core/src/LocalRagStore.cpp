#include "sentinel/core/LocalRagStore.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace sentinel::core {

namespace {

QString stableId(const QString& seed) {
    return QString::fromLatin1(
        QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha1).toHex().left(16));
}

QString nowUtc() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

QSqlDatabase openDatabase(const QString& connectionName, const QString& databasePath) {
    if (QSqlDatabase::contains(connectionName)) {
        auto db = QSqlDatabase::database(connectionName);
        if (!db.isOpen()) {
            db.open();
        }
        return db;
    }

    const QFileInfo info(databasePath);
    if (!info.dir().exists()) {
        QDir().mkpath(info.dir().absolutePath());
    }

    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    db.setDatabaseName(databasePath);
    db.open();
    return db;
}

} // namespace

LocalRagStore::LocalRagStore(QString databasePath) : databasePath_(std::move(databasePath)) {
    ensureSchema();
}

bool LocalRagStore::isAvailable() const {
    return openDatabase(connectionName(), databasePath_).isOpen() && ensureSchema();
}

QString LocalRagStore::statusSummary() const {
    return isAvailable() ? QStringLiteral("Local RAG SQLite store available; workspace scoped.")
                         : QStringLiteral("Local RAG SQLite store unavailable.");
}

RagStoreResult LocalRagStore::addDocument(const RagDocumentRecord& record) {
    if (record.workspaceId.trimmed().isEmpty() || record.fileName.trimmed().isEmpty() ||
        !ensureSchema()) {
        return {false, QStringLiteral("Refused"),
                QStringLiteral("Document requires workspace id and file name.")};
    }

    auto db = openDatabase(connectionName(), databasePath_);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO rag_documents "
        "(id, workspace_id, file_name, file_type, size_bytes, status, source_summary, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
    const auto id =
        record.id.trimmed().isEmpty()
            ? stableId(record.workspaceId + record.fileName + QString::number(record.sizeBytes))
            : record.id.trimmed();
    query.addBindValue(id);
    query.addBindValue(record.workspaceId.trimmed());
    query.addBindValue(record.fileName.trimmed());
    query.addBindValue(record.fileType.trimmed());
    query.addBindValue(record.sizeBytes);
    query.addBindValue(record.status.trimmed().isEmpty() ? QStringLiteral("Attached")
                                                         : record.status.trimmed());
    query.addBindValue(record.sourceSummary.trimmed());
    query.addBindValue(nowUtc());
    if (!query.exec()) {
        return {false, QStringLiteral("Failed"), query.lastError().text()};
    }
    return {
        true, QStringLiteral("Added"),
        QStringLiteral("Added document %1 to workspace knowledge metadata.").arg(record.fileName)};
}

RagStoreResult LocalRagStore::removeDocument(const QString& workspaceId,
                                             const QString& documentId) {
    if (!ensureSchema()) {
        return {false, QStringLiteral("Failed"), QStringLiteral("RAG schema unavailable.")};
    }
    auto db = openDatabase(connectionName(), databasePath_);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM rag_documents WHERE workspace_id = ? AND id = ?"));
    query.addBindValue(workspaceId.trimmed());
    query.addBindValue(documentId.trimmed());
    if (!query.exec()) {
        return {false, QStringLiteral("Failed"), query.lastError().text()};
    }
    return {true, QStringLiteral("Removed"),
            QStringLiteral("Removed document from workspace RAG metadata.")};
}

RagStoreResult LocalRagStore::markReindexed(const QString& workspaceId) {
    if (!ensureSchema()) {
        return {false, QStringLiteral("Failed"), QStringLiteral("RAG schema unavailable.")};
    }
    auto db = openDatabase(connectionName(), databasePath_);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE rag_documents SET status = 'Indexed Manually', updated_at "
                                 "= ? WHERE workspace_id = ?"));
    query.addBindValue(nowUtc());
    query.addBindValue(workspaceId.trimmed());
    if (!query.exec()) {
        return {false, QStringLiteral("Failed"), query.lastError().text()};
    }
    return {true, QStringLiteral("Indexing Completed"),
            QStringLiteral("Manual re-index completed for workspace knowledge metadata.")};
}

RagStoreResult LocalRagStore::clearWorkspace(const QString& workspaceId) {
    if (!ensureSchema()) {
        return {false, QStringLiteral("Failed"), QStringLiteral("RAG schema unavailable.")};
    }
    auto db = openDatabase(connectionName(), databasePath_);
    QSqlQuery docs(db);
    docs.prepare(QStringLiteral("DELETE FROM rag_documents WHERE workspace_id = ?"));
    docs.addBindValue(workspaceId.trimmed());
    QSqlQuery retrievals(db);
    retrievals.prepare(QStringLiteral("DELETE FROM rag_retrievals WHERE workspace_id = ?"));
    retrievals.addBindValue(workspaceId.trimmed());
    if (!docs.exec() || !retrievals.exec()) {
        return {false, QStringLiteral("Failed"),
                docs.lastError().text() + retrievals.lastError().text()};
    }
    return {true, QStringLiteral("Cleared"),
            QStringLiteral("Cleared workspace knowledge base metadata.")};
}

QList<RagDocumentRecord> LocalRagStore::documents(const QString& workspaceId) const {
    QList<RagDocumentRecord> records;
    if (!ensureSchema()) {
        return records;
    }
    auto db = openDatabase(connectionName(), databasePath_);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, workspace_id, file_name, file_type, size_bytes, status, source_summary "
        "FROM rag_documents WHERE workspace_id = ? ORDER BY updated_at DESC, file_name ASC"));
    query.addBindValue(workspaceId.trimmed());
    if (!query.exec()) {
        return records;
    }
    while (query.next()) {
        records.append({query.value(0).toString(), query.value(1).toString(),
                        query.value(2).toString(), query.value(3).toString(),
                        query.value(4).toLongLong(), query.value(5).toString(),
                        query.value(6).toString()});
    }
    return records;
}

QList<RagRetrievalRecord> LocalRagStore::recentRetrievals(const QString& workspaceId) const {
    QList<RagRetrievalRecord> records;
    if (!ensureSchema()) {
        return records;
    }
    auto db = openDatabase(connectionName(), databasePath_);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, workspace_id, document_id, chunk_reference, relevance, summary "
        "FROM rag_retrievals WHERE workspace_id = ? ORDER BY created_at DESC LIMIT 12"));
    query.addBindValue(workspaceId.trimmed());
    if (!query.exec()) {
        return records;
    }
    while (query.next()) {
        records.append({query.value(0).toString(), query.value(1).toString(),
                        query.value(2).toString(), query.value(3).toString(),
                        query.value(4).toDouble(), query.value(5).toString()});
    }
    return records;
}

RagStoreResult LocalRagStore::recordRetrieval(const RagRetrievalRecord& record) {
    if (!ensureSchema()) {
        return {false, QStringLiteral("Failed"), QStringLiteral("RAG schema unavailable.")};
    }
    auto db = openDatabase(connectionName(), databasePath_);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO rag_retrievals "
        "(id, workspace_id, document_id, chunk_reference, relevance, summary, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(record.id.trimmed().isEmpty()
                           ? stableId(record.workspaceId + record.documentId + nowUtc())
                           : record.id.trimmed());
    query.addBindValue(record.workspaceId.trimmed());
    query.addBindValue(record.documentId.trimmed());
    query.addBindValue(record.chunkReference.trimmed());
    query.addBindValue(record.relevance);
    query.addBindValue(record.summary.trimmed());
    query.addBindValue(nowUtc());
    if (!query.exec()) {
        return {false, QStringLiteral("Failed"), query.lastError().text()};
    }
    return {true, QStringLiteral("Retrieval Completed"),
            QStringLiteral("Recorded explainable retrieval metadata.")};
}

bool LocalRagStore::ensureSchema() const {
    auto db = openDatabase(connectionName(), databasePath_);
    if (!db.isOpen()) {
        return false;
    }
    QSqlQuery query(db);
    const auto docs = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS rag_documents ("
        "id TEXT NOT NULL, workspace_id TEXT NOT NULL, file_name TEXT NOT NULL, "
        "file_type TEXT NOT NULL, size_bytes INTEGER NOT NULL, status TEXT NOT NULL, "
        "source_summary TEXT NOT NULL, updated_at TEXT NOT NULL, "
        "PRIMARY KEY (workspace_id, id))"));
    const auto retrievals = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS rag_retrievals ("
        "id TEXT PRIMARY KEY, workspace_id TEXT NOT NULL, document_id TEXT NOT NULL, "
        "chunk_reference TEXT NOT NULL, relevance REAL NOT NULL, summary TEXT NOT NULL, "
        "created_at TEXT NOT NULL)"));
    return docs && retrievals;
}

QString LocalRagStore::connectionName() const {
    return QStringLiteral("sentinel-rag-%1").arg(stableId(databasePath_));
}

QString ragDocumentSummary(const RagDocumentRecord& record) {
    return QStringLiteral("%1 / %2 / %3 bytes / %4")
        .arg(record.fileName, record.fileType, QString::number(record.sizeBytes), record.status);
}

QString ragRetrievalSummary(const RagRetrievalRecord& record) {
    return QStringLiteral("%1 / %2 / relevance %3 / %4")
        .arg(record.documentId, record.chunkReference, QString::number(record.relevance, 'f', 2),
             record.summary);
}

} // namespace sentinel::core
