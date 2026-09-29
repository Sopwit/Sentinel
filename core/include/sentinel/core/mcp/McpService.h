// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/mcp/IMcpService.h"
#include "sentinel/core/runtime/ProcessSandbox.h"
#include "sentinel/core/runtime/ProcessExecutor.h"
#include <QMap>
#include <QSet>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QTemporaryDir>
#include <QDateTime>
#include <QJsonValue>
#include <memory>

namespace sentinel::core {

enum class McpInteractionType { Form, Text, Confirmation, Selection, ExternalUrl };

struct McpInteractionField {
    QString name;
    QString label;
    McpInteractionType type{McpInteractionType::Text};
    QStringList options;
    int maxLength{500};
    bool required{false};
};

struct McpInteractionRequest {
    QString id;
    QString serverId;
    quint64 generation{0};
    int correlationId{-1};
    McpInteractionType type{McpInteractionType::Text};
    QString title;
    QString detail;
    QStringList options;
    QList<McpInteractionField> fields;
    QString url;
    bool sensitive{false};
    QDateTime expiresAt;
};

enum class McpInteractionDecision { Accept, Decline, Cancel };

struct McpServerState {
    McpServerConfig config;
    McpConnectionState state{McpConnectionState::Disconnected};
    QProcess* process{nullptr}; // Raw pointer, managed by McpService
    QString managedProcessId;
    qint64 processGroupId{0};
    std::shared_ptr<QTemporaryDir> sandboxTemporaryDirectory;
    QList<McpToolDefinition> tools;
    QList<McpToolDefinition> lastKnownTools;
    QList<McpResource> resources;
    QList<McpResource> lastKnownResources;
    bool resourcesSupported{false};
    QString errorString;
    McpFailureCategory failureCategory{McpFailureCategory::None};
    int requestId{0};
    int synchronousRequestId{-1};
    QJsonObject synchronousResponse;
    bool synchronousResponseReceived{false};
    int activeSynchronousCalls{0};
    quint64 generation{0};
    QByteArray remoteSessionId;
    QByteArray protocolVersion;
    QByteArray lastEventId;
};

class McpService : public QObject, public IMcpService {
    Q_OBJECT
public:
    explicit McpService(QObject* parent = nullptr);
    ~McpService() override;

    // IMcpService interface
    bool addServer(const McpServerConfig& config) override;
    bool updateServer(const McpServerConfig& config);
    bool removeServer(const QString& serverName) override;
    QList<McpServerConfig> servers() const override;
    McpServerConfig serverConfig(const QString& serverName) const override;

    bool connectToServer(const QString& serverName) override;
    bool disconnectFromServer(const QString& serverName) override;
    McpConnectionState connectionState(const QString& serverName) const override;
    QString lastError(const QString& serverName) const;
    McpFailureCategory failureCategory(const QString& serverName) const;
    bool hasRemoteSession(const QString& serverName) const;
    QList<McpInteractionRequest> pendingInteractions() const;
    bool respondToInteraction(const QString& id, quint64 generation,
                              McpInteractionDecision decision, const QJsonValue& value = {});
    QList<McpToolDefinition> lastKnownTools(const QString& serverName) const;
    QList<McpResource> resources(const QString& serverName = {}) const;
    QList<McpResource> lastKnownResources(const QString& serverName) const;
    bool refreshResources(const QString& serverName);
    bool refreshTools(const QString& serverName);
    bool setEnabled(const QString& serverName, bool enabled);
    void invalidateInventory(const QString& serverName, McpFailureCategory category,
                             const QString& detail);
    void confirmToolInventory(const QString& serverName);

    QList<McpToolDefinition> tools(const QString& serverName = QString()) const override;
    QJsonObject callTool(const QString& serverName, const QString& toolName,
                         const QJsonObject& arguments = {}) override;
    Cancel callToolAsync(const QString& serverName, const QString& toolName,
                         const QJsonObject& arguments, ToolCompletion completion) override;

    bool connectToAll() override;
    void disconnectFromAll() override;

signals:
    void serverAdded(const QString& serverName);
    void serverRemoved(const QString& serverName);
    void serverConfigChanged(const QString& serverName);
    void serverConnected(const QString& serverName) override;
    void serverDisconnected(const QString& serverName) override;
    void serverError(const QString& serverName, const QString& error) override;
    void toolsUpdated(const QString& serverName) override;
    void resourcesUpdated(const QString& serverName);
    void interactionRequested(const McpInteractionRequest& request);
    void interactionResolved(const QString& id);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessErrorOccurred(QProcess::ProcessError error);

private:
    // JSON-RPC communication
    QJsonObject sendJsonRpc(const QString& serverName, const QString& method,
                            const QJsonObject& params = {});
    QNetworkRequest remoteRequest(const McpServerState& state) const;
    QJsonObject parseRemoteReply(QNetworkReply* reply, int id,
                                 bool allowInputRequired = false) const;
    void sendRemoteNotification(McpServerState& state, const QString& method);
    void startRemoteEventStream(const QString& serverName);
    struct RemoteToolCall;
    void startRemoteToolRound(const std::shared_ptr<RemoteToolCall>& call);
    void advanceInteraction(const std::shared_ptr<RemoteToolCall>& call);
    void finishRemoteToolCall(const std::shared_ptr<RemoteToolCall>& call,
                              QJsonObject result);

    // Server operations
    bool connectToLocalServer(McpServerState& state);
    bool connectToRemoteServer(McpServerState& state);
    void disconnectServer(McpServerState& state);

    // Tool listing
    void listTools(McpServerState& state);
    void listResources(McpServerState& state);

    // Helper methods
    McpServerState* findServer(const QString& serverName);
    const McpServerState* findServer(const QString& serverName) const;

    QMap<QString, McpServerState> m_servers;
    int m_processingStdout{0};
    QList<QProcess*> m_processes; // Owned processes for cleanup
    QNetworkAccessManager m_networkManager;
    PlatformProcessSandbox m_processSandbox;
    ProcessExecutor m_processExecutor;
    QMap<QString, QMap<int, ToolCompletion>> m_pendingCalls;
    QMap<QString, QByteArray> m_readBuffers;
    QMap<QString, QList<QPointer<QNetworkReply>>> m_remoteReplies;
    QMap<QString, std::shared_ptr<RemoteToolCall>> m_interactions;
    QSet<QString> m_toolRefreshScheduled;
    QSet<QString> m_resourceRefreshScheduled;
};

} // namespace sentinel::core
