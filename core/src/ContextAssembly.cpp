#include "sentinel/core/ContextAssembly.h"

#include <algorithm>
#include <QRegularExpression>
#include <QSet>

namespace sentinel::core {

namespace {

int toInt(qsizetype value) {
    return static_cast<int>(value);
}

QStringList literalTokens(const QString& text) {
    QStringList tokens;
    const auto parts = text.toCaseFolded().split(QRegularExpression(QStringLiteral("[^a-z0-9]+")),
                                                 Qt::SkipEmptyParts);
    for (const auto& part : parts) {
        if (part.size() >= 3 && !tokens.contains(part)) {
            tokens.append(part);
        }
    }
    return tokens;
}

int overlapCount(const QStringList& left, const QStringList& right) {
    int count = 0;
    for (const auto& token : left) {
        if (right.contains(token)) {
            ++count;
        }
    }
    return count;
}

} // namespace

QString contextAssemblyStatusName(ContextAssemblyStatus status) {
    switch (status) {
    case ContextAssemblyStatus::NotPlanned:
        return QStringLiteral("Not Planned");
    case ContextAssemblyStatus::Disabled:
        return QStringLiteral("Disabled");
    case ContextAssemblyStatus::Empty:
        return QStringLiteral("Empty");
    case ContextAssemblyStatus::Ready:
        return QStringLiteral("Ready");
    }

    return QStringLiteral("Not Planned");
}

QString contextAssemblySourceKindName(ContextAssemblySourceKind kind) {
    switch (kind) {
    case ContextAssemblySourceKind::Conversation:
        return QStringLiteral("Conversation Context");
    case ContextAssemblySourceKind::ConversationSummary:
        return QStringLiteral("Conversation Summary Context");
    case ContextAssemblySourceKind::CommittedMemory:
        return QStringLiteral("Committed Memory Context");
    case ContextAssemblySourceKind::RuntimeMetadata:
        return QStringLiteral("Runtime Metadata Context");
    case ContextAssemblySourceKind::Orchestration:
        return QStringLiteral("Orchestration Context");
    case ContextAssemblySourceKind::SelectedConversationMetadata:
        return QStringLiteral("Selected Conversation Metadata");
    }

    return QStringLiteral("Conversation Context");
}

QString promptContextInjectionStatusName(PromptContextInjectionStatus status) {
    switch (status) {
    case PromptContextInjectionStatus::Disabled:
        return QStringLiteral("Disabled");
    case PromptContextInjectionStatus::Empty:
        return QStringLiteral("Empty");
    case PromptContextInjectionStatus::Injected:
        return QStringLiteral("Injected");
    case PromptContextInjectionStatus::Truncated:
        return QStringLiteral("Truncated");
    }

    return QStringLiteral("Disabled");
}

QString conversationWindowStatusName(ConversationWindowStatus status) {
    switch (status) {
    case ConversationWindowStatus::Disabled:
        return QStringLiteral("Disabled");
    case ConversationWindowStatus::Empty:
        return QStringLiteral("Empty");
    case ConversationWindowStatus::Ready:
        return QStringLiteral("Ready");
    case ConversationWindowStatus::Truncated:
        return QStringLiteral("Truncated");
    }

    return QStringLiteral("Empty");
}

QString conversationSummaryStatusName(ConversationSummaryStatus status) {
    switch (status) {
    case ConversationSummaryStatus::Disabled:
        return QStringLiteral("Disabled");
    case ConversationSummaryStatus::Empty:
        return QStringLiteral("Empty");
    case ConversationSummaryStatus::Ready:
        return QStringLiteral("Ready");
    case ConversationSummaryStatus::Truncated:
        return QStringLiteral("Truncated");
    case ConversationSummaryStatus::Planned:
        return QStringLiteral("Planned");
    case ConversationSummaryStatus::Blocked:
        return QStringLiteral("Blocked");
    }

    return QStringLiteral("Empty");
}

QString retrievalPlanningStatusName(RetrievalPlanningStatus status) {
    switch (status) {
    case RetrievalPlanningStatus::NotPlanned:
        return QStringLiteral("Not Planned");
    case RetrievalPlanningStatus::Disabled:
        return QStringLiteral("Disabled");
    case RetrievalPlanningStatus::Empty:
        return QStringLiteral("Empty");
    case RetrievalPlanningStatus::Ready:
        return QStringLiteral("Ready");
    case RetrievalPlanningStatus::Truncated:
        return QStringLiteral("Truncated");
    }

    return QStringLiteral("Not Planned");
}

QString conversationCompressionStatusName(ConversationCompressionStatus status) {
    switch (status) {
    case ConversationCompressionStatus::Disabled:
        return QStringLiteral("Disabled");
    case ConversationCompressionStatus::NotNeeded:
        return QStringLiteral("Not Needed");
    case ConversationCompressionStatus::Approaching:
        return QStringLiteral("Approaching");
    case ConversationCompressionStatus::Needed:
        return QStringLiteral("Needed");
    case ConversationCompressionStatus::Planned:
        return QStringLiteral("Planned");
    }

    return QStringLiteral("Not Needed");
}

QString retrievalSourcePriorityName(RetrievalSourcePriority priority) {
    switch (priority) {
    case RetrievalSourcePriority::None:
        return QStringLiteral("None");
    case RetrievalSourcePriority::RecentConversation:
        return QStringLiteral("1 Recent Conversation");
    case RetrievalSourcePriority::ConversationSummary:
        return QStringLiteral("2 Conversation Summary");
    case RetrievalSourcePriority::CommittedMemory:
        return QStringLiteral("3 Committed Memory");
    case RetrievalSourcePriority::RuntimeMetadata:
        return QStringLiteral("4 Runtime Metadata");
    case RetrievalSourcePriority::Orchestration:
        return QStringLiteral("5 Orchestration");
    case RetrievalSourcePriority::SelectedConversationMetadata:
        return QStringLiteral("6 Selected Conversation Metadata");
    }

    return QStringLiteral("1 Recent Conversation");
}

RetrievalSourcePriority retrievalSourcePriorityForKind(ContextAssemblySourceKind kind) {
    switch (kind) {
    case ContextAssemblySourceKind::Conversation:
        return RetrievalSourcePriority::RecentConversation;
    case ContextAssemblySourceKind::ConversationSummary:
        return RetrievalSourcePriority::ConversationSummary;
    case ContextAssemblySourceKind::CommittedMemory:
        return RetrievalSourcePriority::CommittedMemory;
    case ContextAssemblySourceKind::RuntimeMetadata:
        return RetrievalSourcePriority::RuntimeMetadata;
    case ContextAssemblySourceKind::Orchestration:
        return RetrievalSourcePriority::Orchestration;
    case ContextAssemblySourceKind::SelectedConversationMetadata:
        return RetrievalSourcePriority::SelectedConversationMetadata;
    }

    return RetrievalSourcePriority::RecentConversation;
}

QString contextExclusionReasonName(ContextExclusionReason reason) {
    switch (reason) {
    case ContextExclusionReason::None:
        return QStringLiteral("Included");
    case ContextExclusionReason::EmptyCandidate:
        return QStringLiteral("Empty candidate");
    case ContextExclusionReason::SourceDisabled:
        return QStringLiteral("Source disabled by policy");
    case ContextExclusionReason::DuplicateCandidate:
        return QStringLiteral("Duplicate candidate");
    case ContextExclusionReason::BudgetExhausted:
        return QStringLiteral("Retrieval budget exhausted");
    case ContextExclusionReason::SourceCountLimit:
        return QStringLiteral("Source count limit reached");
    case ContextExclusionReason::CandidateCountLimit:
        return QStringLiteral("Candidate count limit reached");
    case ContextExclusionReason::NotRelevant:
        return QStringLiteral("No deterministic literal relevance");
    }

    return QStringLiteral("Excluded");
}

ContextAssemblySource makeContextAssemblySource(ContextAssemblySourceKind kind, bool requested,
                                                bool available, int blockCount, int estimatedSize,
                                                const QString& summary) {
    ContextAssemblySource source;
    source.kind = kind;
    source.requested = requested;
    source.available = requested && available;
    source.blockCount = source.available ? blockCount : 0;
    source.estimatedSize = source.available ? estimatedSize : 0;
    source.status = !requested ? QStringLiteral("Not Requested")
                               : (source.available ? QStringLiteral("Available")
                                                   : QStringLiteral("Unavailable"));
    source.summary = QStringLiteral("%1: %2 - %3")
                         .arg(contextAssemblySourceKindName(kind), source.status,
                              summary.trimmed().isEmpty() ? QStringLiteral("No candidate blocks.")
                                                          : summary.simplified());
    return source;
}

ContextAssemblySummary contextAssemblySummaryForRequest(const ContextAssemblyRequest& request,
                                                        const QList<ContextAssemblySource>& sources,
                                                        const ContextAssemblyPolicy& policy) {
    ContextAssemblySummary summary;
    summary.request = request;
    summary.sources = sources;
    summary.checks.append(QStringLiteral("Boundary: planning metadata only"));
    summary.checks.append(QStringLiteral("Provider/model calls: disabled"));
    summary.checks.append(QStringLiteral("Prompt assembly: disabled"));
    summary.checks.append(QStringLiteral("Automatic context attachment: disabled"));
    summary.checks.append(QStringLiteral("Semantic ranking/vector search: disabled"));

    if (!policy.enabled) {
        summary.status = ContextAssemblyStatus::Disabled;
        summary.summary = policy.summary;
        summary.checks.append(QStringLiteral("Policy: disabled"));
        return summary;
    }

    for (const auto& source : sources) {
        if (!source.requested) {
            continue;
        }

        ++summary.requestedSourceCount;
        if (!source.available) {
            continue;
        }

        ++summary.availableSourceCount;
        summary.candidateBlockCount += source.blockCount;
        summary.estimatedSize += source.estimatedSize;
    }

    if (summary.availableSourceCount == 0 || summary.candidateBlockCount == 0) {
        summary.status = ContextAssemblyStatus::Empty;
        summary.summary =
            QStringLiteral("Context assembly planning found no available candidate context "
                           "blocks.");
        return summary;
    }

    summary.status = ContextAssemblyStatus::Ready;
    summary.summary =
        QStringLiteral("Context assembly planning found %1 available %2 with %3 candidate %4 "
                       "and about %5 characters.")
            .arg(summary.availableSourceCount)
            .arg(summary.availableSourceCount == 1 ? QStringLiteral("source")
                                                   : QStringLiteral("sources"))
            .arg(summary.candidateBlockCount)
            .arg(summary.candidateBlockCount == 1 ? QStringLiteral("block")
                                                  : QStringLiteral("blocks"))
            .arg(summary.estimatedSize);
    return summary;
}

QStringList contextAssemblySourceSummaries(const ContextAssemblySummary& summary) {
    QStringList results;
    results.reserve(summary.sources.size());
    for (const auto& source : summary.sources) {
        results.append(
            QStringLiteral("%1 / %2 / %3 %4 / ~%5 chars")
                .arg(source.summary)
                .arg(source.status)
                .arg(source.blockCount)
                .arg(source.blockCount == 1 ? QStringLiteral("block") : QStringLiteral("blocks"))
                .arg(source.estimatedSize));
    }
    return results;
}

ConversationWindowResult
assembleConversationWindow(const QList<ConversationWindowMessage>& messages,
                           const ConversationWindowPolicy& policy) {
    ConversationWindowResult result;
    result.policy = policy;
    result.budget.maxCharacters = std::max(0, policy.maxCharacters);
    result.budget.remainingCharacters = result.budget.maxCharacters;
    result.summary.budget = result.budget;
    result.summary.totalMessageCount = toInt(messages.size());

    if (!policy.enabled) {
        result.status = ConversationWindowStatus::Disabled;
        result.summary.status = result.status;
        result.summary.summary = QStringLiteral("Conversation window assembly is disabled.");
        return result;
    }

    QList<ConversationWindowMessage> candidates;
    candidates.reserve(messages.size());
    for (const auto& message : messages) {
        ConversationWindowMessage candidate = message;
        candidate.role = candidate.role.simplified();
        candidate.content = candidate.content.simplified();
        if (candidate.content.isEmpty()) {
            continue;
        }
        if (!policy.includeSystemMessages && candidate.role == QStringLiteral("system")) {
            continue;
        }
        candidate.originalSize =
            QStringLiteral("%1: %2").arg(candidate.role, candidate.content).size();
        candidate.includedSize = 0;
        candidate.truncated = false;
        candidates.append(candidate);
        result.budget.estimatedCharacters += candidate.originalSize;
    }

    result.summary.candidateMessageCount = toInt(candidates.size());
    int remaining = result.budget.maxCharacters;

    QList<ConversationWindowMessage> includedNewestFirst;
    includedNewestFirst.reserve(candidates.size());
    for (auto it = candidates.crbegin(); it != candidates.crend(); ++it) {
        if (remaining <= 0) {
            continue;
        }

        ConversationWindowMessage included = *it;
        const auto line = QStringLiteral("%1: %2").arg(included.role, included.content);
        if (line.size() > remaining) {
            const auto prefix = QStringLiteral("%1: ").arg(included.role);
            const auto contentBudget = std::max(0, remaining - static_cast<int>(prefix.size()));
            included.content = included.content.left(contentBudget).trimmed();
            included.truncated = true;
            ++result.summary.truncatedMessageCount;
        }

        const auto includedLine = QStringLiteral("%1: %2").arg(included.role, included.content);
        included.includedSize = toInt(includedLine.size());
        if (included.includedSize <= 0 || included.content.isEmpty()) {
            continue;
        }

        remaining -= included.includedSize;
        includedNewestFirst.append(included);
    }

    result.summary.includedMessageCount = toInt(includedNewestFirst.size());
    result.summary.omittedMessageCount =
        result.summary.candidateMessageCount - result.summary.includedMessageCount;

    for (auto it = includedNewestFirst.crbegin(); it != includedNewestFirst.crend(); ++it) {
        result.messages.append(*it);
    }

    result.budget.includedCharacters = result.budget.maxCharacters - std::max(0, remaining);
    result.budget.remainingCharacters = std::max(0, remaining);
    result.budget.summary =
        QStringLiteral("%1 of %2 estimated conversation characters included within %3 character "
                       "budget.")
            .arg(result.budget.includedCharacters)
            .arg(result.budget.estimatedCharacters)
            .arg(result.budget.maxCharacters);

    result.summary.budget = result.budget;
    result.summary.messageSummaries.reserve(result.messages.size());
    QStringList lines;
    for (const auto& message : result.messages) {
        lines.append(QStringLiteral("%1: %2").arg(message.role, message.content));
        result.summary.messageSummaries.append(
            QStringLiteral("%1 #%2 / %3 chars%4")
                .arg(message.role)
                .arg(message.originalIndex)
                .arg(message.includedSize)
                .arg(message.truncated ? QStringLiteral(" / truncated") : QStringLiteral("")));
    }

    if (result.summary.candidateMessageCount == 0) {
        result.status = ConversationWindowStatus::Empty;
        result.summary.status = result.status;
        result.summary.summary = QStringLiteral("No conversation messages are available for the "
                                                "bounded context window.");
        return result;
    }

    if (result.messages.isEmpty()) {
        result.status = ConversationWindowStatus::Truncated;
        result.summary.status = result.status;
        result.summary.summary =
            QStringLiteral("Conversation window budget omitted %1 available %2.")
                .arg(result.summary.candidateMessageCount)
                .arg(result.summary.candidateMessageCount == 1 ? QStringLiteral("message")
                                                               : QStringLiteral("messages"));
        return result;
    }

    result.text =
        QStringLiteral("%1\n%2\n%3")
            .arg(policy.delimiterStart, lines.join(QStringLiteral("\n")), policy.delimiterEnd);
    result.status =
        (result.summary.omittedMessageCount > 0 || result.summary.truncatedMessageCount > 0)
            ? ConversationWindowStatus::Truncated
            : ConversationWindowStatus::Ready;
    result.summary.status = result.status;
    result.summary.summary =
        QStringLiteral("Conversation window includes %1 of %2 candidate %3 with %4 truncated. %5")
            .arg(result.summary.includedMessageCount)
            .arg(result.summary.candidateMessageCount)
            .arg(result.summary.candidateMessageCount == 1 ? QStringLiteral("message")
                                                           : QStringLiteral("messages"))
            .arg(result.summary.truncatedMessageCount)
            .arg(result.budget.summary);
    return result;
}

ConversationSummaryResult
assembleConversationSummary(const QList<ConversationWindowMessage>& messages,
                            const QList<ConversationWindowMessage>& recentWindowMessages,
                            const ConversationSummaryPolicy& policy) {
    ConversationSummaryResult result;
    result.policy = policy;
    result.budget.maxCharacters = std::max(0, policy.maxCharacters);
    result.budget.remainingCharacters = result.budget.maxCharacters;
    result.window.totalMessageCount = toInt(messages.size());

    if (!policy.enabled) {
        result.status = ConversationSummaryStatus::Disabled;
        result.summary = QStringLiteral("Conversation summary assembly is disabled.");
        result.window.summary = result.summary;
        return result;
    }

    QList<int> recentIndexes;
    recentIndexes.reserve(recentWindowMessages.size());
    for (const auto& message : recentWindowMessages) {
        recentIndexes.append(message.originalIndex);
    }

    QList<ConversationWindowMessage> candidates;
    candidates.reserve(messages.size());
    for (const auto& message : messages) {
        ConversationWindowMessage candidate = message;
        candidate.role = candidate.role.simplified();
        candidate.content = candidate.content.simplified();
        if (candidate.content.isEmpty() || recentIndexes.contains(candidate.originalIndex)) {
            continue;
        }
        candidate.originalSize = QStringLiteral("#%1 %2: %3")
                                     .arg(candidate.originalIndex)
                                     .arg(candidate.role, candidate.content)
                                     .size();
        candidates.append(candidate);
        result.budget.estimatedCharacters += candidate.originalSize;
    }

    if (candidates.isEmpty()) {
        result.status = ConversationSummaryStatus::Empty;
        result.summary = QStringLiteral("No older omitted conversation messages require summary.");
        result.window.summary = result.summary;
        result.budget.summary =
            QStringLiteral("0 of %1 estimated summary characters included within %2 character "
                           "budget.")
                .arg(result.budget.estimatedCharacters)
                .arg(result.budget.maxCharacters);
        return result;
    }

    const auto messagesPerBlock = std::max(1, policy.maxMessagesPerBlock);
    int remaining = result.budget.maxCharacters;
    QStringList blockTexts;
    int blockIndex = 1;

    for (int start = 0; start < candidates.size() && remaining > 0; start += messagesPerBlock) {
        const auto end = std::min(start + messagesPerBlock, static_cast<int>(candidates.size()));
        const auto firstOriginalIndex = candidates.at(start).originalIndex;
        const auto lastOriginalIndex = candidates.at(end - 1).originalIndex;

        QStringList lines;
        QStringList roleSummaries;
        int userCount = 0;
        int assistantCount = 0;
        int systemCount = 0;

        for (int i = start; i < end; ++i) {
            const auto& message = candidates.at(i);
            if (message.role == QStringLiteral("user")) {
                ++userCount;
            } else if (message.role == QStringLiteral("assistant")) {
                ++assistantCount;
            } else if (message.role == QStringLiteral("system")) {
                ++systemCount;
            }
            lines.append(QStringLiteral("- #%1 %2: %3")
                             .arg(message.originalIndex)
                             .arg(message.role, message.content));
        }

        if (systemCount > 0) {
            roleSummaries.append(QStringLiteral("%1 system").arg(systemCount));
        }
        if (userCount > 0) {
            roleSummaries.append(QStringLiteral("%1 user").arg(userCount));
        }
        if (assistantCount > 0) {
            roleSummaries.append(QStringLiteral("%1 assistant").arg(assistantCount));
        }

        ConversationSummaryBlock block;
        block.blockIndex = blockIndex++;
        block.firstOriginalIndex = firstOriginalIndex;
        block.lastOriginalIndex = lastOriginalIndex;
        block.messageCount = end - start;
        block.roleSummaries = roleSummaries;
        block.text = QStringLiteral("Summary block %1 (messages #%2-#%3, %4): %5\n%6")
                         .arg(block.blockIndex)
                         .arg(block.firstOriginalIndex)
                         .arg(block.lastOriginalIndex)
                         .arg(block.messageCount)
                         .arg(roleSummaries.isEmpty() ? QStringLiteral("roles unavailable")
                                                      : roleSummaries.join(QStringLiteral(", ")))
                         .arg(lines.join(QStringLiteral("\n")));
        block.originalSize = toInt(block.text.size());

        if (block.text.size() > remaining) {
            block.text = block.text.left(remaining).trimmed();
            block.truncated = true;
            ++result.budget.truncatedBlockCount;
        }

        block.includedSize = toInt(block.text.size());
        if (block.includedSize <= 0) {
            break;
        }

        remaining -= block.includedSize;
        result.blocks.append(block);
        blockTexts.append(block.text);
        result.budget.includedCharacters += block.includedSize;
        result.window.summarizedMessageCount += block.messageCount;
    }

    result.window.blockCount = toInt(result.blocks.size());
    result.window.omittedFromSummaryCount =
        toInt(candidates.size()) - result.window.summarizedMessageCount;
    result.budget.remainingCharacters = std::max(0, remaining);
    result.budget.blockCount = toInt(result.blocks.size());
    result.budget.summary =
        QStringLiteral("%1 of %2 estimated summary characters included within %3 character "
                       "budget across %4 %5.")
            .arg(result.budget.includedCharacters)
            .arg(result.budget.estimatedCharacters)
            .arg(result.budget.maxCharacters)
            .arg(result.budget.blockCount)
            .arg(result.budget.blockCount == 1 ? QStringLiteral("block")
                                               : QStringLiteral("blocks"));

    if (result.blocks.isEmpty()) {
        result.status = ConversationSummaryStatus::Truncated;
        result.summary = QStringLiteral("Conversation summary budget omitted %1 older %2.")
                             .arg(candidates.size())
                             .arg(candidates.size() == 1 ? QStringLiteral("message")
                                                         : QStringLiteral("messages"));
        result.window.summary = result.summary;
        return result;
    }

    result.text =
        QStringLiteral("%1\n%2\n%3")
            .arg(policy.delimiterStart, blockTexts.join(QStringLiteral("\n")), policy.delimiterEnd);
    result.status =
        (result.window.omittedFromSummaryCount > 0 || result.budget.truncatedBlockCount > 0)
            ? ConversationSummaryStatus::Truncated
            : ConversationSummaryStatus::Ready;
    result.window.summary =
        QStringLiteral("Conversation summary covers %1 older %2 in %3 %4; %5 omitted. %6")
            .arg(result.window.summarizedMessageCount)
            .arg(result.window.summarizedMessageCount == 1 ? QStringLiteral("message")
                                                           : QStringLiteral("messages"))
            .arg(result.window.blockCount)
            .arg(result.window.blockCount == 1 ? QStringLiteral("block") : QStringLiteral("blocks"))
            .arg(result.window.omittedFromSummaryCount)
            .arg(result.budget.summary);
    result.summary = result.window.summary;
    return result;
}

PromptContextInjectionResult injectPromptContext(const QString& prompt,
                                                 const QList<PromptContextBlock>& candidateBlocks,
                                                 const PromptContextInjectionPolicy& policy) {
    PromptContextInjectionResult result;
    result.policy = policy;
    result.originalPrompt = prompt;
    result.prompt = prompt;

    if (!policy.enabled) {
        return result;
    }

    const auto budget = std::max(0, policy.maxCharacters);
    QStringList blockTexts;
    QStringList sourceNames;
    int remaining = budget;

    for (const auto& candidate : candidateBlocks) {
        const auto normalized = candidate.content.trimmed();
        if (normalized.isEmpty() || remaining <= 0) {
            continue;
        }

        PromptContextBlock block = candidate;
        block.title = block.title.trimmed().isEmpty() ? contextAssemblySourceKindName(block.source)
                                                      : block.title.trimmed();
        block.content = normalized;
        block.originalSize = toInt(normalized.size());
        result.bundle.originalSize += block.originalSize;

        if (block.content.size() > remaining) {
            block.content = block.content.left(remaining).trimmed();
            block.truncated = true;
            result.bundle.truncated = true;
        }

        block.injectedSize = toInt(block.content.size());
        if (block.injectedSize <= 0) {
            continue;
        }

        remaining -= block.injectedSize;
        result.bundle.injectedSize += block.injectedSize;
        result.bundle.blocks.append(block);
        blockTexts.append(QStringLiteral("--- %1 ---\n%2").arg(block.title, block.content));
        sourceNames.append(contextAssemblySourceKindName(block.source));
    }

    if (result.bundle.blocks.isEmpty()) {
        result.status = PromptContextInjectionStatus::Empty;
        result.summary =
            QStringLiteral("Prompt context injection found no available approved local context.");
        result.sourceSummary = QStringLiteral("No context sources injected.");
        return result;
    }

    result.bundle.text =
        QStringLiteral("%1\n%2\n%3")
            .arg(policy.delimiterStart, blockTexts.join(QStringLiteral("\n")), policy.delimiterEnd);
    result.prompt =
        QStringLiteral("%1\n\nUser prompt:\n%2").arg(result.bundle.text, prompt.trimmed());
    result.status = result.bundle.truncated ? PromptContextInjectionStatus::Truncated
                                            : PromptContextInjectionStatus::Injected;
    result.injectedBlockCount = toInt(result.bundle.blocks.size());
    result.injectedCharacterCount = result.bundle.injectedSize;
    result.sourceSummary = sourceNames.join(QStringLiteral(", "));
    result.sizeSummary =
        QStringLiteral("%1 of %2 context characters injected within %3 character budget.")
            .arg(result.bundle.injectedSize)
            .arg(result.bundle.originalSize)
            .arg(budget);
    result.summary = QStringLiteral("%1 %2 from %3. %4")
                         .arg(result.injectedBlockCount)
                         .arg(result.injectedBlockCount == 1 ? QStringLiteral("context block")
                                                             : QStringLiteral("context blocks"))
                         .arg(result.sourceSummary, result.sizeSummary);
    result.bundle.summary = result.summary;
    return result;
}

QStringList promptContextBlockSummaries(const PromptContextInjectionResult& result) {
    QStringList summaries;
    summaries.reserve(result.bundle.blocks.size());
    for (const auto& block : result.bundle.blocks) {
        summaries.append(
            QStringLiteral("%1 / %2 chars%3")
                .arg(contextAssemblySourceKindName(block.source))
                .arg(block.injectedSize)
                .arg(block.truncated ? QStringLiteral(" / truncated") : QStringLiteral("")));
    }
    return summaries;
}

QStringList conversationSummaryBlockSummaries(const ConversationSummaryResult& result) {
    QStringList summaries;
    summaries.reserve(result.blocks.size());
    for (const auto& block : result.blocks) {
        summaries.append(
            QStringLiteral("Block %1 / messages #%2-#%3 / %4 %5 / %6 chars%7")
                .arg(block.blockIndex)
                .arg(block.firstOriginalIndex)
                .arg(block.lastOriginalIndex)
                .arg(block.messageCount)
                .arg(block.messageCount == 1 ? QStringLiteral("message")
                                             : QStringLiteral("messages"))
                .arg(block.includedSize)
                .arg(block.truncated ? QStringLiteral(" / truncated") : QStringLiteral("")));
    }
    return summaries;
}

RetrievalPlanningResult planRetrieval(const QList<RetrievalCandidate>& candidates,
                                      const RetrievalPlanningPolicy& policy) {
    RetrievalPlanningResult result;
    result.policy = policy;
    result.budget.maxCharacters = std::max(0, policy.maxCharacters);
    result.budget.remainingCharacters = result.budget.maxCharacters;
    result.checks.append(QStringLiteral("Boundary: deterministic retrieval planning only"));
    result.checks.append(QStringLiteral("Semantic/vector search: disabled"));
    result.checks.append(QStringLiteral("Embeddings: disabled"));
    result.checks.append(QStringLiteral("Provider/model calls: disabled"));
    result.checks.append(QStringLiteral("Automatic memory writes: disabled"));
    result.checks.append(QStringLiteral("Prompt mutation during planning: disabled"));

    if (!policy.enabled) {
        result.status = RetrievalPlanningStatus::Disabled;
        result.summary = policy.summary;
        result.checks.append(QStringLiteral("Policy: disabled"));
        return result;
    }

    QList<RetrievalCandidate> normalized;
    normalized.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        RetrievalCandidate item = candidate;
        item.title = item.title.simplified();
        item.content = item.content.trimmed();
        item.priority = retrievalSourcePriorityForKind(item.source);
        item.originalSize = toInt(item.content.size());
        item.selectedSize = 0;
        item.selected = false;
        item.truncated = false;
        item.exclusionReason.clear();

        const bool sourceAllowed = (item.source == ContextAssemblySourceKind::Conversation &&
                                    policy.includeRecentConversation) ||
                                   (item.source == ContextAssemblySourceKind::ConversationSummary &&
                                    policy.includeConversationSummary) ||
                                   (item.source == ContextAssemblySourceKind::CommittedMemory &&
                                    policy.includeCommittedMemory) ||
                                   (item.source == ContextAssemblySourceKind::RuntimeMetadata &&
                                    policy.includeRuntimeMetadata) ||
                                   (item.source == ContextAssemblySourceKind::Orchestration &&
                                    policy.includeOrchestration) ||
                                   (item.source ==
                                        ContextAssemblySourceKind::SelectedConversationMetadata &&
                                    policy.includeSelectedConversationMetadata);

        if (item.content.isEmpty()) {
            item.exclusionReason = contextExclusionReasonName(ContextExclusionReason::EmptyCandidate);
        } else if (!sourceAllowed) {
            item.exclusionReason = contextExclusionReasonName(ContextExclusionReason::SourceDisabled);
        }

        normalized.append(item);
    }

