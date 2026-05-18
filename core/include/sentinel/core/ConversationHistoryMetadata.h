#pragma once

#include <QList>
#include <QString>
#include <QStringList>

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

struct ConversationSearchQuery {
    QString text;
    bool includeSystemMessages = true;
};

struct ConversationSearchResult {
    int messageId = 0;
    QString role;
    int matchIndex = -1;
    QString preview;
};

enum class ConversationSearchStatus {
    EmptyQuery,
    Completed,
};

inline QString conversationSearchStatusName(ConversationSearchStatus status) {
    switch (status) {
    case ConversationSearchStatus::EmptyQuery:
        return QStringLiteral("Empty Query");
    case ConversationSearchStatus::Completed:
        return QStringLiteral("Completed");
    }

    return QStringLiteral("Empty Query");
}

struct ConversationSearchSummary {
    ConversationSearchQuery query;
    ConversationSearchStatus status = ConversationSearchStatus::EmptyQuery;
    int transcriptMessageCount = 0;
    int resultCount = 0;
    QList<ConversationSearchResult> results;
    QString summary = QStringLiteral("No transcript search query entered.");
};

enum class ConversationExportFormat {
    PlainText,
    Markdown,
    Json,
};

inline QString conversationExportFormatName(ConversationExportFormat format) {
    switch (format) {
    case ConversationExportFormat::PlainText:
        return QStringLiteral("Plain Text");
    case ConversationExportFormat::Markdown:
        return QStringLiteral("Markdown");
    case ConversationExportFormat::Json:
        return QStringLiteral("JSON");
    }

    return QStringLiteral("Plain Text");
}

struct ConversationExportRequest {
    ConversationExportFormat format = ConversationExportFormat::PlainText;
    bool includeSystemMessages = true;
};

struct ConversationExportReadiness {
    bool available = true;
    bool writesFiles = true;
    QString status = QStringLiteral("Ready");
    QString summary =
        QStringLiteral("Current transcript export is available for Markdown and JSON.");
    QStringList checks{
        QStringLiteral("Scope: Current transcript only"),
        QStringLiteral("Output: App-controlled export directory"),
        QStringLiteral("Formats: Markdown, JSON"),
        QStringLiteral("Import: Disabled"),
    };
};

struct ConversationExportResult {
    ConversationExportRequest request;
    bool success = false;
    bool wroteFile = false;
    QString status = QStringLiteral("Not Run");
    QString outputPath;
    QString outputFileName;
    int messageCount = 0;
    QString exportedAtUtc;
    QString errorSummary;
    QString refusalSummary;
    QString summary = QStringLiteral("Transcript export has not run.");
};

} // namespace sentinel::core
