#pragma once

#include "sentinel/core/AgentActivityLog.h"
#include "sentinel/core/AgentPipelineResult.h"
#include "sentinel/core/AgentRuntimeContext.h"
#include "sentinel/core/ChatSession.h"
#include "sentinel/core/ConversationSession.h"
#include "sentinel/core/ConversationStateGraph.h"
#include "sentinel/core/IAgentRegistry.h"
#include "sentinel/core/IAgentRuntime.h"
#include "sentinel/core/IApprovalPolicy.h"
#include "sentinel/core/IChatHistoryStore.h"
#include "sentinel/core/IChatProvider.h"
#include "sentinel/core/IMemoryCatalog.h"
#include "sentinel/core/IMemoryStore.h"
#include "sentinel/core/IModelRouter.h"
#include "sentinel/core/IProviderCatalog.h"
#include "sentinel/core/ISandboxPolicy.h"
#include "sentinel/core/ITaskPlanner.h"
#include "sentinel/core/IToolExecutor.h"
#include "sentinel/core/OrchestrationDiagnostics.h"
#include "sentinel/core/OrchestrationSnapshot.h"

#include <QObject>
#include <QStringList>
#include <memory>

namespace sentinel::core {

class ApplicationController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString providerName READ providerName CONSTANT)
    Q_PROPERTY(QString providerStatus READ providerStatus CONSTANT)
    Q_PROPERTY(QString agentStatus READ agentStatus NOTIFY agentStatusChanged)
    Q_PROPERTY(QString lastAgentResponse READ lastAgentResponse NOTIFY agentResponseChanged)
    Q_PROPERTY(QString latestToolPlanStatus READ latestToolPlanStatus NOTIFY toolPlanChanged)
    Q_PROPERTY(QString latestToolPlanSummary READ latestToolPlanSummary NOTIFY toolPlanChanged)
    Q_PROPERTY(QString latestApprovalStatus READ latestApprovalStatus NOTIFY approvalChanged)
    Q_PROPERTY(QString latestApprovalSummary READ latestApprovalSummary NOTIFY approvalChanged)
    Q_PROPERTY(QString latestSandboxStatus READ latestSandboxStatus NOTIFY sandboxChanged)
    Q_PROPERTY(QString latestSandboxSummary READ latestSandboxSummary NOTIFY sandboxChanged)
    Q_PROPERTY(QString latestToolExecutionStatus READ latestToolExecutionStatus NOTIFY
                   toolExecutionChanged)
    Q_PROPERTY(QString latestToolExecutionSummary READ latestToolExecutionSummary NOTIFY
                   toolExecutionChanged)
    Q_PROPERTY(QString latestAgentPipelineStatus READ latestAgentPipelineStatus NOTIFY
                   agentPipelineChanged)
    Q_PROPERTY(QString latestAgentPipelineSummary READ latestAgentPipelineSummary NOTIFY
                   agentPipelineChanged)
    Q_PROPERTY(QString runtimeSessionId READ runtimeSessionId NOTIFY runtimeContextChanged)
    Q_PROPERTY(QString runtimeContextStatus READ runtimeContextStatus NOTIFY runtimeContextChanged)
    Q_PROPERTY(
        QString runtimeContextSummary READ runtimeContextSummary NOTIFY runtimeContextChanged)
    Q_PROPERTY(QStringList runtimeContextActiveToolIds READ runtimeContextActiveToolIds NOTIFY
                   runtimeContextChanged)
    Q_PROPERTY(
        QString conversationSessionId READ conversationSessionId NOTIFY conversationSessionChanged)
    Q_PROPERTY(QString conversationSessionStatus READ conversationSessionStatus NOTIFY
                   conversationSessionChanged)
    Q_PROPERTY(QString interactionMode READ interactionMode NOTIFY conversationSessionChanged)
    Q_PROPERTY(QString attentionState READ attentionState NOTIFY conversationSessionChanged)
    Q_PROPERTY(
        QString contextWindowSummary READ contextWindowSummary NOTIFY conversationSessionChanged)
    Q_PROPERTY(QString conversationState READ conversationState NOTIFY conversationStateChanged)
    Q_PROPERTY(QString conversationTransitionStatus READ conversationTransitionStatus NOTIFY
                   conversationStateChanged)
    Q_PROPERTY(QString conversationTransitionSummary READ conversationTransitionSummary NOTIFY
                   conversationStateChanged)
    Q_PROPERTY(int agentActivityCount READ agentActivityCount NOTIFY agentActivityChanged)
    Q_PROPERTY(QString latestAgentActivitySummary READ latestAgentActivitySummary NOTIFY
                   agentActivityChanged)
    Q_PROPERTY(QString currentRoutingMode READ currentRoutingMode WRITE setRoutingModeByName NOTIFY
                   modelRoutingChanged)
    Q_PROPERTY(QString modelRoutingStatus READ modelRoutingStatus NOTIFY modelRoutingChanged)
    Q_PROPERTY(QString selectedModelProviderSummary READ selectedModelProviderSummary NOTIFY
                   modelRoutingChanged)
    Q_PROPERTY(QString latestTaskPlanStatus READ latestTaskPlanStatus NOTIFY taskPlanChanged)
    Q_PROPERTY(QString latestTaskPlanSummary READ latestTaskPlanSummary NOTIFY taskPlanChanged)
    Q_PROPERTY(int plannedTaskStepCount READ plannedTaskStepCount NOTIFY taskPlanChanged)
    Q_PROPERTY(int registeredAgentCount READ registeredAgentCount CONSTANT)
    Q_PROPERTY(QStringList activeAgentSummaries READ activeAgentSummaries CONSTANT)
    Q_PROPERTY(QString currentAgentSummary READ currentAgentSummary NOTIFY taskPlanChanged)
    Q_PROPERTY(QString currentMemoryAffinitySummary READ currentMemoryAffinitySummary NOTIFY
                   taskPlanChanged)
    Q_PROPERTY(int providerCatalogCount READ providerCatalogCount CONSTANT)
    Q_PROPERTY(QStringList providerCatalogSummaries READ providerCatalogSummaries CONSTANT)
    Q_PROPERTY(int memoryCatalogCount READ memoryCatalogCount CONSTANT)
    Q_PROPERTY(QStringList memoryCatalogSummaries READ memoryCatalogSummaries CONSTANT)
    Q_PROPERTY(QString orchestrationSnapshotStatus READ orchestrationSnapshotStatus NOTIFY
                   orchestrationSnapshotChanged)
    Q_PROPERTY(QString orchestrationSnapshotSummary READ orchestrationSnapshotSummary NOTIFY
                   orchestrationSnapshotChanged)
    Q_PROPERTY(QStringList orchestrationSignals READ orchestrationSignals NOTIFY
                   orchestrationSnapshotChanged)
    Q_PROPERTY(QString orchestrationReadinessStatus READ orchestrationReadinessStatus NOTIFY
                   orchestrationSnapshotChanged)
    Q_PROPERTY(QString orchestrationReadinessSummary READ orchestrationReadinessSummary NOTIFY
                   orchestrationSnapshotChanged)
    Q_PROPERTY(QStringList orchestrationDiagnostics READ orchestrationDiagnostics NOTIFY
                   orchestrationSnapshotChanged)
    Q_PROPERTY(int availableToolCount READ availableToolCount CONSTANT)
    Q_PROPERTY(QStringList availableToolIds READ availableToolIds CONSTANT)
    Q_PROPERTY(QStringList chatMessages READ chatMessages NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList memoryEntries READ memoryEntries NOTIFY memoryEntriesChanged)
    Q_PROPERTY(QString memoryMaintenanceStatus READ memoryMaintenanceStatus NOTIFY
                   maintenanceStatusChanged)
    Q_PROPERTY(
        QString chatMaintenanceStatus READ chatMaintenanceStatus NOTIFY maintenanceStatusChanged)