    std::stable_sort(normalized.begin(), normalized.end(), [](const auto& left, const auto& right) {
        return static_cast<int>(left.priority) < static_cast<int>(right.priority);
    });

    result.candidates.reserve(normalized.size());
    int remaining = result.budget.maxCharacters;
    QSet<QString> selectedFingerprints;
    QSet<ContextAssemblySourceKind> selectedSources;
    for (auto item : normalized) {
        result.budget.estimatedCharacters += item.originalSize;

        if (!item.exclusionReason.isEmpty()) {
            ++result.excludedCandidateCount;
            result.candidates.append(item);
            continue;
        }

        const auto fingerprint = item.content.simplified().toCaseFolded();
        if (selectedFingerprints.contains(fingerprint)) {
            item.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::DuplicateCandidate);
            ++result.excludedCandidateCount;
            result.candidates.append(item);
            continue;
        }

        if (policy.maxSources > 0 && !selectedSources.contains(item.source) &&
            selectedSources.size() >= policy.maxSources) {
            item.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::SourceCountLimit);
            ++result.excludedCandidateCount;
            result.candidates.append(item);
            continue;
        }

        if (policy.maxCandidates > 0 && result.selectedCandidateCount >= policy.maxCandidates) {
            item.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::CandidateCountLimit);
            ++result.excludedCandidateCount;
            result.candidates.append(item);
            continue;
        }

        if (remaining <= 0) {
            item.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::BudgetExhausted);
            ++result.excludedCandidateCount;
            result.candidates.append(item);
            continue;
        }

        result.budget.allocatedCharacters += std::min(item.originalSize, remaining);
        if (item.content.size() > remaining) {
            item.content = item.content.left(remaining).trimmed();
            item.truncated = true;
            ++result.truncatedCandidateCount;
        }

        item.selectedSize = toInt(item.content.size());
        if (item.selectedSize <= 0) {
            item.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::BudgetExhausted);
            ++result.excludedCandidateCount;
            result.candidates.append(item);
            continue;
        }

        item.selected = true;
        selectedFingerprints.insert(fingerprint);
        selectedSources.insert(item.source);
        remaining -= item.selectedSize;
        result.budget.includedCharacters += item.selectedSize;
        ++result.selectedCandidateCount;
        result.selectedCandidates.append(item);
        result.candidates.append(item);
    }

    result.candidateCount = toInt(result.candidates.size());
    result.budget.remainingCharacters = std::max(0, remaining);
    result.budget.summary =
        QStringLiteral("%1 of %2 estimated retrieval characters selected within %3 character "
                       "budget.")
            .arg(result.budget.includedCharacters)
            .arg(result.budget.estimatedCharacters)
            .arg(result.budget.maxCharacters);
    result.budgetSummary = result.budget.summary;

    QList<ContextAssemblySourceKind> sourceOrder{
        ContextAssemblySourceKind::Conversation,    ContextAssemblySourceKind::ConversationSummary,
        ContextAssemblySourceKind::CommittedMemory, ContextAssemblySourceKind::RuntimeMetadata,
        ContextAssemblySourceKind::Orchestration,
        ContextAssemblySourceKind::SelectedConversationMetadata,
    };
    QStringList selectedSourceNames;
    QStringList excludedSourceNames;
    for (const auto source : sourceOrder) {
        RetrievalSelectionSummary summary;
        summary.source = source;
        summary.priority = retrievalSourcePriorityForKind(source);

        for (const auto& candidate : result.candidates) {
            if (candidate.source != source) {
                continue;
            }
            ++summary.candidateCount;
            summary.estimatedCharacters += candidate.originalSize;
            summary.allocatedCharacters += candidate.selected ? candidate.selectedSize : 0;
            summary.includedCharacters += candidate.selected ? candidate.selectedSize : 0;
            if (candidate.selected) {
                ++summary.selectedCount;
            } else {
                ++summary.excludedCount;
            }
            if (candidate.truncated) {
                ++summary.truncatedCount;
            }
        }

        if (summary.candidateCount == 0) {
            continue;
        }

        if (summary.selectedCount > 0) {
            ++result.selectedSourceCount;
            selectedSourceNames.append(contextAssemblySourceKindName(source));
        }
        if (summary.excludedCount > 0) {
            ++result.excludedSourceCount;
            excludedSourceNames.append(contextAssemblySourceKindName(source));
        }

        summary.summary =
            QStringLiteral("%1 / priority %2 / %3 selected / %4 excluded / %5 truncated / %6 "
                           "chars")
                .arg(contextAssemblySourceKindName(source),
                     retrievalSourcePriorityName(summary.priority))
                .arg(summary.selectedCount)
                .arg(summary.excludedCount)
                .arg(summary.truncatedCount)
                .arg(summary.includedCharacters);
        result.sourceSummaries.append(summary);
    }

    result.sourceSummary = selectedSourceNames.isEmpty()
                               ? QStringLiteral("No retrieval sources selected.")
                               : QStringLiteral("%1 selected; excluded sources: %2")
                                     .arg(selectedSourceNames.join(QStringLiteral(", ")),
                                          excludedSourceNames.isEmpty()
                                              ? QStringLiteral("none")
                                              : excludedSourceNames.join(QStringLiteral(", ")));

    if (result.candidateCount == 0 || result.selectedCandidateCount == 0) {
        result.status = RetrievalPlanningStatus::Empty;
        result.summary = QStringLiteral("Retrieval planning found no selectable local context.");
        return result;
    }

    result.status = (result.excludedCandidateCount > 0 || result.truncatedCandidateCount > 0)
                        ? RetrievalPlanningStatus::Truncated
                        : RetrievalPlanningStatus::Ready;
    result.summary =
        QStringLiteral("Retrieval planning selected %1 of %2 candidate %3 from %4 %5; %6 excluded, "
                       "%7 truncated. %8")
            .arg(result.selectedCandidateCount)
            .arg(result.candidateCount)
            .arg(result.candidateCount == 1 ? QStringLiteral("block") : QStringLiteral("blocks"))
            .arg(result.selectedSourceCount)
            .arg(result.selectedSourceCount == 1 ? QStringLiteral("source")
                                                 : QStringLiteral("sources"))
            .arg(result.excludedCandidateCount)
            .arg(result.truncatedCandidateCount)
            .arg(result.budget.summary);
    return result;
}

