// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/mcp/McpService.h"
#include "sentinel/core/mcp/McpClient.h"
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTimer>

namespace sentinel::core {

McpService::McpService(QObject* parent) : QObject(parent) {}

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

    qDebug()
        << QStringLiteral("McpService: Added server '%1' (type: %2)").arg(config.name, config.type);
    return true;
}

bool McpService::removeServer(const QString& serverName) {
    auto it = m_servers.find(serverName);
    if (it == m_servers.end()) {
        return false;
    }

    if (it->state != McpConnectionState::Disconnected) {
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

    auto pending = m_pendingCalls.take(serverName);
    for (auto& completion : pending)
        completion({{"error", QJsonObject{{"message", "MCP server disconnected"}}}});
    m_readBuffers.remove(serverName);
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

QJsonObject McpService::callTool(const QString& serverName, const QString& toolName,
                                 const QJsonObject& arguments) {
    QJsonObject params;
    params["name"] = toolName;
    params["arguments"] = arguments;

    return sendJsonRpc(serverName, "tools/call", params);
}

IMcpService::Cancel McpService::callToolAsync(const QString& serverName, const QString& toolName,
                                              const QJsonObject& arguments,
                                              ToolCompletion completion) {
    auto* state = findServer(serverName);
    if (!state || state->state != McpConnectionState::Connected) {
        completion({{"error", QJsonObject{{"message", "MCP server unavailable"}}}});
        return {};
    }
    const int id = state->requestId++;
    QJsonObject request{{"jsonrpc", "2.0"},
                        {"id", id},
                        {"method", "tools/call"},
                        {"params", QJsonObject{{"name", toolName}, {"arguments", arguments}}}};
    const auto payload = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    if (state->config.type == QStringLiteral("local") && state->process) {
        m_pendingCalls[serverName].insert(id, std::move(completion));
        state->process->write(payload);
        QPointer<McpService> self(this);
        QTimer::singleShot(30000, this, [self, serverName, id] {
            if (!self)
                return;
            auto callback = self->m_pendingCalls[serverName].take(id);
            if (callback)
                callback({{"error", QJsonObject{{"message", "MCP local request timed out"}}}});
        });
        return [self, serverName, id] {
            if (self)
                self->m_pendingCalls[serverName].remove(id);
        };
    }
    if (state->config.type == QStringLiteral("remote")) {
        QNetworkRequest networkRequest(QUrl(state->config.url));
        networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                                 QStringLiteral("application/json"));
        for (auto it = state->config.headers.constBegin(); it != state->config.headers.constEnd();
             ++it)
            networkRequest.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
        auto* reply = m_networkManager.post(networkRequest, payload);
        auto* timer = new QTimer(reply);
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout, reply, [reply] { reply->abort(); });
        timer->start(30000);
        QObject::connect(
            reply, &QNetworkReply::finished, this,
            [reply, completion = std::move(completion)]() mutable {
                QJsonObject result;
                if (reply->error() != QNetworkReply::NoError)
                    result = {{"error", QJsonObject{{"message", reply->errorString()}}}};
                else {
                    QJsonParseError error;
                    const auto document = QJsonDocument::fromJson(reply->readAll(), &error);
                    result = error.error == QJsonParseError::NoError && document.isObject()
                                 ? document.object()
                                 : QJsonObject{
                                       {"error", QJsonObject{{"message", "Invalid MCP response"}}}};
                }
                reply->deleteLater();
                completion(std::move(result));
            });
        QPointer<QNetworkReply> safeReply(reply);
        return [safeReply] {
            if (safeReply)
                safeReply->abort();
        };
    }
    completion({{"error", QJsonObject{{"message", "Unsupported MCP transport"}}}});
    return {};
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
    const auto names = m_servers.keys();
    for (const auto& name : names)
        disconnectFromServer(name);
}

