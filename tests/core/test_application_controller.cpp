#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/IChatHistoryStore.h"
#include "sentinel/core/InMemoryStore.h"
#include "sentinel/core/LocalEchoProvider.h"
#include "sentinel/core/LocalInference.h"
#include "sentinel/core/NullAgentRuntime.h"
#include "sentinel/core/RuntimePermissions.h"

#include <QSignalSpy>
#include <QtTest>

#include <memory>

using sentinel::core::ApplicationController;
using sentinel::core::ChatProviderReply;
using sentinel::core::ChatProviderStatus;
using sentinel::core::IChatHistoryStore;
using sentinel::core::IChatProvider;
using sentinel::core::ILocalInferenceClient;
using sentinel::core::InMemoryStore;
using sentinel::core::LocalEchoProvider;
using sentinel::core::LocalInferenceRequest;
using sentinel::core::LocalInferenceResponse;
using sentinel::core::LocalInferenceStatus;

class UnavailableProvider final : public IChatProvider {
public:
    QString name() const override {
        return QStringLiteral("UnavailableProvider");
    }

    ChatProviderStatus status() const override {
        return ChatProviderStatus::Unavailable;
    }

    ChatProviderReply sendMessage(const QString& message) override {
        Q_UNUSED(message);
        return {false, {}, QStringLiteral("not available")};
    }
};

class ErrorProvider final : public IChatProvider {
public:
    QString name() const override {
        return QStringLiteral("ErrorProvider");
    }

    ChatProviderStatus status() const override {
        return ChatProviderStatus::Ready;
    }

    ChatProviderReply sendMessage(const QString& message) override {
        Q_UNUSED(message);
        return {false, {}, QStringLiteral("deterministic failure")};
    }
};

class RecordingChatHistoryStore final : public IChatHistoryStore {
public:
    explicit RecordingChatHistoryStore(QList<sentinel::core::ChatMessage> messages = {},
                                       bool available = true)
        : messages_(std::move(messages)), available_(available) {}

    QList<sentinel::core::ChatMessage> loadMessages() const override {
        return available_ ? messages_ : QList<sentinel::core::ChatMessage>{};
    }

    void appendMessage(const sentinel::core::ChatMessage& message) override {
        if (available_) {
            messages_.append(message);
        }
    }

    void clear() override {
        if (available_) {
            wasCleared_ = true;
            messages_.clear();
        }
    }

    bool isAvailable() const override {
        return available_;
    }

    QString lastError() const override {
        return available_ ? QString() : QStringLiteral("unavailable");
    }

    QList<sentinel::core::ChatMessage> messages_;
    bool available_ = true;
    bool wasCleared_ = false;
};

class FixedPlanRuntime final : public sentinel::core::IAgentRuntime {
public:
    FixedPlanRuntime(QList<sentinel::core::ToolDescriptor> tools,
                     sentinel::core::ToolInvocationPlan plan)
        : tools_(std::move(tools)), plan_(std::move(plan)) {}

    QString name() const override {
        return QStringLiteral("FixedPlanRuntime");
    }

    sentinel::core::AgentStatus status() const override {
        return sentinel::core::AgentStatus::Ready;
    }

    QList<sentinel::core::AgentCapabilityDescriptor> capabilities() const override {
        return {};
    }

    QList<sentinel::core::ToolDescriptor> availableTools() const override {
        return tools_;
    }

    sentinel::core::ToolInvocationPlan
    plan(const sentinel::core::AgentRequest& request) const override {
        Q_UNUSED(request);
        return plan_;
    }

    sentinel::core::AgentResponse execute(const sentinel::core::AgentRequest& request) override {
        return {
            true,
            QStringLiteral("Fixed local agent placeholder processed: %1")
                .arg(request.prompt.trimmed()),
            sentinel::core::AgentStatus::Ready,
        };
    }

private:
    QList<sentinel::core::ToolDescriptor> tools_;
    sentinel::core::ToolInvocationPlan plan_;
};

class UnavailableMemoryStore final : public sentinel::core::IMemoryStore {
public:
    void put(QString key, QString value) override {
        Q_UNUSED(key);
        Q_UNUSED(value);
    }

    QString get(const QString& key) const override {
        Q_UNUSED(key);
        return {};
    }

    sentinel::core::MemoryEntries entries() const override {
        return {};
    }

    void clear() override {}

    bool isAvailable() const override {
        return false;
    }

    QString lastError() const override {
        return QStringLiteral("unavailable");
    }
};

class AllowLocalInferencePolicy final : public sentinel::core::IRuntimePermissionPolicy {
public:
    sentinel::core::RuntimePermissionDecision
    evaluate(const sentinel::core::RuntimePermissionRequest& request) const override {
        return {
            sentinel::core::RuntimePermissionDecisionStatus::Allowed,
            request,
            QStringLiteral("Local inference permission allowed for injected test policy."),
        };
    }
};

class FakeLocalInferenceClient final : public ILocalInferenceClient {
public:
    LocalInferenceResponse infer(const LocalInferenceRequest& request) override {
        called = true;
        lastRequest = request;
        LocalInferenceResponse response;
        response.status = LocalInferenceStatus::Succeeded;
        response.model = request.options.model;
        response.endpoint = QStringLiteral("http://127.0.0.1:11434");
        response.text = QStringLiteral("fake local completion");
        response.summary = QStringLiteral("Fake local inference completed.");
        response.traces = {
            sentinel::core::LocalInferenceTrace{
                1,
                QStringLiteral("Fake Client"),
                QStringLiteral("Succeeded"),
                QStringLiteral("Injected fake local inference client was called."),
            },
        };
        return response;
    }

    QString statusSummary() const override {
        return QStringLiteral("Fake local inference client is ready.");
    }

    bool called = false;
    LocalInferenceRequest lastRequest;
};

class ApplicationControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesProviderNameAndInitialSystemMessage();
    void exposesProviderStatus();
    void exposesAgentStatusWithoutRuntime();
    void exposesModelRoutingMetadata();
    void exposesTaskPlanMetadata();
    void exposesAgentRegistryMetadata();
    void exposesProviderCatalogMetadata();
    void exposesMemoryCatalogMetadata();
    void exposesOrchestrationSnapshotMetadata();
    void exposesOrchestrationReadinessDiagnostics();
    void exposesLocalRuntimeMetadata();
    void exposesOllamaRuntimeBoundaryMetadata();
    void exposesLocalInferenceBoundaryMetadata();
    void blocksLocalInferenceByDefaultPermission();
    void runsInjectedLocalInferenceWhenPermissionAllows();
    void exposesConversationSessionMetadata();
    void exposesConversationStateMetadata();
    void updatesModelRoutingModeMetadata();
    void keepsConversationSessionSeparateFromChatAndRuntimeSessions();
    void executesDeterministicAgentRequestWithRuntime();
    void exposesAgentToolMetadata();
    void exposesLatestToolPlanStatusWithRuntime();
    void exposesLatestApprovalStatusWithRuntime();
    void exposesLatestSandboxStatusWithRuntime();
    void exposesLatestToolExecutionStatusWithRuntime();
    void exposesSuccessfulPipelineResultWithRuntime();
    void exposesRuntimeContextForPipelineResult();
    void exposesAgentActivityForPipelineResult();
    void reportsRiskyToolPlanRequiresApproval();
    void reportsSandboxBlockedPipelineResult();
    void reportsEmptyPlanPipelineResult();
    void reportsUnknownToolPipelineResult();
    void exposesMemoryStatus();
    void sendsMessageThroughProvider();
    void updatesConversationStateForChatFlow();
    void ignoresBlankChatMessages();
    void handlesUnavailableProvider();
    void handlesProviderErrorReply();
    void clearsChatHistory();
    void loadsPersistedChatHistoryAtStartup();
    void appendsNewChatMessagesToHistoryStore();
    void clearsPersistentChatHistoryWhenAvailable();
    void keepsRuntimeChatWorkingWhenHistoryStoreUnavailable();
    void storesRuntimeMemoryEntries();
    void clearsRuntimeMemoryEntries();
    void failsSafeWhenMemoryStoreUnavailable();
    void rejectsBlankMemoryKeys();
    void overwritesMemoryEntriesThroughStoreBackend();
    void reportsRuntimeOnlyWhenChatStoreUnavailableOnClear();
};

static std::unique_ptr<ApplicationController> makeController() {
    return std::make_unique<ApplicationController>(std::make_unique<LocalEchoProvider>(),
                                                   std::make_unique<InMemoryStore>());
}

void ApplicationControllerTest::exposesProviderNameAndInitialSystemMessage() {
    const auto controller = makeController();

    QCOMPARE(controller->providerName(), QStringLiteral("LocalEchoProvider"));
    QCOMPARE(controller->chatMessages().size(), 1);
    QCOMPARE(controller->chatMessages().first(), QStringLiteral("Sentinel: Sentinel Core online."));
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->chatHistory().first().role, sentinel::core::ChatRole::System);
    QVERIFY(controller->memoryEntries().isEmpty());
}

void ApplicationControllerTest::exposesProviderStatus() {
    const auto controller = makeController();

    QCOMPARE(controller->providerStatus(), QStringLiteral("Ready"));
}

void ApplicationControllerTest::exposesAgentStatusWithoutRuntime() {
    const auto controller = makeController();

    QCOMPARE(controller->agentStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(controller->lastAgentResponse(), QStringLiteral("No agent request yet."));
    QCOMPARE(controller->latestToolPlanStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller->latestToolPlanSummary(), QStringLiteral("No tool plan yet."));
    QCOMPARE(controller->latestApprovalStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller->latestApprovalSummary(), QStringLiteral("No approval decision yet."));
    QCOMPARE(controller->latestSandboxStatus(), QStringLiteral("Not Evaluated"));
    QCOMPARE(controller->latestSandboxSummary(), QStringLiteral("No sandbox evaluation yet."));
    QCOMPARE(controller->latestToolExecutionStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller->latestToolExecutionSummary(),
             QStringLiteral("No tool execution boundary result yet."));
    QCOMPARE(controller->runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(controller->runtimeContextStatus(), QStringLiteral("Empty"));
    QCOMPARE(controller->runtimeContextSummary(), QStringLiteral("No runtime context yet."));
    QVERIFY(controller->runtimeContextActiveToolIds().isEmpty());
    QCOMPARE(controller->agentActivityCount(), 0);
    QCOMPARE(controller->latestAgentActivitySummary(), QStringLiteral("No agent activity yet."));

    const auto ran = controller->runAgentRequest(QStringLiteral("plan"));

    QVERIFY(!ran);
    QCOMPARE(controller->lastAgentResponse(), QStringLiteral("Agent runtime unavailable."));
    QCOMPARE(controller->agentActivityCount(), 2);
    QCOMPARE(controller->latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline blocked: runtime unavailable."));
}

void ApplicationControllerTest::exposesModelRoutingMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->currentRoutingMode(), QStringLiteral("Local Only"));
    QCOMPARE(controller->modelRoutingStatus(), QStringLiteral("Routed"));
    QCOMPARE(controller->selectedModelProviderSummary(),
             QStringLiteral("Local Only -> Local Metadata Provider / Sentinel Local Placeholder"));
}

void ApplicationControllerTest::exposesTaskPlanMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->latestTaskPlanStatus(), QStringLiteral("Fallback Planned"));
    QCOMPARE(controller->plannedTaskStepCount(), 2);
    QCOMPARE(controller->latestTaskPlanSummary(),
             QStringLiteral("Unknown task uses safe local metadata fallback: Local Metadata "
                            "Provider / Sentinel Local Placeholder."));
    QCOMPARE(controller->currentMemoryAffinitySummary(),
             QStringLiteral("Ambient (Available, Public Metadata, Session)"));
}

void ApplicationControllerTest::exposesAgentRegistryMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->registeredAgentCount(), 6);
    QCOMPARE(controller->activeAgentSummaries().size(), 6);
    QVERIFY(controller->activeAgentSummaries().contains(
        QStringLiteral("Atlas (Coordinator, Available, Local)")));
    QVERIFY(controller->activeAgentSummaries().contains(
        QStringLiteral("Vela (Researcher, Available, Cloud)")));
    QCOMPARE(controller->currentAgentSummary(),
             QStringLiteral("Atlas (Coordinator, Available, Local)"));
}

void ApplicationControllerTest::exposesProviderCatalogMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->providerCatalogCount(), 4);
    QCOMPARE(controller->providerCatalogSummaries().size(), 4);
    QVERIFY(controller->providerCatalogSummaries().contains(
        QStringLiteral("Local Metadata Provider (Local, Available)")));
    QVERIFY(controller->providerCatalogSummaries().contains(
        QStringLiteral("OpenAI Cloud (Cloud, Not Configured)")));
}