QStringList retrievalSourceSummaries(const RetrievalPlanningResult& result) {
    QStringList summaries;
    summaries.reserve(result.sourceSummaries.size());
    for (const auto& summary : result.sourceSummaries) {
        summaries.append(summary.summary);
    }
    return summaries;
}

QStringList retrievalCandidateTraceSummaries(const RetrievalPlanningResult& result) {
    QStringList summaries;
    summaries.reserve(result.candidates.size());
    for (const auto& candidate : result.candidates) {
        summaries.append(QStringLiteral("%1 / %2 / %3 chars / %4")
                             .arg(candidate.selected ? QStringLiteral("included")
                                                     : QStringLiteral("excluded"))
                             .arg(contextAssemblySourceKindName(candidate.source))
                             .arg(candidate.selected ? candidate.selectedSize
                                                     : candidate.originalSize)
                             .arg(candidate.selected
                                      ? QStringLiteral("deterministic priority")
                                      : candidate.exclusionReason.simplified()));
    }
    return summaries;
}

MemoryRelevanceSummary rankMemoryRelevance(
    const QList<MemoryRelevanceCandidate>& candidates, const QString& prompt,
    const QString& activeConversationTitle, const QString& recentConversationText,
    const MemoryRelevancePolicy& policy) {
    MemoryRelevanceSummary summary;
    summary.policy = policy;
    summary.budget.maxCharacters = std::max(0, policy.maxCharacters);
    summary.budget.remainingCharacters = summary.budget.maxCharacters;

    if (!policy.enabled) {
        summary.summary = policy.summary;
        summary.trace.summary = QStringLiteral("Memory relevance policy is disabled.");
        return summary;
    }

    const auto promptTokens = literalTokens(prompt);
    const auto titleTokens = literalTokens(activeConversationTitle);
    const auto recentTokens = literalTokens(recentConversationText);

    QList<MemoryRelevanceSelection> evaluated;
    evaluated.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        MemoryRelevanceSelection selection;
        selection.candidate = candidate;
        selection.candidate.key = candidate.key.simplified();
        selection.candidate.value = candidate.value.simplified();
        selection.selectedText =
            QStringLiteral("%1 = %2").arg(selection.candidate.key, selection.candidate.value);
        summary.budget.estimatedCharacters += toInt(selection.selectedText.size());

        if (selection.candidate.key.trimmed().isEmpty() ||
            selection.candidate.value.trimmed().isEmpty()) {
            selection.reason.exclusionReason = contextExclusionReasonName(
                ContextExclusionReason::EmptyCandidate);
            evaluated.append(selection);
            continue;
        }

        const auto keyTokens = literalTokens(selection.candidate.key);
        const auto valueTokens = literalTokens(selection.candidate.value);
        const auto memoryTokens = literalTokens(selection.selectedText);
        selection.score.keyOverlap = overlapCount(keyTokens, promptTokens);
        selection.score.valueOverlap = overlapCount(valueTokens, promptTokens);
        selection.score.activeConversationTitleOverlap = overlapCount(memoryTokens, titleTokens);
        selection.score.recentConversationTermOverlap = overlapCount(memoryTokens, recentTokens);
        selection.score.committedPriority = selection.candidate.committed ? 1 : 0;
        selection.score.pinnedPriority = selection.candidate.pinned ? 8 : 0;
        selection.score.total = selection.score.keyOverlap * 40 + selection.score.valueOverlap * 24 +
                                selection.score.activeConversationTitleOverlap * 18 +
                                selection.score.recentConversationTermOverlap * 10 +
                                selection.score.pinnedPriority + selection.score.committedPriority;

        if (selection.score.keyOverlap > 0) {
            selection.reason.includedReasons.append(QStringLiteral("literal key overlap"));
        }
        if (selection.score.valueOverlap > 0) {
            selection.reason.includedReasons.append(QStringLiteral("literal value overlap"));
        }
        if (selection.score.activeConversationTitleOverlap > 0) {
            selection.reason.includedReasons.append(
                QStringLiteral("active conversation title overlap"));
        }
        if (selection.score.recentConversationTermOverlap > 0) {
            selection.reason.includedReasons.append(QStringLiteral("recent conversation terms"));
        }
        if (selection.candidate.pinned) {
            selection.reason.includedReasons.append(QStringLiteral("explicit pinned priority"));
        }
        if (selection.candidate.committed) {
            selection.reason.includedReasons.append(QStringLiteral("committed memory priority"));
        }

        const bool hasLiteralRelevance =
            selection.score.keyOverlap > 0 || selection.score.valueOverlap > 0 ||
            selection.score.activeConversationTitleOverlap > 0 ||
            selection.score.recentConversationTermOverlap > 0;
        if (!hasLiteralRelevance &&
            !(policy.includePinnedWithoutOverlap && selection.candidate.pinned)) {
            selection.reason.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::NotRelevant);
        }
        selection.reason.summary = !selection.reason.exclusionReason.isEmpty()
                                       ? selection.reason.exclusionReason
                                       : selection.reason.includedReasons.join(QStringLiteral(", "));
        evaluated.append(selection);
    }

    std::stable_sort(evaluated.begin(), evaluated.end(), [](const auto& left, const auto& right) {
        if (left.score.total != right.score.total) {
            return left.score.total > right.score.total;
        }
        if (left.score.keyOverlap != right.score.keyOverlap) {
            return left.score.keyOverlap > right.score.keyOverlap;
        }
        if (left.score.valueOverlap != right.score.valueOverlap) {
            return left.score.valueOverlap > right.score.valueOverlap;
        }
        return left.candidate.originalIndex < right.candidate.originalIndex;
    });

    int remaining = summary.budget.maxCharacters;
    QSet<QString> fingerprints;
    for (auto selection : evaluated) {
        if (!selection.reason.exclusionReason.isEmpty()) {
            ++summary.excludedCount;
            summary.selections.append(selection);
            continue;
        }

        const auto fingerprint = selection.selectedText.toCaseFolded();
        if (fingerprints.contains(fingerprint)) {
            selection.duplicate = true;
            selection.reason.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::DuplicateCandidate);
            selection.reason.summary = selection.reason.exclusionReason;
            ++summary.duplicateCount;
            ++summary.excludedCount;
            summary.selections.append(selection);
            continue;
        }

        if (policy.maxCandidates > 0 && summary.includedCount >= policy.maxCandidates) {
            selection.reason.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::CandidateCountLimit);
            selection.reason.summary = selection.reason.exclusionReason;
            ++summary.excludedCount;
            summary.selections.append(selection);
            continue;
        }

        if (remaining <= 0) {
            selection.reason.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::BudgetExhausted);
            selection.reason.summary = selection.reason.exclusionReason;
            ++summary.excludedCount;
            summary.selections.append(selection);
            continue;
        }

        if (selection.selectedText.size() > remaining) {
            selection.selectedText = selection.selectedText.left(remaining).trimmed();
            selection.truncated = true;
            ++summary.truncatedCount;
        }
        selection.selectedCharacters = toInt(selection.selectedText.size());
        if (selection.selectedCharacters <= 0) {
            selection.reason.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::BudgetExhausted);
            selection.reason.summary = selection.reason.exclusionReason;
            ++summary.excludedCount;
            summary.selections.append(selection);
            continue;
        }

        selection.included = true;
        fingerprints.insert(fingerprint);
        remaining -= selection.selectedCharacters;
        summary.budget.includedCharacters += selection.selectedCharacters;
        ++summary.includedCount;
        summary.selections.append(selection);
    }

    summary.candidateCount = toInt(summary.selections.size());
    summary.budget.remainingCharacters = std::max(0, remaining);
    summary.budget.summary =
        QStringLiteral("%1 of %2 memory context characters selected within %3 character budget.")
            .arg(summary.budget.includedCharacters)
            .arg(summary.budget.estimatedCharacters)
            .arg(summary.budget.maxCharacters);

    for (const auto& selection : summary.selections) {
        const auto line = QStringLiteral("%1 / score %2 / %3 chars / %4")
                              .arg(selection.candidate.key)
                              .arg(selection.score.total)
                              .arg(selection.included ? selection.selectedCharacters
                                                      : toInt(selection.selectedText.size()))
                              .arg(selection.reason.summary);
        if (selection.included) {
            summary.trace.includedSummaries.append(line);
        } else {
            summary.trace.excludedSummaries.append(line);
        }
    }
    summary.trace.summary =
        QStringLiteral("%1 included / %2 excluded / %3 duplicates / %4 truncated.")
            .arg(summary.includedCount)
            .arg(summary.excludedCount)
            .arg(summary.duplicateCount)
            .arg(summary.truncatedCount);
    summary.summary =
        QStringLiteral("Memory relevance selected %1 of %2 committed %3; %4 excluded. %5")
            .arg(summary.includedCount)
            .arg(summary.candidateCount)
            .arg(summary.candidateCount == 1 ? QStringLiteral("memory")
                                             : QStringLiteral("memories"))
            .arg(summary.excludedCount)
            .arg(summary.budget.summary);
    return summary;
}

