// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QString>
#include <functional>
#include <memory>

namespace sentinel::core {

struct LspRequest {
    int id;
    QString method;
    QJsonObject params;
    std::function<void(const QJsonObject&)> callback;
};

class LspClient : public QObject {
    Q_OBJECT
public:
    explicit LspClient(const QString& serverName, QObject* parent = nullptr);
    ~LspClient() override;

    // Connection
    bool start(const QString& command, const QStringList& arguments = {});
    bool stop();
    bool isRunning() const;

    // JSON-RPC operations
    QJsonObject sendRequest(const QString& method, const QJsonObject& params = {});
    void sendRequestAsync(const QString& method, const QJsonObject& params,
                          std::function<void(const QJsonObject&)> callback);
    void sendNotification(const QString& method, const QJsonObject& params = {});

    // LSP-specific operations
    void initialize(const QString& rootPath);
    void initialized();
    void shutdown();
    void exit();

    QString errorString() const;

signals:
    void started();
    void stopped();
    void error(const QString& error);
    void notificationReceived(const QString& method, const QJsonObject& params);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode);

private:
    void handleResponse(const QJsonObject& response);
    void handleNotification(const QJsonObject& notification);
    QByteArray buildMessage(const QByteArray& content) const;

    QString m_serverName;
    std::unique_ptr<QProcess> m_process;
    int m_nextRequestId{1};
    QMap<int, LspRequest> m_pendingRequests;
    QByteArray m_buffer;
    QString m_errorString;
};

} // namespace sentinel::core