void ApplicationControllerTest::exposesMemoryCatalogMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->memoryCatalogCount(), 5);
    QCOMPARE(controller->memoryCatalogSummaries().size(), 5);
    QVERIFY(controller->memoryCatalogSummaries().contains(
        QStringLiteral("Episodic (Available, Private, User Controlled)")));
    QVERIFY(controller->memoryCatalogSummaries().contains(
        QStringLiteral("Semantic (Available, Local Only, Durable)")));
    QCOMPARE(controller->currentMemoryAffinitySummary(),
             QStringLiteral("Ambient (Available, Public Metadata, Session)"));
}

void ApplicationControllerTest::exposesOrchestrationSnapshotMetadata() {
    const auto controller = makeController();
    const auto snapshot = controller->currentOrchestrationSnapshot();

    QCOMPARE(snapshot.healthStatus, sentinel::core::OrchestrationHealthStatus::Ready);
    QCOMPARE(snapshot.workspace.routingMode, QStringLiteral("Local Only"));
    QCOMPARE(snapshot.workspace.routingStatus, QStringLiteral("Routed"));
    QCOMPARE(snapshot.workspace.taskPlanStatus, QStringLiteral("Fallback Planned"));
    QCOMPARE(snapshot.workspace.providerCatalogCount, 4);
    QCOMPARE(snapshot.workspace.registeredAgentCount, 6);
    QCOMPARE(snapshot.workspace.memoryCatalogCount, 5);
    QCOMPARE(snapshot.workspace.preferredAgentSummary,
             QStringLiteral("Atlas (Coordinator, Available, Local)"));
    QCOMPARE(snapshot.workspace.memoryAffinitySummary,
             QStringLiteral("Ambient (Available, Public Metadata, Session)"));
    QVERIFY(!snapshot.executionEnabled);
    QCOMPARE(controller->orchestrationSnapshotStatus(), QStringLiteral("Ready"));
    QVERIFY(controller->orchestrationSnapshotSummary().contains(
        QStringLiteral("4 provider entries, 6 agents, 5 memory categories")));
    QVERIFY(controller->orchestrationSignals().contains(
        QStringLiteral("Catalogs: 4 providers / 6 agents / 5 memory")));
}

void ApplicationControllerTest::exposesOrchestrationReadinessDiagnostics() {
    const auto controller = makeController();
    const auto report = controller->currentOrchestrationReadinessReport();

    QCOMPARE(report.status, QStringLiteral("Ready"));
    QCOMPARE(report.checks.size(), 10);
    QCOMPARE(controller->orchestrationReadinessStatus(), QStringLiteral("Ready"));
    QCOMPARE(controller->orchestrationReadinessSummary(),
             QStringLiteral("Ready orchestration readiness: 10 deterministic metadata checks, 10 "
                            "diagnostic entries."));
    QVERIFY(controller->orchestrationDiagnostics().contains(
        QStringLiteral("Info: Routing Mode - Local Only routing mode is set.")));
    QVERIFY(controller->orchestrationDiagnostics().contains(
        QStringLiteral("Info: Cloud Providers - Cloud provider metadata remains not configured.")));
    QVERIFY(controller->orchestrationDiagnostics().contains(
        QStringLiteral("Info: Execution Capability - Execution capability remains disabled.")));
}

