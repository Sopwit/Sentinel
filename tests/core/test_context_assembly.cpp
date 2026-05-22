#include "sentinel/core/ContextAssembly.h"

#include <QtTest>

using sentinel::core::assembleConversationSummary;
using sentinel::core::assembleConversationWindow;
using sentinel::core::ContextAssemblyPolicy;
using sentinel::core::ContextAssemblyRequest;
using sentinel::core::ContextAssemblySourceKind;
using sentinel::core::contextAssemblySourceSummaries;
using sentinel::core::ContextAssemblyStatus;
using sentinel::core::contextAssemblyStatusName;
using sentinel::core::contextAssemblySummaryForRequest;
using sentinel::core::ConversationSummaryPolicy;
using sentinel::core::ConversationSummaryStatus;
using sentinel::core::ConversationSalienceCandidate;
using sentinel::core::ConversationSaliencePolicy;
using sentinel::core::ConversationWindowMessage;
using sentinel::core::ConversationWindowPolicy;
using sentinel::core::ConversationWindowStatus;
using sentinel::core::ConversationCompressionPolicy;
using sentinel::core::ConversationCompressionStatus;
using sentinel::core::makeContextAssemblySource;
using sentinel::core::MemoryRelevanceCandidate;
using sentinel::core::MemoryRelevancePolicy;
using sentinel::core::planRetrieval;
using sentinel::core::rankConversationSalience;
using sentinel::core::rankMemoryRelevance;
using sentinel::core::planConversationCompression;
using sentinel::core::RetrievalCandidate;
using sentinel::core::RetrievalPlanningPolicy;
using sentinel::core::RetrievalPlanningStatus;
using sentinel::core::retrievalCandidateTraceSummaries;
using sentinel::core::retrievalSourceSummaries;

class ContextAssemblyTest final : public QObject {
    Q_OBJECT

private slots:
    void createsDeterministicAssemblySummary();
    void reportsEmptyContextWithoutPromptAssembly();
    void conversationWindowPrioritizesRecentMessagesDeterministically();
    void conversationWindowEnforcesCharacterBudget();
    void conversationSummarySummarizesOlderOmittedMessagesChronologically();
    void conversationSummaryEnforcesDeterministicBudget();
    void retrievalPlanningSelectsSourcesByDeterministicPriority();
    void retrievalPlanningAllocatesBudgetAndTruncatesDeterministically();
    void retrievalPlanningPreservesChronologyWithinSource();
    void retrievalPlanningSuppressesDuplicatesAndReportsReasons();
    void retrievalPlanningEnforcesCandidateAndSourceLimits();
    void memoryRelevanceOrdersByDeterministicLiteralScore();
    void memoryRelevanceSuppressesDuplicatesAndReportsExclusions();
    void memoryRelevanceEnforcesBudgetAndStableTies();
    void conversationSalienceScoresDeterministically();
    void conversationSalienceAllocatesAdaptiveBudgetDeterministically();
    void conversationSalienceSuppressesDuplicatesAndReportsCounts();
    void conversationCompressionDisabledAndLowPressureStayMetadataOnly();
    void conversationCompressionPlansHighPressureCandidatesDeterministically();
    void conversationCompressionEnforcesBudgetBounds();
};

void ContextAssemblyTest::createsDeterministicAssemblySummary() {
    const auto summary = contextAssemblySummaryForRequest(
        ContextAssemblyRequest{},
        {
            makeContextAssemblySource(ContextAssemblySourceKind::Conversation, true, true, 2, 42,
                                      QStringLiteral("Two transcript messages.")),
            makeContextAssemblySource(ContextAssemblySourceKind::CommittedMemory, true, true, 1, 18,
                                      QStringLiteral("One committed memory entry.")),
            makeContextAssemblySource(ContextAssemblySourceKind::RuntimeMetadata, true, false, 0, 0,
                                      QStringLiteral("Runtime metadata missing.")),
        },
        ContextAssemblyPolicy{});

    QCOMPARE(summary.status, ContextAssemblyStatus::Ready);
    QCOMPARE(contextAssemblyStatusName(summary.status), QStringLiteral("Ready"));
    QCOMPARE(summary.requestedSourceCount, 3);
    QCOMPARE(summary.availableSourceCount, 2);
    QCOMPARE(summary.candidateBlockCount, 3);
    QCOMPARE(summary.estimatedSize, 60);
    QVERIFY(summary.summary.contains(QStringLiteral("2 available sources")));
    QVERIFY(contextAssemblySourceSummaries(summary)
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("Committed Memory Context")));
}