QStringList memoryRelevanceTraceSummaries(const MemoryRelevanceSummary& summary) {
    QStringList lines;
    for (const auto& line : summary.trace.includedSummaries) {
        lines.append(QStringLiteral("included / %1").arg(line));
    }
    for (const auto& line : summary.trace.excludedSummaries) {
        lines.append(QStringLiteral("excluded / %1").arg(line));
    }
    return lines;
}

QStringList memoryRelevanceExclusionSummaries(const MemoryRelevanceSummary& summary) {
    QStringList lines;
    for (const auto& selection : summary.selections) {
        if (!selection.included) {
            lines.append(QStringLiteral("%1 / %2")
                             .arg(selection.candidate.key, selection.reason.exclusionReason));
        }
    }
    return lines;
}

namespace {

enum class SalienceBudgetGroup : std::uint8_t {
    ActiveConversation,
    CommittedMemory,
    RuntimeMetadata,
};

SalienceBudgetGroup salienceBudgetGroupForSource(ContextAssemblySourceKind source) {
    switch (source) {
    case ContextAssemblySourceKind::Conversation:
    case ContextAssemblySourceKind::ConversationSummary:
        return SalienceBudgetGroup::ActiveConversation;
    case ContextAssemblySourceKind::CommittedMemory:
        return SalienceBudgetGroup::CommittedMemory;
    case ContextAssemblySourceKind::RuntimeMetadata:
    case ContextAssemblySourceKind::Orchestration:
    case ContextAssemblySourceKind::SelectedConversationMetadata:
        return SalienceBudgetGroup::RuntimeMetadata;
    }

    return SalienceBudgetGroup::ActiveConversation;
}

int salienceGroupBudget(const ConversationSalienceBudget& budget, SalienceBudgetGroup group) {
    switch (group) {
    case SalienceBudgetGroup::ActiveConversation:
        return budget.activeConversationBudget;
    case SalienceBudgetGroup::CommittedMemory:
        return budget.committedMemoryBudget;
    case SalienceBudgetGroup::RuntimeMetadata:
        return budget.runtimeMetadataBudget;
    }

    return budget.maxCharacters;
}

int salienceGroupUsed(const ConversationSalienceBudget& budget, SalienceBudgetGroup group) {
    switch (group) {
    case SalienceBudgetGroup::ActiveConversation:
        return budget.activeConversationCharacters;
    case SalienceBudgetGroup::CommittedMemory:
        return budget.committedMemoryCharacters;
    case SalienceBudgetGroup::RuntimeMetadata:
        return budget.runtimeMetadataCharacters;
    }

    return budget.includedCharacters;
}

void addSalienceGroupUsed(ConversationSalienceBudget& budget, SalienceBudgetGroup group,
                          int characters) {
    switch (group) {
    case SalienceBudgetGroup::ActiveConversation:
        budget.activeConversationCharacters += characters;
        break;
    case SalienceBudgetGroup::CommittedMemory:
        budget.committedMemoryCharacters += characters;
        break;
    case SalienceBudgetGroup::RuntimeMetadata:
        budget.runtimeMetadataCharacters += characters;
        break;
    }
}

QString salienceGroupName(SalienceBudgetGroup group) {
    switch (group) {
    case SalienceBudgetGroup::ActiveConversation:
        return QStringLiteral("active conversation");
    case SalienceBudgetGroup::CommittedMemory:
        return QStringLiteral("committed memory");
    case SalienceBudgetGroup::RuntimeMetadata:
        return QStringLiteral("runtime metadata");
    }

    return QStringLiteral("context");
}

} // namespace

