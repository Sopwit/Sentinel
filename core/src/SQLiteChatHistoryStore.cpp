#include "sentinel/core/SQLiteChatHistoryStore.h"

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

bool tableHasColumn(const QSqlDatabase& database, const QString& table, const QString& column) {
    QSqlQuery query(database);
    if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table))) {
        return false;
    }
    while (query.next()) {
        if (query.value(1).toString() == column) {
            return true;
        }
    }
    return false;
}

} // namespace

SQLiteChatHistoryStore::SQLiteChatHistoryStore(QString databasePath)
    : databasePath_(std::move(databasePath)),
      connectionName_(
          QStringLiteral("sentinel_chat_%1").arg(QUuid::createUuid().toString(QUuid::Id128))) {
    open();
    initializeSchema();
}

SQLiteChatHistoryStore::~SQLiteChatHistoryStore() {
    const auto connectionName = connectionName_;
    database_ = {};
    QSqlDatabase::removeDatabase(connectionName);
}

QList<ChatMessage> SQLiteChatHistoryStore::loadMessages() const {
    QList<ChatMessage> result;
    if (!database_.isOpen()) {
        setLastError(QStringLiteral("SQLite chat history database is not open."));
        return result;
    }

    QSqlQuery query(database_);
    if (!query.exec(QStringLiteral("SELECT id, role, content, timestamp, status, "
                                   "provider_used, model_used, role_used, "
                                   "response_duration_ms, first_token_latency_ms, "
                                   "approx_tokens_per_second "
                                   "FROM chat_messages ORDER BY id ASC"))) {
        setLastError(query.lastError().text());
        return result;
    }

    setLastError({});
    while (query.next()) {
        const auto timestamp = QDateTime::fromString(query.value(3).toString(), Qt::ISODateWithMs);
        result.append(ChatMessage{
            query.value(0).toInt(),
            roleFromName(query.value(1).toString()),
            query.value(2).toString(),
            timestamp,
            statusFromName(query.value(4).toString()),
            query.value(5).toString(),
            query.value(6).toString(),
            query.value(7).toString(),
            query.value(8).isNull() ? -1 : query.value(8).toLongLong(),
            query.value(9).isNull() ? -1 : query.value(9).toLongLong(),
            query.value(10).isNull() ? 0.0 : query.value(10).toDouble(),
        });
    }

    return result;
}

void SQLiteChatHistoryStore::appendMessage(const ChatMessage& message) {
    if (!database_.isOpen()) {
        setLastError(QStringLiteral("SQLite chat history database is not open."));
        return;
    }

    QSqlQuery query(database_);
    query.prepare(QStringLiteral("INSERT INTO chat_messages("
                                 "id, role, content, timestamp, status, provider_used, "
                                 "model_used, role_used, response_duration_ms, "
                                 "first_token_latency_ms, approx_tokens_per_second) "
                                 "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
                                 "ON CONFLICT(id) DO UPDATE SET "
                                 "role = excluded.role,"
                                 "content = excluded.content,"
                                 "timestamp = excluded.timestamp,"
                                 "status = excluded.status,"
                                 "provider_used = excluded.provider_used,"
                                 "model_used = excluded.model_used,"
                                 "role_used = excluded.role_used,"
                                 "response_duration_ms = excluded.response_duration_ms,"
                                 "first_token_latency_ms = excluded.first_token_latency_ms,"
                                 "approx_tokens_per_second = excluded.approx_tokens_per_second"));
    query.addBindValue(message.id);
    query.addBindValue(chatRoleName(message.role));
    query.addBindValue(message.content);
    query.addBindValue(message.timestamp.toUTC().toString(Qt::ISODateWithMs));
    query.addBindValue(chatMessageStatusName(message.status));
    query.addBindValue(message.providerUsed);
    query.addBindValue(message.modelUsed);
    query.addBindValue(message.roleUsed);
    query.addBindValue(message.responseDurationMs >= 0 ? QVariant(message.responseDurationMs)
                                                       : QVariant{});
    query.addBindValue(message.firstTokenLatencyMs >= 0 ? QVariant(message.firstTokenLatencyMs)
                                                        : QVariant{});
    query.addBindValue(message.approximateTokensPerSecond > 0.0
                           ? QVariant(message.approximateTokensPerSecond)
                           : QVariant{});
    if (!query.exec()) {
        setLastError(query.lastError().text());
    } else {
        setLastError({});
    }
}