void ContextAssemblyTest::reportsEmptyContextWithoutPromptAssembly() {
    const auto summary = contextAssemblySummaryForRequest(
        ContextAssemblyRequest{},
        {
            makeContextAssemblySource(ContextAssemblySourceKind::Conversation, true, false, 0, 0,
                                      QStringLiteral("No transcript.")),
            makeContextAssemblySource(ContextAssemblySourceKind::CommittedMemory, true, false, 0, 0,
                                      QStringLiteral("No committed memory.")),
        },
        ContextAssemblyPolicy{});

    QCOMPARE(summary.status, ContextAssemblyStatus::Empty);
    QCOMPARE(summary.availableSourceCount, 0);
    QCOMPARE(summary.candidateBlockCount, 0);
    QVERIFY(summary.checks.contains(QStringLiteral("Prompt assembly: disabled")));
    QVERIFY(summary.checks.contains(QStringLiteral("Automatic context attachment: disabled")));
}

void ContextAssemblyTest::conversationWindowPrioritizesRecentMessagesDeterministically() {
    ConversationWindowPolicy policy;
    policy.maxCharacters = 44;

    const auto result = assembleConversationWindow(
        {
            ConversationWindowMessage{1, QStringLiteral("user"), QStringLiteral("old message")},
            ConversationWindowMessage{2, QStringLiteral("assistant"),
                                      QStringLiteral("middle message")},
            ConversationWindowMessage{3, QStringLiteral("user"), QStringLiteral("new message")},
        },
        policy);

    QCOMPARE(result.status, ConversationWindowStatus::Truncated);
    QCOMPARE(result.summary.includedMessageCount, 2);
    QCOMPARE(result.summary.omittedMessageCount, 1);
    QVERIFY(!result.text.contains(QStringLiteral("old message")));
    QVERIFY(result.text.contains(QStringLiteral("middle message")));
    QVERIFY(result.text.contains(QStringLiteral("new message")));
    QVERIFY(result.text.indexOf(QStringLiteral("middle message")) <
            result.text.indexOf(QStringLiteral("new message")));
}

void ContextAssemblyTest::conversationWindowEnforcesCharacterBudget() {
    ConversationWindowPolicy policy;
    policy.maxCharacters = 18;

    const auto result = assembleConversationWindow(
        {
            ConversationWindowMessage{1, QStringLiteral("user"),
                                      QStringLiteral("abcdefghijklmnopqrstuvwxyz")},
        },
        policy);

    QCOMPARE(result.status, ConversationWindowStatus::Truncated);
    QCOMPARE(result.summary.includedMessageCount, 1);
    QCOMPARE(result.summary.truncatedMessageCount, 1);
    QVERIFY(result.budget.includedCharacters <= policy.maxCharacters);
    QVERIFY(result.text.contains(QStringLiteral("[Conversation History]")));
    QVERIFY(result.text.contains(QStringLiteral("[/Conversation History]")));
}

void ContextAssemblyTest::conversationSummarySummarizesOlderOmittedMessagesChronologically() {
    ConversationWindowPolicy windowPolicy;
    windowPolicy.maxCharacters = 20;
    const QList<ConversationWindowMessage> messages{
        ConversationWindowMessage{1, QStringLiteral("system"), QStringLiteral("system seed")},
        ConversationWindowMessage{2, QStringLiteral("user"), QStringLiteral("old question")},
        ConversationWindowMessage{3, QStringLiteral("assistant"), QStringLiteral("old answer")},
        ConversationWindowMessage{4, QStringLiteral("user"), QStringLiteral("new question")},
    };
    const auto window = assembleConversationWindow(messages, windowPolicy);

    ConversationSummaryPolicy summaryPolicy;
    summaryPolicy.maxCharacters = 400;
    summaryPolicy.maxMessagesPerBlock = 2;
    const auto result = assembleConversationSummary(messages, window.messages, summaryPolicy);

    QCOMPARE(result.status, ConversationSummaryStatus::Ready);
    QCOMPARE(result.window.summarizedMessageCount, 3);
    QCOMPARE(result.window.blockCount, 2);
    QVERIFY(result.text.contains(QStringLiteral("[Conversation Summary]")));
    QVERIFY(result.text.contains(QStringLiteral("#1 system")));
    QVERIFY(result.text.contains(QStringLiteral("#2 user")));
    QVERIFY(result.text.contains(QStringLiteral("#3 assistant")));
    QVERIFY(result.text.indexOf(QStringLiteral("#1 system")) <
            result.text.indexOf(QStringLiteral("#3 assistant")));
    QVERIFY(!result.text.contains(QStringLiteral("new question")));
}

