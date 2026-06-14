#pragma once

#include "sentinel/core/IChatHistoryStore.h"

#include <QSqlDatabase>
#include <QString>

namespace sentinel::core {

class SQLiteChatHistoryStore final : public IChatHistoryStore {
public:
    explicit SQLiteChatHistoryStore(QString databasePath);
    ~SQLiteChatHistoryStore() override;

    SQLiteChatHistoryStore(const SQLiteChatHistoryStore&) = delete;
    SQLiteChatHistoryStore& operator=(const SQLiteChatHistoryStore&) = delete;
    SQLiteChatHistoryStore(SQLiteChatHistoryStore&&) = delete;
    SQLiteChatHistoryStore& operator=(SQLiteChatHistoryStore&&) = delete;

    QList<ChatMessage> loadMessages() const override;
    void appendMessage(const ChatMessage& message) override;
    void clear() override;
    bool isAvailable() const override;
    QString lastError() const override;

    QString databasePath() const;
    int schemaVersion() const;

private:
    void open();
    void initializeSchema();
    void setLastError(QString error) const;

    static constexpr int currentSchemaVersion = 2;

    QString databasePath_;
    QString connectionName_;
    QSqlDatabase database_;
    mutable QString lastError_;
};

} // namespace sentinel::core