void ApplicationControllerTest::exposesLocalRuntimeMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->localRuntimeStatus(), QStringLiteral("Metadata Only"));
    QCOMPARE(controller->localRuntimeHealth(), QStringLiteral("Not Executable"));
    QCOMPARE(controller->localRuntimeSummary(),
             QStringLiteral("Null Local Runtime is metadata-only; local inference execution is "
                            "disabled."));
    QCOMPARE(controller->localRuntimeResponseStatus(), QStringLiteral("Refused"));
    QCOMPARE(controller->localRuntimeResponseSummary(),
             QStringLiteral("Local runtime boundary is metadata-only; execution is disabled."));
    QCOMPARE(controller->localRuntimeCapabilities().size(), 3);
    QVERIFY(controller->localRuntimeCapabilities().contains(
        QStringLiteral("Local Inference (Disabled): Inference execution is intentionally "
                       "disabled.")));
    QCOMPARE(controller->localRuntimeSessionCount(), 1);
    QCOMPARE(controller->localRuntimeSessionStatus(), QStringLiteral("Reserved"));
    QCOMPARE(controller->localRuntimeSessionHealth(), QStringLiteral("Placeholder Only"));
    QCOMPARE(controller->localRuntimeSessionSummary(),
             QStringLiteral("local-runtime-session-1: Reserved placeholder local runtime "
                            "metadata."));
    QCOMPARE(controller->localRuntimeAllocationSummary(),
             QStringLiteral("Metadata-only local runtime allocation; no model or process is "
                            "started."));
    QCOMPARE(controller->localRuntimeReservationSummary(),
             QStringLiteral("Placeholder reservation is held for metadata visibility only."));
    QCOMPARE(controller->localRuntimeSessionSummaries(),
             QStringList{QStringLiteral("local-runtime-session-1: Reserved placeholder local "
                                        "runtime metadata.")});
    QCOMPARE(controller->runtimeCapabilityCount(), 13);
    QCOMPARE(controller->enabledRuntimeCapabilitySummaries().size(), 2);
    QVERIFY(controller->enabledRuntimeCapabilitySummaries().contains(
        QStringLiteral("Privacy-Safe Mode (Security, Enabled): Privacy-safe runtime metadata "
                       "mode is active.")));
    QCOMPARE(controller->disabledRuntimeCapabilitySummaries().size(), 11);
    QVERIFY(controller->disabledRuntimeCapabilitySummaries().contains(
        QStringLiteral("Tool Bridge (Integration, Unavailable): Tool bridge execution is "
                       "unavailable.")));
    QCOMPARE(controller->runtimeNegotiationProfileSummary(),
             QStringLiteral("Metadata-only negotiation profile; no runtime capability is "
                            "activated."));
    QCOMPARE(controller->runtimeNegotiationSummary(),
             QStringLiteral("Metadata-only runtime negotiation: 13 capabilities described, 2 "
                            "enabled as safety metadata."));
    QCOMPARE(controller->localOnlyRuntimeEnforcementSummary(),
             QStringLiteral("Local-only enforcement is active; cloud relay and external runtime "
                            "execution remain unavailable."));
    QCOMPARE(controller->runtimePermissionDecision(), QStringLiteral("Denied"));
    QCOMPARE(controller->runtimePermissionSummary(),
             QStringLiteral("Runtime permission policy is metadata-only and denies execution by "
                            "default."));
    QCOMPARE(controller->runtimeSafetyDecision(), QStringLiteral("Compliant"));
    QCOMPARE(controller->runtimeSafetySummary(),
             QStringLiteral("Runtime safety policy report: local-only and no-execution posture is "
                            "enforced with deterministic metadata rules."));
    QCOMPARE(controller->runtimePipelineStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller->runtimePipelineSummary(),
             QStringLiteral("Runtime request pipeline blocked execution metadata by permission and "
                            "safety policy."));
    QCOMPARE(controller->runtimePipelineTraceSummaries().size(), 4);
    QVERIFY(controller->runtimePipelineTraceSummaries().contains(
        QStringLiteral("Request Received [Metadata Received]: Runtime request pipeline metadata "
                       "evaluation.")));
    QVERIFY(controller->runtimePipelineTraceSummaries().contains(
        QStringLiteral("Execution Boundary [Blocked]: Execution boundary remained blocked; no "
                       "runtime action was performed.")));
    QCOMPARE(controller->executionLifecycleState(), QStringLiteral("Blocked"));
    QCOMPARE(controller->executionLifecycleStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller->executionLifecycleSummary(),
             QStringLiteral("Execution lifecycle reached blocked metadata state; no execution is "
                            "permitted."));
    QCOMPARE(controller->executionLifecycleTraceSummaries().size(), 7);
    QVERIFY(controller->executionLifecycleTraceSummaries().contains(
        QStringLiteral("6. Ready Placeholder [Warning]: Ready placeholder is descriptive and not "
                       "executable.")));
    QVERIFY(controller->executionLifecycleTraceSummaries().contains(
        QStringLiteral("7. Blocked [Blocked]: Execution remains intentionally blocked.")));
    QCOMPARE(controller->executionSessionId(), QStringLiteral("execution-session-1"));
    QCOMPARE(controller->executionSessionStatus(), QStringLiteral("Reserved"));
    QCOMPARE(controller->executionSessionOwnership(), QStringLiteral("Application Controller"));
    QCOMPARE(controller->executionCoordinationMode(), QStringLiteral("Metadata Only"));
    QCOMPARE(controller->executionSessionSummary(),
             QStringLiteral("Execution session is reserved for metadata only."));
    QCOMPARE(controller->executionCoordinationSnapshotSummary(),
             QStringLiteral("Execution coordination snapshot is read-only for "
                            "execution-session-1; lifecycle is Blocked and execution is "
                            "blocked."));
    QCOMPARE(controller->localRuntimeAdapterStatus(), QStringLiteral("Placeholder"));
    QCOMPARE(controller->localRuntimeAdapterHealth(), QStringLiteral("Metadata Only"));
    QCOMPARE(controller->localRuntimeAdapterSummary(),
             QStringLiteral("Ollama local runtime adapter contract is metadata-only; no runtime "
                            "connection is configured."));
    QCOMPARE(controller->localRuntimeAdapterCapabilitySummaries().size(), 3);
    QVERIFY(controller->localRuntimeAdapterCapabilitySummaries().contains(
        QStringLiteral("Model Discovery (Unavailable, Not Executable): Model discovery is "
                       "intentionally disabled.")));
    QCOMPARE(controller->providerRuntimeBridgeStatus(), QStringLiteral("Not Connected"));
    QCOMPARE(controller->providerRuntimeBridgeSummary(),
             QStringLiteral("Provider runtime bridge is not connected and cannot execute provider "
                            "requests."));
    QCOMPARE(controller->providerRuntimeBridgeResponseSummary(),
             QStringLiteral("Provider runtime bridge is metadata-only; no provider or local "
                            "runtime request was executed."));
    QCOMPARE(controller->runtimeIntegrationReadinessStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller->runtimeIntegrationReadinessSummary(),
             QStringLiteral("Runtime integration readiness is blocked: Ollama local "
                            "health/discovery metadata is available, but provider bridge "
                            "execution and inference remain disabled."));
    QCOMPARE(controller->runtimeIntegrationReadinessChecks().size(), 5);
    QVERIFY(controller->runtimeIntegrationReadinessChecks().contains(
        QStringLiteral("Pass: Endpoint Configuration - Safe local Ollama endpoint is configured "
                       "for loopback-only health checks.")));
    QVERIFY(controller->runtimeIntegrationReadinessChecks().contains(
        QStringLiteral("Pass: Model Discovery - Installed model discovery boundary is available "
                       "for read-only local metadata.")));
    QVERIFY(controller->runtimeIntegrationReadinessChecks().contains(
        QStringLiteral("Blocked: Execution Permission - Execution lifecycle, runtime permission, "
                       "safety, and pipeline boundaries still block execution.")));
}

void ApplicationControllerTest::exposesOllamaRuntimeBoundaryMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->ollamaEndpoint(), QStringLiteral("http://127.0.0.1:11434"));
    QCOMPARE(controller->ollamaConnectionStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(controller->ollamaHealthStatus(), QStringLiteral("Unavailable"));
    QVERIFY(controller->ollamaHealthSummary().contains(QStringLiteral("no local health check")));
    QCOMPARE(controller->ollamaModelCount(), 0);
    QVERIFY(controller->ollamaModelSummaries().isEmpty());
}

void ApplicationControllerTest::exposesLocalInferenceBoundaryMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Not Requested"));
    QVERIFY(controller->localInferenceSummary().contains(QStringLiteral("loopback-only")));
    QCOMPARE(controller->localInferenceLastResponseSummary(),
             QStringLiteral("No local inference request yet."));
    QVERIFY(controller->localInferenceTraceSummaries().isEmpty());
}

void ApplicationControllerTest::blocksLocalInferenceByDefaultPermission() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::localInferenceChanged);

    const auto ran =
        controller->runLocalInference(QStringLiteral("hello"), QStringLiteral("llama3.2"));

    QVERIFY(!ran);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller->localInferenceSummary(),
             QStringLiteral("Local inference blocked by runtime permission policy."));
    QVERIFY(controller->localInferenceTraceSummaries().contains(
        QStringLiteral("2. Permission Policy [Denied]: Runtime permission policy is "
                       "metadata-only and denies execution by default.")));
}

