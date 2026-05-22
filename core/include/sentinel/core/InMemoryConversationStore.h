#pragma once

#include "sentinel/core/IConversationStore.h"

#include <QHash>
#include <QList>

namespace sentinel::core {

class InMemoryConversationStore final : public IConversationStore {
public:
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
    ConversationStoreError lastError() const override;

private:
    void setLastError(ConversationStoreErrorCode code, const QString& summary) const;
    ConversationRecord* findConversation(const QString& conversationId);
    const ConversationRecord* findConversation(const QString& conversationId) const;
    static QString normalizedTitle(const QString& title);

    int nextConversationNumber_ = 1;
    QHash<QString, ConversationRecord> conversations_;
    QList<QString> conversationOrder_;
    QHash<QString, QList<ConversationMessageRecord>> messagesByConversation_;
    mutable ConversationStoreError lastError_;
};

} // namespace sentinel::core
