// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/mcp/McpService.h"
#include "sentinel/core/mcp/McpClient.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace sentinel::core {

McpService::McpService(QObject* parent)
    : QObject(parent)
{
}

McpService::~McpService() {
    disconnectFromAll();
    // Clean up owned processes
    for (QProcess* process : m_processes) {
        if (process) {
            process->terminate();
            if (!process->waitForFinished(3000)) {
                process->kill();
            }
            delete process;
        }
    }
}

bool McpService::addServer(const McpServerConfig& config) {
    if (m_servers.contains(config.name)) {
        qWarning() << QStringLiteral("McpService: Server '%1' already exists").arg(config.name);
        return false;
    }

    McpServerState state;
    state.config = config;
    m_servers[config.name] = state;

    qDebug() << QStringLiteral("McpService: Added server '%1' (type: %2)").arg(config.name, config.type);
    return true;
}

bool McpService::removeServer(const QString& serverName) {
    auto it = m_servers.find(serverName);
    if (it == m_servers.end()) {
        return false;
    }

    if (it->state == McpConnectionState::Connected) {
        disconnectFromServer(serverName);
    }

    m_servers.erase(it);
    qDebug() << QStringLiteral("McpService: Removed server '%1'").arg(serverName);
    return true;
}

QList<McpServerConfig> McpService::servers() const {
    QList<McpServerConfig> configs;
    for (const auto& state : m_servers) {
        configs.append(state.config);
    }
    return configs;
}

McpServerConfig McpService::serverConfig(const QString& serverName) const {
    auto it = m_servers.find(serverName);
    if (it == m_servers.end()) {
        return {};
    }
    return it->config;
}

bool McpService::connectToServer(const QString& serverName) {
    auto* state = findServer(serverName);
    if (!state) {
        return false;
    }

    if (state->state == McpConnectionState::Connected) {
        return true;
    }

    state->state = McpConnectionState::Connecting;
    state->errorString.clear();

    bool success = false;
    if (state->config.type == "local") {
        success = connectToLocalServer(*state);
    } else if (state->config.type == "remote") {
        success = connectToRemoteServer(*state);
    } else {
        state->errorString = QStringLiteral("Unknown server type: %1").arg(state->config.type);
        state->state = McpConnectionState::Error;
        emit serverError(serverName, state->errorString);
        return false;
    }

    if (success) {
        state->state = McpConnectionState::Connected;
        emit serverConnected(serverName);

        // List tools after connection
        listTools(*state);
    } else {
        state->state = McpConnectionState::Error;
        emit serverError(serverName, state->errorString);
    }

    return success;
}

bool McpService::disconnectFromServer(const QString& serverName) {
    auto* state = findServer(serverName);
    if (!state) {
        return false;
    }

    if (state->state == McpConnectionState::Disconnected) {
        return true;
    }

    disconnectServer(*state);
    state->state = McpConnectionState::Disconnected;
    state->tools.clear();
    emit serverDisconnected(serverName);

    return true;
}

McpConnectionState McpService::connectionState(const QString& serverName) const {
    const auto* state = findServer(serverName);
    if (!state) {
        return McpConnectionState::Disconnected;
    }
    return state->state;
}

QList<McpToolDefinition> McpService::tools(const QString& serverName) const {
    if (serverName.isEmpty()) {
        QList<McpToolDefinition> allTools;
        for (const auto& state : m_servers) {
            allTools.append(state.tools);
        }
        return allTools;
    }

    const auto* state = findServer(serverName);
    if (!state) {
        return {};
    }
    return state->tools;
}

QJsonObject McpService::callTool(const QString& serverName, const QString& toolName, const QJsonObject& arguments) {
    QJsonObject params;
    params["name"] = toolName;
    params["arguments"] = arguments;

    return sendJsonRpc(serverName, "tools/call", params);
}

bool McpService::connectToAll() {
    bool allSuccess = true;
    for (auto it = m_servers.begin(); it != m_servers.end(); ++it) {
        if (it->config.enabled) {
            if (!connectToServer(it.key())) {
                allSuccess = false;
            }
        }
    }
    return allSuccess;
}

void McpService::disconnectFromAll() {
    for (auto it = m_servers.begin(); it != m_servers.end(); ++it) {
        disconnectServer(it.value());
        it->state = McpConnectionState::Disconnected;
    }
}

bool McpService::connectToLocalServer(McpServerState& state) {
    QProcess* process = new QProcess(this);
    m_processes.append(process);
    state.process = process;

    connect(process, &QProcess::readyReadStandardOutput,
            this, &McpService::onProcessReadyRead);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &McpService::onProcessFinished);
    connect(process, &QProcess::errorOccurred,
            this, &McpService::onProcessErrorOccurred);

    process->start(state.config.command, state.config.arguments);
    if (!process->waitForStarted(5000)) {
        state.errorString = QStringLiteral("Failed to start MCP server process");
        state.process = nullptr;
        return false;
    }

    // Send initialize request
    QJsonObject params;
    params["protocolVersion"] = "2024-11-05";
    params["capabilities"] = QJsonObject();
    params["clientInfo"] = QJsonObject{
        {"name", "Sentinel"},
        {"version", "1.0.0"}
    };

    QJsonObject response = sendJsonRpc(state.config.name, "initialize", params);
    if (response.contains("error")) {
        state.errorString = response["error"].toObject()["message"].toString();
        return false;
    }

    // Send initialized notification
    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = "notifications/initialized";
    process->write(QJsonDocument(notification).toJson(QJsonDocument::Compact) + "\n");

    return true;
}

