#ifndef SENTINEL_DAEMON_DAEMONIPCSERVER_H
#define SENTINEL_DAEMON_DAEMONIPCSERVER_H

#include <QLocalServer>
#include <QObject>

namespace sentinel::daemon {

class DaemonIpcServer final : public QObject {
    Q_OBJECT
public:
    explicit DaemonIpcServer(QObject* parent = nullptr);
    ~DaemonIpcServer() override;

    bool startServer(const QString& serverName = QStringLiteral("sentinel-daemon-ipc"));
    void stopServer();

private slots:
    void handleNewConnection();

private:
    QLocalServer m_server;
};

} // namespace sentinel::daemon

#endif // SENTINEL_DAEMON_DAEMONIPCSERVER_H