void ApplicationControllerTest::runsInjectedLocalInferenceWhenPermissionAllows() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, std::move(fakeClient));
    QSignalSpy spy(controller.get(), &ApplicationController::localInferenceChanged);

    const auto ran =
        controller->runLocalInference(QStringLiteral("hello"), QStringLiteral("local-model"));

    QVERIFY(ran);
    QVERIFY(fakeClientPtr->called);
    QCOMPARE(fakeClientPtr->lastRequest.prompt, QStringLiteral("hello"));
    QCOMPARE(fakeClientPtr->lastRequest.options.model, QStringLiteral("local-model"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Succeeded"));
    QCOMPARE(controller->localInferenceLastResponseSummary(),
             QStringLiteral("Local inference completed for local-model (21 characters)."));
    QVERIFY(controller->localInferenceTraceSummaries().contains(
        QStringLiteral("1. Permission Policy [Allowed]: Local inference permission allowed for "
                       "injected test policy.")));
    QVERIFY(controller->localInferenceTraceSummaries().contains(
        QStringLiteral("2. Safety Policy [Compliant]: Runtime safety policy report: local-only "
                       "and no-execution posture is enforced with deterministic metadata rules.")));
}

void ApplicationControllerTest::exposesConversationSessionMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->conversationSessionId(), QStringLiteral("conversation-session-1"));
    QCOMPARE(controller->conversationSessionStatus(), QStringLiteral("Active"));
    QCOMPARE(controller->interactionMode(), QStringLiteral("Companion"));
    QCOMPARE(controller->attentionState(), QStringLiteral("Observing"));
    QCOMPARE(controller->currentConversationSession().revision, 1);
    QCOMPARE(controller->contextWindowSummary(),
             QStringLiteral("Workspace context window: Local Only route, Atlas (Coordinator, "
                            "Available, Local), Ambient (Available, Public Metadata, Session)."));
    QCOMPARE(controller->currentConversationSession().contextWindow.currentRoutingMode,
             QStringLiteral("Local Only"));
    QCOMPARE(controller->currentConversationSession().contextWindow.preferredAgentSummary,
             QStringLiteral("Atlas (Coordinator, Available, Local)"));
}

void ApplicationControllerTest::exposesConversationStateMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->conversationState(), QStringLiteral("Idle"));
    QCOMPARE(controller->conversationTransitionStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller->conversationTransitionSummary(),
             QStringLiteral("No conversation transition yet."));
}

void ApplicationControllerTest::updatesModelRoutingModeMetadata() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::modelRoutingChanged);
    QSignalSpy taskPlanSpy(controller.get(), &ApplicationController::taskPlanChanged);
    QSignalSpy conversationSpy(controller.get(),
                               &ApplicationController::conversationSessionChanged);
    QSignalSpy snapshotSpy(controller.get(), &ApplicationController::orchestrationSnapshotChanged);

    controller->setRoutingModeByName(QStringLiteral("Balanced"));

    QCOMPARE(controller->currentRoutingMode(), QStringLiteral("Balanced"));
    QCOMPARE(controller->modelRoutingStatus(), QStringLiteral("Routed"));
    QCOMPARE(controller->selectedModelProviderSummary(),
             QStringLiteral("Balanced -> Local Metadata Provider / Sentinel Local Placeholder"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(taskPlanSpy.count(), 1);
    QCOMPARE(controller->latestTaskPlanStatus(), QStringLiteral("Fallback Planned"));
    QCOMPARE(controller->currentOrchestrationSnapshot().workspace.routingMode,
             QStringLiteral("Balanced"));
    QCOMPARE(controller->currentConversationSession().revision, 2);
    QCOMPARE(controller->currentConversationSession().contextWindow.currentRoutingMode,
             QStringLiteral("Balanced"));
    QVERIFY(controller->contextWindowSummary().contains(QStringLiteral("Balanced route")));
    QCOMPARE(conversationSpy.count(), 1);
    QCOMPARE(controller->orchestrationSnapshotStatus(), QStringLiteral("Ready"));
    QCOMPARE(snapshotSpy.count(), 1);

    controller->setRoutingModeByName(QStringLiteral("unknown"));
    QCOMPARE(controller->currentRoutingMode(), QStringLiteral("Local Only"));
    QCOMPARE(spy.count(), 2);
    QCOMPARE(taskPlanSpy.count(), 2);
    QCOMPARE(conversationSpy.count(), 2);
    QCOMPARE(snapshotSpy.count(), 2);
}

void ApplicationControllerTest::keepsConversationSessionSeparateFromChatAndRuntimeSessions() {
    const auto controller = makeController();

    QCOMPARE(controller->conversationSessionId(), QStringLiteral("conversation-session-1"));
    QCOMPARE(controller->runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->conversationState(), QStringLiteral("Idle"));
    QCOMPARE(controller->conversationTransitionStatus(), QStringLiteral("Not Requested"));

    controller->setRoutingModeByName(QStringLiteral("Quality"));

    QCOMPARE(controller->conversationSessionId(), QStringLiteral("conversation-session-1"));
    QCOMPARE(controller->runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(controller->runtimeContextStatus(), QStringLiteral("Empty"));
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->chatMessages().first(), QStringLiteral("Sentinel: Sentinel Core online."));
    QCOMPARE(controller->conversationState(), QStringLiteral("Idle"));
    QCOMPARE(controller->conversationTransitionStatus(), QStringLiteral("Not Requested"));
}

void ApplicationControllerTest::executesDeterministicAgentRequestWithRuntime() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());
    QSignalSpy statusSpy(&controller, &ApplicationController::agentStatusChanged);
    QSignalSpy responseSpy(&controller, &ApplicationController::agentResponseChanged);
    QSignalSpy planSpy(&controller, &ApplicationController::toolPlanChanged);
    QSignalSpy approvalSpy(&controller, &ApplicationController::approvalChanged);
    QSignalSpy sandboxSpy(&controller, &ApplicationController::sandboxChanged);
    QSignalSpy toolExecutionSpy(&controller, &ApplicationController::toolExecutionChanged);
    QSignalSpy runtimeContextSpy(&controller, &ApplicationController::runtimeContextChanged);
    QSignalSpy activitySpy(&controller, &ApplicationController::agentActivityChanged);
    QSignalSpy conversationStateSpy(&controller, &ApplicationController::conversationStateChanged);

    const auto ran = controller.runAgentRequest(QStringLiteral("check local plan"));

    QVERIFY(ran);
    QCOMPARE(controller.agentStatus(), QStringLiteral("Ready"));
    QCOMPARE(controller.lastAgentResponse(),
             QStringLiteral("Local agent placeholder processed: check local plan"));
    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(controller.latestToolPlanSummary(),
             QStringLiteral("Metadata-only tool plan prepared."));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Required"));
    QCOMPARE(controller.latestApprovalSummary(),
             QStringLiteral("Planned tool invocations do not require approval."));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Allowed"));
    QCOMPARE(controller.latestSandboxSummary(),
             QStringLiteral("Planned tool capabilities are allowed by sandbox metadata policy."));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
    QCOMPARE(controller.runtimeContextStatus(), QStringLiteral("Active"));
    QCOMPARE(controller.runtimeContextSummary(),
             QStringLiteral("Runtime context captured pipeline result: Placeholder Succeeded"));
    QCOMPARE(controller.runtimeContextActiveToolIds(),
             QStringList{QStringLiteral("local-plan-summary")});
    QCOMPARE(controller.agentActivityCount(), 6);
    QCOMPARE(controller.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Placeholder Succeeded"));
    QCOMPARE(controller.conversationState(), QStringLiteral("Completed"));
    QCOMPARE(controller.conversationTransitionStatus(), QStringLiteral("Accepted"));
    QCOMPARE(controller.conversationTransitionSummary(),
             QStringLiteral("Accepted conversation transition: Responding -> Completed: agent "
                            "response metadata completed"));
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(responseSpy.count(), 1);
    QCOMPARE(planSpy.count(), 1);
    QCOMPARE(approvalSpy.count(), 1);
    QCOMPARE(sandboxSpy.count(), 1);
    QCOMPARE(toolExecutionSpy.count(), 1);
    QCOMPARE(runtimeContextSpy.count(), 1);
    QCOMPARE(activitySpy.count(), 1);
    QVERIFY(conversationStateSpy.count() >= 6);
}

