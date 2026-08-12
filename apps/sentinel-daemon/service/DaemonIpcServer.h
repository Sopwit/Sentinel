// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SENTINEL_DAEMON_DAEMONIPCSERVER_H
#define SENTINEL_DAEMON_DAEMONIPCSERVER_H

#include <QLocalServer>
#include <QObject>

namespace sentinel::core {
class ApplicationController;
}

namespace sentinel::daemon {

class DaemonIpcServer final : public QObject {
    Q_OBJECT
public:
    explicit DaemonIpcServer(sentinel::core::ApplicationController* controller = nullptr,
                             QObject* parent = nullptr);
    ~DaemonIpcServer() override;

    bool startServer(const QString& serverName = QStringLiteral("sentinel-daemon-ipc"));
    void stopServer();

    void setController(sentinel::core::ApplicationController* controller);

private slots:
    void handleNewConnection();

private:
    void handleRequest(sentinel::core::ApplicationController* controller, const QByteArray& data,
                       QLocalSocket* socket);
    QByteArray buildStatusResponse(sentinel::core::ApplicationController* controller) const;

    QLocalServer m_server;
    sentinel::core::ApplicationController* m_controller = nullptr;
};

} // namespace sentinel::daemon

#endif // SENTINEL_DAEMON_DAEMONIPCSERVER_H