void ContextAssemblyTest::conversationSummaryEnforcesDeterministicBudget() {
    ConversationSummaryPolicy policy;
    policy.maxCharacters = 80;
    policy.maxMessagesPerBlock = 3;

    const QList<ConversationWindowMessage> messages{
        ConversationWindowMessage{1, QStringLiteral("user"), QString(90, QLatin1Char('a'))},
        ConversationWindowMessage{2, QStringLiteral("assistant"), QString(90, QLatin1Char('b'))},
        ConversationWindowMessage{3, QStringLiteral("user"), QString(90, QLatin1Char('c'))},
    };
    const auto result = assembleConversationSummary(messages, {}, policy);

    QCOMPARE(result.status, ConversationSummaryStatus::Truncated);
    QCOMPARE(result.window.blockCount, 1);
    QCOMPARE(result.budget.truncatedBlockCount, 1);
    QVERIFY(result.budget.includedCharacters <= policy.maxCharacters);
    QVERIFY(result.text.contains(QStringLiteral("[Conversation Summary]")));
    QVERIFY(result.text.contains(QStringLiteral("[/Conversation Summary]")));
}

void ContextAssemblyTest::retrievalPlanningSelectsSourcesByDeterministicPriority() {
    RetrievalPlanningPolicy policy;
    policy.maxCharacters = 80;

    const auto result = planRetrieval(
        {
            RetrievalCandidate{ContextAssemblySourceKind::Orchestration,
                               {},
                               QStringLiteral("Orchestration"),
                               QStringLiteral("orchestration")},
            RetrievalCandidate{ContextAssemblySourceKind::CommittedMemory,
                               {},
                               QStringLiteral("Memory"),
                               QStringLiteral("memory")},
            RetrievalCandidate{ContextAssemblySourceKind::Conversation,
                               {},
                               QStringLiteral("Window"),
                               QStringLiteral("recent")},
            RetrievalCandidate{ContextAssemblySourceKind::ConversationSummary,
                               {},
                               QStringLiteral("Summary"),
                               QStringLiteral("summary")},
        },
        policy);

    QCOMPARE(result.status, RetrievalPlanningStatus::Ready);
    QCOMPARE(result.selectedCandidateCount, 4);
    QCOMPARE(result.selectedCandidates.at(0).source, ContextAssemblySourceKind::Conversation);
    QCOMPARE(result.selectedCandidates.at(1).source,
             ContextAssemblySourceKind::ConversationSummary);
    QCOMPARE(result.selectedCandidates.at(2).source, ContextAssemblySourceKind::CommittedMemory);
    QCOMPARE(result.selectedCandidates.at(3).source, ContextAssemblySourceKind::Orchestration);
    QVERIFY(retrievalSourceSummaries(result)
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("Committed Memory Context")));
}

