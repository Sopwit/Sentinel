#include "DaemonIpcServer.h"

#include <QDebug>
#include <QLocalSocket>

namespace sentinel::daemon {

DaemonIpcServer::DaemonIpcServer(QObject* parent) : QObject(parent) {
    connect(&m_server, &QLocalServer::newConnection, this, &DaemonIpcServer::handleNewConnection);
}

DaemonIpcServer::~DaemonIpcServer() {
    stopServer();
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
        connect(socket, &QLocalSocket::readyRead, socket, [socket]() {
            const QByteArray data = socket->readAll();
            qInfo().noquote() << "Daemon received IPC request:" << QString::fromUtf8(data);

            const QByteArray response = "{\"status\":\"ok\",\"service\":\"sentinel-daemon\"}\n";
            socket->write(response);
            socket->flush();
            socket->disconnectFromServer();
        });
        connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
    }
}

} // namespace sentinel::daemon