bool McpService::connectToLocalServer(McpServerState& state) {
    auto temporaryDirectory = std::make_shared<QTemporaryDir>();
    if (!temporaryDirectory->isValid()) {
        state.errorString = QStringLiteral("MCP sandbox temporary directory is unavailable.");
        return false;
    }
    SandboxExecutionPlan plan;
    plan.workingDirectory = QDir::currentPath();
    plan.readablePaths = {plan.workingDirectory};
    plan.temporaryDirectory = temporaryDirectory->path();
    const auto environment = QProcessEnvironment::systemEnvironment();
    const auto executable = QStandardPaths::findExecutable(state.config.command,
        environment.value(QStringLiteral("PATH")).split(QDir::listSeparator(), Qt::SkipEmptyParts));
    const auto launch = m_processSandbox.prepare(plan, executable, state.config.arguments,
                                                  environment);
    if (!launch.permitted) {
        state.errorString = QStringLiteral("MCP sandbox unavailable: %1.")
                                .arg(launch.result.failureCategory);
        return false;
    }
    QProcess* process = new QProcess(this);
    m_processes.append(process);
    state.process = process;
    state.sandboxTemporaryDirectory = std::move(temporaryDirectory);
    process->setWorkingDirectory(plan.workingDirectory);
    process->setProcessEnvironment(launch.environment);

    connect(process, &QProcess::readyReadStandardOutput, this, &McpService::onProcessReadyRead);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &McpService::onProcessFinished);
    connect(process, &QProcess::errorOccurred, this, &McpService::onProcessErrorOccurred);

    process->start(launch.program, launch.arguments);
    if (!process->waitForStarted(5000)) {
        state.errorString = QStringLiteral("Failed to start MCP server process");
        state.process = nullptr;
        return false;
    }

    // Send initialize request
    QJsonObject params;
    params["protocolVersion"] = "2024-11-05";
    params["capabilities"] = QJsonObject();
    params["clientInfo"] = QJsonObject{{"name", "Sentinel"}, {"version", "1.0.0"}};

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
    if (!url.isValid() ||
        (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
        state.errorString = QStringLiteral("Remote MCP URL must use HTTP or HTTPS.");
        return false;
    }

    // Send initialize request
    QJsonObject params;
    params["protocolVersion"] = "2024-11-05";
    params["capabilities"] = QJsonObject();
    params["clientInfo"] = QJsonObject{{"name", "Sentinel"}, {"version", "1.0.0"}};

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
                          .arg(state.config.name,
                               response["error"].toObject()["message"].toString());
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
        // Explicit MCP tool metadata opts into Sentinel's validated filesystem evidence adapter.
        tool.filesystemSemanticContract = toolObj.value(QStringLiteral("_meta")).toObject()
            .value(QStringLiteral("sentinel.failureSemanticContract")).toString() == QLatin1String("filesystem");
        state.tools.append(tool);
    }

    emit toolsUpdated(state.config.name);
    qDebug() << QStringLiteral("McpService: Listed %1 tools for server '%2'")
                    .arg(state.tools.size())
                    .arg(state.config.name);
}

QJsonObject McpService::sendJsonRpc(const QString& serverName, const QString& method,
                                    const QJsonObject& params) {
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
        if (state.process && state.process->bytesAvailable() > 0 &&
            !m_pendingCalls.value(state.config.name).isEmpty()) {
            auto& buffer = m_readBuffers[state.config.name];
            buffer += state.process->readAllStandardOutput();
            while (true) {
                const auto end = buffer.indexOf('\n');
                if (end < 0)
                    break;
                const auto line = buffer.left(end);
                buffer.remove(0, end + 1);
                const auto document = QJsonDocument::fromJson(line);
                if (!document.isObject())
                    continue;
                const auto response = document.object();
                const auto id = response.value(QStringLiteral("id")).toInt(-1);
                auto completion = m_pendingCalls[state.config.name].take(id);
                if (completion)
                    completion(response);
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
            auto pending = m_pendingCalls.take(state.config.name);
            for (auto& completion : pending)
                completion({{"error", QJsonObject{{"message", "MCP process exited"}}}});
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
