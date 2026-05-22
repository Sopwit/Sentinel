#pragma once

#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

enum class ContextAssemblyStatus : std::uint8_t {
    NotPlanned,
    Disabled,
    Empty,
    Ready,
};

enum class ContextAssemblySourceKind : std::uint8_t {
    Conversation,
    ConversationSummary,
    CommittedMemory,
    RuntimeMetadata,
    Orchestration,
    SelectedConversationMetadata,
};

using ContextSourceKind = ContextAssemblySourceKind;

enum class ContextExclusionReason : std::uint8_t {
    None,
    EmptyCandidate,
    SourceDisabled,
    DuplicateCandidate,
    BudgetExhausted,
    SourceCountLimit,
    CandidateCountLimit,
    NotRelevant,
};

using ContextCandidateReason = ContextExclusionReason;

enum class PromptContextInjectionStatus : std::uint8_t {
    Disabled,
    Empty,
    Injected,
    Truncated,
};

enum class ConversationWindowStatus : std::uint8_t {
    Disabled,
    Empty,
    Ready,
    Truncated,
};

enum class ConversationSummaryStatus : std::uint8_t {
    Disabled,
    Empty,
    Ready,
    Truncated,
};

enum class RetrievalPlanningStatus : std::uint8_t {
    NotPlanned,
    Disabled,
    Empty,
    Ready,
    Truncated,
};

enum class RetrievalSourcePriority : std::uint8_t {
    None = 0,
    RecentConversation = 1,
    ConversationSummary = 2,
    CommittedMemory = 3,
    RuntimeMetadata = 4,
    Orchestration = 5,
    SelectedConversationMetadata = 6,
};

QString contextAssemblyStatusName(ContextAssemblyStatus status);
QString contextAssemblySourceKindName(ContextAssemblySourceKind kind);
QString promptContextInjectionStatusName(PromptContextInjectionStatus status);
QString conversationWindowStatusName(ConversationWindowStatus status);
QString conversationSummaryStatusName(ConversationSummaryStatus status);
QString retrievalPlanningStatusName(RetrievalPlanningStatus status);
QString retrievalSourcePriorityName(RetrievalSourcePriority priority);
RetrievalSourcePriority retrievalSourcePriorityForKind(ContextAssemblySourceKind kind);

struct ContextAssemblyPolicy {
    bool enabled = true;
    bool includeConversationContext = true;
    bool includeCommittedMemoryContext = true;
    bool includeRuntimeMetadataContext = true;
    bool includeOrchestrationContext = true;
    bool includeSelectedConversationMetadata = true;
    bool promptAssemblyEnabled = false;
    bool automaticAttachmentEnabled = false;
    QString status = QStringLiteral("Planning Only");
    QString summary = QStringLiteral(
        "Context assembly planning estimates source participation without prompt injection.");
};

struct ContextAssemblyRequest {
    QString requestId = QStringLiteral("context-assembly-readiness");
    QString purpose = QStringLiteral("Readiness planning");
    bool includeConversationContext = true;
    bool includeCommittedMemoryContext = true;
    bool includeRuntimeMetadataContext = true;
    bool includeOrchestrationContext = true;
    bool includeSelectedConversationMetadata = true;
};

struct ContextAssemblySource {
    ContextAssemblySourceKind kind = ContextAssemblySourceKind::Conversation;
    bool requested = true;
    bool available = false;
    int blockCount = 0;
    int estimatedSize = 0;
    QString status = QStringLiteral("Unavailable");
    QString summary;
};

struct ContextAssemblyResult {
    ContextAssemblyStatus status = ContextAssemblyStatus::NotPlanned;
    ContextAssemblyRequest request;
    QList<ContextAssemblySource> sources;
    int requestedSourceCount = 0;
    int availableSourceCount = 0;
    int candidateBlockCount = 0;
    int estimatedSize = 0;
    QString summary = QStringLiteral("No context assembly plan has been generated.");
    QStringList checks;
};

using ContextAssemblySummary = ContextAssemblyResult;

struct PromptContextBlock {
    ContextAssemblySourceKind source = ContextAssemblySourceKind::Conversation;
    QString title;
    QString content;
    int originalSize = 0;
    int injectedSize = 0;
    bool truncated = false;
};

struct PromptContextBundle {
    QList<PromptContextBlock> blocks;
    int originalSize = 0;
    int injectedSize = 0;
    bool truncated = false;
    QString text;
    QString summary = QStringLiteral("No prompt context has been assembled.");
};

