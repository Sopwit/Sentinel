#include "sentinel/desktop/DesktopShellViewModel.h"

#include "sentinel/core/AppSettings.h"
#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/IChatHistoryStore.h"
#include "sentinel/core/InMemorySettingsStore.h"
#include "sentinel/core/InMemoryStore.h"
#include "sentinel/core/LocalEchoProvider.h"
#include "sentinel/core/ModeManager.h"
#include "sentinel/core/NullAgentRuntime.h"
#include "sentinel/core/OllamaRuntime.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QMetaProperty>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

using sentinel::core::ApplicationController;
using sentinel::core::AppSettings;
using sentinel::core::IChatHistoryStore;
using sentinel::core::InMemorySettingsStore;
using sentinel::core::InMemoryStore;
using sentinel::core::LocalEchoProvider;
using sentinel::core::ModeManager;
using sentinel::core::OllamaConfig;
using sentinel::core::OllamaHealthCheckResult;
using sentinel::core::OllamaModelSummary;
using sentinel::desktop::ChatMessageListModel;
using sentinel::desktop::DesktopShellViewModel;

class DesktopShellViewModelTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesInitialShellState();
    void exposesAgentStatusWithoutRuntime();
    void exposesAgentToolMetadata();
    void exposesLatestToolPlanStatus();
    void exposesLatestApprovalStatus();
    void exposesLatestSandboxStatus();
    void exposesLatestToolExecutionStatus();
    void exposesRuntimeContextStatus();
    void exposesAgentActivityStatus();
    void exposesModelRoutingMetadata();
    void exposesTaskPlanMetadata();
    void exposesAgentRegistryMetadata();
    void exposesProviderCatalogMetadata();
    void exposesMemoryCatalogMetadata();
    void exposesOrchestrationSnapshotMetadata();
    void exposesOrchestrationReadinessDiagnostics();
    void exposesLocalRuntimeMetadata();
    void exposesOllamaRuntimeBoundaryMetadata();
    void exposesDiscoveredModelSelectionMetadata();
    void exposesModelManagementReadinessMetadata();
    void exposesVoiceReadinessMetadata();
    void exposesVoiceConfigurationMetadata();
    void exposesLocalInferenceBoundaryMetadata();
    void forwardsBlockedLocalInferenceRequest();
    void exposesConversationSessionMetadata();
    void exposesConversationStateMetadata();
    void exposesConversationRuntimeMetadata();
    void updatesAndPersistsRoutingModeMetadata();
    void updatesVisibleAgentValuesForBlockedPipeline();
    void exposesOnlyQmlSafeAgentVisibilityProperties();
    void exposesChatHistoryStatus();
    void exposesConversationHistorySummaryMetadata();
    void exposesConversationBrowserMetadata();
    void exposesMultiConversationReadinessMetadata();
    void exposesConversationSearchAndExportMetadata();
    void exposesMaintenanceStatuses();
    void exposesStartupLoadedMessages();
    void forwardsChatActions();
    void forwardsDeterministicAgentRequest();
    void ignoresBlankChatActions();
    void clearsMemoryActions();
    void clearsChatActions();
    void reportsRuntimeOnlyChatMaintenanceWhenStoreUnavailable();
    void forwardsModeChanges();
    void forwardsMemoryWrites();
    void forwardsSettingsChanges();
    void keepsSettingsSeparateFromClearActions();
    void tracksNavigationState();
    void ignoresRepeatedAndUnknownNavigationChanges();
};

class ViewModelFixture {
public:
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
};

class StaticChatHistoryStore final : public IChatHistoryStore {
public:
    explicit StaticChatHistoryStore(QList<sentinel::core::ChatMessage> messages = {},
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
            messages_.clear();
        }
    }

    bool isAvailable() const override {
        return available_;
    }

private:
    QList<sentinel::core::ChatMessage> messages_;
    bool available_ = true;
};

class FakeOllamaRuntimeClient final : public sentinel::core::IOllamaRuntimeClient {
public:
    explicit FakeOllamaRuntimeClient(QList<OllamaModelSummary> models)
        : models_(std::move(models)) {}

    OllamaConfig config() const override {
        return {};
    }

    OllamaHealthCheckResult healthCheck() const override {
        OllamaHealthCheckResult result;
        result.connectionStatus = sentinel::core::OllamaConnectionStatus::Connected;
        result.healthStatus = sentinel::core::OllamaHealthStatus::Healthy;
        result.summary = QStringLiteral("Fake Ollama health metadata.");
        return result;
    }

    QList<OllamaModelSummary> installedModels() const override {
        return models_;
    }

private:
    QList<OllamaModelSummary> models_;
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

void DesktopShellViewModelTest::exposesInitialShellState() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.providerName(), QStringLiteral("LocalEchoProvider"));
    QCOMPARE(fixture.viewModel.providerStatus(), QStringLiteral("Ready"));
    QCOMPARE(fixture.viewModel.memoryStatus(), QStringLiteral("Available"));
    QCOMPARE(fixture.viewModel.chatHistoryStatus(), QStringLiteral("Runtime Only"));
    QCOMPARE(fixture.viewModel.currentModeName(), QStringLiteral("Companion Mode"));
    QVERIFY(fixture.viewModel.availableModes().contains(QStringLiteral("Tactical Mode")));
    QCOMPARE(fixture.viewModel.themeName(), QStringLiteral("Sentinel Dark"));
    QCOMPARE(fixture.viewModel.configurationProfile(), QStringLiteral("Desktop Alpha"));
    QCOMPARE(fixture.viewModel.currentPage(), QStringLiteral("Dashboard"));
    QCOMPARE(fixture.viewModel.availablePages(),
             QStringList({QStringLiteral("Memory"), QStringLiteral("Dashboard"),
                          QStringLiteral("Agents"), QStringLiteral("Settings")}));
}

void DesktopShellViewModelTest::exposesAgentStatusWithoutRuntime() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.agentStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(fixture.viewModel.lastAgentResponse(), QStringLiteral("No agent request yet."));
    QCOMPARE(fixture.viewModel.latestToolPlanStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(fixture.viewModel.latestToolPlanSummary(), QStringLiteral("No tool plan yet."));
    QCOMPARE(fixture.viewModel.latestApprovalStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(fixture.viewModel.latestApprovalSummary(),
             QStringLiteral("No approval decision yet."));
    QCOMPARE(fixture.viewModel.latestSandboxStatus(), QStringLiteral("Not Evaluated"));
    QCOMPARE(fixture.viewModel.latestSandboxSummary(),
             QStringLiteral("No sandbox evaluation yet."));
    QCOMPARE(fixture.viewModel.latestToolExecutionStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(fixture.viewModel.latestToolExecutionSummary(),
             QStringLiteral("No tool execution boundary result yet."));
    QCOMPARE(fixture.viewModel.latestAgentPipelineStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(fixture.viewModel.latestAgentPipelineSummary(),
             QStringLiteral("No agent pipeline result yet."));
    QCOMPARE(fixture.viewModel.runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(fixture.viewModel.runtimeContextStatus(), QStringLiteral("Empty"));
    QCOMPARE(fixture.viewModel.runtimeContextSummary(), QStringLiteral("No runtime context yet."));
    QVERIFY(fixture.viewModel.runtimeContextActiveToolIds().isEmpty());
    QCOMPARE(fixture.viewModel.agentActivityCount(), 0);
    QCOMPARE(fixture.viewModel.latestAgentActivitySummary(),
             QStringLiteral("No agent activity yet."));
    QCOMPARE(fixture.viewModel.availableToolCount(), 0);
    QVERIFY(fixture.viewModel.availableToolIds().isEmpty());
}

void DesktopShellViewModelTest::exposesModelRoutingMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.currentRoutingMode(), QStringLiteral("Local Only"));
    QCOMPARE(fixture.viewModel.modelRoutingStatus(), QStringLiteral("Routed"));
    QCOMPARE(fixture.viewModel.selectedModelProviderSummary(),
             QStringLiteral("Local Only -> Local Metadata Provider / Sentinel Local Placeholder"));
}

void DesktopShellViewModelTest::exposesTaskPlanMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.latestTaskPlanStatus(), QStringLiteral("Fallback Planned"));
    QCOMPARE(fixture.viewModel.plannedTaskStepCount(), 2);
    QCOMPARE(fixture.viewModel.latestTaskPlanSummary(),
             QStringLiteral("Unknown task uses safe local metadata fallback: Local Metadata "
                            "Provider / Sentinel Local Placeholder."));
    QCOMPARE(fixture.viewModel.currentMemoryAffinitySummary(),
             QStringLiteral("Ambient (Available, Public Metadata, Session)"));
}

void DesktopShellViewModelTest::exposesAgentRegistryMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.registeredAgentCount(), 6);
    QCOMPARE(fixture.viewModel.activeAgentSummaries().size(), 6);
    QVERIFY(fixture.viewModel.activeAgentSummaries().contains(
        QStringLiteral("Atlas (Coordinator, Available, Local)")));
    QVERIFY(fixture.viewModel.activeAgentSummaries().contains(
        QStringLiteral("Nyx (Guardian, Available, Local)")));
    QCOMPARE(fixture.viewModel.currentAgentSummary(),
             QStringLiteral("Atlas (Coordinator, Available, Local)"));
}

void DesktopShellViewModelTest::exposesProviderCatalogMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.providerCatalogCount(), 4);
    QCOMPARE(fixture.viewModel.providerCatalogSummaries().size(), 4);
    QVERIFY(fixture.viewModel.providerCatalogSummaries().contains(
        QStringLiteral("Local Metadata Provider (Local, Available)")));
    QVERIFY(fixture.viewModel.providerCatalogSummaries().contains(
        QStringLiteral("Anthropic Cloud (Cloud, Not Configured)")));
}

void DesktopShellViewModelTest::exposesMemoryCatalogMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.memoryCatalogCount(), 5);
    QCOMPARE(fixture.viewModel.memoryCatalogSummaries().size(), 5);
    QVERIFY(fixture.viewModel.memoryCatalogSummaries().contains(
        QStringLiteral("Reflective (Available, Sensitive, User Controlled)")));
    QVERIFY(fixture.viewModel.memoryCatalogSummaries().contains(
        QStringLiteral("Ambient (Available, Public Metadata, Session)")));
    QCOMPARE(fixture.viewModel.currentMemoryAffinitySummary(),
             QStringLiteral("Ambient (Available, Public Metadata, Session)"));
}

