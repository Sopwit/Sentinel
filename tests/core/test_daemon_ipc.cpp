// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/app/ApplicationController.h"
#include "sentinel/core/chat/LocalEchoProvider.h"
#include "sentinel/core/memory/InMemoryStore.h"
#include "service/DaemonIpcServer.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>

namespace {

class DaemonIpcServerTest : public QObject {
    Q_OBJECT

private slots:
    void respondsToPing();
    void respondsToStatusWithControllerData();
    void rejectsUnknownCommand();
};

QJsonObject sendRequest(const QString& serverName, const QByteArray& payload) {
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (!socket.waitForConnected(2000)) {
        return {};
    }

    QEventLoop loop;
    QByteArray data;
    QObject::connect(&socket, &QLocalSocket::readyRead, &loop, [&socket, &data, &loop]() {
        data += socket.readAll();
        loop.quit();
    });
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);

    socket.write(payload);
    socket.flush();
    loop.exec();

    const auto doc = QJsonDocument::fromJson(data);
    return doc.isObject() ? doc.object() : QJsonObject{};
}

void DaemonIpcServerTest::respondsToPing() {
    sentinel::core::ApplicationController controller(
        std::make_unique<sentinel::core::LocalEchoProvider>(),
        std::make_unique<sentinel::core::InMemoryStore>());
    sentinel::daemon::DaemonIpcServer server(&controller);
    const QString serverName =
        QStringLiteral("sentinel-daemon-test-ping-%1").arg(QCoreApplication::applicationPid());
    QVERIFY(server.startServer(serverName));

    const QJsonObject root = sendRequest(serverName, "{\"command\":\"ping\"}\n");
    QVERIFY(!root.isEmpty());
    QCOMPARE(root.value("status").toString(), QStringLiteral("ok"));
    QCOMPARE(root.value("service").toString(), QStringLiteral("sentinel-daemon"));
    QVERIFY(root.value("pong").toBool());
}

void DaemonIpcServerTest::respondsToStatusWithControllerData() {
    sentinel::core::ApplicationController controller(
        std::make_unique<sentinel::core::LocalEchoProvider>(),
        std::make_unique<sentinel::core::InMemoryStore>());
    sentinel::daemon::DaemonIpcServer server(&controller);
    const QString serverName =
        QStringLiteral("sentinel-daemon-test-status-%1").arg(QCoreApplication::applicationPid());
    QVERIFY(server.startServer(serverName));

    const QJsonObject root = sendRequest(serverName, "{\"command\":\"status\"}\n");
    QVERIFY(!root.isEmpty());
    QCOMPARE(root.value("status").toString(), QStringLiteral("ok"));
    QVERIFY(root.value("controllerAvailable").toBool());
    QCOMPARE(root.value("providerName").toString(), QStringLiteral("LocalEchoProvider"));
    QVERIFY(root.contains("ollamaEndpoint"));
    QVERIFY(root.contains("ollamaHealth"));
    QVERIFY(root.contains("ollamaModelCount"));
}

void DaemonIpcServerTest::rejectsUnknownCommand() {
    sentinel::core::ApplicationController controller(
        std::make_unique<sentinel::core::LocalEchoProvider>(),
        std::make_unique<sentinel::core::InMemoryStore>());
    sentinel::daemon::DaemonIpcServer server(&controller);
    const QString serverName =
        QStringLiteral("sentinel-daemon-test-unknown-%1").arg(QCoreApplication::applicationPid());
    QVERIFY(server.startServer(serverName));

    const QJsonObject root = sendRequest(serverName, "{\"command\":\"bogus\"}\n");
    QVERIFY(!root.isEmpty());
    QCOMPARE(root.value("status").toString(), QStringLiteral("error"));
    QCOMPARE(root.value("error").toString(), QStringLiteral("unknown-command"));
}

} // namespace

QTEST_GUILESS_MAIN(DaemonIpcServerTest)

#include "test_daemon_ipc.moc"