void SQLiteChatHistoryStore::clear() {
    if (!database_.isOpen()) {
        setLastError(QStringLiteral("SQLite chat history database is not open."));
        return;
    }

    QSqlQuery query(database_);
    if (!query.exec(QStringLiteral("DELETE FROM chat_messages"))) {
        setLastError(query.lastError().text());
    } else {
        setLastError({});
    }
}

bool SQLiteChatHistoryStore::isAvailable() const {
    return database_.isOpen();
}

QString SQLiteChatHistoryStore::lastError() const {
    return lastError_;
}

QString SQLiteChatHistoryStore::databasePath() const {
    return databasePath_;
}

int SQLiteChatHistoryStore::schemaVersion() const {
    if (!database_.isOpen()) {
        return 0;
    }

    QSqlQuery query(database_);
    query.prepare(QStringLiteral("SELECT value FROM chat_history_schema_metadata WHERE key = ?"));
    query.addBindValue(QStringLiteral("schema_version"));

    if (!query.exec() || !query.next()) {
        return 0;
    }

    return query.value(0).toInt();
}

void SQLiteChatHistoryStore::open() {
    const QFileInfo fileInfo(databasePath_);
    if (!fileInfo.dir().exists()) {
        QDir().mkpath(fileInfo.dir().absolutePath());
    }

    database_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    database_.setDatabaseName(databasePath_);
    if (!database_.open()) {
        setLastError(database_.lastError().text());
    } else {
        setLastError({});
    }
}

void SQLiteChatHistoryStore::initializeSchema() {
    if (!database_.isOpen()) {
        setLastError(QStringLiteral("SQLite chat history database is not open."));
        return;
    }

    QSqlQuery query(database_);
    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS chat_messages("
                                   "id INTEGER PRIMARY KEY NOT NULL,"
                                   "role TEXT NOT NULL,"
                                   "content TEXT NOT NULL,"
                                   "timestamp TEXT NOT NULL,"
                                   "status TEXT NOT NULL,"
                                   "provider_used TEXT,"
                                   "model_used TEXT,"
                                   "role_used TEXT,"
                                   "response_duration_ms INTEGER,"
                                   "first_token_latency_ms INTEGER,"
                                   "approx_tokens_per_second REAL)"))) {
        setLastError(query.lastError().text());
        return;
    }

    const QList<QPair<QString, QString>> columns{
        {QStringLiteral("provider_used"), QStringLiteral("TEXT")},
        {QStringLiteral("model_used"), QStringLiteral("TEXT")},
        {QStringLiteral("role_used"), QStringLiteral("TEXT")},
        {QStringLiteral("response_duration_ms"), QStringLiteral("INTEGER")},
        {QStringLiteral("first_token_latency_ms"), QStringLiteral("INTEGER")},
        {QStringLiteral("approx_tokens_per_second"), QStringLiteral("REAL")},
    };
    for (const auto& column : columns) {
        if (!tableHasColumn(database_, QStringLiteral("chat_messages"), column.first) &&
            !query.exec(QStringLiteral("ALTER TABLE chat_messages ADD COLUMN %1 %2")
                            .arg(column.first, column.second))) {
            setLastError(query.lastError().text());
            return;
        }
    }

    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS chat_history_schema_metadata("
                                   "key TEXT PRIMARY KEY NOT NULL,"
                                   "value INTEGER NOT NULL)"))) {
        setLastError(query.lastError().text());
        return;
    }

    query.prepare(QStringLiteral("INSERT INTO chat_history_schema_metadata(key, value) "
                                 "VALUES(?, ?) "
                                 "ON CONFLICT(key) DO UPDATE SET value = excluded.value"));
    query.addBindValue(QStringLiteral("schema_version"));
    query.addBindValue(currentSchemaVersion);
    if (!query.exec()) {
        setLastError(query.lastError().text());
        return;
    }

    setLastError({});
}

void SQLiteChatHistoryStore::setLastError(QString error) const {
    lastError_ = std::move(error);
}

} // namespace sentinel::core
