// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <functional>

namespace sentinel::test {
// Disposable loopback HTTP fixture. Dispatch only after the complete request body;
// callbacks never depend on TCP packet boundaries. No fixed ports or external hosts.
class ProviderHttpFixture final : public QTcpServer {
public:
    struct Request {
        QByteArray target;
        QByteArray headers;
        QJsonObject body;
    };
    QList<Request> requests;
    std::function<void(QTcpSocket*, const Request&)> respond;

    ProviderHttpFixture() {
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (hasPendingConnections()) {
                auto* socket = nextPendingConnection();
                connect(socket, &QTcpSocket::readyRead, socket,
                        [this, socket, bytes = QByteArray{}, handled = false]() mutable {
                            if (handled)
                                return;
                            bytes += socket->readAll();
                            const auto split = bytes.indexOf("\r\n\r\n");
                            if (split < 0)
                                return;
                            const auto headers = bytes.left(split);
                            int length = 0;
                            for (const auto& line : headers.split('\n'))
                                if (line.toLower().startsWith("content-length:"))
                                    length = line.mid(15).trimmed().toInt();
                            if (bytes.size() < split + 4 + length)
                                return;
                            handled = true;
                            Request request{
                                headers.split('\n').first().split(' ').value(1), headers,
                                QJsonDocument::fromJson(bytes.mid(split + 4, length)).object()};
                            requests.append(request);
                            if (respond)
                                respond(socket, request);
                        });
            }
        });
    }
    QUrl endpoint() const {
        return QUrl(QStringLiteral("http://127.0.0.1:%1").arg(serverPort()));
    }
    static void reply(QTcpSocket* socket, const QByteArray& body, int status = 200,
                      const QByteArray& type = "application/json") {
        socket->write("HTTP/1.1 " + QByteArray::number(status) + " Fixture\r\nContent-Type: " +
                      type + "\r\nContent-Length: " + QByteArray::number(body.size()) +
                      "\r\nConnection: close\r\n\r\n" + body);
        socket->disconnectFromHost();
    }
};
} // namespace sentinel::test
