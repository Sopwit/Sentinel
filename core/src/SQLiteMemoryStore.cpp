#include "sentinel/core/SQLiteMemoryStore.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

namespace sentinel::core {

SQLiteMemoryStore::SQLiteMemoryStore(QString databasePath)
    : databasePath_(std::move(databasePath)),
      connectionName_(
          QStringLiteral("sentinel_memory_%1").arg(QUuid::createUuid().toString(QUuid::Id128))) {
    open();
    initializeSchema();
}

SQLiteMemoryStore::~SQLiteMemoryStore() {
    const auto connectionName = connectionName_;
    database_ = {};
    QSqlDatabase::removeDatabase(connectionName);
}

void SQLiteMemoryStore::put(QString key, QString value) {
    if (!database_.isOpen()) {
        setLastError(QStringLiteral("SQLite memory database is not open."));
        return;
    }

    QSqlQuery query(database_);
    if (key.isEmpty()) {
        query.prepare(QStringLiteral("INSERT INTO memory_entries(key, value) VALUES('', ?) "
                                     "ON CONFLICT(key) DO UPDATE SET value = excluded.value"));
    } else {
        query.prepare(QStringLiteral("INSERT INTO memory_entries(key, value) VALUES(?, ?) "
                                     "ON CONFLICT(key) DO UPDATE SET value = excluded.value"));
        query.addBindValue(key);
    }
    query.addBindValue(value);
    if (!query.exec()) {
        setLastError(query.lastError().text());
    } else {
        setLastError({});
    }
}

QString SQLiteMemoryStore::get(const QString& key) const {
    if (!database_.isOpen()) {
        setLastError(QStringLiteral("SQLite memory database is not open."));
        return {};
    }

    QSqlQuery query(database_);
    if (key.isEmpty()) {
        query.prepare(QStringLiteral("SELECT value FROM memory_entries WHERE key = ''"));
    } else {
        query.prepare(QStringLiteral("SELECT value FROM memory_entries WHERE key = ?"));
        query.addBindValue(key);
    }

    if (!query.exec()) {
        setLastError(query.lastError().text());
        return {};
    }

    setLastError({});
    if (!query.next()) {
        return {};
    }

    return query.value(0).toString();
}

MemoryEntries SQLiteMemoryStore::entries() const {
    MemoryEntries result;
    if (!database_.isOpen()) {
        setLastError(QStringLiteral("SQLite memory database is not open."));
        return result;
    }

    QSqlQuery query(database_);

    if (!query.exec(QStringLiteral("SELECT key, value FROM memory_entries ORDER BY key ASC"))) {
        setLastError(query.lastError().text());
        return result;
    }

    setLastError({});
    while (query.next()) {
        result.append({query.value(0).toString(), query.value(1).toString()});
    }

    return result;
}

void SQLiteMemoryStore::clear() {
    if (!database_.isOpen()) {
        setLastError(QStringLiteral("SQLite memory database is not open."));
        return;
    }

    QSqlQuery query(database_);
    if (!query.exec(QStringLiteral("DELETE FROM memory_entries"))) {
        setLastError(query.lastError().text());
    } else {
        setLastError({});
    }
}

bool SQLiteMemoryStore::isAvailable() const {
    return database_.isOpen();
}

QString SQLiteMemoryStore::lastError() const {
    return lastError_;
}

QString SQLiteMemoryStore::databasePath() const {
    return databasePath_;
}

int SQLiteMemoryStore::schemaVersion() const {
    if (!database_.isOpen()) {
        return 0;
    }

    QSqlQuery query(database_);
    query.prepare(QStringLiteral("SELECT value FROM memory_schema_metadata WHERE key = ?"));
    query.addBindValue(QStringLiteral("schema_version"));

    if (!query.exec() || !query.next()) {
        return 0;
    }

    return query.value(0).toInt();
}

void SQLiteMemoryStore::open() {
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

void SQLiteMemoryStore::initializeSchema() {
    if (!database_.isOpen()) {
        setLastError(QStringLiteral("SQLite memory database is not open."));
        return;
    }

    QSqlQuery query(database_);
    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS memory_entries("
                                   "key TEXT PRIMARY KEY NOT NULL,"
                                   "value TEXT NOT NULL)"))) {
        setLastError(query.lastError().text());
        return;
    }

    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS memory_schema_metadata("
                                   "key TEXT PRIMARY KEY NOT NULL,"
                                   "value INTEGER NOT NULL)"))) {
        setLastError(query.lastError().text());
        return;
    }

    query.prepare(QStringLiteral("INSERT INTO memory_schema_metadata(key, value) VALUES(?, ?) "
                                 "ON CONFLICT(key) DO UPDATE SET value = excluded.value"));
    query.addBindValue(QStringLiteral("schema_version"));
    query.addBindValue(currentSchemaVersion);
    if (!query.exec()) {
        setLastError(query.lastError().text());
        return;
    }

    setLastError({});
}

void SQLiteMemoryStore::setLastError(QString error) const {
    lastError_ = std::move(error);
}

} // namespace sentinel::core
