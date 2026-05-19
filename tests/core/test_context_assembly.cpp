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
using sentinel::core::ConversationWindowMessage;
using sentinel::core::ConversationWindowPolicy;
using sentinel::core::ConversationWindowStatus;
using sentinel::core::makeContextAssemblySource;
using sentinel::core::planRetrieval;
using sentinel::core::RetrievalCandidate;
using sentinel::core::RetrievalPlanningPolicy;
using sentinel::core::RetrievalPlanningStatus;
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

QTEST_MAIN(ContextAssemblyTest)

#include "test_context_assembly.moc"
