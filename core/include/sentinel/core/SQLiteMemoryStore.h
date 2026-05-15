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

    QString databasePath() const;

private:
    void open();
    void initializeSchema();

    QString databasePath_;
    QString connectionName_;
    QSqlDatabase database_;
};

} // namespace sentinel::core
