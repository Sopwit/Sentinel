#include "sentinel/core/SQLiteMemoryStore.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QUuid>

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
    query.exec();
}

QString SQLiteMemoryStore::get(const QString& key) const {
    if (!database_.isOpen()) {
        return {};
    }

    QSqlQuery query(database_);
    if (key.isEmpty()) {
        query.prepare(QStringLiteral("SELECT value FROM memory_entries WHERE key = ''"));
    } else {
        query.prepare(QStringLiteral("SELECT value FROM memory_entries WHERE key = ?"));
        query.addBindValue(key);
    }

    if (!query.exec() || !query.next()) {
        return {};
    }

    return query.value(0).toString();
}

MemoryEntries SQLiteMemoryStore::entries() const {
    MemoryEntries result;
    if (!database_.isOpen()) {
        return result;
    }

    QSqlQuery query(database_);

    if (!query.exec(QStringLiteral("SELECT key, value FROM memory_entries ORDER BY key ASC"))) {
        return result;
    }

    while (query.next()) {
        result.append({query.value(0).toString(), query.value(1).toString()});
    }

    return result;
}

QString SQLiteMemoryStore::databasePath() const {
    return databasePath_;
}

void SQLiteMemoryStore::open() {
    const QFileInfo fileInfo(databasePath_);
    if (!fileInfo.dir().exists()) {
        QDir().mkpath(fileInfo.dir().absolutePath());
    }

    database_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    database_.setDatabaseName(databasePath_);
    database_.open();
}

void SQLiteMemoryStore::initializeSchema() {
    if (!database_.isOpen()) {
        return;
    }

    QSqlQuery query(database_);
    query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS memory_entries("
                              "key TEXT PRIMARY KEY NOT NULL,"
                              "value TEXT NOT NULL)"));
}

} // namespace sentinel::core