void ContextAssemblyTest::retrievalPlanningAllocatesBudgetAndTruncatesDeterministically() {
    RetrievalPlanningPolicy policy;
    policy.maxCharacters = 12;

    const auto result = planRetrieval(
        {
            RetrievalCandidate{ContextAssemblySourceKind::Conversation,
                               {},
                               QStringLiteral("Window"),
                               QStringLiteral("abcdefghij")},
            RetrievalCandidate{ContextAssemblySourceKind::CommittedMemory,
                               {},
                               QStringLiteral("Memory"),
                               QStringLiteral("klmnopqrst")},
            RetrievalCandidate{ContextAssemblySourceKind::RuntimeMetadata,
                               {},
                               QStringLiteral("Runtime"),
                               QStringLiteral("uv")},
        },
        policy);

    QCOMPARE(result.status, RetrievalPlanningStatus::Truncated);
    QCOMPARE(result.selectedCandidateCount, 2);
    QCOMPARE(result.excludedCandidateCount, 1);
    QCOMPARE(result.truncatedCandidateCount, 1);
    QCOMPARE(result.budget.includedCharacters, 12);
    QCOMPARE(result.selectedCandidates.at(0).content, QStringLiteral("abcdefghij"));
    QCOMPARE(result.selectedCandidates.at(1).content, QStringLiteral("kl"));
    QVERIFY(!result.selectedCandidates.at(1).content.contains(QStringLiteral("m")));
}

void ContextAssemblyTest::retrievalPlanningPreservesChronologyWithinSource() {
    RetrievalPlanningPolicy policy;
    policy.maxCharacters = 100;

    const auto result = planRetrieval(
        {
            RetrievalCandidate{ContextAssemblySourceKind::Conversation,
                               {},
                               QStringLiteral("First"),
                               QStringLiteral("#1 user: old")},
            RetrievalCandidate{ContextAssemblySourceKind::Conversation,
                               {},
                               QStringLiteral("Second"),
                               QStringLiteral("#2 assistant: new")},
            RetrievalCandidate{ContextAssemblySourceKind::CommittedMemory,
                               {},
                               QStringLiteral("Memory"),
                               QStringLiteral("stable = fact")},
        },
        policy);

    QCOMPARE(result.status, RetrievalPlanningStatus::Ready);
    QCOMPARE(result.selectedCandidates.at(0).title, QStringLiteral("First"));
    QCOMPARE(result.selectedCandidates.at(1).title, QStringLiteral("Second"));
    QCOMPARE(result.selectedCandidates.at(2).source, ContextAssemblySourceKind::CommittedMemory);
}

void ContextAssemblyTest::retrievalPlanningSuppressesDuplicatesAndReportsReasons() {
    RetrievalPlanningPolicy policy;
    policy.maxCharacters = 100;

    const auto result = planRetrieval(
        {
            RetrievalCandidate{ContextAssemblySourceKind::Conversation,
                               {},
                               QStringLiteral("First"),
                               QStringLiteral("same content")},
            RetrievalCandidate{ContextAssemblySourceKind::Conversation,
                               {},
                               QStringLiteral("Duplicate"),
                               QStringLiteral("same content")},
            RetrievalCandidate{ContextAssemblySourceKind::CommittedMemory,
                               {},
                               QStringLiteral("Empty"),
                               QString()},
        },
        policy);

    QCOMPARE(result.status, RetrievalPlanningStatus::Truncated);
    QCOMPARE(result.selectedCandidateCount, 1);
    QCOMPARE(result.excludedCandidateCount, 2);
    const auto trace = retrievalCandidateTraceSummaries(result).join(QStringLiteral("\n"));
    QVERIFY(trace.contains(QStringLiteral("Duplicate candidate")));
    QVERIFY(trace.contains(QStringLiteral("Empty candidate")));
}

void ContextAssemblyTest::retrievalPlanningEnforcesCandidateAndSourceLimits() {
    RetrievalPlanningPolicy policy;
    policy.maxCharacters = 100;
    policy.maxCandidates = 1;
    policy.maxSources = 1;

    const auto result = planRetrieval(
        {
            RetrievalCandidate{ContextAssemblySourceKind::Conversation,
                               {},
                               QStringLiteral("Conversation"),
                               QStringLiteral("recent")},
            RetrievalCandidate{ContextAssemblySourceKind::CommittedMemory,
                               {},
                               QStringLiteral("Memory"),
                               QStringLiteral("memory")},
            RetrievalCandidate{ContextAssemblySourceKind::RuntimeMetadata,
                               {},
                               QStringLiteral("Runtime"),
                               QStringLiteral("runtime")},
        },
        policy);

    QCOMPARE(result.status, RetrievalPlanningStatus::Truncated);
    QCOMPARE(result.selectedCandidateCount, 1);
    QCOMPARE(result.excludedCandidateCount, 2);
    QVERIFY(retrievalCandidateTraceSummaries(result)
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("Source count limit reached")));
}