void DesktopShellViewModelTest::exposesOrchestrationSnapshotMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.orchestrationSnapshotStatus(), QStringLiteral("Ready"));
    QVERIFY(fixture.viewModel.orchestrationSnapshotSummary().contains(
        QStringLiteral("4 provider entries, 6 agents, 5 memory categories")));
    QVERIFY(fixture.viewModel.orchestrationSignals().contains(
        QStringLiteral("Routing: Local Only / Routed")));
    QVERIFY(fixture.viewModel.orchestrationSignals().contains(
        QStringLiteral("Agent: Atlas (Coordinator, Available, Local)")));
    QVERIFY(fixture.viewModel.orchestrationSignals().contains(
        QStringLiteral("Memory: Ambient (Available, Public Metadata, Session)")));
}

void DesktopShellViewModelTest::exposesOrchestrationReadinessDiagnostics() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.orchestrationReadinessStatus(), QStringLiteral("Ready"));
    QCOMPARE(fixture.viewModel.orchestrationReadinessSummary(),
             QStringLiteral("Ready orchestration readiness: 10 deterministic metadata checks, 10 "
                            "diagnostic entries."));
    QCOMPARE(fixture.viewModel.orchestrationDiagnostics().size(), 10);
    QVERIFY(fixture.viewModel.orchestrationDiagnostics().contains(
        QStringLiteral("Info: Privacy Posture - Local-only routing posture is active.")));
    QVERIFY(fixture.viewModel.orchestrationDiagnostics().contains(
        QStringLiteral("Info: Execution Capability - Execution capability remains disabled.")));
}

void DesktopShellViewModelTest::exposesLocalRuntimeMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.localRuntimeStatus(), QStringLiteral("Metadata Only"));
    QCOMPARE(fixture.viewModel.localRuntimeHealth(), QStringLiteral("Not Executable"));
    QCOMPARE(fixture.viewModel.localRuntimeSummary(),
             QStringLiteral("Null Local Runtime is metadata-only; local inference execution is "
                            "disabled."));
    QCOMPARE(fixture.viewModel.localRuntimeResponseStatus(), QStringLiteral("Refused"));
    QCOMPARE(fixture.viewModel.localRuntimeResponseSummary(),
             QStringLiteral("Local runtime boundary is metadata-only; execution is disabled."));
    QCOMPARE(fixture.viewModel.localRuntimeCapabilities().size(), 3);
    QVERIFY(fixture.viewModel.localRuntimeCapabilities().contains(
        QStringLiteral("Streaming (Disabled): Streaming is intentionally disabled.")));
    QCOMPARE(fixture.viewModel.localRuntimeSessionCount(), 1);
    QCOMPARE(fixture.viewModel.localRuntimeSessionStatus(), QStringLiteral("Reserved"));
    QCOMPARE(fixture.viewModel.localRuntimeSessionHealth(), QStringLiteral("Placeholder Only"));
    QCOMPARE(fixture.viewModel.localRuntimeSessionSummary(),
             QStringLiteral("local-runtime-session-1: Reserved placeholder local runtime "
                            "metadata."));
    QCOMPARE(fixture.viewModel.localRuntimeAllocationSummary(),
             QStringLiteral("Metadata-only local runtime allocation; no model or process is "
                            "started."));
    QCOMPARE(fixture.viewModel.localRuntimeReservationSummary(),
             QStringLiteral("Placeholder reservation is held for metadata visibility only."));
    QCOMPARE(fixture.viewModel.runtimeCapabilityCount(), 13);
    QCOMPARE(fixture.viewModel.enabledRuntimeCapabilitySummaries().size(), 2);
    QVERIFY(fixture.viewModel.enabledRuntimeCapabilitySummaries().contains(
        QStringLiteral("Local-Only Enforcement (Security, Enabled): Negotiation metadata enforces "
                       "a local-only posture.")));
    QCOMPARE(fixture.viewModel.disabledRuntimeCapabilitySummaries().size(), 11);
    QVERIFY(fixture.viewModel.disabledRuntimeCapabilitySummaries().contains(
        QStringLiteral("Streaming (Inference, Disabled): Token streaming is disabled.")));
    QCOMPARE(fixture.viewModel.runtimeNegotiationProfileSummary(),
             QStringLiteral("Metadata-only negotiation profile; no runtime capability is "
                            "activated."));
    QCOMPARE(fixture.viewModel.runtimeNegotiationSummary(),
             QStringLiteral("Metadata-only runtime negotiation: 13 capabilities described, 2 "
                            "enabled as safety metadata."));
    QCOMPARE(fixture.viewModel.localOnlyRuntimeEnforcementSummary(),
             QStringLiteral("Local-only enforcement is active; cloud relay and external runtime "
                            "execution remain unavailable."));
    QCOMPARE(fixture.viewModel.runtimePermissionDecision(), QStringLiteral("Denied"));
    QCOMPARE(fixture.viewModel.runtimePermissionSummary(),
             QStringLiteral("Runtime permission policy is metadata-only and denies execution by "
                            "default."));
    QCOMPARE(fixture.viewModel.runtimeSafetyDecision(), QStringLiteral("Compliant"));
    QCOMPARE(fixture.viewModel.runtimeSafetySummary(),
             QStringLiteral("Runtime safety policy report: local-only and no-execution posture is "
                            "enforced with deterministic metadata rules."));
    QCOMPARE(fixture.viewModel.runtimePipelineStatus(), QStringLiteral("Blocked"));
    QCOMPARE(fixture.viewModel.runtimePipelineSummary(),
             QStringLiteral("Runtime request pipeline blocked execution metadata by permission and "
                            "safety policy."));
    QCOMPARE(fixture.viewModel.runtimePipelineTraceSummaries().size(), 4);
    QVERIFY(fixture.viewModel.runtimePipelineTraceSummaries().contains(
        QStringLiteral("Permission Policy [Denied]: Runtime permission policy is metadata-only and "
                       "denies execution by default.")));
    QCOMPARE(fixture.viewModel.executionLifecycleState(), QStringLiteral("Blocked"));
    QCOMPARE(fixture.viewModel.executionLifecycleStatus(), QStringLiteral("Blocked"));
    QCOMPARE(fixture.viewModel.executionLifecycleSummary(),
             QStringLiteral("Execution lifecycle reached blocked metadata state; no execution is "
                            "permitted."));
    QCOMPARE(fixture.viewModel.executionLifecycleTraceSummaries().size(), 7);
    QVERIFY(fixture.viewModel.executionLifecycleTraceSummaries().contains(
        QStringLiteral("7. Blocked [Blocked]: Execution remains intentionally blocked.")));
    QCOMPARE(fixture.viewModel.executionSessionId(), QStringLiteral("execution-session-1"));
    QCOMPARE(fixture.viewModel.executionSessionStatus(), QStringLiteral("Reserved"));
    QCOMPARE(fixture.viewModel.executionSessionOwnership(),
             QStringLiteral("Application Controller"));
    QCOMPARE(fixture.viewModel.executionCoordinationMode(), QStringLiteral("Metadata Only"));
    QCOMPARE(fixture.viewModel.executionSessionSummary(),
             QStringLiteral("Execution session is reserved for metadata only."));
    QCOMPARE(fixture.viewModel.executionCoordinationSnapshotSummary(),
             QStringLiteral("Execution coordination snapshot is read-only for "
                            "execution-session-1; lifecycle is Blocked and execution is "
                            "blocked."));
    QCOMPARE(fixture.viewModel.localRuntimeAdapterStatus(), QStringLiteral("Placeholder"));
    QCOMPARE(fixture.viewModel.localRuntimeAdapterHealth(), QStringLiteral("Metadata Only"));
    QCOMPARE(fixture.viewModel.localRuntimeAdapterSummary(),
             QStringLiteral("Ollama local runtime adapter contract is metadata-only; no runtime "
                            "connection is configured."));
    QCOMPARE(fixture.viewModel.localRuntimeAdapterCapabilitySummaries().size(), 3);
    QVERIFY(fixture.viewModel.localRuntimeAdapterCapabilitySummaries().contains(
        QStringLiteral("Model Discovery (Unavailable, Not Executable): Model discovery is "
                       "intentionally disabled.")));
    QCOMPARE(fixture.viewModel.providerRuntimeBridgeStatus(), QStringLiteral("Not Connected"));
    QCOMPARE(fixture.viewModel.providerRuntimeBridgeSummary(),
             QStringLiteral("Provider runtime bridge is not connected and cannot execute provider "
                            "requests."));
    QCOMPARE(fixture.viewModel.providerRuntimeBridgeResponseSummary(),
             QStringLiteral("Provider runtime bridge is metadata-only; no provider or local "
                            "runtime request was executed."));
    QCOMPARE(fixture.viewModel.runtimeIntegrationReadinessStatus(), QStringLiteral("Blocked"));
    QCOMPARE(fixture.viewModel.runtimeIntegrationReadinessSummary(),
             QStringLiteral("Runtime integration readiness is blocked: Ollama local "
                            "health/discovery metadata is available, but provider bridge "
                            "execution and inference remain disabled."));
    QCOMPARE(fixture.viewModel.runtimeIntegrationReadinessChecks().size(), 5);
    QVERIFY(fixture.viewModel.runtimeIntegrationReadinessChecks().contains(
        QStringLiteral("Pass: Endpoint Configuration - Safe local Ollama endpoint is configured "
                       "for loopback-only health checks.")));
}

void DesktopShellViewModelTest::exposesOllamaRuntimeBoundaryMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.ollamaEndpoint(), QStringLiteral("http://127.0.0.1:11434"));
    QCOMPARE(fixture.viewModel.ollamaConnectionStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(fixture.viewModel.ollamaHealthStatus(), QStringLiteral("Unavailable"));
    QVERIFY(
        fixture.viewModel.ollamaHealthSummary().contains(QStringLiteral("no local health check")));
    QCOMPARE(fixture.viewModel.ollamaModelCount(), 0);
    QVERIFY(fixture.viewModel.ollamaModelNames().isEmpty());
    QVERIFY(fixture.viewModel.ollamaModelSummaries().isEmpty());
}

