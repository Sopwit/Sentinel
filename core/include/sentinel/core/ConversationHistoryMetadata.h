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

enum class ConversationBrowserStatus {
    Ready,
    EmptyTranscript,
};

inline QString conversationBrowserStatusName(ConversationBrowserStatus status) {
    switch (status) {
    case ConversationBrowserStatus::Ready:
        return QStringLiteral("Ready");
    case ConversationBrowserStatus::EmptyTranscript:
        return QStringLiteral("Empty Transcript");
    }

    return QStringLiteral("Ready");
}

struct ConversationDisplayTitle {
    QString text = QStringLiteral("Current Transcript");
    QString summary = QStringLiteral("Single local transcript entry.");
};

struct ConversationListEntry {
    ConversationDisplayTitle displayTitle;
    int messageCount = 0;
    ConversationPersistenceStatus persistenceStatus = ConversationPersistenceStatus::RuntimeOnly;
    QString lastUpdatedSummary = QStringLiteral("No transcript update recorded.");
    QString searchAvailabilitySummary =
        QStringLiteral("Search is available for the current transcript.");
    QString exportAvailabilitySummary =
        QStringLiteral("Export is available for Markdown and JSON.");
    QString summary = QStringLiteral("Current transcript entry metadata is not available.");
};

struct ConversationListSummary {
    ConversationBrowserStatus status = ConversationBrowserStatus::Ready;
    int entryCount = 1;
    QList<ConversationListEntry> entries;
    QString summary = QStringLiteral("Single current transcript entry.");
};

struct ConversationId {
    QString value = QStringLiteral("current-transcript");
};

enum class ConversationLifecycleStatus {
    Active,
    Archived,
    Planned,
};

inline QString conversationLifecycleStatusName(ConversationLifecycleStatus status) {
    switch (status) {
    case ConversationLifecycleStatus::Active:
        return QStringLiteral("Active");
    case ConversationLifecycleStatus::Archived:
        return QStringLiteral("Archived");
    case ConversationLifecycleStatus::Planned:
        return QStringLiteral("Planned");
    }

    return QStringLiteral("Active");
}

enum class ConversationStorageMode {
    SingleTranscript,
    MultiConversation,
};

inline QString conversationStorageModeName(ConversationStorageMode mode) {
    switch (mode) {
    case ConversationStorageMode::SingleTranscript:
        return QStringLiteral("Single Transcript");
    case ConversationStorageMode::MultiConversation:
        return QStringLiteral("Multi Conversation");
    }

    return QStringLiteral("Single Transcript");
}

enum class ConversationMigrationReadiness {
    NotStarted,
    Planned,
};

inline QString conversationMigrationReadinessName(ConversationMigrationReadiness readiness) {
    switch (readiness) {
    case ConversationMigrationReadiness::NotStarted:
        return QStringLiteral("Not Started");
    case ConversationMigrationReadiness::Planned:
        return QStringLiteral("Planned");
    }

    return QStringLiteral("Not Started");
}

struct ConversationDescriptor {
    ConversationId id;
    ConversationDisplayTitle displayTitle;
    ConversationLifecycleStatus lifecycleStatus = ConversationLifecycleStatus::Active;
    ConversationStorageMode storageMode = ConversationStorageMode::SingleTranscript;
    QString summary = QStringLiteral("Current transcript descriptor metadata is not available.");
};

struct ConversationSchemaPlan {
    ConversationStorageMode currentStorageMode = ConversationStorageMode::SingleTranscript;
    ConversationStorageMode futureStorageMode = ConversationStorageMode::MultiConversation;
    ConversationMigrationReadiness migrationReadiness = ConversationMigrationReadiness::NotStarted;
    QString planningStatus = QStringLiteral("Planned");
    bool schemaMutationApplied = false;
    QString schemaStatusSummary =
        QStringLiteral("No conversation schema migration applied; single-transcript schema remains "
                       "active.");
    QString summary = QStringLiteral("Not Started / Planned");
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

enum class ConversationDeletePolicyStatus {
    DisabledByDefault,
};

inline QString conversationDeletePolicyStatusName(ConversationDeletePolicyStatus status) {
    switch (status) {
    case ConversationDeletePolicyStatus::DisabledByDefault:
        return QStringLiteral("Disabled By Default");
    }

    return QStringLiteral("Disabled By Default");
}

struct ConversationDeletePolicy {
    ConversationDeletePolicyStatus status = ConversationDeletePolicyStatus::DisabledByDefault;
    bool permanentDeleteEnabled = false;
    bool archiveFirst = true;
    QString summary =
        QStringLiteral("Archive-first policy active; permanent delete is disabled by default.");
    QStringList requirements{
        QStringLiteral("Archive remains the supported safe removal flow"),
        QStringLiteral("Permanent delete requires an explicit future phase gate"),
        QStringLiteral("Permanent delete requires destructive-mutation tests"),
        QStringLiteral("Permanent delete requires guarded UI confirmation"),
    };
};

struct ConversationDeleteReadiness {
    ConversationDeletePolicy policy;
    bool available = false;
    QString status = QStringLiteral("Disabled");
    QString summary =
        QStringLiteral("Permanent delete is disabled. Archive or unarchive conversations instead.");
    QStringList checks{
        QStringLiteral("Policy: Archive first"),
        QStringLiteral("Permanent delete: Disabled by default"),
        QStringLiteral("Storage mutation: Refused"),
        QStringLiteral("UI: No destructive delete control"),
    };
};

struct ConversationDeleteResult {
    bool accepted = false;
    bool mutatedStorage = false;
    QString status = QStringLiteral("Not Run");
    QString conversationId;
    QString refusalSummary;
    QString summary = QStringLiteral("Permanent delete has not been requested.");
};

} // namespace sentinel::core
