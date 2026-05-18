#include "sentinel/core/SQLiteConversationStore.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

namespace sentinel::core {

namespace {

ChatRole roleFromName(const QString& role) {
    if (role == QStringLiteral("user")) {
        return ChatRole::User;
    }
    if (role == QStringLiteral("assistant")) {
        return ChatRole::Assistant;
    }
    return ChatRole::System;
}

ChatMessageStatus statusFromName(const QString& status) {
    if (status == QStringLiteral("sent")) {
        return ChatMessageStatus::Sent;
    }
    if (status == QStringLiteral("error")) {
        return ChatMessageStatus::Error;
    }
    return ChatMessageStatus::Received;
}

QString boolText(bool value) {
    return value ? QStringLiteral("1") : QStringLiteral("0");
}

} // namespace

SQLiteConversationStore::SQLiteConversationStore(QString databasePath)
    : databasePath_(std::move(databasePath)),
      connectionName_(QStringLiteral("sentinel_conversations_%1")
                          .arg(QUuid::createUuid().toString(QUuid::Id128))) {
    open();
    initializeSchema();
}

SQLiteConversationStore::~SQLiteConversationStore() {
    const auto connectionName = connectionName_;
    database_ = {};
    QSqlDatabase::removeDatabase(connectionName);
}

QString SQLiteConversationStore::normalizedTitle(const QString& title) {
    const auto trimmed = title.trimmed();
    return trimmed.isEmpty() ? QStringLiteral("Untitled Conversation") : trimmed;
}

ConversationRecord SQLiteConversationStore::createConversation(const QString& title) {
    ConversationRecord record;
    if (!database_.isOpen()) {
        setLastError(ConversationStoreErrorCode::Unavailable,
                     QStringLiteral("SQLite conversation database is not open."));
        return record;
    }

    record.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    record.title = normalizedTitle(title);
    record.createdAtUtc = QDateTime::currentDateTimeUtc();
    record.updatedAtUtc = record.createdAtUtc;
    record.summary = conversationRecordSummary(record);

    QSqlQuery query(database_);
    query.prepare(QStringLiteral("INSERT INTO conversations("
                                 "id, title, created_at, updated_at, archived, deleted) "
                                 "VALUES(?, ?, ?, ?, 0, 0)"));
    query.addBindValue(record.id);
    query.addBindValue(record.title);
    query.addBindValue(record.createdAtUtc.toString(Qt::ISODateWithMs));
    query.addBindValue(record.updatedAtUtc.toString(Qt::ISODateWithMs));
    if (!query.exec()) {
        setLastError(ConversationStoreErrorCode::StorageFailure, query.lastError().text());
        return {};
    }

    setLastError(ConversationStoreErrorCode::None, {});
    return record;
}

QList<ConversationRecord> SQLiteConversationStore::listConversations() const {
    QList<ConversationRecord> records;
    if (!database_.isOpen()) {
        setLastError(ConversationStoreErrorCode::Unavailable,
                     QStringLiteral("SQLite conversation database is not open."));
        return records;
    }

    QSqlQuery query(database_);
    if (!query.exec(QStringLiteral("SELECT c.id, c.title, c.created_at, c.updated_at, "
                                   "c.archived, c.deleted, COUNT(m.message_id) "
                                   "FROM conversations c "
                                   "LEFT JOIN conversation_messages m ON m.conversation_id = c.id "
                                   "WHERE c.deleted = 0 "
                                   "GROUP BY c.id "
                                   "ORDER BY c.rowid ASC"))) {
        setLastError(ConversationStoreErrorCode::StorageFailure, query.lastError().text());
        return records;
    }

    while (query.next()) {
        ConversationRecord record;
        record.id = query.value(0).toString();
        record.title = query.value(1).toString();
        record.createdAtUtc = QDateTime::fromString(query.value(2).toString(), Qt::ISODateWithMs);
        record.updatedAtUtc = QDateTime::fromString(query.value(3).toString(), Qt::ISODateWithMs);
        record.archived = query.value(4).toBool();
        record.deleted = query.value(5).toBool();
        record.messageCount = query.value(6).toInt();
        record.summary = conversationRecordSummary(record);
        records.append(record);
    }

    setLastError(ConversationStoreErrorCode::None, {});
    return records;
}

bool SQLiteConversationStore::appendMessage(const ConversationMessageRecord& message) {
    if (!database_.isOpen()) {
        setLastError(ConversationStoreErrorCode::Unavailable,
                     QStringLiteral("SQLite conversation database is not open."));
        return false;
    }
    if (message.conversationId.trimmed().isEmpty() || !conversationExists(message.conversationId)) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return false;
    }
    if (conversationArchived(message.conversationId)) {
        setLastError(ConversationStoreErrorCode::UnsupportedOperation,
                     QStringLiteral("Archived conversation cannot accept new messages."));
        return false;
    }