void DesktopShellViewModelTest::exposesDiscoveredModelSelectionMetadata() {
    ApplicationController controller{
        std::make_unique<LocalEchoProvider>(),
        std::make_unique<InMemoryStore>(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(QList<OllamaModelSummary>{
            {QStringLiteral("llama3.2"), QStringLiteral("2026-05-01T10:00:00Z"), 2147483648},
            {QStringLiteral("mistral"), {}, 1024},
        })};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
    QSignalSpy spy(&viewModel, &DesktopShellViewModel::localModelSelectionChanged);

    QCOMPARE(viewModel.ollamaModelCount(), 2);
    QCOMPARE(viewModel.ollamaModelNames(),
             QStringList({QStringLiteral("llama3.2"), QStringLiteral("mistral")}));
    QVERIFY(viewModel.ollamaModelSummaries().contains(
        QStringLiteral("llama3.2 (2.0 GiB, modified 2026-05-01T10:00:00Z, Local Only)")));
    QCOMPARE(viewModel.selectedLocalModelStatus(), QStringLiteral("Fallback"));
    QCOMPARE(viewModel.selectedLocalModelMetadataSummary(),
             QStringLiteral("Fallback model: llama3.2 (2.0 GiB, modified "
                            "2026-05-01T10:00:00Z, Local Only)"));

    viewModel.setSelectedLocalModel(QStringLiteral("mistral"));

    QCOMPARE(settings.selectedLocalModel(), QStringLiteral("mistral"));
    QCOMPARE(viewModel.selectedLocalModel(), QStringLiteral("mistral"));
    QCOMPARE(viewModel.selectedLocalModelStatus(), QStringLiteral("Available"));
    QCOMPARE(viewModel.selectedLocalModelMetadataSummary(),
             QStringLiteral("Selected model: mistral (1.0 KiB, Local Only)"));
    QVERIFY(spy.count() >= 1);

    viewModel.setSelectedLocalModel(QStringLiteral("missing"));

    QCOMPARE(settings.selectedLocalModel(), QStringLiteral("missing"));
    QCOMPARE(viewModel.selectedLocalModelStatus(), QStringLiteral("Invalid"));
    QCOMPARE(viewModel.selectedLocalModelMetadataSummary(),
             QStringLiteral("Invalid selection: missing is not in discovered local model "
                            "metadata."));
}

void DesktopShellViewModelTest::exposesModelManagementReadinessMetadata() {
    ApplicationController controller{
        std::make_unique<LocalEchoProvider>(),
        std::make_unique<InMemoryStore>(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(QList<OllamaModelSummary>{
            {QStringLiteral("llama3.2"), QStringLiteral("2026-05-01T10:00:00Z"), 2147483648},
        })};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};

    QCOMPARE(viewModel.modelManagementStatus(), QStringLiteral("Metadata Only"));
    QCOMPARE(viewModel.modelManagementSummary(),
             QStringLiteral("Model management readiness is metadata-only: 1 installed local "
                            "models reported, selected model llama3.2, actions unavailable."));
    QVERIFY(viewModel.modelManagementActionAvailability().contains(
        QStringLiteral("Pull for llama3.2 is unavailable")));
    QCOMPARE(viewModel.modelRecommendationSummaries().size(), 3);
    QCOMPARE(viewModel.modelRequirementSummaries().size(), 3);
    QVERIFY(
        viewModel.modelRecommendationSummaries().first().contains(QStringLiteral("llama3.2:3b")));
    QVERIFY(viewModel.modelRequirementSummaries().first().contains(
        QStringLiteral("approx disk about 3 GB")));
}

void DesktopShellViewModelTest::exposesVoiceReadinessMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.voiceRuntimeMode(), QStringLiteral("Disabled"));
    QVERIFY(!fixture.viewModel.voiceEnabled());
    QCOMPARE(fixture.viewModel.voiceReadinessStatus(), QStringLiteral("Disabled"));
    QVERIFY(fixture.viewModel.voiceReadinessSummary().contains(QStringLiteral("metadata-only")));
    QCOMPARE(fixture.viewModel.textToSpeechStatus(), QStringLiteral("Disabled"));
    QCOMPARE(fixture.viewModel.speechToTextStatus(), QStringLiteral("Disabled"));
    QVERIFY(
        fixture.viewModel.textToSpeechSummary().contains(QStringLiteral("disabled placeholder")));
    QVERIFY(
        fixture.viewModel.speechToTextSummary().contains(QStringLiteral("disabled placeholder")));
    QCOMPARE(fixture.viewModel.voiceCapabilitySummaries().size(), 2);
    QVERIFY(fixture.viewModel.voiceCapabilitySummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Speech To Text")));
    QVERIFY(fixture.viewModel.voiceReadinessChecks()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("No microphone access")));
    QVERIFY(fixture.viewModel.voiceReadinessChecks()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("No Piper")));
    QCOMPARE(fixture.viewModel.voiceSessionId(), QStringLiteral("voice-session-1"));
    QCOMPARE(fixture.viewModel.voiceSessionStatus(), QStringLiteral("completed"));
    QVERIFY(fixture.viewModel.voiceSessionSummary().contains(QStringLiteral("metadata-only")));
    QCOMPARE(fixture.viewModel.voicePipelineStatus(), QStringLiteral("completed"));
    QVERIFY(fixture.viewModel.voicePipelineSummary().contains(QStringLiteral("metadata-only")));
    QCOMPARE(fixture.viewModel.voicePipelineTraceSummaries().size(), 7);
    QVERIFY(fixture.viewModel.voicePipelineTraceSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("synthesis-placeholder")));
    QCOMPARE(fixture.viewModel.voiceRuntimeStatus(), QStringLiteral("Unavailable"));
    QVERIFY(fixture.viewModel.voiceRuntimeSummary().contains(QStringLiteral("playback disabled")));
    QCOMPARE(fixture.viewModel.voiceRuntimeCheckSummaries().size(), 7);
    QVERIFY(!fixture.viewModel.voiceRuntimeAvailable());
    QVERIFY(!fixture.viewModel.voiceTextToSpeechAvailable());
    QVERIFY(!fixture.viewModel.voiceSpeechToTextAvailable());
    QVERIFY(!fixture.viewModel.voiceMicrophoneEnabled());
    QVERIFY(!fixture.viewModel.voicePlaybackEnabled());
    QVERIFY(fixture.viewModel.voiceLocalOnlyPolicy());
    QVERIFY(!fixture.viewModel.voiceProcessExecutionEnabled());
    QCOMPARE(fixture.viewModel.voiceRuntimeEnvironmentStatus(), QStringLiteral("Blocked"));
    QVERIFY(fixture.viewModel.voiceRuntimeEnvironmentSummary().contains(
        QStringLiteral("metadata-only")));
    QCOMPARE(fixture.viewModel.voiceBinarySummaries().size(), 2);
    QVERIFY(fixture.viewModel.voiceBinarySummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Whisper Binary: Missing")));
    QCOMPARE(fixture.viewModel.voiceModelSummaries().size(), 2);
    QVERIFY(fixture.viewModel.voiceModelSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Piper Voice Model: Missing")));
    QCOMPARE(fixture.viewModel.voiceRuntimePermissionSummaries().size(), 4);
    QVERIFY(fixture.viewModel.voiceRuntimePermissionSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Microphone: Denied")));
    QCOMPARE(fixture.viewModel.voiceRuntimeSafetyStatus(), QStringLiteral("Blocked"));
    QVERIFY(
        fixture.viewModel.voiceRuntimeSafetySummary().contains(QStringLiteral("blocks execution")));
    QCOMPARE(fixture.viewModel.voiceRuntimeSafetyChecks().size(), 7);
    QVERIFY(!fixture.viewModel.voiceRuntimeExecutionAllowed());
    QCOMPARE(fixture.viewModel.piperTtsStatus(), QStringLiteral("Disabled"));
    QVERIFY(fixture.viewModel.piperTtsSummary().contains(QStringLiteral("disabled by default")));
    QCOMPARE(fixture.viewModel.piperTtsReadinessChecks().size(), 9);
    QVERIFY(fixture.viewModel.piperTtsReadinessChecks()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Piper binary")));
    QVERIFY(!fixture.viewModel.piperTtsReady());
    QCOMPARE(fixture.viewModel.piperTtsFileOutputStatus(), QStringLiteral("Disabled"));
    QVERIFY(fixture.viewModel.piperTtsFileOutputSummary().contains(
        QStringLiteral("No playback or microphone access")));
}

void DesktopShellViewModelTest::exposesVoiceConfigurationMetadata() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto piperBinaryPath = dir.filePath(QStringLiteral("piper"));
    const auto piperModelPath = dir.filePath(QStringLiteral("voice.onnx"));
    const auto whisperBinaryPath = dir.filePath(QStringLiteral("whisper"));
    const auto whisperModelPath = dir.filePath(QStringLiteral("whisper-models"));

    QFile piperBinary{piperBinaryPath};
    QVERIFY(piperBinary.open(QIODevice::WriteOnly));
    piperBinary.close();
    QVERIFY(QFile::setPermissions(piperBinaryPath, QFile::ReadOwner | QFile::ExeOwner));

    QFile piperModel{piperModelPath};
    QVERIFY(piperModel.open(QIODevice::WriteOnly));
    piperModel.close();

    QFile whisperBinary{whisperBinaryPath};
    QVERIFY(whisperBinary.open(QIODevice::WriteOnly));
    whisperBinary.close();
    QVERIFY(QFile::setPermissions(whisperBinaryPath, QFile::ReadOwner));
    QVERIFY(QDir{}.mkpath(whisperModelPath));

    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::voiceConfigurationChanged);

    fixture.viewModel.applyVoiceConfigurationPaths(piperBinaryPath, piperModelPath,
                                                   whisperBinaryPath, whisperModelPath);

    QCOMPARE(spy.count(), 4);
    QCOMPARE(fixture.settings.piperBinaryPath(), piperBinaryPath);
    QCOMPARE(fixture.settings.piperModelPath(), piperModelPath);
    QCOMPARE(fixture.settings.whisperBinaryPath(), whisperBinaryPath);
    QCOMPARE(fixture.settings.whisperModelPath(), whisperModelPath);
    QVERIFY(fixture.viewModel.voiceConfigurationReadinessSummary().contains(
        QStringLiteral("Piper file-output TTS: Ready")));
    QVERIFY(fixture.viewModel.voiceConfigurationSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Piper binary metadata only: path exists, readable, "
                                         "executable")));
    QVERIFY(fixture.viewModel.voiceConfigurationSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Whisper binary metadata only: path exists, readable, "
                                         "non-executable")));
    QVERIFY(fixture.viewModel.voiceConfigurationStatusBadges().contains(
        QStringLiteral("Piper file-output TTS: Ready")));
    QVERIFY(fixture.viewModel.voiceConfigurationStatusBadges().contains(
        QStringLiteral("Piper binary: Ready")));
    QVERIFY(fixture.viewModel.voiceConfigurationStatusBadges().contains(
        QStringLiteral("Whisper binary: Blocked (path is not executable)")));
    QVERIFY(fixture.viewModel.voiceConfigurationValidationSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Piper binary: Ready - exists, readable, file, "
                                         "executable")));
    QVERIFY(fixture.viewModel.voiceConfigurationHintSummaries().contains(
        QStringLiteral("Piper binary hint: configured path is executable; no "
                       "suggestion needed.")));
    QVERIFY(fixture.viewModel.voiceConfigurationHintSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("settings are not changed automatically")) ||
            fixture.viewModel.voiceConfigurationHintSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("no executable found in known Homebrew/local locations")));
    QCOMPARE(fixture.viewModel.piperTtsStatus(), QStringLiteral("Safety Blocked"));
    QCOMPARE(fixture.viewModel.piperFileOutputReadinessStatus(), QStringLiteral("Ready"));
    QVERIFY(fixture.viewModel.piperFileOutputReadinessSummary().contains(
        QStringLiteral("Ready for a later controlled file-output TTS phase")));
    QVERIFY(!fixture.viewModel.piperFileOutputExecutionEnabled());
    QCOMPARE(fixture.viewModel.piperFileOutputExecutionStatus(), QStringLiteral("Disabled"));
    QVERIFY(fixture.viewModel.piperFileOutputExecutionSummary().contains(
        QStringLiteral("Piper execution disabled")));

    fixture.viewModel.setPiperFileOutputExecutionEnabled(true);

    QVERIFY(fixture.settings.piperFileOutputExecutionEnabled());
    QVERIFY(fixture.viewModel.piperFileOutputExecutionEnabled());
    QCOMPARE(fixture.viewModel.piperFileOutputExecutionStatus(), QStringLiteral("Ready"));
    QVERIFY(fixture.viewModel.piperFileOutputExecutionSummary().contains(
        QStringLiteral("explicit controlled file output")));
    QCOMPARE(fixture.viewModel.piperFileOutputAudioPathSummary(),
             QStringLiteral("No generated Piper audio file."));
    QCOMPARE(fixture.viewModel.whisperPreparationReadinessStatus(), QStringLiteral("Blocked"));
    QVERIFY(fixture.viewModel.whisperPreparationReadinessSummary().contains(
        QStringLiteral("Whisper binary path is not executable")));
    QVERIFY(fixture.viewModel.piperTtsSummary().contains(QStringLiteral("safety policy")));
    QVERIFY(!fixture.viewModel.piperTtsReady());
}