void ApplicationControllerTest::exposesAgentToolMetadata() {
    const auto controllerWithoutRuntime = makeController();
    QCOMPARE(controllerWithoutRuntime->availableToolCount(), 0);
    QVERIFY(controllerWithoutRuntime->availableToolIds().isEmpty());

    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());

    QCOMPARE(controller.availableToolCount(), 1);
    QCOMPARE(controller.availableToolIds(), QStringList{QStringLiteral("local-plan-summary")});
}

void ApplicationControllerTest::exposesLatestToolPlanStatusWithRuntime() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());

    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller.latestToolPlanSummary(), QStringLiteral("No tool plan yet."));

    QVERIFY(!controller.runAgentRequest(QStringLiteral("   ")));
    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller.latestToolPlanSummary(), QStringLiteral("No tool plan yet."));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));
    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(controller.latestToolPlanSummary(),
             QStringLiteral("Metadata-only tool plan prepared."));
}

void ApplicationControllerTest::exposesLatestApprovalStatusWithRuntime() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());

    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller.latestApprovalSummary(), QStringLiteral("No approval decision yet."));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Required"));
    QCOMPARE(controller.latestApprovalSummary(),
             QStringLiteral("Planned tool invocations do not require approval."));
}

void ApplicationControllerTest::exposesLatestSandboxStatusWithRuntime() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());

    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Not Evaluated"));
    QCOMPARE(controller.latestSandboxSummary(), QStringLiteral("No sandbox evaluation yet."));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Allowed"));
    QCOMPARE(controller.latestSandboxSummary(),
             QStringLiteral("Planned tool capabilities are allowed by sandbox metadata policy."));
}

void ApplicationControllerTest::exposesLatestToolExecutionStatusWithRuntime() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());

    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("No tool execution boundary result yet."));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("No agent pipeline result yet."));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
}

void ApplicationControllerTest::exposesSuccessfulPipelineResultWithRuntime() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());
    QSignalSpy pipelineSpy(&controller, &ApplicationController::agentPipelineChanged);

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));

    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Required"));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Allowed"));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
    QCOMPARE(pipelineSpy.count(), 1);
}

void ApplicationControllerTest::exposesRuntimeContextForPipelineResult() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());
    QSignalSpy runtimeContextSpy(&controller, &ApplicationController::runtimeContextChanged);

    QCOMPARE(controller.runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(controller.runtimeContextStatus(), QStringLiteral("Empty"));
    QCOMPARE(controller.runtimeContextSummary(), QStringLiteral("No runtime context yet."));
    QVERIFY(controller.runtimeContextActiveToolIds().isEmpty());

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));

    QCOMPARE(controller.runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(controller.runtimeContextStatus(), QStringLiteral("Active"));
    QCOMPARE(controller.runtimeContextSummary(),
             QStringLiteral("Runtime context captured pipeline result: Placeholder Succeeded"));
    QCOMPARE(controller.runtimeContextActiveToolIds(),
             QStringList{QStringLiteral("local-plan-summary")});
    QCOMPARE(runtimeContextSpy.count(), 1);
}

void ApplicationControllerTest::exposesAgentActivityForPipelineResult() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());
    QSignalSpy activitySpy(&controller, &ApplicationController::agentActivityChanged);

    QCOMPARE(controller.agentActivityCount(), 0);
    QCOMPARE(controller.latestAgentActivitySummary(), QStringLiteral("No agent activity yet."));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));

    QCOMPARE(controller.agentActivityCount(), 6);
    QCOMPARE(controller.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Placeholder Succeeded"));
    QCOMPARE(activitySpy.count(), 1);
}

void ApplicationControllerTest::reportsRiskyToolPlanRequiresApproval() {
    auto runtime =
        std::make_unique<sentinel::core::NullAgentRuntime>(QList<sentinel::core::ToolDescriptor>{
            sentinel::core::ToolDescriptor{
                QStringLiteral("risky-tool"),
                QStringLiteral("Risky Tool"),
                QStringLiteral("Risk metadata only."),
                sentinel::core::ToolRiskLevel::High,
                sentinel::core::ToolExecutionMode::MetadataOnly,
                {},
            },
        });
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::move(runtime));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft risky plan")));
    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Requires Approval"));
    QCOMPARE(controller.latestApprovalSummary(),
             QStringLiteral("One or more planned tool invocations require approval."));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Blocked By Approval"));
    QCOMPARE(controller.latestSandboxSummary(),
             QStringLiteral("Sandbox capability evaluation is blocked by approval metadata."));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("Execution boundary blocked by approval metadata."));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("Execution boundary blocked by approval metadata."));
    QCOMPARE(controller.agentActivityCount(), 6);
    QCOMPARE(controller.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Blocked"));
    QCOMPARE(controller.conversationState(), QStringLiteral("Waiting For Approval"));
    QCOMPARE(controller.conversationTransitionStatus(), QStringLiteral("Accepted"));
    QCOMPARE(controller.conversationTransitionSummary(),
             QStringLiteral("Accepted conversation transition: Routing -> Waiting For Approval: "
                            "approval metadata required"));
}

