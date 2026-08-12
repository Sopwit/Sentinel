// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QObject>

class QLocalSocket;
class QTimer;

namespace sentinel::desktop {

// Lightweight client for the headless sentinel-daemon IPC service. Used by the
// desktop shell to surface background daemon status and to keep a real
// connection alive instead of a hardcoded placeholder.
class DaemonClient final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool daemonReachable READ daemonReachable NOTIFY daemonReachableChanged)
    Q_PROPERTY(QString statusSummary READ statusSummary NOTIFY statusReceived)

public:
    explicit DaemonClient(QObject* parent = nullptr);
    ~DaemonClient() override;

    bool daemonReachable() const;
    QString statusSummary() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void connectToDaemon();

signals:
    void daemonReachableChanged();
    void statusReceived(const QJsonObject& status);

private:
    void onConnected();
    void onDisconnected();
    void onReadyRead();

    QLocalSocket* m_socket = nullptr;
    QTimer* m_reconnectTimer = nullptr;
    QString m_serverName = QStringLiteral("sentinel-daemon-ipc");
    bool m_daemonReachable = false;
    QString m_statusSummary = QStringLiteral("Daemon not reachable.");
};

} // namespace sentinel::desktop