public:
    ApplicationController(std::unique_ptr<IChatProvider> provider,
                          std::unique_ptr<IMemoryStore> memoryStore,
                          std::unique_ptr<ChatSession> chatSession = nullptr,
                          std::unique_ptr<IChatHistoryStore> chatHistoryStore = nullptr,
                          std::unique_ptr<IAgentRuntime> agentRuntime = nullptr,
                          std::unique_ptr<IApprovalPolicy> approvalPolicy = nullptr,
                          std::unique_ptr<ISandboxPolicy> sandboxPolicy = nullptr,
                          std::unique_ptr<IToolExecutor> toolExecutor = nullptr,
                          std::unique_ptr<IModelRouter> modelRouter = nullptr,
                          std::unique_ptr<IProviderCatalog> providerCatalog = nullptr,
                          std::unique_ptr<ITaskPlanner> taskPlanner = nullptr,
                          std::unique_ptr<IAgentRegistry> agentRegistry = nullptr,
                          std::unique_ptr<IMemoryCatalog> memoryCatalog = nullptr,
                          QObject* parent = nullptr);

    QString providerName() const;
    QString providerStatus() const;
    QString agentStatus() const;
    QString lastAgentResponse() const;
    QString latestToolPlanStatus() const;
    QString latestToolPlanSummary() const;
    QString latestApprovalStatus() const;
    QString latestApprovalSummary() const;
    QString latestSandboxStatus() const;
    QString latestSandboxSummary() const;
    QString latestToolExecutionStatus() const;
    QString latestToolExecutionSummary() const;
    QString latestAgentPipelineStatus() const;
    QString latestAgentPipelineSummary() const;
    QString runtimeSessionId() const;
    QString runtimeContextStatus() const;
    QString runtimeContextSummary() const;
    QStringList runtimeContextActiveToolIds() const;
    const ConversationSession& currentConversationSession() const;
    QString conversationSessionId() const;
    QString conversationSessionStatus() const;
    QString interactionMode() const;
    QString attentionState() const;
    QString contextWindowSummary() const;
    QString conversationState() const;
    QString conversationTransitionStatus() const;
    QString conversationTransitionSummary() const;
    int agentActivityCount() const;
    QString latestAgentActivitySummary() const;
    QString currentRoutingMode() const;
    void setRoutingModeByName(const QString& routingModeName);
    QString modelRoutingStatus() const;
    QString selectedModelProviderSummary() const;
    QString latestTaskPlanStatus() const;
    QString latestTaskPlanSummary() const;
    int plannedTaskStepCount() const;
    int registeredAgentCount() const;
    QStringList activeAgentSummaries() const;
    QString currentAgentSummary() const;
    QString currentMemoryAffinitySummary() const;
    int providerCatalogCount() const;
    QStringList providerCatalogSummaries() const;
    int memoryCatalogCount() const;
    QStringList memoryCatalogSummaries() const;
    OrchestrationSnapshot currentOrchestrationSnapshot() const;
    QString orchestrationSnapshotStatus() const;
    QString orchestrationSnapshotSummary() const;
    QStringList orchestrationSignals() const;
    OrchestrationReadinessReport currentOrchestrationReadinessReport() const;
    QString orchestrationReadinessStatus() const;
    QString orchestrationReadinessSummary() const;
    QStringList orchestrationDiagnostics() const;
    int availableToolCount() const;
    QStringList availableToolIds() const;
    QString memoryStatus() const;
    QString chatHistoryStatus() const;
    QString memoryMaintenanceStatus() const;
    QString chatMaintenanceStatus() const;
    const QList<ChatMessage>& chatHistory() const;
    QStringList chatMessages() const;
    QStringList memoryEntries() const;

    Q_INVOKABLE bool sendMessage(const QString& message);
    Q_INVOKABLE bool runAgentRequest(const QString& request);
    Q_INVOKABLE bool clearMemory();
    Q_INVOKABLE bool clearChat();
    Q_INVOKABLE void remember(const QString& key, const QString& value);

