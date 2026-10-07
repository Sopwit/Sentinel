// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "QtIpcContract.generated.h"

#include <QElapsedTimer>
#include <QHash>
#include <QJsonObject>
#include <QLocalSocket>
#include <QObject>
#include <QTimer>

namespace sentinel::desktop {

// Transport and contract validation only. Runtime policy remains in daemon/core.
class DaemonClient final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool daemonReachable READ daemonReachable NOTIFY daemonReachableChanged)
    Q_PROPERTY(QString statusSummary READ statusSummary NOTIFY statusChanged)
    Q_PROPERTY(ConnectionState connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString serverGeneration READ serverGeneration NOTIFY connectionStateChanged)
    Q_PROPERTY(quint64 connectionEpoch READ connectionEpoch NOTIFY connectionStateChanged)
    Q_PROPERTY(Error lastError READ lastError NOTIFY errorChanged)

public:
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting,
        VersionMismatch,
        Unavailable,
        ShuttingDown
    };
    Q_ENUM(ConnectionState)
    enum class Error {
        None,
        DaemonUnavailable,
        ProtocolMismatch,
        MalformedResponse,
        InvalidRequest,
        RequestTimeout,
        RequestCancelled,
        SessionNotFound,
        ProviderUnavailable,
        ModelUnavailable,
        PermissionDenied,
        ApprovalExpired,
        RunNotFound,
        RuntimeBusy,
        InternalDaemonError
    };
    Q_ENUM(Error)
    using Command = ipc::Command;

    explicit DaemonClient(QObject* parent = nullptr);
    DaemonClient(const QString& endpoint, int requestTimeoutMs, int reconnectMs,
                 QObject* parent = nullptr);
    ~DaemonClient() override;
    static QString defaultSocketPath();
    bool daemonReachable() const;
    QString statusSummary() const {
        return m_statusSummary;
    }
    ConnectionState connectionState() const {
        return m_state;
    }
    Error lastError() const {
        return m_lastError;
    }
    QString serverGeneration() const {
        return m_serverGeneration;
    }
    quint64 connectionEpoch() const {
        return m_epoch;
    }

    // Returns a unique correlation ID; local failures are delivered asynchronously.
    // Never replay mutations after timeout/reconnect: acceptance may be uncertain.
    QString request(Command command, const QJsonObject& payload = {});
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void connectToDaemon();
    Q_INVOKABLE void disconnectFromDaemon();

signals:
    void daemonReachableChanged();
    void connectionStateChanged();
    void statusChanged();
    void errorChanged();
    void statusReceived(const QJsonObject& status);
    void responseReceived(const QString& id, const QString& name, const QJsonObject& payload);
    void eventReceived(const QString& name, const QJsonObject& payload);
    void requestFailed(const QString& id, Error error, const QString& code);

private:
    struct Pending {
        QString name;
        qint64 deadline;
    };
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void consume(const QJsonObject& message);
    void setState(ConnectionState state, const QString& summary);
    void fail(const QString& id, Error error, const QString& code);
    void failPending(Error error, const QString& code);
    void protocolFailure();
    static Error mapError(const QString& code);
    QLocalSocket m_socket;
    QTimer m_reconnectTimer, m_timeoutTimer, m_connectionTimer;
    QElapsedTimer m_clock;
    QHash<QString, Pending> m_pending;
    QByteArray m_input;
    QString m_endpoint, m_helloId, m_serverGeneration;
    QString m_statusSummary = QStringLiteral("Daemon disconnected.");
    ConnectionState m_state = ConnectionState::Disconnected;
    Error m_lastError = Error::None;
    int m_requestTimeoutMs;
    quint64 m_epoch = 0;
    bool m_manualDisconnect = false;
    bool m_everConnected = false;
};
} // namespace sentinel::desktop