void ContextAssemblyTest::memoryRelevanceOrdersByDeterministicLiteralScore() {
    MemoryRelevancePolicy policy;
    policy.maxCharacters = 200;

    const auto result = rankMemoryRelevance(
        {
            MemoryRelevanceCandidate{QStringLiteral("project.alpha"),
                                     QStringLiteral("Uses blue dashboard styling"), 0},
            MemoryRelevanceCandidate{QStringLiteral("tone.preference"),
                                     QStringLiteral("Keep answers concise"), 1},
            MemoryRelevanceCandidate{QStringLiteral("project.beta"),
                                     QStringLiteral("Unrelated local note"), 2},
        },
        QStringLiteral("alpha dashboard"), QStringLiteral("Alpha planning"),
        QStringLiteral("recent dashboard concise notes"), policy);

    QCOMPARE(result.includedCount, 2);
    QCOMPARE(result.excludedCount, 1);
    QCOMPARE(result.selections.at(0).candidate.key, QStringLiteral("project.alpha"));
    QVERIFY(result.selections.at(0).score.keyOverlap > 0);
    QVERIFY(result.selections.at(0).reason.summary.contains(QStringLiteral("literal key overlap")));
    QVERIFY(result.trace.summary.contains(QStringLiteral("2 included")));
}

void ContextAssemblyTest::memoryRelevanceSuppressesDuplicatesAndReportsExclusions() {
    MemoryRelevancePolicy policy;
    policy.maxCharacters = 200;

    const auto result = rankMemoryRelevance(
        {
            MemoryRelevanceCandidate{QStringLiteral("alpha.one"), QStringLiteral("same value"), 0},
            MemoryRelevanceCandidate{QStringLiteral("alpha.one"), QStringLiteral("same value"), 1},
            MemoryRelevanceCandidate{QStringLiteral("empty.value"), QString(), 2},
            MemoryRelevanceCandidate{QStringLiteral("other"), QStringLiteral("not related"), 3},
        },
        QStringLiteral("alpha value"), {}, {}, policy);

    QCOMPARE(result.includedCount, 1);
    QCOMPARE(result.duplicateCount, 1);
    QVERIFY(sentinel::core::memoryRelevanceExclusionSummaries(result)
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("Duplicate candidate")));
    QVERIFY(sentinel::core::memoryRelevanceExclusionSummaries(result)
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("Empty candidate")));
    QVERIFY(sentinel::core::memoryRelevanceExclusionSummaries(result)
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("No deterministic literal relevance")));
}

void ContextAssemblyTest::memoryRelevanceEnforcesBudgetAndStableTies() {
    MemoryRelevancePolicy policy;
    policy.maxCharacters = 24;
    policy.maxCandidates = 4;

    const auto result = rankMemoryRelevance(
        {
            MemoryRelevanceCandidate{QStringLiteral("alpha.first"),
                                     QStringLiteral("one two three four"), 0},
            MemoryRelevanceCandidate{QStringLiteral("alpha.second"),
                                     QStringLiteral("one two three four"), 1},
            MemoryRelevanceCandidate{QStringLiteral("alpha.third"),
                                     QStringLiteral("one two three four"), 2},
        },
        QStringLiteral("alpha"), {}, {}, policy);

    QCOMPARE(result.selections.at(0).candidate.key, QStringLiteral("alpha.first"));
    QVERIFY(result.includedCount >= 1);
    QVERIFY(result.budget.includedCharacters <= policy.maxCharacters);
    QVERIFY(result.excludedCount >= 1);
    QVERIFY(sentinel::core::memoryRelevanceTraceSummaries(result)
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("Retrieval budget exhausted")));
}