void DesktopShellViewModelTest::exposesLocalInferenceBoundaryMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.localInferenceStatus(), QStringLiteral("Not Requested"));
    QVERIFY(fixture.viewModel.localInferenceSummary().contains(QStringLiteral("loopback-only")));
    QCOMPARE(fixture.viewModel.localInferenceLastResponseSummary(),
             QStringLiteral("No local inference request yet."));
    QCOMPARE(fixture.viewModel.selectedLocalModelSummary(),
             QStringLiteral("No local model selected and no discovered Ollama models are "
                            "available; local inference requires a selected or explicit model."));
    QCOMPARE(fixture.viewModel.selectedLocalModelStatus(), QStringLiteral("Missing"));
    QCOMPARE(fixture.viewModel.selectedLocalModelMetadataSummary(),
             QStringLiteral("No local model metadata available."));
    QCOMPARE(fixture.viewModel.activeLocalRuntimeBadge(),
             QStringLiteral("Ollama Local / No Model"));
    QVERIFY(!fixture.viewModel.localChatInferenceEnabled());
    QCOMPARE(fixture.viewModel.localChatInferenceStatus(), QStringLiteral("Disabled"));
    QCOMPARE(fixture.viewModel.localChatInferenceSummary(),
             QStringLiteral("Local chat inference is disabled; chat stays on the local safe "
                            "provider path and no Ollama prompt is sent."));
    QCOMPARE(fixture.viewModel.localInferenceRuntimeState(), QStringLiteral("Unavailable"));
    QCOMPARE(fixture.viewModel.localInferenceLatencySummary(),
             QStringLiteral("No local inference latency recorded."));
    QVERIFY(!fixture.viewModel.localInferenceStreamingAvailable());
    QVERIFY(!fixture.viewModel.localInferenceStreamingEnabled());
    QCOMPARE(fixture.viewModel.localInferenceStreamStatus(), QStringLiteral("Disabled"));
    QCOMPARE(fixture.viewModel.localInferenceStreamSummary(),
             QStringLiteral("Local inference streaming is disabled; responses finalize through "
                            "normal chat history."));
    QVERIFY(fixture.viewModel.localInferenceStreamingText().isEmpty());
    QVERIFY(fixture.viewModel.localInferenceTraceSummaries().isEmpty());
}

void DesktopShellViewModelTest::forwardsBlockedLocalInferenceRequest() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::localInferenceChanged);

    const auto ran =
        fixture.viewModel.runLocalInference(QStringLiteral("hello"), QStringLiteral("llama3.2"));

    QVERIFY(!ran);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(fixture.viewModel.localInferenceStatus(), QStringLiteral("Blocked"));
    QCOMPARE(fixture.viewModel.localInferenceRuntimeState(), QStringLiteral("Failed"));
    QCOMPARE(fixture.viewModel.localInferenceSummary(),
             QStringLiteral("Local inference blocked by runtime permission policy."));
    QVERIFY(fixture.viewModel.localInferenceTraceSummaries().contains(
        QStringLiteral("2. Permission Policy [Denied]: Runtime permission policy is "
                       "metadata-only and denies execution by default.")));
}

void DesktopShellViewModelTest::exposesConversationSessionMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.conversationSessionId(), QStringLiteral("conversation-session-1"));
    QCOMPARE(fixture.viewModel.conversationSessionStatus(), QStringLiteral("Active"));
    QCOMPARE(fixture.viewModel.interactionMode(), QStringLiteral("Companion"));
    QCOMPARE(fixture.viewModel.attentionState(), QStringLiteral("Observing"));
    QCOMPARE(fixture.viewModel.contextWindowSummary(),
             QStringLiteral("Workspace context window: Local Only route, Atlas (Coordinator, "
                            "Available, Local), Ambient (Available, Public Metadata, Session)."));
}

void DesktopShellViewModelTest::exposesConversationStateMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.conversationState(), QStringLiteral("Idle"));
    QCOMPARE(fixture.viewModel.conversationTransitionStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(fixture.viewModel.conversationTransitionSummary(),
             QStringLiteral("No conversation transition yet."));
}

void DesktopShellViewModelTest::exposesConversationRuntimeMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.conversationRuntimeRequestId(), QStringLiteral("None"));
    QCOMPARE(fixture.viewModel.conversationRuntimeActiveModel(), QStringLiteral("None"));
    QCOMPARE(fixture.viewModel.conversationRuntimeActiveRoute(), QStringLiteral("Provider"));
    QVERIFY(!fixture.viewModel.conversationRuntimeStreaming());
    QCOMPARE(fixture.viewModel.conversationRuntimeLastSuccessSummary(),
             QStringLiteral("No successful response yet."));
    QCOMPARE(fixture.viewModel.conversationRuntimeLastErrorSummary(),
             QStringLiteral("No error or refusal yet."));
    QCOMPARE(fixture.viewModel.conversationRuntimeLastLatencySummary(),
             QStringLiteral("No latency recorded."));
    QVERIFY(fixture.viewModel.conversationRuntimeSummary().contains(QStringLiteral("Idle")));
    QVERIFY(fixture.viewModel.conversationRuntimeSummaryLines().contains(
        QStringLiteral("State: Idle")));
}

void DesktopShellViewModelTest::updatesAndPersistsRoutingModeMetadata() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::modelRoutingChanged);
    QSignalSpy taskPlanSpy(&fixture.viewModel, &DesktopShellViewModel::taskPlanChanged);
    QSignalSpy conversationSpy(&fixture.viewModel,
                               &DesktopShellViewModel::conversationSessionChanged);
    QSignalSpy snapshotSpy(&fixture.viewModel,
                           &DesktopShellViewModel::orchestrationSnapshotChanged);

    fixture.viewModel.setRoutingModeByName(QStringLiteral("Quality"));

    QCOMPARE(fixture.viewModel.currentRoutingMode(), QStringLiteral("Quality"));
    QCOMPARE(fixture.viewModel.modelRoutingStatus(), QStringLiteral("Routed"));
    QCOMPARE(fixture.viewModel.selectedModelProviderSummary(),
             QStringLiteral("Quality -> Local Metadata Provider / Sentinel Local Placeholder"));
    QCOMPARE(fixture.settings.routingModeName(), QStringLiteral("Quality"));
    QVERIFY(spy.count() >= 1);
    QCOMPARE(fixture.viewModel.latestTaskPlanStatus(), QStringLiteral("Fallback Planned"));
    QVERIFY(taskPlanSpy.count() >= 1);
    QCOMPARE(fixture.viewModel.orchestrationSnapshotStatus(), QStringLiteral("Ready"));
    QVERIFY(fixture.viewModel.orchestrationSignals().contains(
        QStringLiteral("Routing: Quality / Routed")));
    QVERIFY(fixture.viewModel.contextWindowSummary().contains(QStringLiteral("Quality route")));
    QVERIFY(conversationSpy.count() >= 1);
    QVERIFY(snapshotSpy.count() >= 1);

    fixture.viewModel.setRoutingModeByName(QStringLiteral("unknown"));
    QCOMPARE(fixture.viewModel.currentRoutingMode(), QStringLiteral("Local Only"));
    QCOMPARE(fixture.settings.routingModeName(), QStringLiteral("Local Only"));
}

