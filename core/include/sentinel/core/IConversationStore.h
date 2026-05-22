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
    bool pinned = false;
    bool deleted = false;
    int messageCount = 0;
    QString summary = QStringLiteral("Conversation metadata is not available.");
};

inline QString conversationRecordSummary(const ConversationRecord& record) {
    return QStringLiteral("%1 / %2 %3 / %4")
        .arg(record.title)
        .arg(record.messageCount)
        .arg(record.messageCount == 1 ? QStringLiteral("message") : QStringLiteral("messages"))
        .arg(record.archived ? QStringLiteral("Archived")
             : record.pinned ? QStringLiteral("Pinned")
                             : QStringLiteral("Active"));
}

struct ConversationMessageRecord {
    QString conversationId;
    int messageId = 0;
    ChatRole role = ChatRole::System;
    QString content;
    QDateTime timestampUtc;
    ChatMessageStatus status = ChatMessageStatus::Received;
};

struct ConversationSummaryMetadataRecord {
    QString conversationId;
    QDateTime summaryTimestampUtc;
    int coveredFirstMessageId = 0;
    int coveredLastMessageId = 0;
    int estimatedReductionPercent = 0;
    QString readinessState = QStringLiteral("Blocked");
    QString summaryText;
    QString summary = QStringLiteral("No conversation summary metadata has been persisted.");
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
    virtual bool unarchiveConversation(const QString& conversationId) = 0;
    virtual bool pinConversation(const QString& conversationId) = 0;
    virtual bool unpinConversation(const QString& conversationId) = 0;
    virtual bool deleteConversation(const QString& conversationId) = 0;
    virtual bool saveSummaryMetadata(const ConversationSummaryMetadataRecord& metadata) {
        Q_UNUSED(metadata);
        return false;
    }
    virtual ConversationSummaryMetadataRecord
    loadSummaryMetadata(const QString& conversationId) const {
        Q_UNUSED(conversationId);
        return {};
    }

    virtual ConversationStoreStatus status() const {
        return ConversationStoreStatus::Ready;
    }
    virtual ConversationStoreError lastError() const {
        return {};
    }
};

} // namespace sentinel::core
