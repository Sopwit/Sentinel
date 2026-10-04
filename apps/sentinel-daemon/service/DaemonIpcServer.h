// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QElapsedTimer>
#include <QHash>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QSet>
#include <memory>
namespace sentinel::core {
class ApplicationController;
struct AgentEvent;
} // namespace sentinel::core
namespace sentinel::daemon {
class DaemonIpcServer final : public QObject {
    Q_OBJECT
public:
    explicit DaemonIpcServer(core::ApplicationController* controller = nullptr,
                             QObject* parent = nullptr);
    ~DaemonIpcServer() override;
    static QString defaultSocketPath();
    bool startServer(const QString& serverName = {});
    void stopServer();
    void setController(core::ApplicationController* controller);

private:
    struct Client {
        QByteArray input;
        bool hello = false;
        QSet<QString> sessions;
    };
    void handleNewConnection();
    void handleRequest(const QJsonObject& message, QLocalSocket* socket);
    void send(QLocalSocket* socket, const QJsonObject& message);
    void error(QLocalSocket* socket, const QString& id, const QString& code);
    void publishEvent(const QString& name, const QJsonObject& payload = {});
    void onChatChanged();
    void onAgentEvent(const core::AgentEvent& event);
    QJsonObject session(const QString& id) const;
    QLocalServer m_server;
    core::ApplicationController* m_controller = nullptr;
    QHash<QLocalSocket*, Client> m_clients;
    std::unique_ptr<QLockFile> m_lock;
    QString m_path;
    QString m_agentSubscription;
    QString m_sessionId, m_runId, m_kind, m_approvalId, m_state, m_output;
    QString m_providerId, m_modelId;
    QJsonObject m_approvalPayload;
    bool m_outputTruncated = false;
    int m_chatCursor = 0;
    bool m_starting = false;
    bool m_shuttingDown = false;
    QElapsedTimer m_uptime;
};
} // namespace sentinel::daemon
