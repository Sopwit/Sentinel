#include "sentinel/core/InMemoryConversationStore.h"

#include <algorithm>

namespace sentinel::core {

QString InMemoryConversationStore::normalizedTitle(const QString& title) {
    const auto trimmed = title.trimmed();
    return trimmed.isEmpty() ? QStringLiteral("Untitled Conversation") : trimmed;
}

ConversationRecord InMemoryConversationStore::createConversation(const QString& title) {
    ConversationRecord record;
    record.id = QStringLiteral("conversation-%1").arg(nextConversationNumber_++);
    record.title = normalizedTitle(title);
    record.createdAtUtc = QDateTime::currentDateTimeUtc();
    record.updatedAtUtc = record.createdAtUtc;
    record.summary = QStringLiteral("%1 (0 messages)").arg(record.title);

    conversations_.insert(record.id, record);
    conversationOrder_.append(record.id);
    messagesByConversation_.insert(record.id, {});
    setLastError(ConversationStoreErrorCode::None, {});
    return record;
}

QList<ConversationRecord> InMemoryConversationStore::listConversations() const {
    QList<ConversationRecord> records;
    records.reserve(conversationOrder_.size());
    for (const auto& id : conversationOrder_) {
        const auto it = conversations_.constFind(id);
        if (it == conversations_.constEnd() || it->deleted) {
            continue;
        }
        records.append(*it);
    }
    return records;
}

bool InMemoryConversationStore::appendMessage(const ConversationMessageRecord& message) {
    if (message.conversationId.trimmed().isEmpty()) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation id is required."));
        return false;
    }

    auto* conversation = findConversation(message.conversationId);
    if (!conversation || conversation->deleted) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return false;
    }
    if (conversation->archived) {
        setLastError(ConversationStoreErrorCode::UnsupportedOperation,
                     QStringLiteral("Archived conversation cannot accept new messages."));
        return false;
    }

    auto conversationMessages = messagesByConversation_.value(conversation->id);
    conversationMessages.append(message);
    std::sort(conversationMessages.begin(), conversationMessages.end(),
              [](const ConversationMessageRecord& lhs, const ConversationMessageRecord& rhs) {
                  return lhs.messageId < rhs.messageId;
              });
    messagesByConversation_.insert(conversation->id, conversationMessages);

    conversation->messageCount = static_cast<int>(conversationMessages.size());
    conversation->updatedAtUtc = QDateTime::currentDateTimeUtc();
    conversation->summary = QStringLiteral("%1 (%2 %3)")
                                .arg(conversation->title)
                                .arg(conversation->messageCount)
                                .arg(conversation->messageCount == 1 ? QStringLiteral("message")
                                                                     : QStringLiteral("messages"));
    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

QList<ConversationMessageRecord>
InMemoryConversationStore::loadMessages(const QString& conversationId) const {
    if (conversationId.trimmed().isEmpty()) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation id is required."));
        return {};
    }

    const auto* conversation = findConversation(conversationId);
    if (!conversation || conversation->deleted) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return {};
    }

    auto messages = messagesByConversation_.value(conversationId);
    std::sort(messages.begin(), messages.end(),
              [](const ConversationMessageRecord& lhs, const ConversationMessageRecord& rhs) {
                  return lhs.messageId < rhs.messageId;
              });
    setLastError(ConversationStoreErrorCode::None, {});
    return messages;
}

bool InMemoryConversationStore::renameConversation(const QString& conversationId,
                                                   const QString& title) {
    auto* conversation = findConversation(conversationId);
    if (!conversation || conversation->deleted) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return false;
    }

    conversation->title = normalizedTitle(title);
    conversation->updatedAtUtc = QDateTime::currentDateTimeUtc();
    conversation->summary = QStringLiteral("%1 (%2 %3)")
                                .arg(conversation->title)
                                .arg(conversation->messageCount)
                                .arg(conversation->messageCount == 1 ? QStringLiteral("message")
                                                                     : QStringLiteral("messages"));
    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

