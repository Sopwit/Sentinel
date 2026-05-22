#pragma once

#include "sentinel/core/IConversationStore.h"

#include <QSqlDatabase>

namespace sentinel::core {

class SQLiteConversationStore final : public IConversationStore {
public:
    explicit SQLiteConversationStore(QString databasePath);
    ~SQLiteConversationStore() override;

    SQLiteConversationStore(const SQLiteConversationStore&) = delete;
    SQLiteConversationStore& operator=(const SQLiteConversationStore&) = delete;
    SQLiteConversationStore(SQLiteConversationStore&&) = delete;
    SQLiteConversationStore& operator=(SQLiteConversationStore&&) = delete;

    ConversationRecord createConversation(const QString& title) override;
    QList<ConversationRecord> listConversations() const override;
    bool appendMessage(const ConversationMessageRecord& message) override;
    QList<ConversationMessageRecord> loadMessages(const QString& conversationId) const override;
    bool renameConversation(const QString& conversationId, const QString& title) override;
    bool archiveConversation(const QString& conversationId) override;
    bool unarchiveConversation(const QString& conversationId) override;
    bool pinConversation(const QString& conversationId) override;
    bool unpinConversation(const QString& conversationId) override;
    bool deleteConversation(const QString& conversationId) override;
    ConversationStoreStatus status() const override;
    ConversationStoreError lastError() const override;

    QString databasePath() const;
    int schemaVersion() const;

private:
    static QString normalizedTitle(const QString& title);
    bool conversationExists(const QString& conversationId) const;
    bool conversationArchived(const QString& conversationId) const;
    bool updateConversationMetadata(const QString& conversationId, bool archived, bool deleted);
    bool updatePinnedMetadata(const QString& conversationId, bool pinned);
    void open();
    void initializeSchema();
    void setLastError(ConversationStoreErrorCode code, const QString& summary) const;

    static constexpr int currentSchemaVersion = 2;

    QString databasePath_;
    QString connectionName_;
    QSqlDatabase database_;
    mutable ConversationStoreError lastError_;
};

} // namespace sentinel::core
