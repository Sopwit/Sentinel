#pragma once

#include <QDateTime>
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
    Planned,
    Blocked,
};

enum class RetrievalPlanningStatus : std::uint8_t {
    NotPlanned,
    Disabled,
    Empty,
    Ready,
    Truncated,
};

enum class ConversationCompressionStatus : std::uint8_t {
    Disabled,
    NotNeeded,
    Approaching,
    Needed,
    Planned,
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
QString conversationCompressionStatusName(ConversationCompressionStatus status);
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
    bool manualOnly = true;
    bool localOnly = true;
    bool generationAvailable = false;
    bool backgroundGenerationAllowed = false;
    bool transcriptMutationAllowed = false;
    bool automaticMemoryWriteAllowed = false;
    bool toolsAllowed = false;
    bool filesystemAuthorityAllowed = false;
    int maxCharacters = 700;
    int maxMessagesPerBlock = 4;
    int maxSegments = 5;
    QString delimiterStart = QStringLiteral("[Conversation Summary]");
    QString delimiterEnd = QStringLiteral("[/Conversation Summary]");
    QString status = QStringLiteral("Ready");
    QString summary = QStringLiteral(
        "Older omitted conversation history may be compacted with deterministic local heuristics.");
};

struct ConversationSummaryRequest {
    QString requestId = QStringLiteral("conversation-summary-manual-request");
    QString sourceConversationId;
    QString activeConversationId;
    bool explicitUserAction = false;
    bool backgroundRequested = false;
    bool mutateTranscript = false;
    bool writeCommittedMemory = false;
    bool includeRuntimeMetadata = false;
    bool exposeHiddenPrompt = false;
    QDateTime requestedAtUtc;
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

struct ConversationSummarySegment {
    QString kind;
    QString title;
    QString summary;
    int firstOriginalIndex = 0;
    int lastOriginalIndex = 0;
    int messageCount = 0;
    int estimatedCharacters = 0;
    int selectedCharacters = 0;
    bool selected = false;
    bool excluded = false;
    QString exclusionReason;
};

struct ConversationSummaryTrace {
    QStringList planningSummaries;
    QStringList safetySummaries;
    QString summary = QStringLiteral("No conversation summary generation trace has been prepared.");
};

struct ConversationSummaryFallback {
    QString reason;
    QString summary = QStringLiteral("No conversation summary fallback required.");
};

struct ConversationSummaryPreview {
    bool available = false;
    QString summary = QStringLiteral("No summary preview is available.");
    int estimatedReductionPercent = 0;
    int sourceCharacterCount = 0;
    int previewCharacterCount = 0;
};

struct ConversationSummaryReadiness {
    ConversationSummaryStatus status = ConversationSummaryStatus::Blocked;
    bool available = false;
    QString blockedReason = QStringLiteral("Manual summary generation is unavailable.");
    bool manualActionRequired = true;
    bool activeConversationOnly = true;
    bool localOnly = true;
    bool backgroundAllowed = false;
    bool transcriptMutationAllowed = false;
    bool memoryWriteAllowed = false;
    bool toolsAllowed = false;
    bool filesystemAuthorityAllowed = false;
    bool hiddenPromptExposureAllowed = false;
    QString summary = QStringLiteral("Manual summary generation is unavailable.");
};

struct ConversationSummaryResult {
    ConversationSummaryStatus status = ConversationSummaryStatus::Empty;
    ConversationSummaryPolicy policy;
    ConversationSummaryRequest request;
    ConversationSummaryBudget budget;
    ConversationSummaryWindow window;
    QList<ConversationSummaryBlock> blocks;
    QList<ConversationSummarySegment> segments;
    ConversationSummaryTrace trace;
    ConversationSummaryFallback fallback;
    ConversationSummaryPreview preview;
    ConversationSummaryReadiness readiness;
    QString sourceConversationId;
    int coveredFirstMessageIndex = 0;
    int coveredLastMessageIndex = 0;
    int estimatedReductionPercent = 0;
    QDateTime summaryTimestampUtc;
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

struct MemoryRelevancePolicy {
    bool enabled = true;
    int maxCharacters = 1200;
    int maxCandidates = 6;
    bool includePinnedWithoutOverlap = true;
    QString status = QStringLiteral("Ready");
    QString summary = QStringLiteral(
        "Deterministic memory relevance uses literal local overlap only.");
};

struct MemoryRelevanceCandidate {
    QString key;
    QString value;
    int originalIndex = 0;
    bool committed = true;
    bool pinned = false;
};

struct MemoryRelevanceScore {
    int total = 0;
    int keyOverlap = 0;
    int valueOverlap = 0;
    int activeConversationTitleOverlap = 0;
    int recentConversationTermOverlap = 0;
    int committedPriority = 0;
    int pinnedPriority = 0;
};

struct MemoryRelevanceReason {
    QStringList includedReasons;
    QString exclusionReason;
    QString summary = QStringLiteral("No deterministic relevance.");
};

struct MemoryRelevanceBudget {
    int maxCharacters = 1200;
    int estimatedCharacters = 0;
    int includedCharacters = 0;
    int remainingCharacters = 1200;
    QString summary = QStringLiteral("0 of 1200 memory context characters selected.");
};

struct MemoryRelevanceSelection {
    MemoryRelevanceCandidate candidate;
    MemoryRelevanceScore score;
    MemoryRelevanceReason reason;
    bool included = false;
    bool duplicate = false;
    bool truncated = false;
    int selectedCharacters = 0;
    QString selectedText;
};

struct MemoryRelevanceTrace {
    QStringList includedSummaries;
    QStringList excludedSummaries;
    QString summary = QStringLiteral("No memory relevance trace has been generated.");
};

struct MemoryRelevanceSummary {
    MemoryRelevancePolicy policy;
    MemoryRelevanceBudget budget;
    QList<MemoryRelevanceSelection> selections;
    int candidateCount = 0;
    int includedCount = 0;
    int excludedCount = 0;
    int duplicateCount = 0;
    int truncatedCount = 0;
    QString summary = QStringLiteral("No memory relevance has been evaluated.");
    MemoryRelevanceTrace trace;
};

struct ConversationSaliencePolicy {
    bool enabled = true;
    int maxCharacters = 3200;
    int maxCandidates = 8;
    int activeConversationBudgetPercent = 65;
    int committedMemoryBudgetPercent = 30;
    int runtimeMetadataBudgetPercent = 5;
    QString status = QStringLiteral("Ready");
    QString summary = QStringLiteral(
        "Adaptive deterministic salience budgets local context without semantic/vector authority.");
};

struct ConversationSalienceCandidate {
    ContextAssemblySourceKind source = ContextAssemblySourceKind::Conversation;
    QString title;
    QString content;
    int originalIndex = 0;
    int originalSize = 0;
    bool pinned = false;
    bool committedMemory = false;
    int recencyRank = 0;
};

struct ConversationSalienceScore {
    int total = 0;
    int activeConversationTitleOverlap = 0;
    int recentUserMessageOverlap = 0;
    int recentAssistantMessageOverlap = 0;
    int pinnedConversationPriority = 0;
    int committedMemoryOverlap = 0;
    int explicitQueryTermOverlap = 0;
    int recencyWeight = 0;
};

struct ConversationSalienceReason {
    QStringList includedReasons;
    QString exclusionReason;
    QString summary = QStringLiteral("No deterministic salience.");
};

struct ConversationSalienceBudget {
    int maxCharacters = 3200;
    int activeConversationBudget = 2080;
    int committedMemoryBudget = 960;
    int runtimeMetadataBudget = 160;
    int estimatedCharacters = 0;
    int includedCharacters = 0;
    int activeConversationCharacters = 0;
    int committedMemoryCharacters = 0;
    int runtimeMetadataCharacters = 0;
    int remainingCharacters = 3200;
    QString allocationSummary =
        QStringLiteral("Active conversation 2080 chars / committed memory 960 chars / runtime "
                       "metadata 160 chars.");
    QString summary = QStringLiteral("0 of 3200 salience context characters selected.");
};

struct ConversationSalienceSelection {
    ConversationSalienceCandidate candidate;
    ConversationSalienceScore score;
    ConversationSalienceReason reason;
    bool included = false;
    bool duplicate = false;
    bool truncated = false;
    int selectedCharacters = 0;
    QString selectedText;
};

struct ConversationSalienceTrace {
    QStringList includedSummaries;
    QStringList excludedSummaries;
    QString summary = QStringLiteral("No conversation salience trace has been generated.");
};

struct ConversationSalienceSummary {
    ConversationSaliencePolicy policy;
    ConversationSalienceBudget budget;
    QList<ConversationSalienceSelection> selections;
    int candidateCount = 0;
    int includedCount = 0;
    int excludedCount = 0;
    int duplicateCount = 0;
    int truncatedCount = 0;
    QString summary = QStringLiteral("No conversation salience has been evaluated.");
    ConversationSalienceTrace trace;
};

struct ConversationCompressionPolicy {
    bool enabled = true;
    int messageWarningThreshold = 18;
    int messageRequiredThreshold = 28;
    int characterWarningThreshold = 9000;
    int characterRequiredThreshold = 14000;
    int tokenWarningThreshold = 2250;
    int tokenRequiredThreshold = 3500;
    int activeConversationWarningThreshold = 8000;
    int activeConversationRequiredThreshold = 12000;
    int saliencePressureWarningThreshold = 70;
    int saliencePressureRequiredThreshold = 90;
    int maxCandidateCharacters = 2400;
    int maxCandidates = 6;
    QString status = QStringLiteral("Ready");
    QString summary = QStringLiteral(
        "Conversation compression readiness is deterministic metadata only.");
};

struct ConversationCompressionCandidate {
    QString kind;
    QString title;
    QString summary;
    int originalIndex = 0;
    int messageCount = 0;
    int estimatedCharacters = 0;
    int estimatedTokens = 0;
    int selectedCharacters = 0;
    bool selected = false;
    bool excluded = false;
    QString exclusionReason;
};

struct ConversationCompressionBudget {
    int maxCandidateCharacters = 2400;
    int estimatedCharacters = 0;
    int estimatedTokens = 0;
    int selectedCharacters = 0;
    int remainingCharacters = 2400;
    int pressurePercent = 0;
    int saliencePressurePercent = 0;
    QString summary = QStringLiteral("0 compression candidate characters selected.");
};

struct ConversationCompressionReadiness {
    ConversationCompressionStatus status = ConversationCompressionStatus::NotNeeded;
    bool compressionUseful = false;
    int messageCount = 0;
    int estimatedCharacters = 0;
    int estimatedTokens = 0;
    int activeConversationCharacters = 0;
    bool contextInjectionEnabled = false;
    bool existingSummaryAvailable = false;
    int saliencePressurePercent = 0;
    int pressurePercent = 0;
    QString summary = QStringLiteral("Conversation compression is not needed.");
};

struct ConversationCompressionSelection {
    QList<ConversationCompressionCandidate> candidates;
    int candidateCount = 0;
    int selectedCandidateCount = 0;
    int excludedCandidateCount = 0;
    QString summary = QStringLiteral("No compression candidates selected.");
};

struct ConversationCompressionTrace {
    QStringList candidateSummaries;
    QStringList selectionSummaries;
    QString summary = QStringLiteral("No conversation compression trace has been generated.");
};

struct ConversationCompressionFallback {
    QString reason;
    QString summary = QStringLiteral("No compression fallback required.");
};

struct ConversationCompressionSummary {
    ConversationCompressionPolicy policy;
    ConversationCompressionStatus status = ConversationCompressionStatus::NotNeeded;
    ConversationCompressionReadiness readiness;
    ConversationCompressionBudget budget;
    ConversationCompressionSelection selection;
    ConversationCompressionTrace trace;
    ConversationCompressionFallback fallback;
    QString summary = QStringLiteral("Conversation compression is not needed.");
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
MemoryRelevanceSummary rankMemoryRelevance(
    const QList<MemoryRelevanceCandidate>& candidates, const QString& prompt,
    const QString& activeConversationTitle, const QString& recentConversationText,
    const MemoryRelevancePolicy& policy);
QStringList memoryRelevanceTraceSummaries(const MemoryRelevanceSummary& summary);
QStringList memoryRelevanceExclusionSummaries(const MemoryRelevanceSummary& summary);
ConversationSalienceSummary rankConversationSalience(
    const QList<ConversationSalienceCandidate>& candidates, const QString& explicitUserQuery,
    const QString& activeConversationTitle, const QString& recentUserMessages,
    const QString& recentAssistantMessages, const QString& committedMemoryText,
    const ConversationSaliencePolicy& policy);
QStringList conversationSalienceTraceSummaries(const ConversationSalienceSummary& summary);
QStringList conversationSalienceExclusionSummaries(const ConversationSalienceSummary& summary);
ConversationCompressionSummary planConversationCompression(
    const QList<ConversationWindowMessage>& messages,
    const ConversationSummaryResult& existingSummary,
    const ConversationSalienceSummary& salienceSummary, bool contextInjectionEnabled,
    const ConversationCompressionPolicy& policy);
ConversationSummaryResult planConversationSummaryGeneration(
    const QList<ConversationWindowMessage>& messages,
    const ConversationCompressionSummary& compressionSummary,
    const ConversationSummaryRequest& request, const ConversationSummaryPolicy& policy);
QStringList conversationCompressionCandidateSummaries(
    const ConversationCompressionSummary& summary);
QStringList conversationCompressionTraceSummaries(const ConversationCompressionSummary& summary);
QStringList conversationSummarySegmentSummaries(const ConversationSummaryResult& result);
QStringList conversationSummaryTraceSummaries(const ConversationSummaryResult& result);

} // namespace sentinel::core