bool InMemoryConversationStore::archiveConversation(const QString& conversationId) {
    auto* conversation = findConversation(conversationId);
    if (!conversation || conversation->deleted) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return false;
    }

    conversation->archived = true;
    conversation->updatedAtUtc = QDateTime::currentDateTimeUtc();
    conversation->summary = QStringLiteral("%1 (archived, %2 %3)")
                                .arg(conversation->title)
                                .arg(conversation->messageCount)
                                .arg(conversation->messageCount == 1 ? QStringLiteral("message")
                                                                     : QStringLiteral("messages"));
    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

bool InMemoryConversationStore::unarchiveConversation(const QString& conversationId) {
    auto* conversation = findConversation(conversationId);
    if (!conversation || conversation->deleted) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return false;
    }

    conversation->archived = false;
    conversation->updatedAtUtc = QDateTime::currentDateTimeUtc();
    conversation->summary = QStringLiteral("%1 (%2 %3)")
                                .arg(conversation->title)
                                .arg(conversation->messageCount)
                                .arg(conversation->messageCount == 1 ? QStringLiteral("message")
                                                                     : QStringLiteral("messages"));
    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

bool InMemoryConversationStore::pinConversation(const QString& conversationId) {
    auto* conversation = findConversation(conversationId);
    if (!conversation || conversation->deleted) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return false;
    }

    conversation->pinned = true;
    conversation->updatedAtUtc = QDateTime::currentDateTimeUtc();
    conversation->summary = conversationRecordSummary(*conversation);
    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

bool InMemoryConversationStore::unpinConversation(const QString& conversationId) {
    auto* conversation = findConversation(conversationId);
    if (!conversation || conversation->deleted) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return false;
    }

    conversation->pinned = false;
    conversation->updatedAtUtc = QDateTime::currentDateTimeUtc();
    conversation->summary = conversationRecordSummary(*conversation);
    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

bool InMemoryConversationStore::deleteConversation(const QString& conversationId) {
    auto* conversation = findConversation(conversationId);
    if (!conversation) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return false;
    }

    conversation->deleted = true;
    conversation->archived = true;
    conversation->updatedAtUtc = QDateTime::currentDateTimeUtc();
    conversation->summary = QStringLiteral("%1 (deleted metadata)").arg(conversation->title);
    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

bool InMemoryConversationStore::saveSummaryMetadata(
    const ConversationSummaryMetadataRecord& metadata) {
    auto* conversation = findConversation(metadata.conversationId);
    if (!conversation || conversation->deleted) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return false;
    }

    summaryMetadataByConversation_.insert(metadata.conversationId, metadata);
    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

ConversationSummaryMetadataRecord
InMemoryConversationStore::loadSummaryMetadata(const QString& conversationId) const {
    const auto* conversation = findConversation(conversationId);
    if (!conversation || conversation->deleted) {
        setLastError(ConversationStoreErrorCode::InvalidConversationId,
                     QStringLiteral("Conversation does not exist."));
        return {};
    }

    setLastError(ConversationStoreErrorCode::None, {});
    return summaryMetadataByConversation_.value(conversationId);
}

ConversationStoreError InMemoryConversationStore::lastError() const {
    return lastError_;
}

void InMemoryConversationStore::setLastError(ConversationStoreErrorCode code,
                                             const QString& summary) const {
    lastError_.code = code;
    lastError_.summary = summary;
}

ConversationRecord* InMemoryConversationStore::findConversation(const QString& conversationId) {
    auto it = conversations_.find(conversationId);
    return it == conversations_.end() ? nullptr : &it.value();
}

const ConversationRecord*
InMemoryConversationStore::findConversation(const QString& conversationId) const {
    const auto it = conversations_.constFind(conversationId);
    return it == conversations_.constEnd() ? nullptr : &it.value();
}

} // namespace sentinel::core