ConversationSalienceSummary rankConversationSalience(
    const QList<ConversationSalienceCandidate>& candidates, const QString& explicitUserQuery,
    const QString& activeConversationTitle, const QString& recentUserMessages,
    const QString& recentAssistantMessages, const QString& committedMemoryText,
    const ConversationSaliencePolicy& policy) {
    ConversationSalienceSummary summary;
    summary.policy = policy;
    summary.budget.maxCharacters = std::max(0, policy.maxCharacters);
    const auto activePercent = std::max(0, policy.activeConversationBudgetPercent);
    const auto memoryPercent = std::max(0, policy.committedMemoryBudgetPercent);
    const auto runtimePercent = std::max(0, policy.runtimeMetadataBudgetPercent);
    const auto totalPercent = std::max(1, activePercent + memoryPercent + runtimePercent);
    summary.budget.activeConversationBudget =
        summary.budget.maxCharacters * activePercent / totalPercent;
    summary.budget.committedMemoryBudget =
        summary.budget.maxCharacters * memoryPercent / totalPercent;
    summary.budget.runtimeMetadataBudget =
        summary.budget.maxCharacters - summary.budget.activeConversationBudget -
        summary.budget.committedMemoryBudget;
    summary.budget.remainingCharacters = summary.budget.maxCharacters;
    summary.budget.allocationSummary =
        QStringLiteral("Active conversation %1 chars / committed memory %2 chars / runtime "
                       "metadata %3 chars.")
            .arg(summary.budget.activeConversationBudget)
            .arg(summary.budget.committedMemoryBudget)
            .arg(summary.budget.runtimeMetadataBudget);

    if (!policy.enabled) {
        summary.summary = policy.summary;
        summary.trace.summary = QStringLiteral("Conversation salience policy is disabled.");
        return summary;
    }

    const auto queryTokens = literalTokens(explicitUserQuery);
    const auto titleTokens = literalTokens(activeConversationTitle);
    const auto recentUserTokens = literalTokens(recentUserMessages);
    const auto recentAssistantTokens = literalTokens(recentAssistantMessages);
    const auto committedMemoryTokens = literalTokens(committedMemoryText);

    QList<ConversationSalienceSelection> evaluated;
    evaluated.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        ConversationSalienceSelection selection;
        selection.candidate = candidate;
        selection.candidate.title = candidate.title.simplified();
        selection.candidate.content = candidate.content.simplified();
        selection.selectedText = selection.candidate.content;
        selection.candidate.originalSize =
            selection.candidate.originalSize > 0 ? selection.candidate.originalSize
                                                : toInt(selection.selectedText.size());
        summary.budget.estimatedCharacters += selection.candidate.originalSize;

        if (selection.candidate.content.trimmed().isEmpty()) {
            selection.reason.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::EmptyCandidate);
            selection.reason.summary = selection.reason.exclusionReason;
            evaluated.append(selection);
            continue;
        }

        const auto candidateTokens =
            literalTokens(QStringLiteral("%1 %2").arg(selection.candidate.title,
                                                     selection.candidate.content));
        selection.score.activeConversationTitleOverlap = overlapCount(candidateTokens, titleTokens);
        selection.score.recentUserMessageOverlap = overlapCount(candidateTokens, recentUserTokens);
        selection.score.recentAssistantMessageOverlap =
            overlapCount(candidateTokens, recentAssistantTokens);
        selection.score.pinnedConversationPriority = selection.candidate.pinned ? 14 : 0;
        selection.score.committedMemoryOverlap = selection.candidate.committedMemory
                                                    ? overlapCount(candidateTokens,
                                                                   committedMemoryTokens)
                                                    : 0;
        selection.score.explicitQueryTermOverlap = overlapCount(candidateTokens, queryTokens);
        selection.score.recencyWeight = std::max(0, 8 - selection.candidate.recencyRank);
        selection.score.total =
            selection.score.explicitQueryTermOverlap * 42 +
            selection.score.activeConversationTitleOverlap * 24 +
            selection.score.recentUserMessageOverlap * 18 +
            selection.score.recentAssistantMessageOverlap * 12 +
            selection.score.committedMemoryOverlap * 10 +
            selection.score.pinnedConversationPriority + selection.score.recencyWeight;

        if (selection.score.explicitQueryTermOverlap > 0) {
            selection.reason.includedReasons.append(QStringLiteral("explicit query literal terms"));
        }
        if (selection.score.activeConversationTitleOverlap > 0) {
            selection.reason.includedReasons.append(
                QStringLiteral("active conversation title overlap"));
        }
        if (selection.score.recentUserMessageOverlap > 0) {
            selection.reason.includedReasons.append(QStringLiteral("recent user message overlap"));
        }
        if (selection.score.recentAssistantMessageOverlap > 0) {
            selection.reason.includedReasons.append(
                QStringLiteral("recent assistant message overlap"));
        }
        if (selection.score.committedMemoryOverlap > 0) {
            selection.reason.includedReasons.append(QStringLiteral("committed memory overlap"));
        }
        if (selection.candidate.pinned) {
            selection.reason.includedReasons.append(QStringLiteral("pinned conversation metadata"));
        }
        if (selection.score.recencyWeight > 0) {
            selection.reason.includedReasons.append(QStringLiteral("deterministic recency"));
        }

        if (selection.reason.includedReasons.isEmpty()) {
            selection.reason.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::NotRelevant);
        }
        selection.reason.summary = !selection.reason.exclusionReason.isEmpty()
                                       ? selection.reason.exclusionReason
                                       : selection.reason.includedReasons.join(QStringLiteral(", "));
        evaluated.append(selection);
    }

    std::stable_sort(evaluated.begin(), evaluated.end(), [](const auto& left, const auto& right) {
        const auto leftPriority = retrievalSourcePriorityForKind(left.candidate.source);
        const auto rightPriority = retrievalSourcePriorityForKind(right.candidate.source);
        if (leftPriority != rightPriority) {
            return static_cast<int>(leftPriority) < static_cast<int>(rightPriority);
        }
        if (left.score.total != right.score.total) {
            return left.score.total > right.score.total;
        }
        if (left.score.explicitQueryTermOverlap != right.score.explicitQueryTermOverlap) {
            return left.score.explicitQueryTermOverlap > right.score.explicitQueryTermOverlap;
        }
        if (left.score.recencyWeight != right.score.recencyWeight) {
            return left.score.recencyWeight > right.score.recencyWeight;
        }
        return left.candidate.originalIndex < right.candidate.originalIndex;
    });

    QSet<QString> fingerprints;
    for (auto selection : evaluated) {
        if (!selection.reason.exclusionReason.isEmpty()) {
            ++summary.excludedCount;
            summary.selections.append(selection);
            continue;
        }

        const auto fingerprint = selection.selectedText.toCaseFolded();
        if (fingerprints.contains(fingerprint)) {
            selection.duplicate = true;
            selection.reason.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::DuplicateCandidate);
            selection.reason.summary = selection.reason.exclusionReason;
            ++summary.duplicateCount;
            ++summary.excludedCount;
            summary.selections.append(selection);
            continue;
        }

        if (policy.maxCandidates > 0 && summary.includedCount >= policy.maxCandidates) {
            selection.reason.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::CandidateCountLimit);
            selection.reason.summary = selection.reason.exclusionReason;
            ++summary.excludedCount;
            summary.selections.append(selection);
            continue;
        }

        const auto group = salienceBudgetGroupForSource(selection.candidate.source);
        const auto groupRemaining =
            salienceGroupBudget(summary.budget, group) - salienceGroupUsed(summary.budget, group);
        const auto totalRemaining = summary.budget.maxCharacters - summary.budget.includedCharacters;
        const auto allowed = std::min(groupRemaining, totalRemaining);
        if (allowed <= 0) {
            selection.reason.exclusionReason =
                QStringLiteral("%1 budget exhausted").arg(salienceGroupName(group));
            selection.reason.summary = selection.reason.exclusionReason;
            ++summary.excludedCount;
            summary.selections.append(selection);
            continue;
        }

        if (selection.selectedText.size() > allowed) {
            selection.selectedText = selection.selectedText.left(allowed).trimmed();
            selection.truncated = true;
            ++summary.truncatedCount;
        }
        selection.selectedCharacters = toInt(selection.selectedText.size());
        if (selection.selectedCharacters <= 0) {
            selection.reason.exclusionReason =
                contextExclusionReasonName(ContextExclusionReason::BudgetExhausted);
            selection.reason.summary = selection.reason.exclusionReason;
            ++summary.excludedCount;
            summary.selections.append(selection);
            continue;
        }

        selection.included = true;
        fingerprints.insert(fingerprint);
        addSalienceGroupUsed(summary.budget, group, selection.selectedCharacters);
        summary.budget.includedCharacters += selection.selectedCharacters;
        ++summary.includedCount;
        summary.selections.append(selection);
    }

    summary.candidateCount = toInt(summary.selections.size());
    summary.budget.remainingCharacters =
        std::max(0, summary.budget.maxCharacters - summary.budget.includedCharacters);
    summary.budget.summary =
        QStringLiteral("%1 of %2 salience context characters selected within %3 character budget. "
                       "%4 Included: active conversation %5, committed memory %6, runtime metadata "
                       "%7.")
            .arg(summary.budget.includedCharacters)
            .arg(summary.budget.estimatedCharacters)
            .arg(summary.budget.maxCharacters)
            .arg(summary.budget.allocationSummary)
            .arg(summary.budget.activeConversationCharacters)
            .arg(summary.budget.committedMemoryCharacters)
            .arg(summary.budget.runtimeMetadataCharacters);

    for (const auto& selection : summary.selections) {
        const auto line = QStringLiteral("%1 / %2 / score %3 / %4 chars / %5")
                              .arg(contextAssemblySourceKindName(selection.candidate.source),
                                   selection.candidate.title)
                              .arg(selection.score.total)
                              .arg(selection.included ? selection.selectedCharacters
                                                      : selection.candidate.originalSize)
                              .arg(selection.reason.summary);
        if (selection.included) {
            summary.trace.includedSummaries.append(line);
        } else {
            summary.trace.excludedSummaries.append(line);
        }
    }
    summary.trace.summary =
        QStringLiteral("%1 included / %2 excluded / %3 duplicates / %4 truncated. %5")
            .arg(summary.includedCount)
            .arg(summary.excludedCount)
            .arg(summary.duplicateCount)
            .arg(summary.truncatedCount)
            .arg(summary.budget.allocationSummary);
    summary.summary =
        QStringLiteral("Conversation salience selected %1 of %2 deterministic context %3; %4 "
                       "excluded, %5 truncated. %6")
            .arg(summary.includedCount)
            .arg(summary.candidateCount)
            .arg(summary.candidateCount == 1 ? QStringLiteral("candidate")
                                             : QStringLiteral("candidates"))
            .arg(summary.excludedCount)
            .arg(summary.truncatedCount)
            .arg(summary.budget.summary);
    return summary;
}