void DesktopShellViewModelTest::exposesAgentToolMetadata() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};

    QCOMPARE(viewModel.availableToolCount(), 1);
    QCOMPARE(viewModel.availableToolIds(), QStringList{QStringLiteral("local-plan-summary")});
}

void DesktopShellViewModelTest::exposesLatestToolPlanStatus() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
    QSignalSpy planSpy(&viewModel, &DesktopShellViewModel::toolPlanChanged);

    QCOMPARE(viewModel.latestToolPlanStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(viewModel.latestToolPlanSummary(), QStringLiteral("No tool plan yet."));

    QVERIFY(viewModel.runAgentRequest(QStringLiteral("draft local plan")));
    QCOMPARE(viewModel.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(viewModel.latestToolPlanSummary(),
             QStringLiteral("Metadata-only tool plan prepared."));
    QCOMPARE(planSpy.count(), 1);
}

void DesktopShellViewModelTest::exposesLatestApprovalStatus() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
    QSignalSpy approvalSpy(&viewModel, &DesktopShellViewModel::approvalChanged);

    QCOMPARE(viewModel.latestApprovalStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(viewModel.latestApprovalSummary(), QStringLiteral("No approval decision yet."));

    QVERIFY(viewModel.runAgentRequest(QStringLiteral("draft local plan")));
    QCOMPARE(viewModel.latestApprovalStatus(), QStringLiteral("Not Required"));
    QCOMPARE(viewModel.latestApprovalSummary(),
             QStringLiteral("Planned tool invocations do not require approval."));
    QCOMPARE(approvalSpy.count(), 1);
}

void DesktopShellViewModelTest::exposesLatestSandboxStatus() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
    QSignalSpy sandboxSpy(&viewModel, &DesktopShellViewModel::sandboxChanged);

    QCOMPARE(viewModel.latestSandboxStatus(), QStringLiteral("Not Evaluated"));
    QCOMPARE(viewModel.latestSandboxSummary(), QStringLiteral("No sandbox evaluation yet."));

    QVERIFY(viewModel.runAgentRequest(QStringLiteral("draft local plan")));
    QCOMPARE(viewModel.latestSandboxStatus(), QStringLiteral("Allowed"));
    QCOMPARE(viewModel.latestSandboxSummary(),
             QStringLiteral("Planned tool capabilities are allowed by sandbox metadata policy."));
    QCOMPARE(sandboxSpy.count(), 1);
}