bool McpService::connectToRemoteServer(McpServerState& state) {
    const QUrl url(state.config.url);
    if (!url.isValid() || (url.scheme() != QStringLiteral("http") &&
                           url.scheme() != QStringLiteral("https"))) {
        state.errorString = QStringLiteral("Remote MCP URL must use HTTP or HTTPS.");
        return false;
    }

    // Send initialize request
    QJsonObject params;
    params["protocolVersion"] = "2024-11-05";
    params["capabilities"] = QJsonObject();
    params["clientInfo"] = QJsonObject{
        {"name", "Sentinel"},
        {"version", "1.0.0"}
    };

    QJsonObject response = sendJsonRpc(state.config.name, "initialize", params);
    if (response.contains("error")) {
        state.errorString = response["error"].toObject()["message"].toString();
        return false;
    }

    return true;
}

void McpService::disconnectServer(McpServerState& state) {
    if (state.process) {
        state.process->terminate();
        if (!state.process->waitForFinished(3000)) {
            state.process->kill();
        }
        m_processes.removeOne(state.process);
        delete state.process;
        state.process = nullptr;
    }
}

void McpService::listTools(McpServerState& state) {
    QJsonObject response = sendJsonRpc(state.config.name, "tools/list");
    if (response.contains("error")) {
        qWarning() << QStringLiteral("McpService: Failed to list tools for '%1': %2")
                        .arg(state.config.name, response["error"].toObject()["message"].toString());
        return;
    }

    state.tools.clear();
    QJsonArray toolsArray = response["result"].toObject()["tools"].toArray();
    for (const auto& toolValue : toolsArray) {
        QJsonObject toolObj = toolValue.toObject();
        McpToolDefinition tool;
        tool.name = toolObj["name"].toString();
        tool.description = toolObj["description"].toString();
        tool.serverName = state.config.name;
        tool.inputSchema = toolObj["inputSchema"].toObject();
        state.tools.append(tool);
    }

    emit toolsUpdated(state.config.name);
    qDebug() << QStringLiteral("McpService: Listed %1 tools for server '%2'")
                    .arg(state.tools.size())
                    .arg(state.config.name);
}

QJsonObject McpService::sendJsonRpc(const QString& serverName, const QString& method, const QJsonObject& params) {
    auto* state = findServer(serverName);
    if (!state) {
        return QJsonObject{{"error", QJsonObject{{"message", "Server not found"}}}};
    }

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = state->requestId++;
    request["method"] = method;
    if (!params.isEmpty()) {
        request["params"] = params;
    }

    QByteArray jsonData = QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n";

    if (state->config.type == "local" && state->process) {
        state->process->write(jsonData);
        state->process->waitForReadyRead(30000);

        QByteArray responseData = state->process->readAllStandardOutput();
        if (responseData.isEmpty()) {
            return QJsonObject{{"error", QJsonObject{{"message", "No response from server"}}}};
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            return QJsonObject{{"error", QJsonObject{{"message", parseError.errorString()}}}};
        }

        return doc.object();
    }

    if (state->config.type == "remote") {
        QNetworkRequest networkRequest(QUrl(state->config.url));
        networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                                 QStringLiteral("application/json"));
        for (auto it = state->config.headers.constBegin(); it != state->config.headers.constEnd();
             ++it) {
            networkRequest.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
        }
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QNetworkReply* reply = m_networkManager.post(networkRequest, jsonData);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(30000);
        loop.exec();
        if (!reply->isFinished()) {
            reply->abort();
            reply->deleteLater();
            return QJsonObject{{"error", QJsonObject{{"message", "MCP remote request timed out"}}}};
        }
        const auto payload = reply->readAll();
        const auto networkError = reply->error() != QNetworkReply::NoError;
        reply->deleteLater();
        if (networkError) {
            return QJsonObject{{"error", QJsonObject{{"message", "MCP remote request failed"}}}};
        }
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            return QJsonObject{{"error", QJsonObject{{"message", "Invalid MCP remote response"}}}};
        }
        return document.object();
    }

    return QJsonObject{{"error", QJsonObject{{"message", "Not connected"}}}};
}

void McpService::onProcessReadyRead() {
    for (auto& state : m_servers) {
        if (state.process && state.process->bytesAvailable() > 0) {
            QByteArray data = state.process->readAllStandardOutput();
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                handleJsonRpcResponse(state.config.name, doc.object());
            }
        }
    }
}

void McpService::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    for (auto& state : m_servers) {
        if (state.process && state.process->state() == QProcess::NotRunning) {
            state.state = McpConnectionState::Disconnected;
            emit serverDisconnected(state.config.name);
        }
    }
}

void McpService::onProcessErrorOccurred(QProcess::ProcessError error) {
    Q_UNUSED(error)

    for (auto& state : m_servers) {
        if (state.process) {
            state.errorString = state.process->errorString();
            state.state = McpConnectionState::Error;
            emit serverError(state.config.name, state.errorString);
        }
    }
}

void McpService::handleJsonRpcResponse(const QString& serverName, const QJsonObject& response) {
    Q_UNUSED(serverName)
    Q_UNUSED(response)

    // Handle response for pending request
    // This is simplified - in production, you'd match by request ID
}

McpServerState* McpService::findServer(const QString& serverName) {
    auto it = m_servers.find(serverName);
    if (it == m_servers.end()) {
        return nullptr;
    }
    return &it.value();
}

const McpServerState* McpService::findServer(const QString& serverName) const {
    auto it = m_servers.find(serverName);
    if (it == m_servers.end()) {
        return nullptr;
    }
    return &it.value();
}

} // namespace sentinel::core
