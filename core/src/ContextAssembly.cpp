#include "sentinel/core/ContextAssembly.h"

#include <algorithm>

namespace sentinel::core {

namespace {

int toInt(qsizetype value) {
    return static_cast<int>(value);
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
    }

    return RetrievalSourcePriority::RecentConversation;
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
                                    policy.includeOrchestration);

        if (item.content.isEmpty()) {
            item.exclusionReason = QStringLiteral("Empty candidate");
        } else if (!sourceAllowed) {
            item.exclusionReason = QStringLiteral("Source disabled by policy");
        }

        normalized.append(item);
    }

    std::stable_sort(normalized.begin(), normalized.end(), [](const auto& left, const auto& right) {
        return static_cast<int>(left.priority) < static_cast<int>(right.priority);
    });

    result.candidates.reserve(normalized.size());
    int remaining = result.budget.maxCharacters;
    for (auto item : normalized) {
        result.budget.estimatedCharacters += item.originalSize;

        if (!item.exclusionReason.isEmpty()) {
            ++result.excludedCandidateCount;
            result.candidates.append(item);
            continue;
        }

        if (remaining <= 0) {
            item.exclusionReason = QStringLiteral("Retrieval budget exhausted");
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
            item.exclusionReason = QStringLiteral("Retrieval budget exhausted");
            ++result.excludedCandidateCount;
            result.candidates.append(item);
            continue;
        }

        item.selected = true;
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

} // namespace sentinel::core
