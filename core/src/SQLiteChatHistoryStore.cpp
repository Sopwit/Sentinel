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
    if (!query.exec(QStringLiteral("SELECT id, role, content, timestamp, status "
                                   "FROM chat_messages ORDER BY id ASC"))) {
        setLastError(query.lastError().text());
        return result;
    }

    setLastError({});
    while (query.next()) {
        const auto timestamp =
            QDateTime::fromString(query.value(3).toString(), Qt::ISODateWithMs);
        result.append(ChatMessage{
            query.value(0).toInt(),
            roleFromName(query.value(1).toString()),
            query.value(2).toString(),
            timestamp,
            statusFromName(query.value(4).toString()),
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
    query.prepare(QStringLiteral("INSERT INTO chat_messages(id, role, content, timestamp, status) "
                                 "VALUES(?, ?, ?, ?, ?) "
                                 "ON CONFLICT(id) DO UPDATE SET "
                                 "role = excluded.role,"
                                 "content = excluded.content,"
                                 "timestamp = excluded.timestamp,"
                                 "status = excluded.status"));
    query.addBindValue(message.id);
    query.addBindValue(chatRoleName(message.role));
    query.addBindValue(message.content);
    query.addBindValue(message.timestamp.toUTC().toString(Qt::ISODateWithMs));
    query.addBindValue(chatMessageStatusName(message.status));
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
                                   "status TEXT NOT NULL)"))) {
        setLastError(query.lastError().text());
        return;
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
