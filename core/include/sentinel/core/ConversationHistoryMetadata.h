#pragma once

#include <QString>

namespace sentinel::core {

enum class ConversationPersistenceStatus {
    Persisted,
    RuntimeOnly,
};

inline QString conversationPersistenceStatusName(ConversationPersistenceStatus status) {
    switch (status) {
    case ConversationPersistenceStatus::Persisted:
        return QStringLiteral("Persisted");
    case ConversationPersistenceStatus::RuntimeOnly:
        return QStringLiteral("Runtime Only");
    }

    return QStringLiteral("Runtime Only");
}

struct ConversationHistorySummary {
    int messageCount = 0;
    int userMessageCount = 0;
    int assistantMessageCount = 0;
    int systemMessageCount = 0;
    ConversationPersistenceStatus persistenceStatus = ConversationPersistenceStatus::RuntimeOnly;
    QString lastSavedStatus = QStringLiteral("Not saved");
    QString lastRestoredStatus = QStringLiteral("No persisted transcript restored");
    QString summary;
};

enum class ConversationClearStatus {
    ClearedPersisted,
    ClearedRuntimeOnly,
    Failed,
};

inline QString conversationClearStatusName(ConversationClearStatus status) {
    switch (status) {
    case ConversationClearStatus::ClearedPersisted:
        return QStringLiteral("Cleared Persisted");
    case ConversationClearStatus::ClearedRuntimeOnly:
        return QStringLiteral("Cleared Runtime Only");
    case ConversationClearStatus::Failed:
        return QStringLiteral("Failed");
    }

    return QStringLiteral("Failed");
}

struct ConversationClearResult {
    ConversationClearStatus status = ConversationClearStatus::Failed;
    int remainingMessageCount = 0;
    bool persistentStoreAvailable = false;
    bool persistentStoreCleared = false;
    QString summary = QStringLiteral("Chat clear has not run.");
};

} // namespace sentinel::core
