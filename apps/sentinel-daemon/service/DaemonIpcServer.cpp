// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DaemonIpcServer.h"

#include "sentinel/core/app/ApplicationController.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>

namespace sentinel::daemon {

DaemonIpcServer::DaemonIpcServer(sentinel::core::ApplicationController* controller, QObject* parent)
    : QObject(parent), m_controller(controller) {
    connect(&m_server, &QLocalServer::newConnection, this, &DaemonIpcServer::handleNewConnection);
}

DaemonIpcServer::~DaemonIpcServer() {
    stopServer();
}

void DaemonIpcServer::setController(sentinel::core::ApplicationController* controller) {
    m_controller = controller;
}

bool DaemonIpcServer::startServer(const QString& serverName) {
    QLocalServer::removeServer(serverName);
    if (!m_server.listen(serverName)) {
        qWarning().noquote() << "Daemon IPC server failed to start:" << m_server.errorString();
        return false;
    }
    qInfo().noquote() << "Daemon IPC server listening on:" << serverName;
    return true;
}

void DaemonIpcServer::stopServer() {
    if (m_server.isListening()) {
        m_server.close();
    }
}

void DaemonIpcServer::handleNewConnection() {
    while (auto* socket = m_server.nextPendingConnection()) {
        connect(socket, &QLocalSocket::readyRead, socket, [this, socket]() {
            while (socket->canReadLine()) {
                const QByteArray line = socket->readLine();
                const QByteArray trimmed = line.trimmed();
                if (!trimmed.isEmpty()) {
                    handleRequest(m_controller, trimmed, socket);
                }
            }
        });
        connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
    }
}

QByteArray DaemonIpcServer::buildStatusResponse(
    sentinel::core::ApplicationController* controller) const {
    QJsonObject root;
    root.insert(QStringLiteral("status"), QStringLiteral("ok"));
    root.insert(QStringLiteral("service"), QStringLiteral("sentinel-daemon"));

    if (!controller) {
        root.insert(QStringLiteral("controllerAvailable"), false);
        root.insert(QStringLiteral("ollamaHealth"),
                    QStringLiteral("Controller unavailable"));
        return QJsonDocument(root).toJson(QJsonDocument::Compact) + '\n';
    }

    root.insert(QStringLiteral("controllerAvailable"), true);
    root.insert(QStringLiteral("ollamaEndpoint"), controller->ollamaEndpoint());
    root.insert(QStringLiteral("ollamaHealth"), controller->ollamaHealthStatus());
    root.insert(QStringLiteral("ollamaConnection"), controller->ollamaConnectionStatus());
    root.insert(QStringLiteral("ollamaModelCount"), controller->ollamaModelCount());

    QJsonArray models;
    const auto modelNames = controller->ollamaModelNames();
    for (const auto& modelName : modelNames) {
        models.append(modelName);
    }
    root.insert(QStringLiteral("ollamaModels"), models);

    root.insert(QStringLiteral("providerName"), controller->providerName());
    root.insert(QStringLiteral("providerStatus"), controller->providerStatus());
    root.insert(QStringLiteral("conversationState"), controller->conversationState());
    root.insert(QStringLiteral("memoryEntryCount"), controller->memoryEntryCount());

    return QJsonDocument(root).toJson(QJsonDocument::Compact) + '\n';
}

void DaemonIpcServer::handleRequest(sentinel::core::ApplicationController* controller,
                                    const QByteArray& data, QLocalSocket* socket) {
    qInfo().noquote() << "Daemon received IPC request:" << QString::fromUtf8(data);

    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(data, &parseError);

    QByteArray response;
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        response = QByteArrayLiteral("{\"status\":\"error\",\"error\":\"invalid-request\"}\n");
    } else {
        const QString command =
            doc.object().value(QStringLiteral("command")).toString(QStringLiteral("status"));
        if (command == QStringLiteral("ping")) {
            response = QByteArrayLiteral("{\"status\":\"ok\",\"service\":\"sentinel-daemon\","
                                         "\"pong\":true}\n");
        } else if (command == QStringLiteral("status") || command == QStringLiteral("health") ||
                   command == QStringLiteral("models")) {
            response = buildStatusResponse(controller);
        } else if (command == QStringLiteral("shutdown")) {
            response = QByteArrayLiteral("{\"status\":\"ok\",\"service\":\"sentinel-daemon\","
                                         "\"shuttingDown\":true}\n");
            socket->write(response);
            socket->flush();
            socket->disconnectFromServer();
            QCoreApplication::exit(0);
            return;
        } else {
            response = QByteArrayLiteral("{\"status\":\"error\",\"error\":\"unknown-command\"}\n");
        }
    }

    socket->write(response);
    socket->flush();
    socket->disconnectFromServer();
}

} // namespace sentinel::daemon
