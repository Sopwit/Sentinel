// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QString>
#include <functional>
#include <memory>

namespace sentinel::core {

struct McpRequest {
    int id;
    QString method;
    QJsonObject params;
    std::function<void(const QJsonObject&)> callback;
};

class McpClient : public QObject {
    Q_OBJECT
public:
    explicit McpClient(QObject* parent = nullptr);
    ~McpClient() override;

    // Local server (stdio) connection
    bool connectLocal(const QString& command, const QStringList& arguments = {});
    bool disconnectLocal();

    // Remote server (HTTP) connection
    bool connectRemote(const QString& url, const QJsonObject& headers = {});
    bool disconnectRemote();

    // JSON-RPC operations
    QJsonObject sendRequest(const QString& method, const QJsonObject& params = {});
    void sendRequestAsync(const QString& method, const QJsonObject& params,
                          std::function<void(const QJsonObject&)> callback);

    bool isConnected() const;
    QString errorString() const;

signals:
    void connectedToServer();
    void disconnectedFromServer();
    void clientError(const QString& error);
    void notificationReceived(const QString& method, const QJsonObject& params);

private slots:
    void onLocalProcessReadyRead();
    void onLocalProcessFinished(int exitCode);

private:
    void handleResponse(const QJsonObject& response);
    void handleNotification(const QJsonObject& notification);

    enum class TransportType { None, Local, Remote };
    TransportType m_transportType{TransportType::None};

    // Local transport (stdio)
    std::unique_ptr<QProcess> m_process;

    // Remote transport (HTTP)
    QNetworkAccessManager* m_networkManager{nullptr};
    QString m_remoteUrl;
    QJsonObject m_remoteHeaders;

    // JSON-RPC state
    int m_nextRequestId{1};
    QMap<int, McpRequest> m_pendingRequests;
    QByteArray m_buffer;

    QString m_errorString;
};

} // namespace sentinel::core
