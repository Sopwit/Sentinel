// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/LocalInference.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

using sentinel::core::LMStudioConfig;
using sentinel::core::LMStudioLocalInferenceClient;
using sentinel::core::LocalInferenceError;
using sentinel::core::LocalInferenceRequest;
using sentinel::core::LocalInferenceStatus;
using sentinel::core::LocalInferenceStreamStatus;
using sentinel::core::NullLocalInferenceClient;
using sentinel::core::NullLocalInferenceStreamClient;
using sentinel::core::OllamaConfig;
using sentinel::core::OllamaLocalInferenceClient;

class LocalInferenceTest final : public QObject {
    Q_OBJECT

private slots:
    void nullClientDeterministicallyRefuses();
    void blankPromptRejectedBeforeOllamaCall();
    void missingModelRejectedBeforeOllamaCall();
    void unavailableModelRejectedBeforeGeneration();
    void invalidEndpointIsBlocked();
    void streamSkeletonIsDeterministicallyDisabled();
    void cloudEndpointWithoutKeyIsBlocked();
    void cloudOpenAiCompatibleRequestCarriesBearerKey();
};

void LocalInferenceTest::nullClientDeterministicallyRefuses() {
    NullLocalInferenceClient client;

    const auto response = client.infer(LocalInferenceRequest{
        QStringLiteral("request-1"),
        QStringLiteral("hello"),
        {QStringLiteral("llama3.2"), 100, false, false},
    });

    QCOMPARE(response.status, LocalInferenceStatus::Refused);
    QCOMPARE(response.error, LocalInferenceError::ClientUnavailable);
    QVERIFY(response.summary.contains(QStringLiteral("unavailable")));
    QCOMPARE(response.traces.size(), 1);
}

void LocalInferenceTest::blankPromptRejectedBeforeOllamaCall() {
    OllamaLocalInferenceClient client{OllamaConfig::fromEndpoint(QStringLiteral("bad")), 1};

    const auto response = client.infer(LocalInferenceRequest{
        QStringLiteral("request-1"),
        QStringLiteral("   "),
        {QStringLiteral("llama3.2"), 1, false, false},
    });

    QCOMPARE(response.status, LocalInferenceStatus::InvalidRequest);
    QCOMPARE(response.error, LocalInferenceError::BlankPrompt);
    QVERIFY(response.summary.contains(QStringLiteral("prompt is blank")));
}

void LocalInferenceTest::missingModelRejectedBeforeOllamaCall() {
    OllamaLocalInferenceClient client{OllamaConfig::fromEndpoint(QStringLiteral("bad")), 1};

    const auto response = client.infer(LocalInferenceRequest{
        QStringLiteral("request-1"),
        QStringLiteral("hello"),
        {QString(), 1, false, false},
    });

    QCOMPARE(response.status, LocalInferenceStatus::InvalidRequest);
    QCOMPARE(response.error, LocalInferenceError::MissingModel);
    QVERIFY(response.summary.contains(QStringLiteral("model is required")));
}

void LocalInferenceTest::unavailableModelRejectedBeforeGeneration() {
    auto config = OllamaConfig::fromEndpoint(QStringLiteral("http://127.0.0.1:11434"));
    config.modelDiscoveryEnabled = false;
    OllamaLocalInferenceClient client{config, 1};

    const auto response = client.infer(LocalInferenceRequest{
        QStringLiteral("request-1"),
        QStringLiteral("hello"),
        {QStringLiteral("__sentinel_missing_model__"), 1, false, false},
    });

    QCOMPARE(response.status, LocalInferenceStatus::ModelUnavailable);
    QCOMPARE(response.error, LocalInferenceError::ModelUnavailable);
    QVERIFY(response.summary.contains(QStringLiteral("model is not installed")));
}

void LocalInferenceTest::invalidEndpointIsBlocked() {
    OllamaConfig config;
    config.endpoint.url = QUrl(QStringLiteral("https://example.com"));
    config.endpoint.valid = false;
    config.endpoint.normalizedFromInvalid = false;
    OllamaLocalInferenceClient client{config, 1};

    const auto response = client.infer(LocalInferenceRequest{
        QStringLiteral("request-1"),
        QStringLiteral("hello"),
        {QStringLiteral("llama3.2"), 1, false, false},
    });

    QCOMPARE(response.status, LocalInferenceStatus::Blocked);
    QCOMPARE(response.error, LocalInferenceError::EndpointBlocked);
    QVERIFY(response.summary.contains(QStringLiteral("loopback HTTP")));
}