QStringList conversationSalienceTraceSummaries(const ConversationSalienceSummary& summary) {
    QStringList lines;
    for (const auto& line : summary.trace.includedSummaries) {
        lines.append(QStringLiteral("included / %1").arg(line));
    }
    for (const auto& line : summary.trace.excludedSummaries) {
        lines.append(QStringLiteral("excluded / %1").arg(line));
    }
    return lines;
}

QStringList conversationSalienceExclusionSummaries(const ConversationSalienceSummary& summary) {
    QStringList lines;
    for (const auto& selection : summary.selections) {
        if (!selection.included) {
            lines.append(QStringLiteral("%1 / %2 / %3")
                             .arg(contextAssemblySourceKindName(selection.candidate.source),
                                  selection.candidate.title,
                                  selection.reason.exclusionReason));
        }
    }
    return lines;
}

ConversationCompressionSummary planConversationCompression(
    const QList<ConversationWindowMessage>& messages,
    const ConversationSummaryResult& existingSummary,
    const ConversationSalienceSummary& salienceSummary, bool contextInjectionEnabled,
    const ConversationCompressionPolicy& policy) {
    ConversationCompressionSummary summary;
    summary.policy = policy;
    summary.budget.maxCandidateCharacters = std::max(0, policy.maxCandidateCharacters);
    summary.budget.remainingCharacters = summary.budget.maxCandidateCharacters;

    if (!policy.enabled) {
        summary.status = ConversationCompressionStatus::Disabled;
        summary.readiness.status = summary.status;
        summary.readiness.summary =
            QStringLiteral("Conversation compression readiness is disabled.");
        summary.summary = summary.readiness.summary;
        summary.fallback.reason = QStringLiteral("policy disabled");
        summary.fallback.summary =
            QStringLiteral("Compression planning disabled by local policy.");
        return summary;
    }

    int estimatedCharacters = 0;
    int candidateMessageCount = 0;
    QStringList userFactLines;
    QStringList repeatedLines;
    QSet<QString> repeatedFingerprints;
    QSet<QString> seenFingerprints;
    for (const auto& message : messages) {
        if (message.role.compare(QStringLiteral("system"), Qt::CaseInsensitive) == 0) {
            continue;
        }
        const auto content = message.content.simplified();
        if (content.isEmpty()) {
            continue;
        }
        ++candidateMessageCount;
        estimatedCharacters += toInt(content.size());

        const auto folded = content.toCaseFolded();
        const auto userRole = message.role.compare(QStringLiteral("user"), Qt::CaseInsensitive) == 0;
        if (userRole && (folded.contains(QStringLiteral("prefer")) ||
                         folded.contains(QStringLiteral("remember")) ||
                         folded.contains(QStringLiteral("my ")))) {
            userFactLines.append(content.left(160));
        }
        if (seenFingerprints.contains(folded) && !repeatedFingerprints.contains(folded)) {
            repeatedFingerprints.insert(folded);
            repeatedLines.append(content.left(140));
        }
        seenFingerprints.insert(folded);
    }

    const auto estimatedTokens = (estimatedCharacters + 3) / 4;
    summary.budget.saliencePressurePercent =
        salienceSummary.budget.maxCharacters > 0
            ? std::min(100, salienceSummary.budget.includedCharacters * 100 /
                                salienceSummary.budget.maxCharacters)
            : 0;
    const auto characterPressure =
        policy.characterRequiredThreshold > 0
            ? estimatedCharacters * 100 / policy.characterRequiredThreshold
            : 0;
    const auto messagePressure = policy.messageRequiredThreshold > 0
                                     ? candidateMessageCount * 100 / policy.messageRequiredThreshold
                                     : 0;
    const auto tokenPressure =
        policy.tokenRequiredThreshold > 0 ? estimatedTokens * 100 / policy.tokenRequiredThreshold
                                          : 0;
    const auto activePressure = policy.activeConversationRequiredThreshold > 0
                                    ? estimatedCharacters * 100 /
                                          policy.activeConversationRequiredThreshold
                                    : 0;
    summary.budget.pressurePercent =
        std::min(100, std::max({characterPressure, messagePressure, tokenPressure, activePressure,
                                summary.budget.saliencePressurePercent}));

    summary.readiness.messageCount = candidateMessageCount;
    summary.readiness.estimatedCharacters = estimatedCharacters;
    summary.readiness.estimatedTokens = estimatedTokens;
    summary.readiness.activeConversationCharacters = estimatedCharacters;
    summary.readiness.contextInjectionEnabled = contextInjectionEnabled;
    summary.readiness.existingSummaryAvailable = !existingSummary.text.trimmed().isEmpty();
    summary.readiness.saliencePressurePercent = summary.budget.saliencePressurePercent;
    summary.readiness.pressurePercent = summary.budget.pressurePercent;

    const auto required = candidateMessageCount >= policy.messageRequiredThreshold ||
                          estimatedCharacters >= policy.characterRequiredThreshold ||
                          estimatedTokens >= policy.tokenRequiredThreshold ||
                          estimatedCharacters >= policy.activeConversationRequiredThreshold ||
                          summary.budget.saliencePressurePercent >=
                              policy.saliencePressureRequiredThreshold;
    const auto warning = candidateMessageCount >= policy.messageWarningThreshold ||
                         estimatedCharacters >= policy.characterWarningThreshold ||
                         estimatedTokens >= policy.tokenWarningThreshold ||
                         estimatedCharacters >= policy.activeConversationWarningThreshold ||
                         summary.budget.saliencePressurePercent >=
                             policy.saliencePressureWarningThreshold;
    summary.status = required ? ConversationCompressionStatus::Needed
                              : (warning ? ConversationCompressionStatus::Approaching
                                         : ConversationCompressionStatus::NotNeeded);
    summary.readiness.status = summary.status;
    summary.readiness.compressionUseful = summary.status != ConversationCompressionStatus::NotNeeded;
    summary.readiness.summary =
        QStringLiteral("%1: %2 messages / %3 chars / approx %4 tokens / pressure %5% / salience "
                       "%6% / context %7 / existing summary %8.")
            .arg(conversationCompressionStatusName(summary.status))
            .arg(candidateMessageCount)
            .arg(estimatedCharacters)
            .arg(estimatedTokens)
            .arg(summary.budget.pressurePercent)
            .arg(summary.budget.saliencePressurePercent)
            .arg(contextInjectionEnabled ? QStringLiteral("enabled") : QStringLiteral("disabled"))
            .arg(summary.readiness.existingSummaryAvailable ? QStringLiteral("available")
                                                            : QStringLiteral("missing"));

    if (summary.status == ConversationCompressionStatus::NotNeeded) {
        summary.summary = summary.readiness.summary;
        summary.fallback.reason = QStringLiteral("low pressure");
        summary.fallback.summary =
            QStringLiteral("No compression candidates planned because pressure is low.");
        summary.trace.summary = summary.fallback.summary;
        return summary;
    }

    QList<ConversationCompressionCandidate> candidates;
    const auto recentCount = std::min(6, candidateMessageCount);
    if (recentCount > 0) {
        candidates.append(ConversationCompressionCandidate{
            QStringLiteral("recent-window"),
            QStringLiteral("Recent Conversation Window"),
            QStringLiteral("Keep the latest %1 visible transcript messages as live context.")
                .arg(recentCount),
            0,
            recentCount,
            std::min(estimatedCharacters, std::max(1, policy.maxCandidateCharacters / 3)),
            std::min(estimatedTokens, std::max(1, policy.maxCandidateCharacters / 12)),
        });
    }

    const auto olderCount = std::max(0, candidateMessageCount - recentCount);
    if (olderCount > 0) {
        candidates.append(ConversationCompressionCandidate{
            QStringLiteral("older-segment"),
            QStringLiteral("Older Conversation Segment"),
            QStringLiteral("Plan deterministic summary metadata for %1 older transcript messages.")
                .arg(olderCount),
            1,
            olderCount,
            std::min(std::max(0, estimatedCharacters - policy.maxCandidateCharacters / 3),
                     std::max(1, policy.maxCandidateCharacters / 2)),
            std::min(std::max(0, estimatedTokens - policy.maxCandidateCharacters / 12),
                     std::max(1, policy.maxCandidateCharacters / 8)),
        });
    }

    if (!userFactLines.isEmpty()) {
        candidates.append(ConversationCompressionCandidate{
            QStringLiteral("high-salience-user-facts"),
            QStringLiteral("High-Salience User Facts"),
            userFactLines.join(QStringLiteral(" / ")).left(320),
            2,
            toInt(userFactLines.size()),
            toInt(userFactLines.join(QStringLiteral(" ")).size()),
            toInt(userFactLines.join(QStringLiteral(" ")).size() / 4),
        });
    }

    if (!repeatedLines.isEmpty()) {
        candidates.append(ConversationCompressionCandidate{
            QStringLiteral("low-salience-repeated-turns"),
            QStringLiteral("Low-Salience Repeated Turns"),
            repeatedLines.join(QStringLiteral(" / ")).left(320),
            3,
            toInt(repeatedLines.size()),
            toInt(repeatedLines.join(QStringLiteral(" ")).size()),
            toInt(repeatedLines.join(QStringLiteral(" ")).size() / 4),
        });
    }

    candidates.append(ConversationCompressionCandidate{
        QStringLiteral("system-runtime-exclusion"),
        QStringLiteral("System/Runtime Metadata Exclusion"),
        QStringLiteral("System messages, runtime traces, prompt dumps, and tool metadata stay "
                       "excluded from compression candidates."),
        4,
        0,
        0,
        0,
    });

    std::stable_sort(candidates.begin(), candidates.end(), [](const auto& left, const auto& right) {
        return left.originalIndex < right.originalIndex;
    });

    for (auto candidate : candidates) {
        ++summary.selection.candidateCount;
        summary.budget.estimatedCharacters += candidate.estimatedCharacters;
        summary.budget.estimatedTokens += candidate.estimatedTokens;
        if (candidate.kind == QStringLiteral("system-runtime-exclusion")) {
            candidate.excluded = true;
            candidate.exclusionReason = QStringLiteral("Excluded safety boundary");
        } else if (policy.maxCandidates > 0 &&
                   summary.selection.selectedCandidateCount >= policy.maxCandidates) {
            candidate.excluded = true;
            candidate.exclusionReason = QStringLiteral("Candidate count limit reached");
        } else if (summary.budget.remainingCharacters <= 0) {
            candidate.excluded = true;
            candidate.exclusionReason = QStringLiteral("Compression budget exhausted");
        } else {
            candidate.selected = true;
            candidate.selectedCharacters =
                std::min(candidate.estimatedCharacters, summary.budget.remainingCharacters);
            summary.budget.selectedCharacters += candidate.selectedCharacters;
            summary.budget.remainingCharacters =
                std::max(0, summary.budget.maxCandidateCharacters -
                                summary.budget.selectedCharacters);
            ++summary.selection.selectedCandidateCount;
        }

        if (candidate.excluded) {
            ++summary.selection.excludedCandidateCount;
        }
        const auto line = QStringLiteral("%1 / %2 / %3 messages / %4 chars / %5")
                              .arg(candidate.selected ? QStringLiteral("selected")
                                                      : QStringLiteral("excluded"),
                                   candidate.title)
                              .arg(candidate.messageCount)
                              .arg(candidate.selected ? candidate.selectedCharacters
                                                      : candidate.estimatedCharacters)
                              .arg(candidate.exclusionReason.isEmpty()
                                       ? QStringLiteral("metadata only")
                                       : candidate.exclusionReason);
        summary.trace.candidateSummaries.append(line);
        if (candidate.selected) {
            summary.trace.selectionSummaries.append(line);
        }
        summary.selection.candidates.append(candidate);
    }

    if (summary.selection.selectedCandidateCount > 0 &&
        summary.status != ConversationCompressionStatus::Approaching) {
        summary.status = ConversationCompressionStatus::Planned;
        summary.readiness.status = summary.status;
    }
    summary.selection.summary =
        QStringLiteral("%1 selected / %2 excluded / %3 candidates.")
            .arg(summary.selection.selectedCandidateCount)
            .arg(summary.selection.excludedCandidateCount)
            .arg(summary.selection.candidateCount);
    summary.budget.summary =
        QStringLiteral("%1 of %2 compression candidate chars selected within %3 char metadata "
                       "budget; pressure %4%.")
            .arg(summary.budget.selectedCharacters)
            .arg(summary.budget.estimatedCharacters)
            .arg(summary.budget.maxCandidateCharacters)
            .arg(summary.budget.pressurePercent);
    summary.trace.summary = QStringLiteral("%1 %2")
                                .arg(summary.readiness.summary, summary.selection.summary);
    summary.summary = QStringLiteral("Conversation compression %1. %2")
                          .arg(conversationCompressionStatusName(summary.status),
                               summary.selection.summary);
    return summary;
}