struct PromptContextInjectionPolicy {
    bool enabled = false;
    int maxCharacters = 3200;
    QString delimiterStart = QStringLiteral("[Sentinel Local Context]");
    QString delimiterEnd = QStringLiteral("[/Sentinel Local Context]");
    QString status = QStringLiteral("Disabled");
    QString summary = QStringLiteral(
        "Prompt context injection is disabled until explicitly enabled by the user.");
};

struct PromptContextInjectionResult {
    PromptContextInjectionStatus status = PromptContextInjectionStatus::Disabled;
    PromptContextInjectionPolicy policy;
    PromptContextBundle bundle;
    QString originalPrompt;
    QString prompt;
    int injectedBlockCount = 0;
    int injectedCharacterCount = 0;
    QString sourceSummary = QStringLiteral("No context sources injected.");
    QString sizeSummary = QStringLiteral("0 context characters injected.");
    QString summary = QStringLiteral("Prompt context injection is disabled.");
};

struct ConversationWindowBudget {
    int maxCharacters = 1200;
    int estimatedCharacters = 0;
    int includedCharacters = 0;
    int remainingCharacters = 1200;
    QString summary = QStringLiteral("0 of 1200 conversation characters included.");
};

struct ConversationWindowPolicy {
    bool enabled = true;
    int maxCharacters = 1200;
    bool includeSystemMessages = true;
    QString delimiterStart = QStringLiteral("[Conversation History]");
    QString delimiterEnd = QStringLiteral("[/Conversation History]");
    QString status = QStringLiteral("Ready");
    QString summary =
        QStringLiteral("Recent conversation messages are included within a deterministic "
                       "character budget.");
};

struct ConversationWindowMessage {
    int originalIndex = 0;
    QString role;
    QString content;
    int originalSize = 0;
    int includedSize = 0;
    bool truncated = false;
};

struct ConversationWindowSummary {
    ConversationWindowStatus status = ConversationWindowStatus::Empty;
    ConversationWindowBudget budget;
    int totalMessageCount = 0;
    int candidateMessageCount = 0;
    int includedMessageCount = 0;
    int omittedMessageCount = 0;
    int truncatedMessageCount = 0;
    QString summary = QStringLiteral("No conversation history is included.");
    QStringList messageSummaries;
};

struct ConversationWindowResult {
    ConversationWindowStatus status = ConversationWindowStatus::Empty;
    ConversationWindowPolicy policy;
    ConversationWindowBudget budget;
    ConversationWindowSummary summary;
    QList<ConversationWindowMessage> messages;
    QString text;
};

struct ConversationSummaryBudget {
    int maxCharacters = 700;
    int estimatedCharacters = 0;
    int includedCharacters = 0;
    int remainingCharacters = 700;
    int blockCount = 0;
    int truncatedBlockCount = 0;
    QString summary = QStringLiteral("0 of 700 summary characters included.");
};

struct ConversationSummaryPolicy {
    bool enabled = true;
    int maxCharacters = 700;
    int maxMessagesPerBlock = 4;
    QString delimiterStart = QStringLiteral("[Conversation Summary]");
    QString delimiterEnd = QStringLiteral("[/Conversation Summary]");
    QString status = QStringLiteral("Ready");
    QString summary = QStringLiteral(
        "Older omitted conversation history may be compacted with deterministic local heuristics.");
};

struct ConversationSummaryBlock {
    int blockIndex = 0;
    int firstOriginalIndex = 0;
    int lastOriginalIndex = 0;
    int messageCount = 0;
    int originalSize = 0;
    int includedSize = 0;
    bool truncated = false;
    QString text;
    QStringList roleSummaries;
};

struct ConversationSummaryWindow {
    int totalMessageCount = 0;
    int summarizedMessageCount = 0;
    int omittedFromSummaryCount = 0;
    int blockCount = 0;
    QString summary = QStringLiteral("No older conversation messages are summarized.");
};

struct ConversationSummaryResult {
    ConversationSummaryStatus status = ConversationSummaryStatus::Empty;
    ConversationSummaryPolicy policy;
    ConversationSummaryBudget budget;
    ConversationSummaryWindow window;
    QList<ConversationSummaryBlock> blocks;
    QString text;
    QString summary = QStringLiteral("No conversation summary is available.");
};