void ContextAssemblyTest::conversationSalienceScoresDeterministically() {
    ConversationSaliencePolicy policy;
    policy.maxCharacters = 240;

    const auto result = rankConversationSalience(
        {
            ConversationSalienceCandidate{ContextAssemblySourceKind::CommittedMemory,
                                          QStringLiteral("Memory Alpha"),
                                          QStringLiteral("alpha dashboard preference"), 1, 26,
                                          false, true, 1},
            ConversationSalienceCandidate{ContextAssemblySourceKind::Conversation,
                                          QStringLiteral("Recent Window"),
                                          QStringLiteral("user discussed alpha budget"), 0, 27,
                                          true, false, 0},
            ConversationSalienceCandidate{ContextAssemblySourceKind::RuntimeMetadata,
                                          QStringLiteral("Runtime"),
                                          QStringLiteral("idle local route"), 2, 16, false, false,
                                          4},
        },
        QStringLiteral("alpha dashboard"), QStringLiteral("Alpha Planning"),
        QStringLiteral("alpha budget"), QStringLiteral("assistant dashboard note"),
        QStringLiteral("alpha dashboard preference"), policy);

    QCOMPARE(result.includedCount, 3);
    QCOMPARE(result.selections.at(0).candidate.source, ContextAssemblySourceKind::Conversation);
    QVERIFY(result.selections.at(0).reason.summary.contains(QStringLiteral("pinned")));
    QCOMPARE(result.selections.at(1).candidate.source, ContextAssemblySourceKind::CommittedMemory);
    QVERIFY(result.selections.at(1).score.committedMemoryOverlap > 0);
    QVERIFY(result.trace.summary.contains(QStringLiteral("3 included")));
}

void ContextAssemblyTest::conversationSalienceAllocatesAdaptiveBudgetDeterministically() {
    ConversationSaliencePolicy policy;
    policy.maxCharacters = 40;
    policy.maxCandidates = 4;
    policy.activeConversationBudgetPercent = 50;
    policy.committedMemoryBudgetPercent = 25;
    policy.runtimeMetadataBudgetPercent = 25;

    const auto result = rankConversationSalience(
        {
            ConversationSalienceCandidate{ContextAssemblySourceKind::Conversation,
                                          QStringLiteral("Window"),
                                          QStringLiteral("alpha conversation text that is long"),
                                          0},
            ConversationSalienceCandidate{ContextAssemblySourceKind::CommittedMemory,
                                          QStringLiteral("Memory"),
                                          QStringLiteral("alpha memory text that is long"), 1, 0,
                                          false, true},
            ConversationSalienceCandidate{ContextAssemblySourceKind::RuntimeMetadata,
                                          QStringLiteral("Runtime"),
                                          QStringLiteral("alpha runtime text that is long"), 2},
        },
        QStringLiteral("alpha"), {}, QStringLiteral("alpha"), {}, QStringLiteral("alpha"), policy);

    QCOMPARE(result.budget.activeConversationBudget, 20);
    QCOMPARE(result.budget.committedMemoryBudget, 10);
    QCOMPARE(result.budget.runtimeMetadataBudget, 10);
    QCOMPARE(result.includedCount, 3);
    QCOMPARE(result.truncatedCount, 3);
    QCOMPARE(result.budget.includedCharacters, 40);
    QVERIFY(result.budget.summary.contains(QStringLiteral("Active conversation 20")));
}

void ContextAssemblyTest::conversationSalienceSuppressesDuplicatesAndReportsCounts() {
    ConversationSaliencePolicy policy;
    policy.maxCharacters = 100;

    const auto result = rankConversationSalience(
        {
            ConversationSalienceCandidate{ContextAssemblySourceKind::Conversation,
                                          QStringLiteral("First"), QStringLiteral("alpha same"), 0},
            ConversationSalienceCandidate{ContextAssemblySourceKind::Conversation,
                                          QStringLiteral("Duplicate"),
                                          QStringLiteral("alpha same"), 1},
            ConversationSalienceCandidate{ContextAssemblySourceKind::CommittedMemory,
                                          QStringLiteral("Empty"), QString(), 2, 0, false, true},
        },
        QStringLiteral("alpha"), {}, QStringLiteral("alpha"), {}, QStringLiteral("alpha"), policy);

    QCOMPARE(result.includedCount, 1);
    QCOMPARE(result.excludedCount, 2);
    QCOMPARE(result.duplicateCount, 1);
    QVERIFY(sentinel::core::conversationSalienceExclusionSummaries(result)
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("Duplicate candidate")));
    QVERIFY(sentinel::core::conversationSalienceExclusionSummaries(result)
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("Empty candidate")));
}