void ApplicationControllerTest::reportsSandboxBlockedPipelineResult() {
    const sentinel::core::ToolDescriptor tool{
        QStringLiteral("local-tool"),
        QStringLiteral("Local Tool"),
        QStringLiteral("Metadata only."),
        sentinel::core::ToolRiskLevel::Low,
        sentinel::core::ToolExecutionMode::MetadataOnly,
        {},
    };
    sentinel::core::ToolInvocationPlan plan{
        sentinel::core::ToolInvocationPlanStatus::Planned,
        QStringLiteral("Metadata-only tool plan prepared."),
        {
            sentinel::core::PlannedToolInvocation{
                tool.id,
                tool.name,
                QStringLiteral("Plan metadata."),
                QStringLiteral("Metadata-only rationale."),
                tool.riskLevel,
                tool.executionMode,
                {},
                {
                    sentinel::core::CapabilityDescriptor{
                        QStringLiteral("tool.blocked.capability"),
                        QStringLiteral("Blocked metadata capability."),
                    },
                },
            },
        },
    };
    ApplicationController controller(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        std::make_unique<FixedPlanRuntime>(QList<sentinel::core::ToolDescriptor>{tool}, plan));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft sandbox blocked plan")));

    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Required"));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Denied"));
    QCOMPARE(
        controller.latestSandboxSummary(),
        QStringLiteral("One or more planned capabilities are outside sandbox metadata policy."));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("Execution boundary blocked by sandbox capability metadata."));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("Execution boundary blocked by sandbox capability metadata."));
    QCOMPARE(controller.agentActivityCount(), 6);
    QCOMPARE(controller.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Blocked"));
}

void ApplicationControllerTest::reportsEmptyPlanPipelineResult() {
    sentinel::core::ToolInvocationPlan plan{
        sentinel::core::ToolInvocationPlanStatus::NoToolsAvailable,
        QStringLiteral("No tool metadata is available for planning."),
        {},
    };
    ApplicationController controller(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        std::make_unique<FixedPlanRuntime>(QList<sentinel::core::ToolDescriptor>{}, plan));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft empty plan")));

    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("No Tools Available"));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Not Evaluated"));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Empty Plan"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("No planned tool invocation reached the execution boundary."));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Empty Plan"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("No planned tool invocation reached the execution boundary."));
    QCOMPARE(controller.agentActivityCount(), 6);
    QCOMPARE(controller.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Empty Plan"));
}

void ApplicationControllerTest::reportsUnknownToolPipelineResult() {
    const sentinel::core::ToolDescriptor knownTool{
        QStringLiteral("known-tool"),
        QStringLiteral("Known Tool"),
        QStringLiteral("Metadata only."),
        sentinel::core::ToolRiskLevel::Low,
        sentinel::core::ToolExecutionMode::MetadataOnly,
        {},
    };
    sentinel::core::ToolInvocationPlan plan{
        sentinel::core::ToolInvocationPlanStatus::Planned,
        QStringLiteral("Metadata-only tool plan prepared."),
        {
            sentinel::core::PlannedToolInvocation{
                QStringLiteral("missing-tool"),
                QStringLiteral("Missing Tool"),
                QStringLiteral("Plan metadata."),
                QStringLiteral("Metadata-only rationale."),
                sentinel::core::ToolRiskLevel::Low,
                sentinel::core::ToolExecutionMode::MetadataOnly,
                {},
                {},
            },
        },
    };
    ApplicationController controller(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        std::make_unique<FixedPlanRuntime>(QList<sentinel::core::ToolDescriptor>{knownTool}, plan));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft unknown tool plan")));

    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Required"));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Allowed"));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Unknown Tool"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("Execution boundary rejected unknown tool metadata: missing-tool"));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Unknown Tool"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("Execution boundary rejected unknown tool metadata: missing-tool"));
    QCOMPARE(controller.agentActivityCount(), 6);
    QCOMPARE(controller.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Unknown Tool"));
}

void ApplicationControllerTest::exposesMemoryStatus() {
    const auto controller = makeController();

    QCOMPARE(controller->memoryStatus(), QStringLiteral("Available"));
}

void ApplicationControllerTest::sendsMessageThroughProvider() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    const auto sent = controller->sendMessage(QStringLiteral("status"));

    const auto messages = controller->chatMessages();
    QVERIFY(sent);
    QCOMPARE(messages.size(), 3);
    QCOMPARE(messages.at(1), QStringLiteral("You: status"));
    QCOMPARE(messages.at(2),
             QStringLiteral("Sentinel: Sentinel Core online. Local chat pipeline is active."));
    QCOMPARE(controller->chatHistory().at(1).status, sentinel::core::ChatMessageStatus::Sent);
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Received);
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::updatesConversationStateForChatFlow() {
    const auto controller = makeController();
    QSignalSpy stateSpy(controller.get(), &ApplicationController::conversationStateChanged);

    const auto sent = controller->sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(controller->conversationState(), QStringLiteral("Completed"));
    QCOMPARE(controller->conversationTransitionStatus(), QStringLiteral("Accepted"));
    QCOMPARE(controller->conversationTransitionSummary(),
             QStringLiteral("Accepted conversation transition: Responding -> Completed: chat "
                            "response metadata completed"));
    QCOMPARE(controller->runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(controller->runtimeContextStatus(), QStringLiteral("Empty"));
    QVERIFY(stateSpy.count() >= 6);
}

void ApplicationControllerTest::ignoresBlankChatMessages() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    const auto sent = controller->sendMessage(QStringLiteral("   "));

    QVERIFY(!sent);
    QCOMPARE(controller->chatMessages().size(), 1);
    QCOMPARE(spy.count(), 0);
}

void ApplicationControllerTest::handlesUnavailableProvider() {
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<UnavailableProvider>(), std::make_unique<InMemoryStore>());
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    const auto sent = controller->sendMessage(QStringLiteral("status"));

    QVERIFY(!sent);
    QCOMPARE(controller->providerName(), QStringLiteral("UnavailableProvider"));
    QCOMPARE(controller->providerStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(controller->chatMessages().size(), 3);
    QCOMPARE(controller->chatMessages().last(),
             QStringLiteral("Sentinel: Provider unavailable. Status: Unavailable"));
    QCOMPARE(controller->chatHistory().last().status, sentinel::core::ChatMessageStatus::Error);
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::handlesProviderErrorReply() {
    auto controller = std::make_unique<ApplicationController>(std::make_unique<ErrorProvider>(),
                                                              std::make_unique<InMemoryStore>());
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    const auto sent = controller->sendMessage(QStringLiteral("status"));

    QVERIFY(!sent);
    QCOMPARE(controller->chatMessages().size(), 3);
    QCOMPARE(controller->chatMessages().last(),
             QStringLiteral("Sentinel: Provider error: deterministic failure"));
    QCOMPARE(controller->chatHistory().last().status, sentinel::core::ChatMessageStatus::Error);
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::clearsChatHistory() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    controller->sendMessage(QStringLiteral("status"));
    controller->clearChat();

    QCOMPARE(controller->chatMessages(),
             QStringList{QStringLiteral("Sentinel: Sentinel Core online.")});
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->chatHistory().first().id, 1);
    QCOMPARE(spy.count(), 2);
}

