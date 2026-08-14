// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/lsp/LspClient.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

namespace sentinel::core {

LspClient::LspClient(const QString& serverName, QObject* parent)
    : QObject(parent)
    , m_serverName(serverName)
{
}

LspClient::~LspClient() {
    stop();
}

bool LspClient::start(const QString& command, const QStringList& arguments) {
    if (m_process && m_process->state() == QProcess::Running) {
        return true;
    }

    m_process = std::make_unique<QProcess>();

    connect(m_process.get(), &QProcess::readyReadStandardOutput,
            this, &LspClient::onProcessReadyRead);
    connect(m_process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LspClient::onProcessFinished);

    m_process->start(command, arguments);
    if (!m_process->waitForStarted(5000)) {
        m_errorString = QStringLiteral("Failed to start LSP server: %1").arg(command);
        emit error(m_errorString);
        return false;
    }

    emit started();
    return true;
}

bool LspClient::stop() {
    if (!m_process) {
        return true;
    }

    if (m_process->state() == QProcess::Running) {
        shutdown();
        exit();
        m_process->waitForFinished(3000);
        if (m_process->state() == QProcess::Running) {
            m_process->kill();
        }
    }

    m_process.reset();
    emit stopped();
    return true;
}

bool LspClient::isRunning() const {
    return m_process && m_process->state() == QProcess::Running;
}

QJsonObject LspClient::sendRequest(const QString& method, const QJsonObject& params) {
    if (!isRunning()) {
        return QJsonObject{{"error", QJsonObject{{"message", "Server not running"}}}};
    }

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = m_nextRequestId++;
    request["method"] = method;
    if (!params.isEmpty()) {
        request["params"] = params;
    }

    QByteArray jsonData = QJsonDocument(request).toJson(QJsonDocument::Compact);
    QByteArray message = buildMessage(jsonData);

    m_process->write(message);
    m_process->waitForReadyRead(30000);

    QByteArray responseData = m_process->readAllStandardOutput();
    if (responseData.isEmpty()) {
        return QJsonObject{{"error", QJsonObject{{"message", "No response"}}}};
    }

    // Parse LSP response (Content-Length header)
    int headerEnd = responseData.indexOf("\r\n\r\n");
    if (headerEnd == -1) {
        return QJsonObject{{"error", QJsonObject{{"message", "Invalid response format"}}}};
    }

    QByteArray body = responseData.mid(headerEnd + 4);
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return QJsonObject{{"error", QJsonObject{{"message", parseError.errorString()}}}};
    }

    return doc.object();
}

void LspClient::sendRequestAsync(const QString& method, const QJsonObject& params,
                                  std::function<void(const QJsonObject&)> callback) {
    if (!isRunning()) {
        if (callback) {
            callback(QJsonObject{{"error", QJsonObject{{"message", "Server not running"}}}});
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

    LspRequest lspRequest;
    lspRequest.id = requestId;
    lspRequest.method = method;
    lspRequest.params = params;
    lspRequest.callback = std::move(callback);
    m_pendingRequests[requestId] = lspRequest;

    QByteArray jsonData = QJsonDocument(request).toJson(QJsonDocument::Compact);
    QByteArray message = buildMessage(jsonData);

    m_process->write(message);
}

void LspClient::sendNotification(const QString& method, const QJsonObject& params) {
    if (!isRunning()) {
        return;
    }

    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = method;
    if (!params.isEmpty()) {
        notification["params"] = params;
    }

    QByteArray jsonData = QJsonDocument(notification).toJson(QJsonDocument::Compact);
    QByteArray message = buildMessage(jsonData);

    m_process->write(message);
}

void LspClient::initialize(const QString& rootPath) {
    QJsonObject params;
    params["processId"] = QCoreApplication::applicationPid();
    params["rootUri"] = QStringLiteral("file://%1").arg(rootPath);
    params["capabilities"] = QJsonObject{
        {"textDocument", QJsonObject{
            {"synchronization", QJsonObject{
                {"didOpen", true},
                {"didChange", true},
                {"didClose", true}
            }},
            {"definition", QJsonObject{{"dynamicRegistration", false}}},
            {"references", QJsonObject{{"dynamicRegistration", false}}},
            {"hover", QJsonObject{{"dynamicRegistration", false}}},
            {"documentSymbol", QJsonObject{{"dynamicRegistration", false}}}
        }}
    };

    sendRequest("initialize", params);
}

void LspClient::initialized() {
    sendNotification("initialized");
}

void LspClient::shutdown() {
    sendRequest("shutdown");
}

void LspClient::exit() {
    sendNotification("exit");
}

QString LspClient::errorString() const {
    return m_errorString;
}

void LspClient::onProcessReadyRead() {
    if (!m_process) {
        return;
    }

    QByteArray data = m_process->readAllStandardOutput();
    m_buffer.append(data);

    // Try to read complete LSP messages
    while (true) {
        // Check for Content-Length header
        int headerEnd = m_buffer.indexOf("\r\n\r\n");
        if (headerEnd == -1) {
            break; // Wait for more data
        }

        // Parse Content-Length
        QByteArray header = m_buffer.left(headerEnd);
        int contentLength = 0;
        for (const QByteArray& line : header.split('\r')) {
            if (line.startsWith("Content-Length: ")) {
                contentLength = line.mid(16).toInt();
                break;
            }
        }

        if (contentLength <= 0) {
            m_buffer.clear();
            break;
        }

        // Check if we have enough data
        int messageEnd = headerEnd + 4 + contentLength;
        if (m_buffer.size() < messageEnd) {
            break; // Wait for more data
        }

        // Extract message body
        QByteArray body = m_buffer.mid(headerEnd + 4, contentLength);
        m_buffer.remove(0, messageEnd);

        // Parse JSON
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject message = doc.object();

            if (message.contains("id") && message["id"].isDouble()) {
                handleResponse(message);
            } else if (message.contains("method")) {
                handleNotification(message);
            }
        }
    }
}

void LspClient::onProcessFinished(int exitCode) {
    Q_UNUSED(exitCode)

    m_errorString = "LSP server process exited";
    emit error(m_errorString);
}

void LspClient::handleResponse(const QJsonObject& response) {
    int requestId = response["id"].toInt();

    auto it = m_pendingRequests.find(requestId);
    if (it != m_pendingRequests.end()) {
        if (it->callback) {
            it->callback(response);
        }
        m_pendingRequests.erase(it);
    }
}

void LspClient::handleNotification(const QJsonObject& notification) {
    QString method = notification["method"].toString();
    QJsonObject params = notification["params"].toObject();

    emit notificationReceived(method, params);
}

QByteArray LspClient::buildMessage(const QByteArray& content) const {
    QByteArray header = "Content-Length: " + QByteArray::number(content.size()) + "\r\n\r\n";
    return header + content;
}

} // namespace sentinel::core
