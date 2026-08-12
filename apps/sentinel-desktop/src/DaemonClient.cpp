// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/desktop/DaemonClient.h"

#include <QJsonDocument>
#include <QLocalSocket>
#include <QTimer>

namespace sentinel::desktop {

DaemonClient::DaemonClient(QObject* parent) : QObject(parent) {
    m_socket = new QLocalSocket(this);
    connect(m_socket, &QLocalSocket::connected, this, &DaemonClient::onConnected);
    connect(m_socket, &QLocalSocket::disconnected, this, &DaemonClient::onDisconnected);
    connect(m_socket, &QLocalSocket::readyRead, this, &DaemonClient::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, [this]() { onDisconnected(); });

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(15000);
    connect(m_reconnectTimer, &QTimer::timeout, this, &DaemonClient::connectToDaemon);

    connectToDaemon();
    m_reconnectTimer->start();
}

DaemonClient::~DaemonClient() = default;

bool DaemonClient::daemonReachable() const {
    return m_daemonReachable;
}

QString DaemonClient::statusSummary() const {
    return m_statusSummary;
}

void DaemonClient::connectToDaemon() {
    if (m_socket->state() == QLocalSocket::ConnectingState ||
        m_socket->state() == QLocalSocket::ConnectedState) {
        return;
    }
    m_socket->abort();
    m_socket->connectToServer(m_serverName);
}

void DaemonClient::onConnected() {
    const auto previous = m_daemonReachable;
    m_daemonReachable = true;
    if (previous != m_daemonReachable) {
        emit daemonReachableChanged();
    }
    refresh();
}

void DaemonClient::onDisconnected() {
    const auto previous = m_daemonReachable;
    m_daemonReachable = false;
    m_statusSummary = QStringLiteral("Daemon not reachable.");
    if (previous != m_daemonReachable) {
        emit daemonReachableChanged();
    }
}

void DaemonClient::refresh() {
    if (m_socket->state() != QLocalSocket::ConnectedState) {
        connectToDaemon();
        return;
    }

    const QJsonObject request{{QStringLiteral("command"), QStringLiteral("status")}};
    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n');
    m_socket->flush();
}

void DaemonClient::onReadyRead() {
    while (m_socket->canReadLine()) {
        const QByteArray line = m_socket->readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        QJsonParseError parseError;
        const auto doc = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            continue;
        }
        const QJsonObject status = doc.object();
        const QString service =
            status.value(QStringLiteral("service")).toString(QStringLiteral("unknown"));
        if (service != QStringLiteral("sentinel-daemon")) {
            continue;
        }
        const QString health =
            status.value(QStringLiteral("ollamaHealth")).toString(QStringLiteral("unknown"));
        m_statusSummary = QStringLiteral("Daemon online — Ollama: %1").arg(health);
        emit statusReceived(status);
    }
}

} // namespace sentinel::desktop