signals:
    void chatMessagesChanged();
    void memoryEntriesChanged();
    void maintenanceStatusChanged();
    void agentStatusChanged();
    void agentResponseChanged();
    void toolPlanChanged();
    void approvalChanged();
    void sandboxChanged();
    void toolExecutionChanged();
    void agentPipelineChanged();
    void runtimeContextChanged();
    void conversationSessionChanged();
    void conversationStateChanged();
    void agentActivityChanged();
    void modelRoutingChanged();
    void taskPlanChanged();
    void orchestrationSnapshotChanged();

private:
    AgentPipelineResult buildAgentPipelineResult(const AgentRequest& request) const;
    void appendPipelineActivity(const AgentPipelineResult& result);
    void resetCompletedConversationState();
    void transitionConversationState(ConversationState nextState, const QString& reason);
    void refreshLatestTaskPlan();
    void refreshConversationSession();
    void setMemoryMaintenanceStatus(const QString& status);
    void setChatMaintenanceStatus(const QString& status);

    std::unique_ptr<IChatProvider> provider_;
    std::unique_ptr<IAgentRuntime> agentRuntime_;
    std::unique_ptr<IApprovalPolicy> approvalPolicy_;
    std::unique_ptr<ISandboxPolicy> sandboxPolicy_;
    std::unique_ptr<IToolExecutor> toolExecutor_;
    std::unique_ptr<IProviderCatalog> providerCatalog_;
    std::unique_ptr<IModelRouter> modelRouter_;
    std::unique_ptr<ITaskPlanner> taskPlanner_;
    std::unique_ptr<IAgentRegistry> agentRegistry_;
    std::unique_ptr<IMemoryCatalog> memoryCatalog_;
    std::unique_ptr<IMemoryStore> memoryStore_;
    std::unique_ptr<ChatSession> chatSession_;
    std::unique_ptr<IChatHistoryStore> chatHistoryStore_;
    QString memoryMaintenanceStatus_ = QStringLiteral("Ready");
    QString chatMaintenanceStatus_ = QStringLiteral("Ready");
    QString lastAgentResponse_ = QStringLiteral("No agent request yet.");
    AgentPipelineResult latestAgentPipelineResult_;
    TaskPlan latestTaskPlan_;
    RuntimeSession runtimeSession_;
    ConversationSessionStore conversationSession_;
    StaticConversationStateGraph conversationStateGraph_;
    AgentActivityLog agentActivityLog_;
};

} // namespace sentinel::core
