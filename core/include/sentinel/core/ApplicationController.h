#pragma once

#include "sentinel/core/AgentActivityLog.h"
#include "sentinel/core/AgentPipelineResult.h"
#include "sentinel/core/AgentRuntimeContext.h"
#include "sentinel/core/ChatSession.h"
#include "sentinel/core/IAgentRuntime.h"
#include "sentinel/core/IApprovalPolicy.h"
#include "sentinel/core/IChatHistoryStore.h"
#include "sentinel/core/IChatProvider.h"
#include "sentinel/core/IMemoryStore.h"
#include "sentinel/core/ISandboxPolicy.h"
#include "sentinel/core/IToolExecutor.h"

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
    Q_PROPERTY(int agentActivityCount READ agentActivityCount NOTIFY agentActivityChanged)
    Q_PROPERTY(QString latestAgentActivitySummary READ latestAgentActivitySummary NOTIFY
                   agentActivityChanged)
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
    int agentActivityCount() const;
    QString latestAgentActivitySummary() const;
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
    void agentActivityChanged();

private:
    AgentPipelineResult buildAgentPipelineResult(const AgentRequest& request) const;
    void appendPipelineActivity(const AgentPipelineResult& result);
    void setMemoryMaintenanceStatus(const QString& status);
    void setChatMaintenanceStatus(const QString& status);

    std::unique_ptr<IChatProvider> provider_;
    std::unique_ptr<IAgentRuntime> agentRuntime_;
    std::unique_ptr<IApprovalPolicy> approvalPolicy_;
    std::unique_ptr<ISandboxPolicy> sandboxPolicy_;
    std::unique_ptr<IToolExecutor> toolExecutor_;
    std::unique_ptr<IMemoryStore> memoryStore_;
    std::unique_ptr<ChatSession> chatSession_;
    std::unique_ptr<IChatHistoryStore> chatHistoryStore_;
    QString memoryMaintenanceStatus_ = QStringLiteral("Ready");
    QString chatMaintenanceStatus_ = QStringLiteral("Ready");
    QString lastAgentResponse_ = QStringLiteral("No agent request yet.");
    AgentPipelineResult latestAgentPipelineResult_;
    RuntimeSession runtimeSession_;
    AgentActivityLog agentActivityLog_;
};

} // namespace sentinel::core
