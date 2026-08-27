// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/mcp/McpClient.h"
#include <QDebug>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QTimer>

namespace sentinel::core {

McpClient::McpClient(QObject* parent) : QObject(parent) {
    m_networkManager = new QNetworkAccessManager(this);
}

McpClient::~McpClient() {
    disconnectLocal();
    disconnectRemote();
}

bool McpClient::connectLocal(const QString& command, const QStringList& arguments) {
    disconnectLocal();

    m_process = std::make_unique<QProcess>();

    connect(m_process.get(), &QProcess::readyReadStandardOutput, this,
            &McpClient::onLocalProcessReadyRead);
    connect(m_process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &McpClient::onLocalProcessFinished);

    m_process->start(command, arguments);
    if (!m_process->waitForStarted(5000)) {
        m_errorString = QStringLiteral("Failed to start MCP server: %1").arg(command);
        emit clientError(m_errorString);
        return false;
    }

    m_transportType = TransportType::Local;
    emit connectedToServer();
    return true;
}

bool McpClient::disconnectLocal() {
    if (m_process) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
        }
        m_process.reset();
    }

    if (m_transportType == TransportType::Local) {
        m_transportType = TransportType::None;
        emit disconnectedFromServer();
    }

    return true;
}

bool McpClient::connectRemote(const QString& url, const QJsonObject& headers) {
    disconnectRemote();

    const QUrl remoteUrl(url);
    if (!remoteUrl.isValid() || (remoteUrl.scheme() != QStringLiteral("http") &&
                                 remoteUrl.scheme() != QStringLiteral("https"))) {
        m_errorString = QStringLiteral("MCP remote URL must use HTTP or HTTPS.");
        emit clientError(m_errorString);
        return false;
    }

    m_remoteUrl = remoteUrl.toString();
    m_remoteHeaders = headers;
    m_transportType = TransportType::Remote;

    emit connectedToServer();
    return true;
}

bool McpClient::disconnectRemote() {
    if (m_transportType == TransportType::Remote) {
        m_remoteUrl.clear();
        m_remoteHeaders = QJsonObject();
        m_transportType = TransportType::None;
        emit disconnectedFromServer();
    }

    return true;
}

QJsonObject McpClient::sendRequest(const QString& method, const QJsonObject& params) {
    QJsonObject response;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    sendRequestAsync(method, params, [&](const QJsonObject& result) {
        response = result;
        loop.quit();
    });
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(30000);
    loop.exec();
    if (response.isEmpty()) {
        return QJsonObject{{"error", QJsonObject{{"message", "MCP request timed out"}}}};
    }
    return response;
}

void McpClient::sendRequestAsync(const QString& method, const QJsonObject& params,
                                 std::function<void(const QJsonObject&)> callback) {
    if (!isConnected()) {
        if (callback) {
            callback(QJsonObject{{"error", QJsonObject{{"message", "Not connected"}}}});
        }
        return;
    }

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    int requestId = m_nextRequestId++;
    request["id"] = requestId;
    request["method"] = method;
    if (!params.isEmpty()) {
        request["params"] = params;
    }

    // Store callback for async response
    McpRequest mcpRequest;
    mcpRequest.id = requestId;
    mcpRequest.method = method;
    mcpRequest.params = params;
    mcpRequest.callback = std::move(callback);
    m_pendingRequests[requestId] = mcpRequest;

    QByteArray jsonData = QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n";

    if (m_transportType == TransportType::Local && m_process) {
        m_process->write(jsonData);
        return;
    }

    if (m_transportType == TransportType::Remote && m_networkManager) {
        QNetworkRequest networkRequest;
        networkRequest.setUrl(QUrl(m_remoteUrl));
        networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                                 QStringLiteral("application/json"));
        for (auto it = m_remoteHeaders.constBegin(); it != m_remoteHeaders.constEnd(); ++it) {
            networkRequest.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
        }
        QNetworkReply* reply = m_networkManager->post(networkRequest, jsonData);
        QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, requestId]() {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QByteArray payload = reply->readAll();
            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
            QJsonObject result;
            if (reply->error() != QNetworkReply::NoError || status >= 400 ||
                parseError.error != QJsonParseError::NoError || !document.isObject()) {
                result = QJsonObject{
                    {"error",
                     QJsonObject{{"message", reply->errorString().isEmpty()
                                                 ? QStringLiteral("Invalid MCP remote response")
                                                 : reply->errorString()}}}};
            } else {
                result = document.object();
            }
            reply->deleteLater();
            auto it = m_pendingRequests.find(requestId);
            if (it != m_pendingRequests.end()) {
                if (it->callback)
                    it->callback(result);
                m_pendingRequests.erase(it);
            }
        });
    }
}

bool McpClient::isConnected() const {
    if (m_transportType == TransportType::Local) {
        return m_process && m_process->state() == QProcess::Running;
    }
    if (m_transportType == TransportType::Remote) {
        return !m_remoteUrl.isEmpty();
    }
    return false;
}

QString McpClient::errorString() const {
    return m_errorString;
}

void McpClient::onLocalProcessReadyRead() {
    if (!m_process) {
        return;
    }

    QByteArray data = m_process->readAllStandardOutput();
    m_buffer.append(data);

    // Try to read complete JSON messages
    while (!m_buffer.isEmpty()) {
        // Find newline delimiter
        int newlinePos = m_buffer.indexOf('\n');
        if (newlinePos == -1) {
            break; // Wait for more data
        }

        QByteArray line = m_buffer.left(newlinePos);
        m_buffer.remove(0, newlinePos + 1);

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject message = doc.object();

            if (message.contains("id") && message["id"].isDouble()) {
                // Response to a request
                handleResponse(message);
            } else if (message.contains("method")) {
                // Notification or request from server
                handleNotification(message);
            }
        }
    }
}

void McpClient::onLocalProcessFinished(int exitCode) {
    Q_UNUSED(exitCode)

    m_errorString = QStringLiteral("MCP server process exited");
    m_transportType = TransportType::None;
    emit clientError(m_errorString);
    emit disconnectedFromServer();
}

void McpClient::handleResponse(const QJsonObject& response) {
    int requestId = response["id"].toInt();

    auto it = m_pendingRequests.find(requestId);
    if (it != m_pendingRequests.end()) {
        if (it->callback) {
            it->callback(response);
        }
        m_pendingRequests.erase(it);
    }
}

void McpClient::handleNotification(const QJsonObject& notification) {
    QString method = notification["method"].toString();
    QJsonObject params = notification["params"].toObject();

    emit notificationReceived(method, params);
}

} // namespace sentinel::core
