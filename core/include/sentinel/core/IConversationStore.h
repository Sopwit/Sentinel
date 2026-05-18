#pragma once

#include "sentinel/core/ChatMessage.h"

#include <QDateTime>
#include <QList>
#include <QString>

namespace sentinel::core {

enum class ConversationStoreStatus {
    Ready,
    Unavailable,
};

inline QString conversationStoreStatusName(ConversationStoreStatus status) {
    switch (status) {
    case ConversationStoreStatus::Ready:
        return QStringLiteral("Ready");
    case ConversationStoreStatus::Unavailable:
        return QStringLiteral("Unavailable");
    }

    return QStringLiteral("Unavailable");
}

enum class ConversationStoreErrorCode {
    None,
    Unavailable,
    InvalidConversationId,
    InvalidTitle,
    StorageFailure,
    UnsupportedOperation,
};

struct ConversationStoreError {
    ConversationStoreErrorCode code = ConversationStoreErrorCode::None;
    QString summary;
};

struct ConversationRecord {
    QString id;
    QString title = QStringLiteral("Untitled Conversation");
    QDateTime createdAtUtc;
    QDateTime updatedAtUtc;
    bool archived = false;
    bool deleted = false;
    int messageCount = 0;
    QString summary = QStringLiteral("Conversation metadata is not available.");
};

struct ConversationMessageRecord {
    QString conversationId;
    int messageId = 0;
    ChatRole role = ChatRole::System;
    QString content;
    QDateTime timestampUtc;
    ChatMessageStatus status = ChatMessageStatus::Received;
};

class IConversationStore {
public:
    virtual ~IConversationStore() = default;

    virtual ConversationRecord createConversation(const QString& title) = 0;
    virtual QList<ConversationRecord> listConversations() const = 0;
    virtual bool appendMessage(const ConversationMessageRecord& message) = 0;
    virtual QList<ConversationMessageRecord> loadMessages(const QString& conversationId) const = 0;
    virtual bool renameConversation(const QString& conversationId, const QString& title) = 0;
    virtual bool archiveConversation(const QString& conversationId) = 0;
    virtual bool deleteConversation(const QString& conversationId) = 0;

    virtual ConversationStoreStatus status() const {
        return ConversationStoreStatus::Ready;
    }
    virtual ConversationStoreError lastError() const {
        return {};
    }
};

} // namespace sentinel::core
