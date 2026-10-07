// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QDateTime>
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
class AppSettings;
struct AgentEvent;
struct ToolExecutionResult;
} // namespace sentinel::core
namespace sentinel::daemon {
class DaemonModelHelpers;
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
    bool openSessionProjectionStore(const QString& path);
    void setSettings(core::AppSettings* settings) {
        m_settings = settings;
    }

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
    QJsonObject session(const QString& id, bool presentation = false) const;
    void rememberRun();
    struct ChangeBaseline {
        QString root;
        QString workspaceId;
        QHash<QString, QString> contents;
        QSet<QString> listed;
        bool complete = false;
        bool truncated = false;
    };
    core::ToolExecutionResult inspectFile(const QString& sessionId, const QString& root,
                                          const QString& tool, const QString& path);
    void captureChangeBaseline(const QString& sessionId);
    QJsonObject reviewChanges(const QString& sessionId);
    QHash<QString, ChangeBaseline> m_changeBaselines;
    QString m_projectionConnection;
    QHash<QString, QJsonObject> m_lastRuns;
    QLocalServer m_server;
    core::ApplicationController* m_controller = nullptr;
    core::AppSettings* m_settings = nullptr;
    QHash<QLocalSocket*, Client> m_clients;
    std::unique_ptr<QLockFile> m_lock;
    QString m_path;
    QString m_generation;
    quint64 m_eventSequence = 0;
    QString m_agentSubscription;
    QString m_sessionId, m_runId, m_kind, m_approvalId, m_state, m_output;
    QString m_providerId, m_modelId;
    QJsonObject m_approvalPayload;
    QString m_agentTurnId;
    QSet<QString> m_subagentSessions;
    QHash<QString, QDateTime> m_toolStarts;
    bool m_outputTruncated = false;
    int m_chatCursor = 0;
    bool m_starting = false;
    bool m_shuttingDown = false;
    QElapsedTimer m_uptime;
    std::unique_ptr<DaemonModelHelpers> m_modelHelpers;
};
} // namespace sentinel::daemon