void DesktopShellViewModelTest::exposesLatestToolExecutionStatus() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
    QSignalSpy executionSpy(&viewModel, &DesktopShellViewModel::toolExecutionChanged);

    QCOMPARE(viewModel.latestToolExecutionStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(viewModel.latestToolExecutionSummary(),
             QStringLiteral("No tool execution boundary result yet."));
    QCOMPARE(viewModel.latestAgentPipelineStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(viewModel.latestAgentPipelineSummary(),
             QStringLiteral("No agent pipeline result yet."));

    QVERIFY(viewModel.runAgentRequest(QStringLiteral("draft local plan")));
    QCOMPARE(viewModel.latestToolExecutionStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(viewModel.latestToolExecutionSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
    QCOMPARE(viewModel.latestAgentPipelineStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(viewModel.latestAgentPipelineSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
    QCOMPARE(executionSpy.count(), 1);
}

void DesktopShellViewModelTest::exposesRuntimeContextStatus() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
    QSignalSpy runtimeContextSpy(&viewModel, &DesktopShellViewModel::runtimeContextChanged);

    QCOMPARE(viewModel.runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(viewModel.runtimeContextStatus(), QStringLiteral("Empty"));
    QCOMPARE(viewModel.runtimeContextSummary(), QStringLiteral("No runtime context yet."));
    QVERIFY(viewModel.runtimeContextActiveToolIds().isEmpty());

    QVERIFY(viewModel.runAgentRequest(QStringLiteral("draft local plan")));

    QCOMPARE(viewModel.runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(viewModel.runtimeContextStatus(), QStringLiteral("Active"));
    QCOMPARE(viewModel.runtimeContextSummary(),
             QStringLiteral("Runtime context captured pipeline result: Placeholder Succeeded"));
    QCOMPARE(viewModel.runtimeContextActiveToolIds(),
             QStringList{QStringLiteral("local-plan-summary")});
    QCOMPARE(runtimeContextSpy.count(), 1);
}

void DesktopShellViewModelTest::exposesAgentActivityStatus() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
    QSignalSpy activitySpy(&viewModel, &DesktopShellViewModel::agentActivityChanged);

    QCOMPARE(viewModel.agentActivityCount(), 0);
    QCOMPARE(viewModel.latestAgentActivitySummary(), QStringLiteral("No agent activity yet."));

    QVERIFY(viewModel.runAgentRequest(QStringLiteral("draft local plan")));

    QCOMPARE(viewModel.agentActivityCount(), 6);
    QCOMPARE(viewModel.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Placeholder Succeeded"));
    QCOMPARE(activitySpy.count(), 1);
}

void DesktopShellViewModelTest::updatesVisibleAgentValuesForBlockedPipeline() {
    const sentinel::core::ToolDescriptor tool{
        QStringLiteral("blocked-tool"),
        QStringLiteral("Blocked Tool"),
        QStringLiteral("Metadata-only blocked capability."),
        sentinel::core::ToolRiskLevel::Low,
        sentinel::core::ToolExecutionMode::MetadataOnly,
        {},
    };
    const sentinel::core::ToolInvocationPlan plan{
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
    ApplicationController controller{
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        std::make_unique<FixedPlanRuntime>(QList<sentinel::core::ToolDescriptor>{tool}, plan)};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
    QSignalSpy pipelineSpy(&viewModel, &DesktopShellViewModel::agentPipelineChanged);
    QSignalSpy runtimeContextSpy(&viewModel, &DesktopShellViewModel::runtimeContextChanged);
    QSignalSpy activitySpy(&viewModel, &DesktopShellViewModel::agentActivityChanged);

    QVERIFY(viewModel.runAgentRequest(QStringLiteral("draft blocked plan")));

    QCOMPARE(viewModel.latestAgentPipelineStatus(), QStringLiteral("Blocked"));
    QCOMPARE(viewModel.latestAgentPipelineSummary(),
             QStringLiteral("Execution boundary blocked by sandbox capability metadata."));
    QCOMPARE(viewModel.runtimeContextStatus(), QStringLiteral("Active"));
    QCOMPARE(viewModel.runtimeContextSummary(),
             QStringLiteral("Runtime context captured pipeline result: Blocked"));
    QCOMPARE(viewModel.runtimeContextActiveToolIds(), QStringList{QStringLiteral("blocked-tool")});
    QCOMPARE(viewModel.agentActivityCount(), 6);
    QCOMPARE(viewModel.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Blocked"));
    QCOMPARE(pipelineSpy.count(), 1);
    QCOMPARE(runtimeContextSpy.count(), 1);
    QCOMPARE(activitySpy.count(), 1);
}

void DesktopShellViewModelTest::exposesOnlyQmlSafeAgentVisibilityProperties() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
    const auto* metaObject = viewModel.metaObject();

    const QHash<QString, QByteArray> expectedTypes{
        {QStringLiteral("latestAgentPipelineStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("latestAgentPipelineSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimeSessionId"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimeContextStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimeContextSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimeContextActiveToolIds"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("conversationSessionId"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationSessionStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("interactionMode"), QByteArrayLiteral("QString")},
        {QStringLiteral("attentionState"), QByteArrayLiteral("QString")},
        {QStringLiteral("contextWindowSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationState"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationTransitionStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationTransitionSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationRuntimeSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationRuntimeSummaryLines"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("conversationRuntimeRequestId"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationRuntimeActiveModel"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationRuntimeActiveRoute"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationRuntimeStreaming"), QByteArrayLiteral("bool")},
        {QStringLiteral("conversationRuntimeLastSuccessSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationRuntimeLastErrorSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationRuntimeLastLatencySummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("agentActivityCount"), QByteArrayLiteral("int")},
        {QStringLiteral("latestAgentActivitySummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("latestTaskPlanStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("latestTaskPlanSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("plannedTaskStepCount"), QByteArrayLiteral("int")},
        {QStringLiteral("registeredAgentCount"), QByteArrayLiteral("int")},
        {QStringLiteral("activeAgentSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("currentAgentSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("currentMemoryAffinitySummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("providerCatalogCount"), QByteArrayLiteral("int")},
        {QStringLiteral("providerCatalogSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("memoryCatalogCount"), QByteArrayLiteral("int")},
        {QStringLiteral("memoryCatalogSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("orchestrationSnapshotStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("orchestrationSnapshotSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("orchestrationSignals"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("orchestrationReadinessStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("orchestrationReadinessSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("orchestrationDiagnostics"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("localRuntimeStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeHealth"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeCapabilities"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("localRuntimeResponseStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeResponseSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeSessionCount"), QByteArrayLiteral("int")},
        {QStringLiteral("localRuntimeSessionStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeSessionHealth"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeSessionSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeAllocationSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeReservationSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeSessionSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("runtimeCapabilityCount"), QByteArrayLiteral("int")},
        {QStringLiteral("enabledRuntimeCapabilitySummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("disabledRuntimeCapabilitySummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("runtimeNegotiationProfileSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimeNegotiationSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localOnlyRuntimeEnforcementSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimePermissionDecision"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimePermissionSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimeSafetyDecision"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimeSafetySummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimePipelineStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimePipelineSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimePipelineTraceSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("executionLifecycleState"), QByteArrayLiteral("QString")},
        {QStringLiteral("executionLifecycleStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("executionLifecycleSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("executionLifecycleTraceSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("executionSessionId"), QByteArrayLiteral("QString")},
        {QStringLiteral("executionSessionStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("executionSessionOwnership"), QByteArrayLiteral("QString")},
        {QStringLiteral("executionCoordinationMode"), QByteArrayLiteral("QString")},
        {QStringLiteral("executionSessionSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("executionCoordinationSnapshotSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeAdapterStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeAdapterHealth"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeAdapterSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localRuntimeAdapterCapabilitySummaries"),
         QByteArrayLiteral("QStringList")},
        {QStringLiteral("providerRuntimeBridgeStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("providerRuntimeBridgeSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("providerRuntimeBridgeResponseSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimeIntegrationReadinessStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimeIntegrationReadinessSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("runtimeIntegrationReadinessChecks"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("ollamaEndpoint"), QByteArrayLiteral("QString")},
        {QStringLiteral("ollamaConnectionStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("ollamaHealthStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("ollamaHealthSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("ollamaModelCount"), QByteArrayLiteral("int")},
        {QStringLiteral("ollamaModelNames"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("ollamaModelSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("selectedLocalModelStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("selectedLocalModelSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("selectedLocalModelMetadataSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("activeLocalRuntimeBadge"), QByteArrayLiteral("QString")},
        {QStringLiteral("modelManagementStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("modelManagementSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("modelManagementActionAvailability"), QByteArrayLiteral("QString")},
        {QStringLiteral("modelRecommendationSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("modelRequirementSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("voiceRuntimeMode"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceEnabled"), QByteArrayLiteral("bool")},
        {QStringLiteral("voiceReadinessStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceReadinessSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceReadinessChecks"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("voiceCapabilitySummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("textToSpeechStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("textToSpeechSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("speechToTextStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("speechToTextSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceSessionId"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceSessionStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceSessionSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("voicePipelineStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("voicePipelineSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("voicePipelineTraceSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("voiceRuntimeStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceRuntimeSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceRuntimeCheckSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("voiceRuntimeAvailable"), QByteArrayLiteral("bool")},
        {QStringLiteral("voiceTextToSpeechAvailable"), QByteArrayLiteral("bool")},
        {QStringLiteral("voiceSpeechToTextAvailable"), QByteArrayLiteral("bool")},
        {QStringLiteral("voiceMicrophoneEnabled"), QByteArrayLiteral("bool")},
        {QStringLiteral("voicePlaybackEnabled"), QByteArrayLiteral("bool")},
        {QStringLiteral("voiceLocalOnlyPolicy"), QByteArrayLiteral("bool")},
        {QStringLiteral("voiceProcessExecutionEnabled"), QByteArrayLiteral("bool")},
        {QStringLiteral("voiceRuntimeEnvironmentStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceRuntimeEnvironmentSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceBinarySummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("voiceModelSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("voiceRuntimePermissionSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("voiceRuntimeSafetyStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceRuntimeSafetySummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceRuntimeSafetyChecks"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("voiceRuntimeExecutionAllowed"), QByteArrayLiteral("bool")},
        {QStringLiteral("piperTtsStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("piperTtsSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("piperTtsReadinessChecks"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("piperTtsReady"), QByteArrayLiteral("bool")},
        {QStringLiteral("piperTtsFileOutputStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("piperTtsFileOutputSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceConfigurationSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("voiceConfigurationReadinessSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("voiceConfigurationStatusBadges"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("voiceConfigurationHintSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("voiceConfigurationValidationSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("piperFileOutputReadinessStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("piperFileOutputReadinessSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("piperFileOutputExecutionEnabled"), QByteArrayLiteral("bool")},
        {QStringLiteral("piperFileOutputExecutionStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("piperFileOutputExecutionSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("piperFileOutputAudioPathSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("whisperPreparationReadinessStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("whisperPreparationReadinessSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localChatInferenceStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("localChatInferenceSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localInferenceBusy"), QByteArrayLiteral("bool")},
        {QStringLiteral("localInferenceRuntimeState"), QByteArrayLiteral("QString")},
        {QStringLiteral("localInferenceStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("localInferenceSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localInferenceLastResponseSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localInferenceLatencySummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localInferenceTraceSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("localInferenceStreamingAvailable"), QByteArrayLiteral("bool")},
        {QStringLiteral("localInferenceStreamStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("localInferenceStreamSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("localInferenceStreamingText"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationSearchQueryText"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationSearchStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationSearchSummaryText"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationSearchResultCount"), QByteArrayLiteral("int")},
        {QStringLiteral("conversationSearchResultSummaries"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("conversationBrowserStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationBrowserSummaryText"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationListEntryCount"), QByteArrayLiteral("int")},
        {QStringLiteral("conversationListCurrentTitle"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationListCurrentMessageCount"), QByteArrayLiteral("int")},
        {QStringLiteral("conversationListCurrentPersistenceStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationListCurrentLastUpdatedSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationListCurrentSearchAvailabilitySummary"),
         QByteArrayLiteral("QString")},
        {QStringLiteral("conversationListCurrentExportAvailabilitySummary"),
         QByteArrayLiteral("QString")},
        {QStringLiteral("conversationListCurrentSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationCurrentStorageMode"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationFutureStorageMode"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationMigrationReadiness"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationMigrationStatusSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationSchemaStatusSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationExportAvailable"), QByteArrayLiteral("bool")},
        {QStringLiteral("conversationExportReadinessStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationExportReadinessSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationExportReadinessChecks"), QByteArrayLiteral("QStringList")},
        {QStringLiteral("conversationExportLastResultSummary"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationExportLastStatus"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationExportLastFileName"), QByteArrayLiteral("QString")},
        {QStringLiteral("conversationExportLastMessageCount"), QByteArrayLiteral("int")},
        {QStringLiteral("conversationExportLastTimestamp"), QByteArrayLiteral("QString")},
    };

    const QSet<QString> writableProperties{
        QStringLiteral("piperFileOutputExecutionEnabled"),
    };

    for (auto it = expectedTypes.cbegin(); it != expectedTypes.cend(); ++it) {
        const auto propertyIndex = metaObject->indexOfProperty(it.key().toUtf8().constData());
        QVERIFY2(propertyIndex >= 0,
                 qPrintable(QStringLiteral("Missing property %1").arg(it.key())));
        const auto property = metaObject->property(propertyIndex);
        QCOMPARE(QByteArray(property.typeName()), it.value());
        QCOMPARE(property.isWritable(), writableProperties.contains(it.key()));
    }

    const QStringList forbiddenProperties{
        QStringLiteral("latestAgentPipelineResult"),
        QStringLiteral("runtimeContext"),
        QStringLiteral("conversationSession"),
        QStringLiteral("conversationContextWindow"),
        QStringLiteral("conversationStateGraph"),
        QStringLiteral("conversationRuntimeStateRecord"),
        QStringLiteral("conversationSearchSummary"),
        QStringLiteral("conversationSearchResults"),
        QStringLiteral("conversationExportReadiness"),
        QStringLiteral("conversationExportResult"),
        QStringLiteral("conversationTransitionResult"),
        QStringLiteral("conversationTransitions"),
        QStringLiteral("agentActivityLog"),
        QStringLiteral("agentActivityEntries"),
        QStringLiteral("providerCatalog"),
        QStringLiteral("providerCatalogEntries"),
        QStringLiteral("memoryCatalog"),
        QStringLiteral("memoryCatalogEntries"),
        QStringLiteral("memoryShards"),
        QStringLiteral("orchestrationSnapshot"),
        QStringLiteral("workspaceStateSummary"),
        QStringLiteral("orchestrationReadinessReport"),
        QStringLiteral("orchestrationReadinessChecks"),
        QStringLiteral("orchestrationDiagnosticEntries"),
        QStringLiteral("localRuntime"),
        QStringLiteral("localRuntimeDescriptor"),
        QStringLiteral("localRuntimeRequest"),
        QStringLiteral("localRuntimeResponse"),
        QStringLiteral("localRuntimeSession"),
        QStringLiteral("localRuntimeSessions"),
        QStringLiteral("localRuntimeSessionManager"),
        QStringLiteral("localRuntimeAllocation"),
        QStringLiteral("localRuntimeReservation"),
        QStringLiteral("runtimeCapabilityRegistry"),
        QStringLiteral("runtimeCapabilities"),
        QStringLiteral("runtimeNegotiationProfile"),
        QStringLiteral("runtimeNegotiationResult"),
        QStringLiteral("runtimePermissionPolicy"),
        QStringLiteral("runtimePermissionRequest"),
        QStringLiteral("runtimePermissionDecisionRecord"),
        QStringLiteral("runtimeSafetyPolicy"),
        QStringLiteral("runtimeSafetyReport"),
        QStringLiteral("runtimePipeline"),
        QStringLiteral("runtimePipelineRequest"),
        QStringLiteral("runtimePipelineResult"),
        QStringLiteral("executionLifecycle"),
        QStringLiteral("executionLifecycleResult"),
        QStringLiteral("executionLifecycleTraces"),
        QStringLiteral("executionCoordinator"),
        QStringLiteral("executionCoordinationSnapshot"),
        QStringLiteral("executionSession"),
        QStringLiteral("executionRequest"),
        QStringLiteral("localRuntimeAdapter"),
        QStringLiteral("localRuntimeAdapterDescriptor"),
        QStringLiteral("providerRuntimeBridge"),
        QStringLiteral("providerRuntimeBridgeRequest"),
        QStringLiteral("providerRuntimeBridgeResponse"),
        QStringLiteral("runtimeIntegrationReadiness"),
        QStringLiteral("runtimeIntegrationReport"),
        QStringLiteral("runtimeIntegrationChecks"),
        QStringLiteral("localInferenceClient"),
        QStringLiteral("localInferenceRequest"),
        QStringLiteral("localInferenceResponse"),
        QStringLiteral("localInferenceStreamClient"),
        QStringLiteral("localInferenceStreamResult"),
        QStringLiteral("localInferenceStreamChunk"),
        QStringLiteral("localInferenceTrace"),
        QStringLiteral("ollamaRuntimeClient"),
        QStringLiteral("ollamaConfig"),
        QStringLiteral("ollamaEndpointRecord"),
        QStringLiteral("ollamaHealthCheck"),
        QStringLiteral("ollamaModels"),
        QStringLiteral("modelManagementService"),
        QStringLiteral("modelManagementRequest"),
        QStringLiteral("modelManagementResult"),
        QStringLiteral("textToSpeechProvider"),
        QStringLiteral("speechToTextProvider"),
        QStringLiteral("voiceProviderDescriptor"),
        QStringLiteral("voiceRequest"),
        QStringLiteral("voiceResponse"),
        QStringLiteral("voiceReadinessReport"),
        QStringLiteral("voiceRuntimeCoordinator"),
        QStringLiteral("voiceRuntimeSummaryRecord"),
        QStringLiteral("voiceSession"),
        QStringLiteral("voicePipelineResult"),
        QStringLiteral("voicePipelineTrace"),
        QStringLiteral("agentRegistry"),
        QStringLiteral("agentDescriptors"),
        QStringLiteral("taskPlanner"),
        QStringLiteral("latestTaskPlan"),
        QStringLiteral("controller"),
    };
    for (const auto& propertyName : forbiddenProperties) {
        QCOMPARE(metaObject->indexOfProperty(propertyName.toUtf8().constData()), -1);
    }
}

void DesktopShellViewModelTest::exposesChatHistoryStatus() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr,
                                     std::make_unique<StaticChatHistoryStore>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};

    QCOMPARE(viewModel.chatHistoryStatus(), QStringLiteral("Available"));
}

void DesktopShellViewModelTest::exposesConversationHistorySummaryMetadata() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr,
                                     std::make_unique<StaticChatHistoryStore>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};

    QCOMPARE(viewModel.conversationPersistenceStatus(), QStringLiteral("Persisted"));
    QCOMPARE(viewModel.conversationHistoryMessageCount(), 1);
    QVERIFY(viewModel.conversationHistorySummaryText().contains(QStringLiteral("1 message")));
    QVERIFY(viewModel.conversationLastSavedStatus().contains(QStringLiteral("initial system")));

    QVERIFY(viewModel.sendMessage(QStringLiteral("status")));

    QCOMPARE(viewModel.conversationHistoryMessageCount(), 3);
    QVERIFY(viewModel.conversationHistorySummaryText().contains(QStringLiteral("3 messages")));
    QCOMPARE(viewModel.conversationLastSavedStatus(),
             QStringLiteral("Saved latest assistant message."));
}

void DesktopShellViewModelTest::exposesConversationBrowserMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.conversationBrowserStatus(), QStringLiteral("Empty Transcript"));
    QCOMPARE(fixture.viewModel.conversationListEntryCount(), 1);
    QCOMPARE(fixture.viewModel.conversationListCurrentTitle(),
             QStringLiteral("Current Transcript"));
    QCOMPARE(fixture.viewModel.conversationListCurrentMessageCount(), 1);
    QCOMPARE(fixture.viewModel.conversationListCurrentPersistenceStatus(),
             QStringLiteral("Runtime Only"));
    QVERIFY(fixture.viewModel.conversationListCurrentSearchAvailabilitySummary().contains(
        QStringLiteral("Search ready")));
    QVERIFY(fixture.viewModel.conversationListCurrentExportAvailabilitySummary().contains(
        QStringLiteral("Available")));

    QVERIFY(fixture.viewModel.sendMessage(QStringLiteral("browser token")));

    QCOMPARE(fixture.viewModel.conversationBrowserStatus(), QStringLiteral("Ready"));
    QCOMPARE(fixture.viewModel.conversationListCurrentMessageCount(), 3);
}

void DesktopShellViewModelTest::exposesMultiConversationReadinessMetadata() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.conversationCurrentStorageMode(),
             QStringLiteral("Single Transcript"));
    QCOMPARE(fixture.viewModel.conversationFutureStorageMode(),
             QStringLiteral("Multi Conversation"));
    QCOMPARE(fixture.viewModel.conversationMigrationReadiness(), QStringLiteral("Not Started"));
    QCOMPARE(fixture.viewModel.conversationMigrationStatusSummary(),
             QStringLiteral("Not Started / Planned"));
    QVERIFY(fixture.viewModel.conversationSchemaStatusSummary().contains(
        QStringLiteral("No conversation schema migration applied")));
    QCOMPARE(fixture.viewModel.conversationListEntryCount(), 1);
}

void DesktopShellViewModelTest::exposesConversationSearchAndExportMetadata() {
    ViewModelFixture fixture;
    QSignalSpy searchSpy(&fixture.viewModel, &DesktopShellViewModel::conversationSearchChanged);
    QSignalSpy exportSpy(&fixture.viewModel, &DesktopShellViewModel::conversationExportChanged);

    QCOMPARE(fixture.viewModel.conversationSearchStatus(), QStringLiteral("Empty Query"));
    QCOMPARE(fixture.viewModel.conversationSearchResultCount(), 0);
    QVERIFY(fixture.viewModel.conversationSearchSummaryText().contains(
        QStringLiteral("No transcript search query")));
    QVERIFY(fixture.viewModel.conversationExportAvailable());
    QCOMPARE(fixture.viewModel.conversationExportReadinessStatus(), QStringLiteral("Ready"));
    QVERIFY(fixture.viewModel.conversationExportReadinessChecks().contains(
        QStringLiteral("Output: App-controlled export directory")));
    QCOMPARE(fixture.viewModel.conversationExportLastStatus(), QStringLiteral("Not Run"));

    QVERIFY(fixture.viewModel.sendMessage(QStringLiteral("search token")));
    QVERIFY(fixture.viewModel.searchConversation(QStringLiteral("token")));

    QCOMPARE(fixture.viewModel.conversationSearchQueryText(), QStringLiteral("token"));
    QCOMPARE(fixture.viewModel.conversationSearchStatus(), QStringLiteral("Completed"));
    QCOMPARE(fixture.viewModel.conversationSearchResultCount(), 1);
    QVERIFY(fixture.viewModel.conversationSearchResultSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("user #2")));
    QVERIFY(fixture.viewModel.conversationListCurrentSearchAvailabilitySummary().contains(
        QStringLiteral("Completed")));
    QVERIFY(searchSpy.count() >= 1);

    QTemporaryDir exportDir;
    QVERIFY(exportDir.isValid());
    fixture.controller.setConversationExportDirectory(exportDir.path());
    QVERIFY(fixture.viewModel.exportTranscript(QStringLiteral("json")));
    QCOMPARE(fixture.viewModel.conversationExportLastStatus(), QStringLiteral("Succeeded"));
    QVERIFY(fixture.viewModel.conversationExportLastFileName().endsWith(QStringLiteral(".json")));
    QCOMPARE(fixture.viewModel.conversationExportLastMessageCount(),
             fixture.viewModel.conversationHistoryMessageCount());
    QVERIFY(fixture.viewModel.conversationListCurrentExportAvailabilitySummary().contains(
        QStringLiteral("Last export: Succeeded")));
    QCOMPARE(exportSpy.count(), 1);

    fixture.viewModel.clearConversationSearch();
    QCOMPARE(fixture.viewModel.conversationSearchStatus(), QStringLiteral("Empty Query"));
    QCOMPARE(fixture.viewModel.conversationSearchResultCount(), 0);
}

void DesktopShellViewModelTest::exposesMaintenanceStatuses() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.memoryMaintenanceStatus(), QStringLiteral("Ready"));
    QCOMPARE(fixture.viewModel.chatMaintenanceStatus(), QStringLiteral("Ready"));
}

void DesktopShellViewModelTest::exposesStartupLoadedMessages() {
    QList<sentinel::core::ChatMessage> persisted{
        {7, sentinel::core::ChatRole::System, QStringLiteral("previous system"),
         QDateTime::fromString(QStringLiteral("2026-05-15T12:00:00.000Z"), Qt::ISODateWithMs),
         sentinel::core::ChatMessageStatus::Received},
        {8, sentinel::core::ChatRole::User, QStringLiteral("previous user"),
         QDateTime::fromString(QStringLiteral("2026-05-15T12:01:00.000Z"), Qt::ISODateWithMs),
         sentinel::core::ChatMessageStatus::Sent},
    };
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr,
                                     std::make_unique<StaticChatHistoryStore>(persisted)};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};

    QCOMPARE(viewModel.chatMessages()->rowCount(), 2);
    const auto firstIndex = viewModel.chatMessages()->index(0, 0);
    const auto secondIndex = viewModel.chatMessages()->index(1, 0);
    QCOMPARE(viewModel.chatMessages()->data(firstIndex, ChatMessageListModel::ContentRole),
             QStringLiteral("previous system"));
    QCOMPARE(viewModel.chatMessages()->data(secondIndex, ChatMessageListModel::ContentRole),
             QStringLiteral("previous user"));
    QCOMPARE(viewModel.chatMessages()->data(secondIndex, ChatMessageListModel::StatusRole),
             QStringLiteral("sent"));
}

void DesktopShellViewModelTest::forwardsChatActions() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::chatMessagesChanged);
    QSignalSpy stateSpy(&fixture.viewModel, &DesktopShellViewModel::conversationStateChanged);

    const auto sent = fixture.viewModel.sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(fixture.viewModel.chatMessages()->rowCount(), 3);
    const auto lastIndex = fixture.viewModel.chatMessages()->index(2, 0);
    QCOMPARE(fixture.viewModel.chatMessages()->data(lastIndex, ChatMessageListModel::ContentRole),
             QStringLiteral("Sentinel Core online. Local chat pipeline is active."));
    QCOMPARE(fixture.viewModel.chatMessages()->data(lastIndex, ChatMessageListModel::StatusRole),
             QStringLiteral("received"));
    QCOMPARE(fixture.viewModel.conversationState(), QStringLiteral("Completed"));
    QCOMPARE(fixture.viewModel.conversationTransitionStatus(), QStringLiteral("Accepted"));
    QCOMPARE(fixture.viewModel.conversationTransitionSummary(),
             QStringLiteral("Accepted conversation transition: Responding -> Completed: chat "
                            "response metadata completed"));
    QCOMPARE(spy.count(), 1);
    QVERIFY(stateSpy.count() >= 6);
}

void DesktopShellViewModelTest::forwardsDeterministicAgentRequest() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
    QSignalSpy statusSpy(&viewModel, &DesktopShellViewModel::agentStatusChanged);
    QSignalSpy responseSpy(&viewModel, &DesktopShellViewModel::agentResponseChanged);
    QSignalSpy planSpy(&viewModel, &DesktopShellViewModel::toolPlanChanged);
    QSignalSpy approvalSpy(&viewModel, &DesktopShellViewModel::approvalChanged);
    QSignalSpy sandboxSpy(&viewModel, &DesktopShellViewModel::sandboxChanged);
    QSignalSpy toolExecutionSpy(&viewModel, &DesktopShellViewModel::toolExecutionChanged);
    QSignalSpy pipelineSpy(&viewModel, &DesktopShellViewModel::agentPipelineChanged);
    QSignalSpy runtimeContextSpy(&viewModel, &DesktopShellViewModel::runtimeContextChanged);
    QSignalSpy activitySpy(&viewModel, &DesktopShellViewModel::agentActivityChanged);
    QSignalSpy conversationStateSpy(&viewModel, &DesktopShellViewModel::conversationStateChanged);

    const auto ran = viewModel.runAgentRequest(QStringLiteral("draft local action"));

    QVERIFY(ran);
    QCOMPARE(viewModel.agentStatus(), QStringLiteral("Ready"));
    QCOMPARE(viewModel.lastAgentResponse(),
             QStringLiteral("Local agent placeholder processed: draft local action"));
    QCOMPARE(viewModel.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(viewModel.latestToolPlanSummary(),
             QStringLiteral("Metadata-only tool plan prepared."));
    QCOMPARE(viewModel.latestApprovalStatus(), QStringLiteral("Not Required"));
    QCOMPARE(viewModel.latestApprovalSummary(),
             QStringLiteral("Planned tool invocations do not require approval."));
    QCOMPARE(viewModel.latestSandboxStatus(), QStringLiteral("Allowed"));
    QCOMPARE(viewModel.latestSandboxSummary(),
             QStringLiteral("Planned tool capabilities are allowed by sandbox metadata policy."));
    QCOMPARE(viewModel.latestToolExecutionStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(viewModel.latestToolExecutionSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
    QCOMPARE(viewModel.latestAgentPipelineStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(viewModel.latestAgentPipelineSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
    QCOMPARE(viewModel.runtimeContextStatus(), QStringLiteral("Active"));
    QCOMPARE(viewModel.runtimeContextSummary(),
             QStringLiteral("Runtime context captured pipeline result: Placeholder Succeeded"));
    QCOMPARE(viewModel.runtimeContextActiveToolIds(),
             QStringList{QStringLiteral("local-plan-summary")});
    QCOMPARE(viewModel.agentActivityCount(), 6);
    QCOMPARE(viewModel.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Placeholder Succeeded"));
    QCOMPARE(viewModel.conversationState(), QStringLiteral("Completed"));
    QCOMPARE(viewModel.conversationTransitionStatus(), QStringLiteral("Accepted"));
    QCOMPARE(viewModel.conversationTransitionSummary(),
             QStringLiteral("Accepted conversation transition: Responding -> Completed: agent "
                            "response metadata completed"));
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(responseSpy.count(), 1);
    QCOMPARE(planSpy.count(), 1);
    QCOMPARE(approvalSpy.count(), 1);
    QCOMPARE(sandboxSpy.count(), 1);
    QCOMPARE(toolExecutionSpy.count(), 1);
    QCOMPARE(pipelineSpy.count(), 1);
    QCOMPARE(runtimeContextSpy.count(), 1);
    QCOMPARE(activitySpy.count(), 1);
    QVERIFY(conversationStateSpy.count() >= 6);
}

void DesktopShellViewModelTest::ignoresBlankChatActions() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::chatMessagesChanged);

    const auto sent = fixture.viewModel.sendMessage(QStringLiteral("   "));

    QVERIFY(!sent);
    QCOMPARE(fixture.viewModel.chatMessages()->rowCount(), 1);
    QCOMPARE(spy.count(), 0);
}

void DesktopShellViewModelTest::clearsChatActions() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::chatMessagesChanged);

    fixture.viewModel.sendMessage(QStringLiteral("status"));
    fixture.viewModel.clearChat();

    QCOMPARE(fixture.viewModel.chatMessages()->rowCount(), 1);
    QCOMPARE(spy.count(), 2);
}

void DesktopShellViewModelTest::clearsMemoryActions() {
    ViewModelFixture fixture;
    QSignalSpy memorySpy(&fixture.viewModel, &DesktopShellViewModel::memoryEntriesChanged);
    QSignalSpy maintenanceSpy(&fixture.viewModel, &DesktopShellViewModel::maintenanceStatusChanged);
    fixture.viewModel.remember(QStringLiteral("mode"), QStringLiteral("Companion"));

    const auto cleared = fixture.viewModel.clearMemory();

    QVERIFY(cleared);
    QVERIFY(fixture.viewModel.memoryEntries().isEmpty());
    QCOMPARE(fixture.viewModel.memoryMaintenanceStatus(), QStringLiteral("Clear completed"));
    QCOMPARE(memorySpy.count(), 2);
    QCOMPARE(maintenanceSpy.count(), 1);
}

void DesktopShellViewModelTest::reportsRuntimeOnlyChatMaintenanceWhenStoreUnavailable() {
    ApplicationController controller{
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr,
        std::make_unique<StaticChatHistoryStore>(QList<sentinel::core::ChatMessage>{}, false)};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
    QSignalSpy maintenanceSpy(&viewModel, &DesktopShellViewModel::maintenanceStatusChanged);

    viewModel.sendMessage(QStringLiteral("status"));
    const auto cleared = viewModel.clearChat();

    QVERIFY(!cleared);
    QCOMPARE(viewModel.chatMaintenanceStatus(), QStringLiteral("Runtime Only"));
    QCOMPARE(viewModel.chatMessages()->rowCount(), 1);
    QCOMPARE(maintenanceSpy.count(), 1);
}

void DesktopShellViewModelTest::forwardsModeChanges() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::currentModeChanged);

    fixture.viewModel.setModeByName(QStringLiteral("Mission Mode"));

    QCOMPARE(fixture.viewModel.currentModeName(), QStringLiteral("Mission Mode"));
    QCOMPARE(spy.count(), 1);
}

void DesktopShellViewModelTest::forwardsMemoryWrites() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::memoryEntriesChanged);

    fixture.viewModel.remember(QStringLiteral("mode"), QStringLiteral("Companion"));

    QCOMPARE(fixture.viewModel.memoryEntries(), QStringList{QStringLiteral("mode: Companion")});
    QCOMPARE(spy.count(), 1);
}

void DesktopShellViewModelTest::forwardsSettingsChanges() {
    ViewModelFixture fixture;
    QSignalSpy themeSpy(&fixture.viewModel, &DesktopShellViewModel::themeNameChanged);
    QSignalSpy profileSpy(&fixture.viewModel, &DesktopShellViewModel::configurationProfileChanged);
    QSignalSpy modelSpy(&fixture.viewModel, &DesktopShellViewModel::localModelSelectionChanged);
    QSignalSpy chatRoutingSpy(&fixture.viewModel,
                              &DesktopShellViewModel::localChatInferenceRoutingChanged);
    QSignalSpy inferenceSpy(&fixture.viewModel, &DesktopShellViewModel::localInferenceChanged);

    fixture.viewModel.setThemeName(QStringLiteral("Sentinel Light"));
    fixture.viewModel.setConfigurationProfile(QStringLiteral("Phase 2 Shell"));
    fixture.viewModel.setSelectedLocalModel(QStringLiteral(" local-model "));
    fixture.viewModel.setLocalChatInferenceEnabled(true);
    fixture.viewModel.setLocalInferenceStreamingEnabled(true);

    QCOMPARE(fixture.viewModel.themeName(), QStringLiteral("Sentinel Light"));
    QCOMPARE(fixture.viewModel.configurationProfile(), QStringLiteral("Phase 2 Shell"));
    QCOMPARE(fixture.viewModel.selectedLocalModel(), QStringLiteral("local-model"));
    QCOMPARE(fixture.settings.selectedLocalModel(), QStringLiteral("local-model"));
    QVERIFY(fixture.viewModel.localChatInferenceEnabled());
    QVERIFY(fixture.settings.localChatInferenceEnabled());
    QVERIFY(fixture.viewModel.localInferenceStreamingEnabled());
    QVERIFY(fixture.settings.localInferenceStreamingEnabled());
    QCOMPARE(fixture.viewModel.localChatInferenceStatus(), QStringLiteral("Enabled"));
    QCOMPARE(themeSpy.count(), 1);
    QCOMPARE(profileSpy.count(), 1);
    QCOMPARE(modelSpy.count(), 1);
    QCOMPARE(chatRoutingSpy.count(), 2);
    QCOMPARE(inferenceSpy.count(), 1);
}

void DesktopShellViewModelTest::keepsSettingsSeparateFromClearActions() {
    ViewModelFixture fixture;
    fixture.viewModel.setThemeName(QStringLiteral("Sentinel Light"));
    fixture.viewModel.setConfigurationProfile(QStringLiteral("Desktop Stable"));
    fixture.viewModel.remember(QStringLiteral("mode"), QStringLiteral("Companion"));
    fixture.viewModel.sendMessage(QStringLiteral("status"));

    fixture.viewModel.clearMemory();
    fixture.viewModel.clearChat();

    QCOMPARE(fixture.viewModel.themeName(), QStringLiteral("Sentinel Light"));
    QCOMPARE(fixture.viewModel.configurationProfile(), QStringLiteral("Desktop Stable"));
}

void DesktopShellViewModelTest::tracksNavigationState() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::currentPageChanged);

    fixture.viewModel.setCurrentPage(QStringLiteral("Memory"));

    QCOMPARE(fixture.viewModel.currentPage(), QStringLiteral("Memory"));
    QCOMPARE(spy.count(), 1);

    fixture.viewModel.setCurrentPage(QStringLiteral("Settings"));
    QCOMPARE(fixture.viewModel.currentPage(), QStringLiteral("Settings"));
    QCOMPARE(spy.count(), 2);

    fixture.viewModel.setCurrentPage(QStringLiteral("Agents"));
    QCOMPARE(fixture.viewModel.currentPage(), QStringLiteral("Agents"));
    QCOMPARE(spy.count(), 3);
}

void DesktopShellViewModelTest::ignoresRepeatedAndUnknownNavigationChanges() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::currentPageChanged);

    fixture.viewModel.setCurrentPage(QStringLiteral("Dashboard"));
    QCOMPARE(spy.count(), 0);

    fixture.viewModel.setCurrentPage(QStringLiteral("Unknown"));
    QCOMPARE(fixture.viewModel.currentPage(), QStringLiteral("Dashboard"));
    QCOMPARE(spy.count(), 0);
}

QTEST_MAIN(DesktopShellViewModelTest)

#include "test_desktop_shell_view_model.moc"