QStringList conversationCompressionCandidateSummaries(
    const ConversationCompressionSummary& summary) {
    QStringList lines;
    for (const auto& candidate : summary.selection.candidates) {
        lines.append(QStringLiteral("%1 / %2 / %3")
                         .arg(candidate.selected ? QStringLiteral("selected")
                                                 : QStringLiteral("excluded"),
                              candidate.title,
                              candidate.summary));
    }
    return lines;
}

QStringList conversationCompressionTraceSummaries(const ConversationCompressionSummary& summary) {
    return summary.trace.candidateSummaries;
}

ConversationSummaryResult planConversationSummaryGeneration(
    const QList<ConversationWindowMessage>& messages,
    const ConversationCompressionSummary& compressionSummary,
    const ConversationSummaryRequest& request, const ConversationSummaryPolicy& policy) {
    ConversationSummaryResult result;
    result.policy = policy;
    result.request = request;
    result.sourceConversationId = request.sourceConversationId;
    result.summaryTimestampUtc =
        request.requestedAtUtc.isValid() ? request.requestedAtUtc : QDateTime::currentDateTimeUtc();
    result.budget.maxCharacters = std::max(0, policy.maxCharacters);
    result.budget.remainingCharacters = result.budget.maxCharacters;
    result.readiness.manualActionRequired = policy.manualOnly;
    result.readiness.localOnly = policy.localOnly;
    result.readiness.backgroundAllowed = policy.backgroundGenerationAllowed;
    result.readiness.transcriptMutationAllowed = policy.transcriptMutationAllowed;
    result.readiness.memoryWriteAllowed = policy.automaticMemoryWriteAllowed;
    result.readiness.toolsAllowed = policy.toolsAllowed;
    result.readiness.filesystemAuthorityAllowed = policy.filesystemAuthorityAllowed;
    result.readiness.hiddenPromptExposureAllowed = false;
    result.readiness.activeConversationOnly = true;

    auto block = [&](const QString& reason, const QString& fallback) {
        result.status = ConversationSummaryStatus::Blocked;
        result.readiness.status = result.status;
        result.readiness.available = false;
        result.readiness.blockedReason = reason;
        result.readiness.summary = reason;
        result.fallback.reason = reason;
        result.fallback.summary = fallback;
        result.trace.safetySummaries.append(QStringLiteral("blocked / %1").arg(reason));
        result.trace.summary = result.fallback.summary;
        result.summary = reason;
        return result;
    };

    if (!policy.enabled) {
        return block(QStringLiteral("Manual summary generation is disabled by local policy."),
                     QStringLiteral("Summary generation preparation stopped before execution."));
    }
    if (policy.manualOnly && !request.explicitUserAction) {
        return block(QStringLiteral("Summary generation requires explicit user action."),
                     QStringLiteral("No automatic or background summary is created."));
    }
    if (request.backgroundRequested || policy.backgroundGenerationAllowed) {
        return block(QStringLiteral("Background summary generation is not allowed."),
                     QStringLiteral("Summary generation remains foreground and manual only."));
    }
    if (request.sourceConversationId.trimmed().isEmpty() ||
        request.sourceConversationId != request.activeConversationId) {
        return block(QStringLiteral("Summary generation is limited to the active conversation."),
                     QStringLiteral("Inactive conversation summary requests are refused."));
    }
    if (request.mutateTranscript || policy.transcriptMutationAllowed) {
        return block(QStringLiteral("Summary generation cannot mutate the transcript."),
                     QStringLiteral("Transcript replacement is outside this phase."));
    }
    if (request.writeCommittedMemory || policy.automaticMemoryWriteAllowed) {
        return block(QStringLiteral("Summary generation cannot write committed memory."),
                     QStringLiteral("Memory writes require a separate explicit review/commit path."));
    }
    if (request.includeRuntimeMetadata || request.exposeHiddenPrompt || policy.toolsAllowed ||
        policy.filesystemAuthorityAllowed) {
        return block(QStringLiteral("Summary generation cannot use runtime metadata, tools, "
                                    "filesystem authority, or hidden prompt dumps."),
                     QStringLiteral("Only visible active-conversation transcript metadata is "
                                    "eligible."));
    }
    if (!policy.generationAvailable) {
        result.readiness.blockedReason =
            QStringLiteral("Local summary generation execution is unavailable.");
    }

    QList<ConversationWindowMessage> visibleMessages;
    visibleMessages.reserve(messages.size());
    for (const auto& message : messages) {
        if (message.role.compare(QStringLiteral("system"), Qt::CaseInsensitive) == 0) {
            result.trace.safetySummaries.append(
                QStringLiteral("excluded / system-runtime metadata / message %1")
                    .arg(message.originalIndex));
            continue;
        }
        const auto content = message.content.simplified();
        if (content.isEmpty()) {
            continue;
        }
        visibleMessages.append(ConversationWindowMessage{message.originalIndex,
                                                         message.role,
                                                         content,
                                                         toInt(content.size())});
    }

    if (visibleMessages.isEmpty()) {
        return block(QStringLiteral("No visible conversation messages are available to summarize."),
                     QStringLiteral("Summary generation has no eligible transcript segment."));
    }

    for (const auto& message : visibleMessages) {
        result.budget.estimatedCharacters += message.originalSize;
    }
    result.coveredFirstMessageIndex = visibleMessages.first().originalIndex;
    result.coveredLastMessageIndex = visibleMessages.last().originalIndex;

    const int recentCount = std::min(6, toInt(visibleMessages.size()));
    const int olderCount = std::max(0, toInt(visibleMessages.size()) - recentCount);
    QList<ConversationSummarySegment> segments;
    if (recentCount > 0) {
        const auto first = visibleMessages.at(visibleMessages.size() - recentCount).originalIndex;
        const auto last = visibleMessages.last().originalIndex;
        segments.append(ConversationSummarySegment{
            QStringLiteral("recent-window"),
            QStringLiteral("Recent Window"),
            QStringLiteral("Retain %1 newest visible messages as live transcript context.")
                .arg(recentCount),
            first,
            last,
            recentCount,
            0,
            0,
            true,
            false,
            {},
        });
    }
    if (olderCount > 0) {
        int chars = 0;
        for (int i = 0; i < olderCount; ++i) {
            chars += visibleMessages.at(i).originalSize;
        }
        segments.append(ConversationSummarySegment{
            QStringLiteral("older-window"),
            QStringLiteral("Older Window"),
            QStringLiteral("Prepare manual local summary for %1 older visible messages.")
                .arg(olderCount),
            visibleMessages.first().originalIndex,
            visibleMessages.at(olderCount - 1).originalIndex,
            olderCount,
            chars,
            0,
            true,
            false,
            {},
        });
    }

    for (const auto& candidate : compressionSummary.selection.candidates) {
        if (candidate.kind == QStringLiteral("high-salience-user-facts") ||
            candidate.kind == QStringLiteral("low-salience-repeated-turns") ||
            candidate.kind == QStringLiteral("system-runtime-exclusion")) {
            segments.append(ConversationSummarySegment{
                candidate.kind,
                candidate.title,
                candidate.summary,
                candidate.originalIndex,
                candidate.originalIndex,
                candidate.messageCount,
                candidate.estimatedCharacters,
                0,
                candidate.kind != QStringLiteral("system-runtime-exclusion"),
                candidate.kind == QStringLiteral("system-runtime-exclusion"),
                candidate.kind == QStringLiteral("system-runtime-exclusion")
                    ? QStringLiteral("Excluded safety boundary")
                    : QString{},
            });
        }
    }

    int selectedCount = 0;
    for (auto segment : segments) {
        if (policy.maxSegments > 0 && selectedCount >= policy.maxSegments &&
            !segment.excluded) {
            segment.selected = false;
            segment.excluded = true;
            segment.exclusionReason = QStringLiteral("Summary segment limit reached");
        }
        if (segment.excluded) {
            result.segments.append(segment);
            result.trace.planningSummaries.append(
                QStringLiteral("excluded / %1 / %2").arg(segment.title, segment.exclusionReason));
            continue;
        }
        const int requestedChars = segment.estimatedCharacters > 0
                                       ? segment.estimatedCharacters
                                       : std::max(1, result.budget.maxCharacters / 4);
        segment.selectedCharacters = std::min(requestedChars, result.budget.remainingCharacters);
        if (segment.selectedCharacters <= 0) {
            segment.selected = false;
            segment.excluded = true;
            segment.exclusionReason = QStringLiteral("Summary budget exhausted");
            result.trace.planningSummaries.append(
                QStringLiteral("excluded / %1 / %2").arg(segment.title, segment.exclusionReason));
        } else {
            ++selectedCount;
            result.budget.includedCharacters += segment.selectedCharacters;
            result.budget.remainingCharacters =
                std::max(0, result.budget.maxCharacters - result.budget.includedCharacters);
            result.trace.planningSummaries.append(
                QStringLiteral("selected / %1 / messages %2-%3 / %4 chars")
                    .arg(segment.title)
                    .arg(segment.firstOriginalIndex)
                    .arg(segment.lastOriginalIndex)
                    .arg(segment.selectedCharacters));
        }
        result.segments.append(segment);
    }

    result.budget.blockCount = selectedCount;
    result.budget.truncatedBlockCount =
        result.budget.includedCharacters < result.budget.estimatedCharacters ? 1 : 0;
    result.budget.summary =
        QStringLiteral("%1 of %2 summary planning chars selected within %3 char budget.")
            .arg(result.budget.includedCharacters)
            .arg(result.budget.estimatedCharacters)
            .arg(result.budget.maxCharacters);
    result.estimatedReductionPercent =
        result.budget.estimatedCharacters > 0
            ? std::max(0, 100 - (result.budget.includedCharacters * 100 /
                                 result.budget.estimatedCharacters))
            : 0;
    result.preview.available = selectedCount > 0;
    result.preview.sourceCharacterCount = result.budget.estimatedCharacters;
    result.preview.previewCharacterCount = result.budget.includedCharacters;
    result.preview.estimatedReductionPercent = result.estimatedReductionPercent;
    result.preview.summary =
        QStringLiteral("Manual summary preview plan covers messages %1-%2 with estimated %3% "
                       "compression gain.")
            .arg(result.coveredFirstMessageIndex)
            .arg(result.coveredLastMessageIndex)
            .arg(result.estimatedReductionPercent);
    result.readiness.available = policy.generationAvailable;
    result.readiness.status =
        policy.generationAvailable ? ConversationSummaryStatus::Planned : ConversationSummaryStatus::Blocked;
    result.status = result.readiness.status;
    result.readiness.summary =
        policy.generationAvailable
            ? QStringLiteral("Manual local summary generation is ready for explicit foreground "
                             "execution.")
            : result.readiness.blockedReason;
    result.summary =
        QStringLiteral("Conversation summary generation %1. %2")
            .arg(conversationSummaryStatusName(result.status), result.preview.summary);
    result.trace.summary =
        QStringLiteral("%1 %2").arg(result.readiness.summary, result.budget.summary);
    return result;
}

QStringList conversationSummarySegmentSummaries(const ConversationSummaryResult& result) {
    QStringList lines;
    for (const auto& segment : result.segments) {
        lines.append(QStringLiteral("%1 / %2 / messages %3-%4 / %5")
                         .arg(segment.selected ? QStringLiteral("selected")
                                               : QStringLiteral("excluded"),
                              segment.title)
                         .arg(segment.firstOriginalIndex)
                         .arg(segment.lastOriginalIndex)
                         .arg(segment.exclusionReason.isEmpty()
                                  ? segment.summary
                                  : segment.exclusionReason));
    }
    return lines;
}

QStringList conversationSummaryTraceSummaries(const ConversationSummaryResult& result) {
    auto lines = result.trace.planningSummaries;
    lines.append(result.trace.safetySummaries);
    return lines;
}

} // namespace sentinel::core