struct RetrievalPlanningPolicy {
    bool enabled = true;
    int maxCharacters = 3200;
    int maxCandidates = 8;
    int maxSources = 6;
    bool includeRecentConversation = true;
    bool includeConversationSummary = true;
    bool includeCommittedMemory = true;
    bool includeRuntimeMetadata = true;
    bool includeOrchestration = true;
    bool includeSelectedConversationMetadata = true;
    QString status = QStringLiteral("Ready");
    QString summary = QStringLiteral(
        "Deterministic retrieval planning selects local context sources without semantic search.");
};

using ContextSelectionPolicy = RetrievalPlanningPolicy;

struct RetrievalBudget {
    int maxCharacters = 3200;
    int estimatedCharacters = 0;
    int allocatedCharacters = 0;
    int includedCharacters = 0;
    int remainingCharacters = 3200;
    QString summary = QStringLiteral("0 of 3200 retrieval characters selected.");
};

using ContextBudget = RetrievalBudget;
using ContextBudgetUsage = RetrievalBudget;

struct RetrievalCandidate {
    ContextAssemblySourceKind source = ContextAssemblySourceKind::Conversation;
    RetrievalSourcePriority priority = RetrievalSourcePriority::RecentConversation;
    QString title;
    QString content;
    int originalSize = 0;
    int selectedSize = 0;
    bool selected = false;
    bool truncated = false;
    QString exclusionReason;
};

using ContextCandidate = RetrievalCandidate;

struct RetrievalSelectionSummary {
    ContextAssemblySourceKind source = ContextAssemblySourceKind::Conversation;
    RetrievalSourcePriority priority = RetrievalSourcePriority::RecentConversation;
    int candidateCount = 0;
    int selectedCount = 0;
    int excludedCount = 0;
    int truncatedCount = 0;
    int estimatedCharacters = 0;
    int allocatedCharacters = 0;
    int includedCharacters = 0;
    QString summary = QStringLiteral("No retrieval candidates selected.");
};

struct RetrievalPlanningResult {
    RetrievalPlanningStatus status = RetrievalPlanningStatus::NotPlanned;
    RetrievalPlanningPolicy policy;
    RetrievalBudget budget;
    QList<RetrievalCandidate> candidates;
    QList<RetrievalCandidate> selectedCandidates;
    QList<RetrievalSelectionSummary> sourceSummaries;
    int candidateCount = 0;
    int selectedCandidateCount = 0;
    int excludedCandidateCount = 0;
    int truncatedCandidateCount = 0;
    int selectedSourceCount = 0;
    int excludedSourceCount = 0;
    QString sourceSummary = QStringLiteral("No retrieval sources selected.");
    QString budgetSummary = QStringLiteral("0 retrieval characters selected.");
    QString summary = QStringLiteral("No retrieval planning has been generated.");
    QStringList checks;
};

using ContextSelectionResult = RetrievalPlanningResult;

struct ContextAssemblyTrace {
    QStringList includedSummaries;
    QStringList excludedSummaries;
    QString summary = QStringLiteral("No context assembly trace has been generated.");
};

QString contextExclusionReasonName(ContextExclusionReason reason);

ContextAssemblySource makeContextAssemblySource(ContextAssemblySourceKind kind, bool requested,
                                                bool available, int blockCount, int estimatedSize,
                                                const QString& summary);
ContextAssemblySummary contextAssemblySummaryForRequest(const ContextAssemblyRequest& request,
                                                        const QList<ContextAssemblySource>& sources,
                                                        const ContextAssemblyPolicy& policy);
QStringList contextAssemblySourceSummaries(const ContextAssemblySummary& summary);
ConversationWindowResult
assembleConversationWindow(const QList<ConversationWindowMessage>& messages,
                           const ConversationWindowPolicy& policy);
ConversationSummaryResult
assembleConversationSummary(const QList<ConversationWindowMessage>& messages,
                            const QList<ConversationWindowMessage>& recentWindowMessages,
                            const ConversationSummaryPolicy& policy);
PromptContextInjectionResult injectPromptContext(const QString& prompt,
                                                 const QList<PromptContextBlock>& candidateBlocks,
                                                 const PromptContextInjectionPolicy& policy);
QStringList promptContextBlockSummaries(const PromptContextInjectionResult& result);
QStringList conversationSummaryBlockSummaries(const ConversationSummaryResult& result);
RetrievalPlanningResult planRetrieval(const QList<RetrievalCandidate>& candidates,
                                      const RetrievalPlanningPolicy& policy);
QStringList retrievalSourceSummaries(const RetrievalPlanningResult& result);
QStringList retrievalCandidateTraceSummaries(const RetrievalPlanningResult& result);

} // namespace sentinel::core