void ContextAssemblyTest::conversationCompressionDisabledAndLowPressureStayMetadataOnly() {
    ConversationCompressionPolicy disabledPolicy;
    disabledPolicy.enabled = false;
    const auto disabled = planConversationCompression({}, {}, {}, false, disabledPolicy);
    QCOMPARE(disabled.status, ConversationCompressionStatus::Disabled);
    QCOMPARE(disabled.selection.candidateCount, 0);

    ConversationCompressionPolicy policy;
    const auto low = planConversationCompression(
        {
            ConversationWindowMessage{1, QStringLiteral("User"), QStringLiteral("short note")},
            ConversationWindowMessage{2, QStringLiteral("Assistant"), QStringLiteral("short reply")},
        },
        {}, {}, false, policy);

    QCOMPARE(low.status, ConversationCompressionStatus::NotNeeded);
    QCOMPARE(low.selection.candidateCount, 0);
    QVERIFY(low.fallback.summary.contains(QStringLiteral("pressure is low")));
    QVERIFY(low.readiness.summary.contains(QStringLiteral("context disabled")));
}

void ContextAssemblyTest::conversationCompressionPlansHighPressureCandidatesDeterministically() {
    QList<ConversationWindowMessage> messages;
    for (int i = 0; i < 32; ++i) {
        const auto role = i % 2 == 0 ? QStringLiteral("User") : QStringLiteral("Assistant");
        const auto body = i % 6 == 0
                              ? QStringLiteral("remember my alpha preference repeated")
                              : QStringLiteral("alpha transcript segment %1 %2")
                                    .arg(i)
                                    .arg(QString(220, QLatin1Char('x')));
        messages.append(ConversationWindowMessage{i + 1, role, body});
    }

    ConversationSaliencePolicy saliencePolicy;
    saliencePolicy.maxCharacters = 80;
    const auto salience = rankConversationSalience(
        {ConversationSalienceCandidate{ContextAssemblySourceKind::Conversation,
                                       QStringLiteral("Window"),
                                       QString(80, QLatin1Char('a')), 0}},
        QStringLiteral("alpha"), {}, QStringLiteral("alpha"), {}, {}, saliencePolicy);

    ConversationCompressionPolicy policy;
    const auto result = planConversationCompression(messages, {}, salience, true, policy);

    QVERIFY(result.status == ConversationCompressionStatus::Planned ||
            result.status == ConversationCompressionStatus::Needed);
    QVERIFY(result.readiness.compressionUseful);
    QVERIFY(result.selection.candidateCount >= 4);
    QVERIFY(result.selection.selectedCandidateCount >= 2);
    QCOMPARE(result.selection.candidates.at(0).kind, QStringLiteral("recent-window"));
    QCOMPARE(result.selection.candidates.at(1).kind, QStringLiteral("older-segment"));
    QVERIFY(result.selection.candidates.at(2).kind == QStringLiteral("high-salience-user-facts") ||
            result.selection.candidates.at(2).kind == QStringLiteral("low-salience-repeated-turns"));
    QVERIFY(result.trace.summary.contains(QStringLiteral("context enabled")));
}

void ContextAssemblyTest::conversationCompressionEnforcesBudgetBounds() {
    QList<ConversationWindowMessage> messages;
    for (int i = 0; i < 30; ++i) {
        messages.append(ConversationWindowMessage{
            i + 1,
            QStringLiteral("User"),
            QStringLiteral("remember my bounded compression preference %1 %2")
                .arg(i)
                .arg(QString(260, QLatin1Char('b'))),
        });
    }

    ConversationCompressionPolicy policy;
    policy.maxCandidateCharacters = 320;
    policy.maxCandidates = 2;
    const auto result = planConversationCompression(messages, {}, {}, true, policy);

    QVERIFY(result.selection.selectedCandidateCount <= 2);
    QVERIFY(result.budget.selectedCharacters <= policy.maxCandidateCharacters);
    QVERIFY(result.budget.remainingCharacters >= 0);
    QVERIFY(result.selection.excludedCandidateCount >= 1);
}

QTEST_MAIN(ContextAssemblyTest)

#include "test_context_assembly.moc"