    QSqlQuery query(database_);
    query.prepare(QStringLiteral("INSERT INTO conversation_messages("
                                 "conversation_id, message_id, role, content, timestamp, status) "
                                 "VALUES(?, ?, ?, ?, ?, ?) "
                                 "ON CONFLICT(conversation_id, message_id) DO UPDATE SET "
                                 "role = excluded.role,"
                                 "content = excluded.content,"
                                 "timestamp = excluded.timestamp,"
                                 "status = excluded.status"));
    query.addBindValue(message.conversationId);
    query.addBindValue(message.messageId);
    query.addBindValue(chatRoleName(message.role));
    query.addBindValue(message.content);
    query.addBindValue(message.timestampUtc.toUTC().toString(Qt::ISODateWithMs));
    query.addBindValue(chatMessageStatusName(message.status));
    if (!query.exec()) {
        setLastError(ConversationStoreErrorCode::StorageFailure, query.lastError().text());
        return false;
    }

    QSqlQuery update(database_);
    update.prepare(QStringLiteral("UPDATE conversations SET updated_at = ? WHERE id = ?"));
    update.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    update.addBindValue(message.conversationId);
    if (!update.exec()) {
        setLastError(ConversationStoreErrorCode::StorageFailure, update.lastError().text());
        return false;
    }

    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

QList<ConversationMessageRecord>
SQLiteConversationStore::loadMessages(const QString& conversationId) const {
    QList<ConversationMessageRecord> records;
    if (!database_.isOpen()) {
        setLastError(ConversationStoreErrorCode::Unavailable,
                     QStringLiteral("SQLite conversation database is not open."));
        return records;
    }
    if (conversationId.trimmed().isEmpty() || !conversationExists(conversationId)) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return records;
    }

    QSqlQuery query(database_);
    query.prepare(QStringLiteral("SELECT conversation_id, message_id, role, content, timestamp, "
                                 "status FROM conversation_messages "
                                 "WHERE conversation_id = ? "
                                 "ORDER BY message_id ASC"));
    query.addBindValue(conversationId);
    if (!query.exec()) {
        setLastError(ConversationStoreErrorCode::StorageFailure, query.lastError().text());
        return records;
    }

    while (query.next()) {
        records.append(ConversationMessageRecord{
            query.value(0).toString(),
            query.value(1).toInt(),
            roleFromName(query.value(2).toString()),
            query.value(3).toString(),
            QDateTime::fromString(query.value(4).toString(), Qt::ISODateWithMs),
            statusFromName(query.value(5).toString()),
        });
    }

    setLastError(ConversationStoreErrorCode::None, {});
    return records;
}

bool SQLiteConversationStore::renameConversation(const QString& conversationId,
                                                 const QString& title) {
    if (!database_.isOpen()) {
        setLastError(ConversationStoreErrorCode::Unavailable,
                     QStringLiteral("SQLite conversation database is not open."));
        return false;
    }
    if (!conversationExists(conversationId)) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return false;
    }

    QSqlQuery query(database_);
    query.prepare(QStringLiteral("UPDATE conversations SET title = ?, updated_at = ? "
                                 "WHERE id = ? AND deleted = 0"));
    query.addBindValue(normalizedTitle(title));
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    query.addBindValue(conversationId);
    if (!query.exec()) {
        setLastError(ConversationStoreErrorCode::StorageFailure, query.lastError().text());
        return false;
    }

    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

bool SQLiteConversationStore::archiveConversation(const QString& conversationId) {
    return updateConversationMetadata(conversationId, true, false);
}

bool SQLiteConversationStore::unarchiveConversation(const QString& conversationId) {
    return updateConversationMetadata(conversationId, false, false);
}

bool SQLiteConversationStore::deleteConversation(const QString& conversationId) {
    return updateConversationMetadata(conversationId, true, true);
}

ConversationStoreStatus SQLiteConversationStore::status() const {
    return database_.isOpen() ? ConversationStoreStatus::Ready
                              : ConversationStoreStatus::Unavailable;
}

ConversationStoreError SQLiteConversationStore::lastError() const {
    return lastError_;
}

QString SQLiteConversationStore::databasePath() const {
    return databasePath_;
}