void LocalInferenceTest::streamSkeletonIsDeterministicallyDisabled() {
    NullLocalInferenceStreamClient client;

    const auto result = client.startStream(
        LocalInferenceRequest{
            QStringLiteral("stream-request-1"),
            QStringLiteral("hello"),
            {QStringLiteral("llama3.2"), 1, true, false},
        },
        {});

    QCOMPARE(result.status, LocalInferenceStreamStatus::Disabled);
    QCOMPARE(result.summary,
             QStringLiteral("Local inference streaming is disabled; no stream was opened."));
    QVERIFY(result.chunks.isEmpty());
    QCOMPARE(client.statusSummary(), QStringLiteral("Local inference streaming is disabled."));
    QVERIFY(!client.isAvailable());
}

QTEST_MAIN(LocalInferenceTest)

void LocalInferenceTest::cloudEndpointWithoutKeyIsBlocked() {
    // 127.0.0.2 is not treated as a local loopback endpoint, so it exercises
    // the cloud permission gate: an API key is mandatory.
    LMStudioConfig config;
    config.endpoint = QUrl(QStringLiteral("https://api.openai.com"));
    config.apiKey = QString();

    LMStudioLocalInferenceClient client(config, 1000);
    LocalInferenceRequest request;
    request.prompt = QStringLiteral("hello");
    request.options.model = QStringLiteral("gpt-4o-mini");

    const auto response = client.infer(request);
    QCOMPARE(response.status, LocalInferenceStatus::Blocked);
    QCOMPARE(response.error, LocalInferenceError::EndpointBlocked);
    QVERIFY(response.summary.contains(QStringLiteral("API key")));
}

void LocalInferenceTest::cloudOpenAiCompatibleRequestCarriesBearerKey() {
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        QSKIP("loopback TCP server is not available on this machine.");
    }
    const quint16 port = server.serverPort();

    // "localhost." (trailing dot) resolves to 127.0.0.1 but is not matched by
    // the loopback-host check, so the client takes the cloud request path.
    LMStudioConfig config;
    config.endpoint = QUrl(QStringLiteral("http://localhost.:%1").arg(port));
    config.apiKey = QStringLiteral("test-key-123");

    QString capturedAuth;
    QString capturedRequestLine;
    server.connect(&server, &QTcpServer::newConnection, &server, [&]() {
        QTcpSocket* socket = server.nextPendingConnection();
        socket->connect(socket, &QTcpSocket::readyRead, socket,
                        [socket, &capturedAuth, &capturedRequestLine]() {
                            const QByteArray data = socket->readAll();
                            const int headerEnd = data.indexOf("\r\n\r\n");
                            if (headerEnd < 0) {
                                return;
                            }
                            const QByteArray headers = data.left(headerEnd);
                            if (capturedRequestLine.isEmpty()) {
                                capturedRequestLine =
                                    QString::fromLatin1(headers.split('\r').first());
                            }
                            if (capturedAuth.isEmpty()) {
                                for (const auto& line : headers.split('\n')) {
                                    if (line.startsWith("Authorization:")) {
                                        capturedAuth = QString::fromLatin1(line.trimmed());
                                    }
                                }
                            }

                            const QByteArray body = QByteArrayLiteral(
                                "{\"choices\":[{\"message\":{\"role\":\"assistant\","
                                "\"content\":\"cloud says hi\"}}]}");
                            socket->write(QStringLiteral("HTTP/1.1 200 OK\r\n"
                                                         "Content-Type: application/json\r\n"
                                                         "Content-Length: %1\r\n"
                                                         "Connection: close\r\n\r\n")
                                              .arg(body.size())
                                              .toLatin1() +
                                          body);
                            socket->flush();
                        });
    });

    LMStudioLocalInferenceClient client(config, 5000);
    LocalInferenceRequest request;
    request.prompt = QStringLiteral("hello cloud");
    request.options.model = QStringLiteral("gpt-test");

    const auto response = client.infer(request);
    QCOMPARE(response.status, LocalInferenceStatus::Succeeded);
    QCOMPARE(response.text, QStringLiteral("cloud says hi"));
    QVERIFY(capturedAuth.contains(QStringLiteral("Bearer test-key-123")));
    QVERIFY(capturedRequestLine.contains(QStringLiteral("/v1/chat/completions")));
}

#include "test_local_inference.moc"
