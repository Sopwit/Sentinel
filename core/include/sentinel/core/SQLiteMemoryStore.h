#pragma once

#include "sentinel/core/IMemoryStore.h"

#include <QSqlDatabase>
#include <QString>

namespace sentinel::core {

class SQLiteMemoryStore final : public IMemoryStore {
public:
    explicit SQLiteMemoryStore(QString databasePath);
    ~SQLiteMemoryStore() override;

    SQLiteMemoryStore(const SQLiteMemoryStore&) = delete;
    SQLiteMemoryStore& operator=(const SQLiteMemoryStore&) = delete;
    SQLiteMemoryStore(SQLiteMemoryStore&&) = delete;
    SQLiteMemoryStore& operator=(SQLiteMemoryStore&&) = delete;

    void put(QString key, QString value) override;
    QString get(const QString& key) const override;
    MemoryEntries entries() const override;
    void clear() override;
    bool isAvailable() const override;
    QString lastError() const override;

    QString databasePath() const;
    int schemaVersion() const;

private:
    void open();
    void initializeSchema();
    void setLastError(QString error) const;

    static constexpr int currentSchemaVersion = 1;

    QString databasePath_;
    QString connectionName_;
    QSqlDatabase database_;
    mutable QString lastError_;
};

} // namespace sentinel::core
