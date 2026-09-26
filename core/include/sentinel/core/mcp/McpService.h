// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/mcp/IMcpService.h"
#include "sentinel/core/runtime/ProcessSandbox.h"
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QTemporaryDir>
#include <memory>

namespace sentinel::core {

struct McpServerState {
    McpServerConfig config;
    McpConnectionState state{McpConnectionState::Disconnected};
    QProcess* process{nullptr}; // Raw pointer, managed by McpService
    std::shared_ptr<QTemporaryDir> sandboxTemporaryDirectory;
    QList<McpToolDefinition> tools;
    QString errorString;
    int requestId{0};
};

class McpService : public QObject, public IMcpService {
    Q_OBJECT
public:
    explicit McpService(QObject* parent = nullptr);
    ~McpService() override;

    // IMcpService interface
    bool addServer(const McpServerConfig& config) override;
    bool removeServer(const QString& serverName) override;
    QList<McpServerConfig> servers() const override;
    McpServerConfig serverConfig(const QString& serverName) const override;

    bool connectToServer(const QString& serverName) override;
    bool disconnectFromServer(const QString& serverName) override;
    McpConnectionState connectionState(const QString& serverName) const override;

    QList<McpToolDefinition> tools(const QString& serverName = QString()) const override;
    QJsonObject callTool(const QString& serverName, const QString& toolName,
                         const QJsonObject& arguments = {}) override;
    Cancel callToolAsync(const QString& serverName, const QString& toolName,
                         const QJsonObject& arguments, ToolCompletion completion) override;

    bool connectToAll() override;
    void disconnectFromAll() override;

signals:
    void serverConnected(const QString& serverName) override;
    void serverDisconnected(const QString& serverName) override;
    void serverError(const QString& serverName, const QString& error) override;
    void toolsUpdated(const QString& serverName) override;

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessErrorOccurred(QProcess::ProcessError error);

private:
    // JSON-RPC communication
    QJsonObject sendJsonRpc(const QString& serverName, const QString& method,
                            const QJsonObject& params = {});
    void handleJsonRpcResponse(const QString& serverName, const QJsonObject& response);

    // Server operations
    bool connectToLocalServer(McpServerState& state);
    bool connectToRemoteServer(McpServerState& state);
    void disconnectServer(McpServerState& state);

    // Tool listing
    void listTools(McpServerState& state);

    // Helper methods
    McpServerState* findServer(const QString& serverName);
    const McpServerState* findServer(const QString& serverName) const;

    QMap<QString, McpServerState> m_servers;
    QList<QProcess*> m_processes; // Owned processes for cleanup
    QNetworkAccessManager m_networkManager;
    PlatformProcessSandbox m_processSandbox;
    QMap<QString, QMap<int, ToolCompletion>> m_pendingCalls;
    QMap<QString, QByteArray> m_readBuffers;
};

} // namespace sentinel::core