void ApplicationControllerTest::loadsPersistedChatHistoryAtStartup() {
    auto store = std::make_unique<RecordingChatHistoryStore>(QList<sentinel::core::ChatMessage>{
        {4, sentinel::core::ChatRole::System, QStringLiteral("previous system"),
         QDateTime::fromString(QStringLiteral("2026-05-15T12:00:00.000Z"), Qt::ISODateWithMs),
         sentinel::core::ChatMessageStatus::Received},
        {5, sentinel::core::ChatRole::User, QStringLiteral("previous user"),
         QDateTime::fromString(QStringLiteral("2026-05-15T12:01:00.000Z"), Qt::ISODateWithMs),
         sentinel::core::ChatMessageStatus::Sent},
    });
    const auto storePtr = store.get();

    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    QCOMPARE(controller.chatHistory().size(), 2);
    QCOMPARE(controller.chatHistory().first().id, 4);
    QCOMPARE(controller.chatMessages().first(), QStringLiteral("Sentinel: previous system"));
    QCOMPARE(storePtr->messages_.size(), 2);
}

void ApplicationControllerTest::appendsNewChatMessagesToHistoryStore() {
    auto store = std::make_unique<RecordingChatHistoryStore>();
    const auto storePtr = store.get();
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    const auto sent = controller.sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(storePtr->messages_.size(), 3);
    QCOMPARE(storePtr->messages_.at(0).role, sentinel::core::ChatRole::System);
    QCOMPARE(storePtr->messages_.at(1).role, sentinel::core::ChatRole::User);
    QCOMPARE(storePtr->messages_.at(1).content, QStringLiteral("status"));
    QCOMPARE(storePtr->messages_.at(2).role, sentinel::core::ChatRole::Assistant);
}

void ApplicationControllerTest::clearsPersistentChatHistoryWhenAvailable() {
    auto store = std::make_unique<RecordingChatHistoryStore>();
    const auto storePtr = store.get();
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    controller.sendMessage(QStringLiteral("status"));
    const auto cleared = controller.clearChat();

    QVERIFY(cleared);
    QVERIFY(storePtr->wasCleared_);
    QCOMPARE(storePtr->messages_.size(), 1);
    QCOMPARE(storePtr->messages_.first().id, 1);
    QCOMPARE(storePtr->messages_.first().role, sentinel::core::ChatRole::System);
    QCOMPARE(controller.chatHistory().size(), 1);
    QCOMPARE(controller.chatMaintenanceStatus(), QStringLiteral("Clear completed"));
}

void ApplicationControllerTest::keepsRuntimeChatWorkingWhenHistoryStoreUnavailable() {
    auto store =
        std::make_unique<RecordingChatHistoryStore>(QList<sentinel::core::ChatMessage>{}, false);
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    const auto sent = controller.sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(controller.chatHistory().size(), 3);
    QCOMPARE(controller.chatHistory().at(1).content, QStringLiteral("status"));
}

void ApplicationControllerTest::storesRuntimeMemoryEntries() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::memoryEntriesChanged);

    controller->remember(QStringLiteral(" callsign "), QStringLiteral(" Sentinel "));

    QCOMPARE(controller->memoryEntries(), QStringList{QStringLiteral("callsign: Sentinel")});
    QCOMPARE(spy.count(), 1);

    controller->remember(QString(), QStringLiteral("ignored"));
    QCOMPARE(controller->memoryEntries(), QStringList{QStringLiteral("callsign: Sentinel")});
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::clearsRuntimeMemoryEntries() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::memoryEntriesChanged);
    QSignalSpy maintenanceSpy(controller.get(), &ApplicationController::maintenanceStatusChanged);
    controller->remember(QStringLiteral("mode"), QStringLiteral("Companion"));

    const auto cleared = controller->clearMemory();

    QVERIFY(cleared);
    QVERIFY(controller->memoryEntries().isEmpty());
    QCOMPARE(controller->memoryMaintenanceStatus(), QStringLiteral("Clear completed"));
    QCOMPARE(spy.count(), 2);
    QCOMPARE(maintenanceSpy.count(), 1);
}

void ApplicationControllerTest::failsSafeWhenMemoryStoreUnavailable() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<UnavailableMemoryStore>());
    QSignalSpy spy(&controller, &ApplicationController::memoryEntriesChanged);
    QSignalSpy maintenanceSpy(&controller, &ApplicationController::maintenanceStatusChanged);

    const auto cleared = controller.clearMemory();

    QVERIFY(!cleared);
    QCOMPARE(controller.memoryStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(controller.memoryMaintenanceStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(spy.count(), 0);
    QCOMPARE(maintenanceSpy.count(), 1);
}

void ApplicationControllerTest::rejectsBlankMemoryKeys() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::memoryEntriesChanged);

    controller->remember(QStringLiteral("   "), QStringLiteral("ignored"));

    QVERIFY(controller->memoryEntries().isEmpty());
    QCOMPARE(spy.count(), 0);
}

void ApplicationControllerTest::overwritesMemoryEntriesThroughStoreBackend() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::memoryEntriesChanged);

    controller->remember(QStringLiteral("mode"), QStringLiteral("Companion"));
    controller->remember(QStringLiteral("provider"), QStringLiteral("LocalEchoProvider"));
    controller->remember(QStringLiteral("mode"), QStringLiteral("Tactical"));

    const QStringList expected{
        QStringLiteral("mode: Tactical"),
        QStringLiteral("provider: LocalEchoProvider"),
    };

    QCOMPARE(controller->memoryEntries(), expected);
    QCOMPARE(spy.count(), 3);
}

void ApplicationControllerTest::reportsRuntimeOnlyWhenChatStoreUnavailableOnClear() {
    auto store =
        std::make_unique<RecordingChatHistoryStore>(QList<sentinel::core::ChatMessage>{}, false);
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));
    QSignalSpy maintenanceSpy(&controller, &ApplicationController::maintenanceStatusChanged);

    controller.sendMessage(QStringLiteral("status"));
    const auto cleared = controller.clearChat();

    QVERIFY(!cleared);
    QCOMPARE(controller.chatHistory().size(), 1);
    QCOMPARE(controller.chatMaintenanceStatus(), QStringLiteral("Runtime Only"));
    QCOMPARE(maintenanceSpy.count(), 1);
}

QTEST_MAIN(ApplicationControllerTest)

#include "test_application_controller.moc"
