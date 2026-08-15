// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "sentinel/core/agent/LlmAgentRuntime.h"
#include "sentinel/core/agent/NullAgentRuntime.h"

using namespace sentinel::core;

namespace {

class FakeChatProvider final : public IChatProvider {
public:
    QString name() const override {
        return QStringLiteral("FakeChatProvider");
    }
    ChatProviderStatus status() const override {
        return ChatProviderStatus::Ready;
    }

    ChatProviderReply sendMessage(const QString& message) override {
        prompts.append(message);
        if (failNext) {
            failNext = false;
            return {false, {}, QStringLiteral("provider offline")};
        }
        return {true, scriptedReply, {}};
    }

    QStringList prompts;
    QString scriptedReply;
    bool failNext = false;
};

AgentStepRecord sampleRecord() {
    AgentStepRecord record;
    record.index = 1;
    record.thought = QStringLiteral("first step");
    record.toolId = QStringLiteral("run-command");
    record.toolName = QStringLiteral("Run Command");
    record.statusText = QStringLiteral("Succeeded");
    record.observation = QStringLiteral("hello-from-step");
    return record;
}

} // namespace

class LlmAgentRuntimeTest final : public QObject {
    Q_OBJECT

private slots:
    void parsesPlainToolJson() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral(
            "{\"thought\":\"list files\",\"action\":\"tool\",\"tool\":\"run-command\","
            "\"args\":{\"command\":\"ls -la\"}}");

        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        const auto decision = runtime.nextStep(QStringLiteral("show home files"), {});

        QCOMPARE(decision.kind, AgentStepDecision::Kind::ToolCall);
        QCOMPARE(decision.toolId, QStringLiteral("run-command"));
        QCOMPARE(decision.thought, QStringLiteral("list files"));
        QCOMPARE(decision.arguments.size(), 1);
        QCOMPARE(decision.arguments.first().id, QStringLiteral("command"));
        QCOMPARE(decision.arguments.first().value, QStringLiteral("ls -la"));
        QVERIFY(runtime.lastDecisionUsedLlm());
        QVERIFY(provider.prompts.first().contains(QStringLiteral("run-command")));
        QVERIFY(provider.prompts.first().contains(QStringLiteral("show home files")));
    }

    void parsesFencedJson() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral(
            "Here is my decision:\n```json\n{\"action\":\"tool\",\"tool\":\"web-search\","
            "\"args\":{\"query\":\"quantum news\"},\"thought\":\"search\"}\n```\nThanks.");

        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        const auto decision = runtime.nextStep(QStringLiteral("news"), {});

        QCOMPARE(decision.kind, AgentStepDecision::Kind::ToolCall);
        QCOMPARE(decision.toolId, QStringLiteral("web-search"));
        QCOMPARE(decision.arguments.first().value, QStringLiteral("quantum news"));
        QVERIFY(runtime.lastDecisionUsedLlm());
    }

    void parsesFinalAnswer() {
        FakeChatProvider provider;
        provider.scriptedReply =
            QStringLiteral("{\"action\":\"final\",\"answer\":\"Everything is done.\"}");

        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        const auto decision = runtime.nextStep(QStringLiteral("goal"), {});

        QCOMPARE(decision.kind, AgentStepDecision::Kind::FinalAnswer);
        QCOMPARE(decision.answer, QStringLiteral("Everything is done."));
        QVERIFY(runtime.lastDecisionUsedLlm());
    }

    void fallsBackToHeuristicOnGarbage() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral("Sorry, I cannot produce JSON right now.");

        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        const auto decision = runtime.nextStep(QStringLiteral("run echo hi"), {});

        QCOMPARE(decision.kind, AgentStepDecision::Kind::ToolCall);
        QCOMPARE(decision.toolId, QStringLiteral("run-command"));
        QVERIFY(!runtime.lastDecisionUsedLlm());
    }

    void fallsBackToSummaryWhenProviderFailsWithHistory() {
        FakeChatProvider provider;
        provider.failNext = true;

        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        const auto decision =
            runtime.nextStep(QStringLiteral("goal"), QList<AgentStepRecord>{sampleRecord()});

        QCOMPARE(decision.kind, AgentStepDecision::Kind::FinalAnswer);
        QVERIFY(decision.answer.contains(QStringLiteral("hello-from-step")));
        QVERIFY(!runtime.lastDecisionUsedLlm());
    }

    void rejectsUnknownToolFromLlm() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral(
            "{\"action\":\"tool\",\"tool\":\"warp-drive\",\"args\":{},\"thought\":\"go\"}");

        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        const auto decision = runtime.nextStep(QStringLiteral("run echo hi"), {});

        QCOMPARE(decision.kind, AgentStepDecision::Kind::ToolCall);
        QCOMPARE(decision.toolId, QStringLiteral("run-command"));
        QVERIFY(!runtime.lastDecisionUsedLlm());
    }

    void planReturnsPlannedInvocationFromLlm() {
        FakeChatProvider provider;
        provider.scriptedReply =
            QStringLiteral("{\"action\":\"tool\",\"tool\":\"web-search\",\"thought\":\"lookup\","
                           "\"args\":{\"query\":\"latest ai news\"}}");

        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        const auto plan = runtime.plan(AgentRequest{QStringLiteral("latest ai news"), {}});

        QCOMPARE(plan.status, ToolInvocationPlanStatus::Planned);
        QCOMPARE(plan.invocations.size(), 1);
        QCOMPARE(plan.invocations.first().toolId, QStringLiteral("web-search"));
    }

    void planReportsEmptyRequest() {
        FakeChatProvider provider;
        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        const auto plan = runtime.plan(AgentRequest{QStringLiteral("   "), {}});

        QCOMPARE(plan.status, ToolInvocationPlanStatus::EmptyRequest);
        QCOMPARE(provider.prompts.size(), 0);
    }

    void providerlessRuntimeStillPlansHeuristically() {
        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), nullptr);
        QCOMPARE(runtime.status(), AgentStatus::Unavailable);

        const auto decision = runtime.nextStep(QStringLiteral("run echo hi"), {});
        QCOMPARE(decision.kind, AgentStepDecision::Kind::ToolCall);
        QCOMPARE(decision.toolId, QStringLiteral("run-command"));
        QVERIFY(!runtime.lastDecisionUsedLlm());
    }

    void promptContainsStepHistory() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral("{\"action\":\"final\",\"answer\":\"done\"}");

        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        runtime.nextStep(QStringLiteral("goal"), QList<AgentStepRecord>{sampleRecord()});

        QVERIFY(provider.prompts.first().contains(QStringLiteral("hello-from-step")));
        QVERIFY(provider.prompts.first().contains(QStringLiteral("Succeeded")));
    }
};

QTEST_MAIN(LlmAgentRuntimeTest)
#include "test_llm_agent_runtime.moc"
