// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>
#include "sentinel/core/app/ContextAssembly.h"

using namespace sentinel::core;

namespace {
constexpr int kMessageCount = 60;
constexpr int kCharacterBodySize = 320;

QList<ConversationWindowMessage> makeTranscript() {
    QList<ConversationWindowMessage> messages;
    messages.reserve(kMessageCount);
    for (int i = 0; i < kMessageCount; ++i) {
        const auto role = i % 2 == 0 ? QStringLiteral("User") : QStringLiteral("Assistant");
        const auto body =
            i % 5 == 0
                ? QStringLiteral("remember my compression preference marker %1").arg(i)
                : QStringLiteral("transcript segment %1 %2")
                      .arg(i)
                      .arg(QString(kCharacterBodySize, QLatin1Char('x')));
        messages.append(ConversationWindowMessage{i + 1, role, body});
    }
    return messages;
}

ConversationSalienceSummary makeSalience() {
    ConversationSaliencePolicy policy;
    policy.maxCharacters = 2400;
    const auto salience = rankConversationSalience(
        {ConversationSalienceCandidate{ContextAssemblySourceKind::Conversation,
                                       QStringLiteral("Window"),
                                       QString(2400, QLatin1Char('a')), 0}},
        QStringLiteral("compression"), {}, QStringLiteral("compression preference"), {},
        QStringLiteral("compression"), policy);
    Q_ASSERT(!salience.selections.isEmpty());
    return salience;
}

ConversationSummaryResult makeExistingSummary() {
    ConversationSummaryPolicy policy;
    policy.maxCharacters = 700;
    return assembleConversationSummary(makeTranscript(), {}, policy);
}
} // namespace

class ContextCompressionBenchmark final : public QObject {
    Q_OBJECT

private slots:
    void benchmarkCompressionPlanning();
    void benchmarkSummaryAssembly();
};

void ContextCompressionBenchmark::benchmarkCompressionPlanning() {
    const auto messages = makeTranscript();
    const auto salience = makeSalience();
    const auto existingSummary = makeExistingSummary();

    ConversationCompressionPolicy policy;
    QBENCHMARK {
        const auto result =
            planConversationCompression(messages, existingSummary, salience, true, policy);
        QVERIFY(result.status != ConversationCompressionStatus::Disabled);
    }
}

void ContextCompressionBenchmark::benchmarkSummaryAssembly() {
    const auto messages = makeTranscript();

    ConversationSummaryPolicy policy;
    QBENCHMARK {
        const auto result = assembleConversationSummary(messages, {}, policy);
        QVERIFY(!result.blocks.isEmpty());
    }
}

QTEST_MAIN(ContextCompressionBenchmark)

#include "bench_context_compression.moc"
