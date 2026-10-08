// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/chat/InMemoryConversationStore.h"

#include <algorithm>

namespace sentinel::core {

bool InMemoryConversationStore::clearHistory() {
    conversations_.clear();
    conversationOrder_.clear();
    messagesByConversation_.clear();
    summaryMetadataByConversation_.clear();
    nextConversationNumber_ = 1;
    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

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
    std::sort(records.begin(), records.end(), [](const auto& a, const auto& b) {
        if (a.pinned != b.pinned) return a.pinned;
        if (a.updatedAtUtc != b.updatedAtUtc) return a.updatedAtUtc > b.updatedAtUtc;
        return a.id < b.id;
    });
    return records;
}

QList<ConversationRecord> InMemoryConversationStore::searchConversations(
    const QString& query, int limit) const {
    const auto needle = query.trimmed().left(128);
    if (needle.isEmpty()) return {};
    QList<ConversationRecord> matches;
    for (const auto& record : listConversations()) {
        bool found = record.title.contains(needle, Qt::CaseInsensitive);
        if (!found) {
            for (const auto& message : messagesByConversation_.value(record.id)) {
                if (message.content.contains(needle, Qt::CaseInsensitive)) {
                    found = true;
                    break;
                }
            }
        }
        if (found) matches.append(record);
        if (matches.size() >= qBound(1, limit, 50)) break;
    }
    return matches;
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
    auto existing = std::find_if(conversationMessages.begin(), conversationMessages.end(),
        [&](const auto& current) { return current.messageId == message.messageId; });
    if (existing != conversationMessages.end()) *existing = message;
    else conversationMessages.append(message);
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
    conversation->userRenamed = true;
    conversation->updatedAtUtc = QDateTime::currentDateTimeUtc();
    conversation->summary = QStringLiteral("%1 (%2 %3)")
                                .arg(conversation->title)
                                .arg(conversation->messageCount)
                                .arg(conversation->messageCount == 1 ? QStringLiteral("message")
                                                                     : QStringLiteral("messages"));
    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

bool InMemoryConversationStore::autoTitleConversation(const QString& conversationId,
                                                       const QString& title) {
    auto* conversation = findConversation(conversationId);
    if (!conversation || conversation->deleted || conversation->userRenamed ||
        (conversation->title != QLatin1String("Current Transcript") &&
         conversation->title != QLatin1String("Untitled Conversation") &&
         conversation->title != QLatin1String("New Chat") &&
         conversation->title != QLatin1String("New chat"))) return false;
    conversation->title = normalizedTitle(title);
    conversation->updatedAtUtc = QDateTime::currentDateTimeUtc();
    conversation->summary = conversationRecordSummary(*conversation);
    return true;
}

bool InMemoryConversationStore::updateAutoTitleConversation(const QString& conversationId,
                                                             const QString& title,
                                                             const QString& expectedTitle) {
    auto* conversation = findConversation(conversationId);
    if (!conversation || conversation->deleted || conversation->userRenamed ||
        conversation->title != expectedTitle || title.trimmed().isEmpty())
        return false;
    conversation->title = normalizedTitle(title);
    conversation->updatedAtUtc = QDateTime::currentDateTimeUtc();
    conversation->summary = conversationRecordSummary(*conversation);
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

bool InMemoryConversationStore::discardImportedConversation(const QString& conversationId) {
    if (!conversations_.contains(conversationId)) return false;
    conversations_.remove(conversationId);
    conversationOrder_.removeAll(conversationId);
    messagesByConversation_.remove(conversationId);
    summaryMetadataByConversation_.remove(conversationId);
    setLastError(ConversationStoreErrorCode::None, {});
    return true;
}

int InMemoryConversationStore::pruneCompletedBefore(const QDateTime& cutoffUtc, int limit) {
    if (!cutoffUtc.isValid() || limit < 1 || limit > 100) return -1;
    QStringList ids;
    for (const auto& id : conversationOrder_) {
        const auto record = conversations_.value(id);
        if (record.pinned || record.updatedAtUtc >= cutoffUtc) continue;
        bool active = false;
        for (const auto& message : messagesByConversation_.value(id))
            if (message.status == ChatMessageStatus::Queued ||
                message.status == ChatMessageStatus::Sending ||
                message.status == ChatMessageStatus::Streaming ||
                message.status == ChatMessageStatus::Received) active = true;
        if (!active) ids.append(id);
        if (ids.size() == limit) break;
    }
    for (const auto& id : ids) discardImportedConversation(id);
    return ids.size();
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