int SQLiteConversationStore::schemaVersion() const {
    if (!database_.isOpen()) {
        return 0;
    }

    QSqlQuery query(database_);
    query.prepare(QStringLiteral("SELECT value FROM conversation_schema_metadata WHERE key = ?"));
    query.addBindValue(QStringLiteral("schema_version"));
    if (!query.exec() || !query.next()) {
        return 0;
    }

    return query.value(0).toInt();
}

bool SQLiteConversationStore::conversationExists(const QString& conversationId) const {
    QSqlQuery query(database_);
    query.prepare(QStringLiteral("SELECT 1 FROM conversations WHERE id = ? AND deleted = 0"));
    query.addBindValue(conversationId);
    return query.exec() && query.next();
}

bool SQLiteConversationStore::conversationArchived(const QString& conversationId) const {
    QSqlQuery query(database_);
    query.prepare(
        QStringLiteral("SELECT archived FROM conversations WHERE id = ? AND deleted = 0"));
    query.addBindValue(conversationId);
    return query.exec() && query.next() && query.value(0).toBool();
}

bool SQLiteConversationStore::updateConversationMetadata(const QString& conversationId,
                                                         bool archived, bool deleted) {
    if (!database_.isOpen()) {
        setLastError(ConversationStoreErrorCode::Unavailable,
                     QStringLiteral("SQLite conversation database is not open."));
        return false;
    }
    if (!conversationExists(conversationId)) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return false;
    }

    QSqlQuery query(database_);
    query.prepare(QStringLiteral("UPDATE conversations SET archived = ?, deleted = ?, "
                                 "updated_at = ? WHERE id = ?"));
    query.addBindValue(boolText(archived));
    query.addBindValue(boolText(deleted));
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    query.addBindValue(conversationId);
    if (!query.exec()) {
        setLastError(ConversationStoreErrorCode::StorageFailure, query.lastError().text());
        return false;
    }

    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

void SQLiteConversationStore::open() {
    const QFileInfo fileInfo(databasePath_);
    if (!fileInfo.dir().exists()) {
        QDir().mkpath(fileInfo.dir().absolutePath());
    }

    database_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    database_.setDatabaseName(databasePath_);
    if (!database_.open()) {
        setLastError(ConversationStoreErrorCode::Unavailable, database_.lastError().text());
    } else {
        setLastError(ConversationStoreErrorCode::None, {});
    }
}

void SQLiteConversationStore::initializeSchema() {
    if (!database_.isOpen()) {
        setLastError(ConversationStoreErrorCode::Unavailable,
                     QStringLiteral("SQLite conversation database is not open."));
        return;
    }

    QSqlQuery query(database_);
    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS conversations("
                                   "id TEXT PRIMARY KEY NOT NULL,"
                                   "title TEXT NOT NULL,"
                                   "created_at TEXT NOT NULL,"
                                   "updated_at TEXT NOT NULL,"
                                   "archived INTEGER NOT NULL DEFAULT 0,"
                                   "deleted INTEGER NOT NULL DEFAULT 0)"))) {
        setLastError(ConversationStoreErrorCode::StorageFailure, query.lastError().text());
        return;
    }

    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS conversation_messages("
                                   "conversation_id TEXT NOT NULL,"
                                   "message_id INTEGER NOT NULL,"
                                   "role TEXT NOT NULL,"
                                   "content TEXT NOT NULL,"
                                   "timestamp TEXT NOT NULL,"
                                   "status TEXT NOT NULL,"
                                   "PRIMARY KEY(conversation_id, message_id),"
                                   "FOREIGN KEY(conversation_id) REFERENCES conversations(id))"))) {
        setLastError(ConversationStoreErrorCode::StorageFailure, query.lastError().text());
        return;
    }

    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS conversation_schema_metadata("
                                   "key TEXT PRIMARY KEY NOT NULL,"
                                   "value INTEGER NOT NULL)"))) {
        setLastError(ConversationStoreErrorCode::StorageFailure, query.lastError().text());
        return;
    }

    query.prepare(QStringLiteral("INSERT INTO conversation_schema_metadata(key, value) "
                                 "VALUES(?, ?) "
                                 "ON CONFLICT(key) DO UPDATE SET value = excluded.value"));
    query.addBindValue(QStringLiteral("schema_version"));
    query.addBindValue(currentSchemaVersion);
    if (!query.exec()) {
        setLastError(ConversationStoreErrorCode::StorageFailure, query.lastError().text());
        return;
    }

    setLastError(ConversationStoreErrorCode::None, {});
}

void SQLiteConversationStore::setLastError(ConversationStoreErrorCode code,
                                           const QString& summary) const {
    lastError_.code = code;
    lastError_.summary = summary;
}

} // namespace sentinel::core
