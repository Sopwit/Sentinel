#pragma once

#include "sentinel/core/ChatSession.h"
#include "sentinel/core/IAgentRuntime.h"
#include "sentinel/core/IChatHistoryStore.h"
#include "sentinel/core/IChatProvider.h"
#include "sentinel/core/IMemoryStore.h"

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
                          QObject* parent = nullptr);

    QString providerName() const;
    QString providerStatus() const;
    QString agentStatus() const;
    QString lastAgentResponse() const;
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

private:
    void setMemoryMaintenanceStatus(const QString& status);
    void setChatMaintenanceStatus(const QString& status);

    std::unique_ptr<IChatProvider> provider_;
    std::unique_ptr<IAgentRuntime> agentRuntime_;
    std::unique_ptr<IMemoryStore> memoryStore_;
    std::unique_ptr<ChatSession> chatSession_;
    std::unique_ptr<IChatHistoryStore> chatHistoryStore_;
    QString memoryMaintenanceStatus_ = QStringLiteral("Ready");
    QString chatMaintenanceStatus_ = QStringLiteral("Ready");
    QString lastAgentResponse_ = QStringLiteral("No agent request yet.");
};

} // namespace sentinel::core
