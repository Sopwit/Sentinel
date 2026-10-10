// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QJsonDocument>
#include <QtTest>
#include <algorithm>

#include "sentinel/core/agent/LlmAgentRuntime.h"
#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/model/ModelLibrary.h"
#include "sentinel/core/runtime/BuiltInToolProvider.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"

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

    ChatProviderReply sendRequest(const QString& message,
                                  const ChatRequestOptions& options) override {
        prompts.append(message);
        requestOptions.append(options);
        if (!options.structuredOutput && !options.nativeToolCalling) {
            if (failNext) {
                failNext = false;
                return {false, {}, QStringLiteral("provider offline")};
            }
            if (!scriptedPlainReplies.isEmpty())
                return {true, scriptedPlainReplies.takeFirst(), {}};
            return {true, scriptedReply, {}};
        }
        if (scriptedRequests.isEmpty())
            return {false, {}, QStringLiteral("missing scripted request")};
        return scriptedRequests.takeFirst();
    }

    QStringList prompts;
    QString scriptedReply;
    QStringList scriptedPlainReplies;
    QList<ChatProviderReply> scriptedRequests;
    QList<ChatRequestOptions> requestOptions;
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
    void serializedNativeRequestIsBudgetedBeforeNetwork() {
        ModelService service;
        service.setLmStudioEndpoint(QStringLiteral("http://127.0.0.1:1"));
        ModelBinding binding;
        binding.providerId = QStringLiteral("lm-studio");
        binding.modelId = QStringLiteral("synthetic-budget-test");
        binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
        binding.capabilities.contextWindow = 512;
        auto provider = service.constructProvider(binding);
        QVERIFY(provider);
        ChatRequestOptions options;
        options.nativeToolCalling = true;
        options.tools = BuiltInToolProvider::descriptors();
        const auto reply = provider->sendRequest(QStringLiteral("Read config.json"), options);
        QVERIFY(!reply.success);
        QCOMPARE(reply.category, ChatProviderErrorCategory::RequestRejected);
        QVERIFY(reply.errorMessage.startsWith(QStringLiteral("Context budget exceeded")));
        QCOMPARE(reply.httpStatus, 0);
    }
    void loadedContextIsNotPublishedMaximum() {
        LMStudioNativeCatalogAdapter catalog;
        QVERIFY(catalog.acceptResponse(
            R"({"models":[{"key":"m","max_context_length":1048576,"loaded_instances":[{"config":{"context_length":8192}},{"config":{"context_length":4096}}]}]})"));
        QCOMPARE(catalog.discoveredModels().first().capabilities.contextWindow.value_or(0), 4096);
        QVERIFY(catalog.acceptResponse(
            R"({"models":[{"key":"m","max_context_length":1048576,"loaded_instances":[]}]})"));
        QCOMPARE(catalog.discoveredModels().first().capabilities.contextWindow.value_or(0),
                 1048576);
    }
    void contextExhaustionStopsWithoutRetryOrClaimingSuccess() {
        // The native scripted response exercises provider context rejection.
        auto shared = std::make_shared<FakeChatProvider>();
        ChatProviderReply rejected;
        rejected.errorMessage = QStringLiteral("Context size has been exceeded.");
        rejected.category = ChatProviderErrorCategory::ProviderFailure;
        shared->scriptedRequests.append(rejected);
        LlmAgentRuntime runtime(BuiltInToolProvider::descriptors(), shared.get());
        ModelBinding binding;
        binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
        runtime.bindModel(binding, shared);
        const auto decision = runtime.nextStep(QStringLiteral("Read math.h"), {});
        QCOMPARE(decision.kind, AgentStepDecision::Kind::GiveUp);
        QVERIFY(decision.reason.startsWith(QStringLiteral("Context exhausted:")));
        QCOMPARE(shared->prompts.size(), 1);
    }
    void toolIndexPreservesDiscoveryUnderDetailedContractBudget() {
        AgentContextInput input;
        input.goal = QStringLiteral("Fix the arithmetic function");
        input.tools = BuiltInToolProvider::descriptors();
        std::sort(input.tools.begin(), input.tools.end(),
                  [](const auto& a, const auto& b) { return a.id < b.id; });
        const auto context = ContextEngine{}.build(input);
        QString catalog;
        for (const auto& item : context.items)
            if (item.kind == AgentContextKind::Tool)
                catalog += item.content + QLatin1Char('\n');
        for (const auto& tool : input.tools)
            QVERIFY2(catalog.contains(tool.id + QLatin1Char('(')) ||
                         catalog.contains(tool.id + QStringLiteral(" |")),
                     qPrintable(tool.id));
        QVERIFY(catalog.contains(QStringLiteral("content!")));
        QVERIFY(context.estimatedTokens < 8192);
    }

    void responseProfileReachesPlannerAsPreferenceExactlyOnce() {
        FakeChatProvider provider;
        provider.scriptedReply = "{\"action\":\"final\",\"grounding\":\"context\",\"answer\":\"Helpful answer\"}";
        LlmAgentRuntime runtime({}, &provider);
        AgentContextInput input;
        input.goal = "Explain recursion";
        input.responseProfileInstructions = "Use [PROFILE_EXAMPLE] concrete examples.";
        runtime.setPlanningContext(ContextEngine{}.build(input));
        runtime.nextStep(input.goal, {});
        QCOMPARE(provider.prompts.last().count("[PROFILE_EXAMPLE]"), 1);
        QVERIFY(provider.prompts.last().contains("grants no tool, workspace, network or credential authority"));
        input.responseProfileInstructions.clear();
        runtime.setPlanningContext(ContextEngine{}.build(input));
        runtime.nextStep(input.goal, {});
        QVERIFY(!provider.prompts.last().contains("[PROFILE_EXAMPLE]"));
    }
    void budgetedSkillContextReachesProviderExactlyOnce() {
        FakeChatProvider provider;
        provider.scriptedReply = "{\"action\":\"final\",\"grounding\":\"context\",\"answer\":"
                                 "\"Specific contextual answer\"}";
        LlmAgentRuntime runtime({}, &provider);
        AgentContextInput input;
        input.goal = "task";
        Skill skill;
        skill.name = "test";
        skill.content = "Append [SKILL_OK].";
        input.skills = {skill};
        runtime.setPlanningContext(ContextEngine{}.build(input));
        runtime.nextStep(input.goal, {});
        QCOMPARE(provider.prompts.last().count("[SKILL_OK]"), 1);
        input.skills.first().state = SkillState::Disabled;
        runtime.setPlanningContext(ContextEngine{}.build(input));
        runtime.nextStep(input.goal, {});
        QVERIFY(!provider.prompts.last().contains("[SKILL_OK]"));
    }
    void registryChangesPlannerDiscovery() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral("{\"action\":\"final\",\"answer\":\"done\"}");
        LlmAgentRuntime runtime({}, &provider);
        InMemoryToolRegistry registry;
        ToolDescriptor first{QStringLiteral("tool-a"), QStringLiteral("Tool A"),
                             QStringLiteral("First tool")};
        ToolDescriptor second{QStringLiteral("tool-b"), QStringLiteral("Tool B"),
                              QStringLiteral("Second tool")};
        ToolDescriptor disabled{QStringLiteral("disabled-tool"), QStringLiteral("Disabled"),
                                QStringLiteral("Disabled tool")};
        disabled.enabled = false;
        class Handler final : public IToolHandler {
        public:
            IToolExecutor::Cancel execute(const ToolExecutionRequest&, const QString&,
                                          const QString&, IToolExecutor::Output,
                                          IToolExecutor::Completion completion) override {
                completion({ToolExecutionStatus::Succeeded, QStringLiteral("done")});
                return {};
            }
        };
        auto handler = std::make_shared<Handler>();
        QVERIFY(registry.registerTool({first, handler}));
        QVERIFY(registry.registerTool({second, handler}));
        QVERIFY(registry.registerTool({disabled, handler}));
        runtime.setToolRegistry(&registry);
        QCOMPARE(runtime.availableTools().size(), 2);
        runtime.nextStep(QStringLiteral("goal"), {});
        QVERIFY(provider.prompts.last().contains(QStringLiteral("tool-a")));
        QVERIFY(provider.prompts.last().contains(QStringLiteral("tool-b")));
        QVERIFY(!provider.prompts.last().contains(QStringLiteral("disabled-tool")));
        QVERIFY(registry.setEnabled(QStringLiteral("tool-a"), false));
        runtime.nextStep(QStringLiteral("goal"), {});
        QVERIFY(!provider.prompts.last().contains(QStringLiteral("tool-a")));
        QVERIFY(registry.setEnabled(QStringLiteral("tool-a"), true));
        runtime.nextStep(QStringLiteral("goal"), {});
        QVERIFY(provider.prompts.last().contains(QStringLiteral("tool-a")));
        QVERIFY(registry.unregisterTool(QStringLiteral("tool-b")));
        runtime.nextStep(QStringLiteral("goal"), {});
        QVERIFY(!provider.prompts.last().contains(QStringLiteral("tool-b")));
    }
    void turkishDirectoryRequestsNeverBecomeShellFallback() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral("malformed planner output");
        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        for (const QString& goal :
             {QStringLiteral("masaüstümde hangi dosyalar var bunu bana söyle"),
              QStringLiteral(
                  "~/Desktop dizinindeki dosya ve klasörleri listele. Bunun için uygun filesystem "
                  "tool'unu kullan, kullanıcı mesajımı shell komutu olarak çalıştırma.")}) {
            const auto decision = runtime.nextStep(goal, {});
            QCOMPARE(decision.kind, AgentStepDecision::Kind::GiveUp);
            QCOMPARE(provider.prompts.size() % 2, 0);
        }
    }

    void modelCanSelectDirectoryToolForTurkishGoal() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral("{\"action\":\"tool\",\"tool\":\"list-directory\","
                                                "\"args\":{\"path\":\"~/Desktop\"}}");
        LlmAgentRuntime runtime(BuiltInToolProvider::descriptors(), &provider);
        const auto decision =
            runtime.nextStep(QStringLiteral("masaüstümde hangi dosyalar var bunu bana söyle"), {});
        QCOMPARE(decision.kind, AgentStepDecision::Kind::ToolCall);
        QCOMPARE(decision.toolId, QStringLiteral("list-directory"));
        QCOMPARE(decision.arguments.first().value, QStringLiteral("~/Desktop"));
    }

    void parsesPlainToolJson() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral(
            "{\"thought\":\"list files\",\"action\":\"tool\",\"tool\":\"run-command\","
            "\"args\":{\"command\":\"ls -la\"}}");

        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        const auto decision = runtime.nextStep(QStringLiteral("show home files"), {});

        QCOMPARE(decision.kind, AgentStepDecision::Kind::ToolCall);
        QCOMPARE(decision.toolId, QStringLiteral("run-command"));
        QVERIFY(decision.thought.isEmpty());
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

        QCOMPARE(decision.kind, AgentStepDecision::Kind::GiveUp);
        QVERIFY(!runtime.lastDecisionUsedLlm());
    }

    void parsesFinalAnswer() {
        FakeChatProvider provider;
        provider.scriptedReply =
            QStringLiteral("{\"action\":\"final\",\"grounding\":\"context\","
                           "\"answer\":\"Everything is done.\"}");

        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        const auto decision = runtime.nextStep(QStringLiteral("goal"), {});

        QCOMPARE(decision.kind, AgentStepDecision::Kind::FinalAnswer);
        QCOMPARE(decision.answer, QStringLiteral("Everything is done."));
        QVERIFY(runtime.lastDecisionUsedLlm());
    }

    void conversationalFinalDoesNotRequestTools() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral(
            "{\"action\":\"final\",\"grounding\":\"context\","
            "\"answer\":\"Merhaba! Sana nasıl yardımcı olabilirim?\"}");
        LlmAgentRuntime runtime(BuiltInToolProvider::descriptors(), &provider);
        runtime.setObservationIntent({});
        const auto decision = runtime.nextStep(QStringLiteral("merhaba"), {});
        QCOMPARE(decision.kind, AgentStepDecision::Kind::FinalAnswer);
        QCOMPARE(decision.grounding, GroundingMode::Context);
        QVERIFY(decision.toolId.isEmpty());
        QVERIFY(provider.prompts.first().contains(QStringLiteral("Tools are optional")));
        QVERIFY(!provider.prompts.first().contains(QStringLiteral("authorization=")));
    }

    void acceptsBriefConversationalGreeting() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral(
            "{\"action\":\"final\",\"grounding\":\"context\",\"answer\":\"Merhaba!\"}");
        LlmAgentRuntime runtime({}, &provider);
        runtime.setObservationIntent({});
        QCOMPARE(runtime.nextStep(QStringLiteral("Merhaba"), {}).kind,
                 AgentStepDecision::Kind::FinalAnswer);
    }

    void nativeProviderCanAnswerWithoutToolsOrEvidence() {
        auto provider = std::make_shared<FakeChatProvider>();
        ChatProviderReply reply;
        reply.success = true;
        reply.message = QStringLiteral("Hello! How can I help?");
        provider->scriptedRequests = {reply};
        LlmAgentRuntime runtime(BuiltInToolProvider::descriptors(), provider.get());
        ModelBinding binding;
        binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
        runtime.bindModel(binding, provider);
        runtime.setObservationIntent({});
        const auto decision = runtime.nextStep(QStringLiteral("hello"), {});
        QCOMPARE(decision.kind, AgentStepDecision::Kind::FinalAnswer);
        QCOMPARE(decision.grounding, GroundingMode::Context);
        QVERIFY(decision.toolId.isEmpty());
    }

    void allExposedDescriptorsResolveWithoutDanglingMetadata() {
        const auto tools = BuiltInToolProvider::descriptors();
        FakeChatProvider provider;
        LlmAgentRuntime runtime(tools, &provider);
        for (const auto& tool : tools) {
            if (!tool.enabled || !tool.exposedToModel)
                continue;
            QJsonObject arguments;
            const auto properties = tool.inputSchema.value("properties").toObject();
            for (const auto& required : tool.inputSchema.value("required").toArray()) {
                const auto name = required.toString();
                const auto field = properties.value(name).toObject();
                const auto choices = field.value("enum").toArray();
                const auto type = field.value("type").toString();
                arguments.insert(name, !choices.isEmpty() ? choices.first()
                                      : type == "integer" ? QJsonValue(1)
                                      : type == "array" ? QJsonValue(QJsonArray{})
                                      : type == "object" ? QJsonValue(QJsonObject{})
                                                         : QJsonValue("fixture"));
            }
            provider.scriptedReply = QString::fromUtf8(QJsonDocument(QJsonObject{
                {"action", "tool"}, {"tool", tool.id}, {"args", arguments}})
                    .toJson(QJsonDocument::Compact));
            const auto decision = runtime.nextStep(QStringLiteral("explicit tool request"), {});
            QCOMPARE(decision.kind, AgentStepDecision::Kind::ToolCall);
            QCOMPARE(decision.toolId, tool.id);
            QCOMPARE(decision.toolName, tool.name);
            QCOMPARE(decision.riskLevel, tool.riskLevel);
            QCOMPARE(decision.executionMode, tool.executionMode);
        }
    }

    void repairsArgumentsAgainstRegisteredSchemaBeforeExecution() {
        FakeChatProvider provider;
        provider.scriptedPlainReplies = {
            QStringLiteral("{\"action\":\"tool\",\"tool\":\"read-file\",\"args\":{\"filename\":\"audit.txt\"}}"),
            QStringLiteral("{\"action\":\"tool\",\"tool\":\"read-file\",\"args\":{\"path\":\"audit.txt\"}}")};
        LlmAgentRuntime runtime(BuiltInToolProvider::descriptors(), &provider);
        const auto decision = runtime.nextStep(QStringLiteral("read audit.txt"), {});
        QCOMPARE(decision.kind, AgentStepDecision::Kind::ToolCall);
        QCOMPARE(decision.arguments.first().id, QStringLiteral("path"));
        QCOMPARE(provider.prompts.size(), 2);
        QVERIFY(provider.prompts.last().contains(QStringLiteral("Invalid arguments for read-file")));
    }

    void nativeArgumentRepairDoesNotPublishUnexecutedCalls() {
        auto provider = std::make_shared<FakeChatProvider>();
        ChatProviderReply invalid;
        invalid.success = true;
        invalid.toolCalls.append(
            {QStringLiteral("bad-call"),
             QStringLiteral("read-file"),
             QJsonObject{{QStringLiteral("filename"), QStringLiteral("audit.txt")}},
             {}});
        ChatProviderReply corrected;
        corrected.success = true;
        corrected.toolCalls.append(
            {QStringLiteral("good-call"),
             QStringLiteral("read-file"),
             QJsonObject{{QStringLiteral("path"), QStringLiteral("audit.txt")}},
             {}});
        ChatProviderReply final;
        final.success = true;
        final.message = QStringLiteral("Observed audit content.");
        provider->scriptedRequests = {invalid, corrected, final};
        LlmAgentRuntime runtime(BuiltInToolProvider::descriptors(), provider.get());
        ModelBinding binding;
        binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
        runtime.bindModel(binding, provider);
        const auto tool = runtime.nextStep(QStringLiteral("Read audit.txt"), {});
        QCOMPARE(tool.kind, AgentStepDecision::Kind::ToolCall);
        QCOMPARE(provider->requestOptions.size(), 2);
        QVERIFY(provider->requestOptions.at(1).priorToolCalls.isEmpty());
        QVERIFY(provider->requestOptions.at(1).toolResults.isEmpty());
        auto record = sampleRecord();
        record.toolId = QStringLiteral("read-file");
        record.observation = QString(12000, QLatin1Char('x'));
        const auto answer = runtime.nextStep(QStringLiteral("Read audit.txt"), {record});
        QCOMPARE(answer.kind, AgentStepDecision::Kind::FinalAnswer);
        QCOMPARE(provider->requestOptions.last().toolResults.size(), 1);
        QCOMPARE(provider->requestOptions.last().toolResults.first().callId,
                 QStringLiteral("good-call"));
        const auto excerpt = provider->requestOptions.last().toolResults.first().content;
        QVERIFY(excerpt.size() < 2300);
        QVERIFY(excerpt.contains(QStringLiteral("excerpt truncated")));
        QCOMPARE(record.observation.size(), 12000); // Authoritative evidence stays intact.
    }

    void unknownNativeCallCanRepairBeforeAnyExecution() {
        auto provider = std::make_shared<FakeChatProvider>();
        ChatProviderReply invalid;
        invalid.success = true;
        invalid.toolCalls.append(
            {QStringLiteral("bad-call"), QStringLiteral("invented-tool"), {}, {}});
        ChatProviderReply corrected;
        corrected.success = true;
        corrected.toolCalls.append(
            {QStringLiteral("good-call"),
             QStringLiteral("read-file"),
             QJsonObject{{QStringLiteral("path"), QStringLiteral("audit.txt")}},
             {}});
        provider->scriptedRequests = {invalid, corrected};
        LlmAgentRuntime runtime(BuiltInToolProvider::descriptors(), provider.get());
        ModelBinding binding;
        binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
        runtime.bindModel(binding, provider);
        const auto tool = runtime.nextStep(QStringLiteral("Read audit.txt"), {});
        QCOMPARE(tool.kind, AgentStepDecision::Kind::ToolCall);
        QCOMPARE(tool.toolId, QStringLiteral("read-file"));
        QCOMPARE(provider->requestOptions.size(), 2);
        QVERIFY(provider->requestOptions.last().priorToolCalls.isEmpty());
    }

    void invalidNativeCallsStopAfterTwoPlanningAttempts() {
        auto provider = std::make_shared<FakeChatProvider>();
        ChatProviderReply invalid;
        invalid.success = true;
        invalid.toolCalls.append(
            {QStringLiteral("bad-call"), QStringLiteral("invented-tool"), {}, {}});
        provider->scriptedRequests = {invalid, invalid};
        LlmAgentRuntime runtime(BuiltInToolProvider::descriptors(), provider.get());
        ModelBinding binding;
        binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
        runtime.bindModel(binding, provider);
        QCOMPARE(runtime.nextStep(QStringLiteral("Read audit.txt"), {}).kind,
                 AgentStepDecision::Kind::GiveUp);
        QCOMPARE(provider->requestOptions.size(), 2);
        QVERIFY(!runtime.lastDecisionUsedLlm());
        ChatProviderReply corrected;
        corrected.success = true;
        corrected.toolCalls.append(
            {QStringLiteral("fresh-call"),
             QStringLiteral("read-file"),
             QJsonObject{{QStringLiteral("path"), QStringLiteral("audit.txt")}},
             {}});
        provider->scriptedRequests = {corrected};
        QCOMPARE(runtime.nextStep(QStringLiteral("Read audit.txt"), {}).kind,
                 AgentStepDecision::Kind::ToolCall);
        QVERIFY(provider->requestOptions.last().priorToolCalls.isEmpty());
    }

    void normalizesOnlyRegisteredToolAction() {
        FakeChatProvider provider;
        LlmAgentRuntime runtime(BuiltInToolProvider::descriptors(), &provider);
        provider.scriptedReply = QStringLiteral(
            "{\"action\":\"read-file\",\"args\":{\"path\":\"audit.txt\"}}");
        auto decision = runtime.nextStep(QStringLiteral("read audit.txt"), {});
        QCOMPARE(decision.kind, AgentStepDecision::Kind::ToolCall);
        QCOMPARE(decision.toolId, QStringLiteral("read-file"));
        provider.scriptedReply = QStringLiteral(
            "{\"action\":\"invented-tool\",\"args\":{}}");
        decision = runtime.nextStep(QStringLiteral("read audit.txt"), {});
        QCOMPARE(decision.kind, AgentStepDecision::Kind::GiveUp);
    }

    void acceptsSingleFencedDecision() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral(
            "```json\n{\"action\":\"final\",\"grounding\":\"context\","
            "\"answer\":\"A useful explanation.\"}\n```");
        LlmAgentRuntime runtime({}, &provider);
        QCOMPARE(runtime.nextStep(QStringLiteral("explain"), {}).kind,
                 AgentStepDecision::Kind::FinalAnswer);
    }

    void acceptsNativeToolContinuationFinalWithEvidenceClaims() {
        auto provider = std::make_shared<FakeChatProvider>();
        ChatProviderReply toolCall;
        toolCall.success = true;
        toolCall.toolCalls.append({QStringLiteral("call-1"),
                                   QStringLiteral("list-directory"),
                                   QJsonObject{{QStringLiteral("path"), QStringLiteral(".")}},
                                   {}});
        ChatProviderReply finalReply;
        finalReply.success = true;
        finalReply.message = QStringLiteral("The workspace contains CMakeLists.txt.");
        provider->scriptedRequests = {toolCall, finalReply};

        LlmAgentRuntime runtime(BuiltInToolProvider::descriptors(), provider.get());
        ModelBinding binding;
        binding.providerId = QStringLiteral("local");
        binding.modelId = QStringLiteral("model");
        binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
        runtime.bindModel(binding, provider);
        ObservationIntent intent;
        intent.requirements.append({ObservationDomain::FileSystem, QStringLiteral("CMakeLists.txt"),
                                    EvidenceFreshness::TurnScoped, ObservationPurpose::Inspect,
                                    ClaimType::FileExists, QStringLiteral("claim-1")});
        runtime.setObservationIntent(intent);
        runtime.setStructuredFacts({{QStringLiteral("claim-1"),
                                     QStringLiteral("CMakeLists.txt"),
                                     true,
                                     {QStringLiteral("call-1")}}});

        const auto tool = runtime.nextStep(QStringLiteral("List the workspace"), {});
        QCOMPARE(tool.kind, AgentStepDecision::Kind::ToolCall);
        QCOMPARE(tool.toolId, QStringLiteral("list-directory"));
        QVERIFY(provider->requestOptions.first().nativeToolCalling);
        QVERIFY(provider->requestOptions.first().tools.size() > 0);
        QVERIFY(!provider->prompts.first().contains(QStringLiteral("risk=")));
        QVERIFY(provider->prompts.first().contains(QStringLiteral("native tool protocol")));
        QVERIFY(!provider->prompts.first().contains(QStringLiteral("Return one JSON decision")));
        QVERIFY(!provider->prompts.first().contains(QStringLiteral("NEXT JSON ACTION")));

        auto record = sampleRecord();
        record.toolId = QStringLiteral("list-directory");
        record.observation = QStringLiteral("CMakeLists.txt");
        const auto final = runtime.nextStep(QStringLiteral("List the workspace"), {record});
        QCOMPARE(final.kind, AgentStepDecision::Kind::FinalAnswer);
        QCOMPARE(final.answer, finalReply.message);
        QCOMPARE(final.grounding, GroundingMode::Verified);
        QVERIFY(final.groundingDeclared);
        QCOMPARE(final.claims.size(), 1);
        QCOMPARE(final.claims.first().id, QStringLiteral("claim-1"));
        QVERIFY(final.claims.first().value);
        QCOMPARE(provider->requestOptions.last().toolResults.first().callId,
                 QStringLiteral("call-1"));
        QVERIFY(!provider->prompts.last().contains(QStringLiteral("risk=")));
        QVERIFY(provider->prompts.last().contains(QStringLiteral("only observed evidence")));
    }

    void fallsBackToHeuristicOnGarbage() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral("Sorry, I cannot produce JSON right now.");

        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        const auto decision = runtime.nextStep(QStringLiteral("run echo hi"), {});

        QCOMPARE(decision.kind, AgentStepDecision::Kind::GiveUp);
        QVERIFY(!runtime.lastDecisionUsedLlm());
    }

    void fallsBackToSummaryWhenProviderFailsWithHistory() {
        FakeChatProvider provider;
        provider.failNext = true;

        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        const auto decision =
            runtime.nextStep(QStringLiteral("goal"), QList<AgentStepRecord>{sampleRecord()});

        QCOMPARE(decision.kind, AgentStepDecision::Kind::GiveUp);
        QVERIFY(!runtime.lastDecisionUsedLlm());
    }

    void rejectsUnknownToolFromLlm() {
        FakeChatProvider provider;
        provider.scriptedReply = QStringLiteral(
            "{\"action\":\"tool\",\"tool\":\"warp-drive\",\"args\":{},\"thought\":\"go\"}");

        LlmAgentRuntime runtime(NullAgentRuntime::standardTools(), &provider);
        const auto decision = runtime.nextStep(QStringLiteral("run echo hi"), {});

        QCOMPARE(decision.kind, AgentStepDecision::Kind::GiveUp);
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
        QCOMPARE(decision.kind, AgentStepDecision::Kind::GiveUp);
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
