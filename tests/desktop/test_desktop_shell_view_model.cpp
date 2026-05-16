#include "sentinel/desktop/DesktopShellViewModel.h"

#include "sentinel/core/AppSettings.h"
#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/IChatHistoryStore.h"
#include "sentinel/core/InMemorySettingsStore.h"
#include "sentinel/core/InMemoryStore.h"
#include "sentinel/core/LocalEchoProvider.h"
#include "sentinel/core/ModeManager.h"
#include "sentinel/core/NullAgentRuntime.h"

#include <QHash>
#include <QMetaProperty>
#include <QSignalSpy>
#include <QtTest>

#include <memory>

using sentinel::core::ApplicationController;
using sentinel::core::AppSettings;
using sentinel::core::IChatHistoryStore;
using sentinel::core::InMemorySettingsStore;
using sentinel::core::InMemoryStore;
using sentinel::core::LocalEchoProvider;
using sentinel::core::ModeManager;
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
    void updatesAndPersistsRoutingModeMetadata();
    void updatesVisibleAgentValuesForBlockedPipeline();
    void exposesOnlyQmlSafeAgentVisibilityProperties();
    void exposesChatHistoryStatus();
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
             QStringList({QStringLiteral("Dashboard"), QStringLiteral("Memory"),
                          QStringLiteral("Agents")}));
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

void DesktopShellViewModelTest::updatesAndPersistsRoutingModeMetadata() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::modelRoutingChanged);
    QSignalSpy taskPlanSpy(&fixture.viewModel, &DesktopShellViewModel::taskPlanChanged);

    fixture.viewModel.setRoutingModeByName(QStringLiteral("Quality"));

    QCOMPARE(fixture.viewModel.currentRoutingMode(), QStringLiteral("Quality"));
    QCOMPARE(fixture.viewModel.modelRoutingStatus(), QStringLiteral("Routed"));
    QCOMPARE(fixture.viewModel.selectedModelProviderSummary(),
             QStringLiteral("Quality -> Local Metadata Provider / Sentinel Local Placeholder"));
    QCOMPARE(fixture.settings.routingModeName(), QStringLiteral("Quality"));
    QVERIFY(spy.count() >= 1);
    QCOMPARE(fixture.viewModel.latestTaskPlanStatus(), QStringLiteral("Fallback Planned"));
    QVERIFY(taskPlanSpy.count() >= 1);

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
    };

    for (auto it = expectedTypes.cbegin(); it != expectedTypes.cend(); ++it) {
        const auto propertyIndex = metaObject->indexOfProperty(it.key().toUtf8().constData());
        QVERIFY2(propertyIndex >= 0,
                 qPrintable(QStringLiteral("Missing property %1").arg(it.key())));
        const auto property = metaObject->property(propertyIndex);
        QCOMPARE(QByteArray(property.typeName()), it.value());
        QVERIFY(!property.isWritable());
    }

    const QStringList forbiddenProperties{
        QStringLiteral("latestAgentPipelineResult"),
        QStringLiteral("runtimeContext"),
        QStringLiteral("agentActivityLog"),
        QStringLiteral("agentActivityEntries"),
        QStringLiteral("providerCatalog"),
        QStringLiteral("providerCatalogEntries"),
        QStringLiteral("memoryCatalog"),
        QStringLiteral("memoryCatalogEntries"),
        QStringLiteral("memoryShards"),
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

    const auto sent = fixture.viewModel.sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(fixture.viewModel.chatMessages()->rowCount(), 3);
    const auto lastIndex = fixture.viewModel.chatMessages()->index(2, 0);
    QCOMPARE(fixture.viewModel.chatMessages()->data(lastIndex, ChatMessageListModel::ContentRole),
             QStringLiteral("Sentinel Core online. Local chat pipeline is active."));
    QCOMPARE(fixture.viewModel.chatMessages()->data(lastIndex, ChatMessageListModel::StatusRole),
             QStringLiteral("received"));
    QCOMPARE(spy.count(), 1);
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
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(responseSpy.count(), 1);
    QCOMPARE(planSpy.count(), 1);
    QCOMPARE(approvalSpy.count(), 1);
    QCOMPARE(sandboxSpy.count(), 1);
    QCOMPARE(toolExecutionSpy.count(), 1);
    QCOMPARE(pipelineSpy.count(), 1);
    QCOMPARE(runtimeContextSpy.count(), 1);
    QCOMPARE(activitySpy.count(), 1);
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

    fixture.viewModel.setThemeName(QStringLiteral("Sentinel Light"));
    fixture.viewModel.setConfigurationProfile(QStringLiteral("Phase 2 Shell"));

    QCOMPARE(fixture.viewModel.themeName(), QStringLiteral("Sentinel Light"));
    QCOMPARE(fixture.viewModel.configurationProfile(), QStringLiteral("Phase 2 Shell"));
    QCOMPARE(themeSpy.count(), 1);
    QCOMPARE(profileSpy.count(), 1);
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
