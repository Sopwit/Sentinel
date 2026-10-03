// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/LocalInferenceWorkerFixture.h"
#include "sentinel/core/runtime/LocalInference.h"

#include <QElapsedTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
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
    void workerStreamsOrderedChunksOnceAndCleansUp();
    void workerRejectsConcurrentStreamWhileTerminalDeliveryIsQueued();
    void workerCancellationPropagatesToGatedStreamClient();
    void workerCompletesWithFakeClient();
    void workerReportsClientFailure();
    void workerRejectsConcurrentInference();
    void workerPropagatesCancellationToInferenceClient();
    void cloudEndpointWithoutKeyIsBlocked();
    void cloudOpenAiCompatibleRequestCarriesBearerKey();
    void completedReasoningOnlyIsInvalid();
    void zeroTimeoutCancelsOutstandingRequest();
};

void LocalInferenceTest::nullClientDeterministicallyRefuses() {
    NullLocalInferenceClient client;

    const auto response = client.infer(LocalInferenceRequest{
        QStringLiteral("request-1"),
        QStringLiteral("hello"),
        {.model = QStringLiteral("llama3.2"), .timeoutMs = 100},
    });

    QCOMPARE(response.status, LocalInferenceStatus::Refused);
    QCOMPARE(response.error, LocalInferenceError::ClientUnavailable);
    QVERIFY(response.summary.contains(QStringLiteral("unavailable")));
    QCOMPARE(response.traces.size(), 1);
}

void LocalInferenceTest::blankPromptRejectedBeforeOllamaCall() {
    OllamaLocalInferenceClient client{
        OllamaConfig::fromEndpoint(QStringLiteral("http://127.0.0.1:11434")), 1};

    const auto response = client.infer(LocalInferenceRequest{
        QStringLiteral("request-1"),
        QStringLiteral("   "),
        {.model = QStringLiteral("llama3.2"), .timeoutMs = 1},
    });

    QCOMPARE(response.status, LocalInferenceStatus::InvalidRequest);
    QCOMPARE(response.error, LocalInferenceError::BlankPrompt);
    QVERIFY(response.summary.contains(QStringLiteral("prompt is blank")));
}

void LocalInferenceTest::missingModelRejectedBeforeOllamaCall() {
    OllamaLocalInferenceClient client{
        OllamaConfig::fromEndpoint(QStringLiteral("http://127.0.0.1:11434")), 1};

    const auto response = client.infer(LocalInferenceRequest{
        QStringLiteral("request-1"),
        QStringLiteral("hello"),
        {.model = QString(), .timeoutMs = 1},
    });

    QCOMPARE(response.status, LocalInferenceStatus::InvalidRequest);
    QCOMPARE(response.error, LocalInferenceError::MissingModel);
    QVERIFY(response.summary.contains(QStringLiteral("model is required")));
}

void LocalInferenceTest::unavailableModelRejectedBeforeGeneration() {
    auto config = OllamaConfig::fromEndpoint(QStringLiteral("http://127.0.0.1:11434"));
    config.modelDiscoveryEnabled = false;
    OllamaLocalInferenceClient client{config, 1};
    sentinel::core::OllamaModelDiscoveryResult discovery;
    discovery.lifecycle = sentinel::core::ChatRequestLifecycle::Completed;
    discovery.models.append({QStringLiteral("installed-model")});

    const auto response = client.infer(LocalInferenceRequest{
        QStringLiteral("request-1"),
        QStringLiteral("hello"),
        {.model = QStringLiteral("__sentinel_missing_model__"),
         .modelValidation = discovery, .timeoutMs = 1},
    });

    QCOMPARE(response.status, LocalInferenceStatus::ModelUnavailable);
    QCOMPARE(response.error, LocalInferenceError::ModelUnavailable);
    QVERIFY(response.summary.contains(QStringLiteral("selected model is unavailable")));
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
        {.model = QStringLiteral("llama3.2"), .timeoutMs = 1},
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
            {.model = QStringLiteral("llama3.2"), .timeoutMs = 1,
             .streamingRequested = true},
        },
        {});

    QCOMPARE(result.status, LocalInferenceStreamStatus::Disabled);
    QCOMPARE(result.summary,
             QStringLiteral("Local inference streaming is disabled; no stream was opened."));
    QVERIFY(result.chunks.isEmpty());
    QCOMPARE(client.statusSummary(), QStringLiteral("Local inference streaming is disabled."));
    QVERIFY(!client.isAvailable());
}

void LocalInferenceTest::workerStreamsOrderedChunksOnceAndCleansUp() {
    using namespace sentinel::test;
    auto state = std::make_shared<StreamClientState>();
    state->chunks = {{1, QStringLiteral("hello "), false, false, {}},
                     {2, QStringLiteral("world"), true, false, {}}};
    state->result.status = LocalInferenceStreamStatus::Completed;
    state->result.accumulatedText = QStringLiteral("hello world");
    QObject context;
    sentinel::core::LocalInferenceWorker worker(
        nullptr, std::make_unique<GatedInferenceStreamClient>(state), &context, true, true);
    QStringList received;
    int finalCallbacks = 0;
    QString finalId;
    LocalInferenceStreamStatus finalStatus = LocalInferenceStreamStatus::NotStarted;
    LocalInferenceRequest request;
    request.id = QStringLiteral("stream-ordered-1");
    request.prompt = QStringLiteral("test prompt");
    request.options.model = QStringLiteral("test-model");

    QVERIFY(worker.startStream(
        request,
        [&received](const QString&, const sentinel::core::LocalInferenceStreamChunk& chunk) {
            received.append(chunk.text);
        },
        [&finalCallbacks, &finalId, &finalStatus](
            const QString& id, const sentinel::core::LocalInferenceStreamResult& result) {
            ++finalCallbacks;
            finalId = id;
            finalStatus = result.status;
        }));
    QTRY_VERIFY_WITH_TIMEOUT(state->entered.load(), 3000);
    state->entryRelease.release();
    state->chunkRelease.release(2);
    state->finalRelease.release();
    QTRY_COMPARE_WITH_TIMEOUT(finalCallbacks, 1, 3000);
    QCOMPARE(state->calls.load(), 1);
    QCOMPARE(state->requestId, request.id);
    QCOMPARE(state->chunkCallbacks.load(), 2);
    QCOMPARE(received, QStringList({QStringLiteral("hello "), QStringLiteral("world")}));
    QCOMPARE(finalId, request.id);
    QCOMPARE(finalStatus, LocalInferenceStreamStatus::Completed);
}

void LocalInferenceTest::workerRejectsConcurrentStreamWhileTerminalDeliveryIsQueued() {
    using namespace sentinel::test;
    auto state = std::make_shared<StreamClientState>();
    state->result.status = LocalInferenceStreamStatus::Completed;
    QObject context;
    sentinel::core::LocalInferenceWorker worker(
        nullptr, std::make_unique<GatedInferenceStreamClient>(state), &context, true, true);
    LocalInferenceRequest request;
    request.id = QStringLiteral("stream-gated-1");
    request.prompt = QStringLiteral("test prompt");
    request.options.model = QStringLiteral("test-model");
    int firstFinalCallbacks = 0;
    QVERIFY(worker.startStream(
        request, {},
        [&firstFinalCallbacks](const QString&, const sentinel::core::LocalInferenceStreamResult&) {
            ++firstFinalCallbacks;
        }));
    QTRY_VERIFY_WITH_TIMEOUT(state->entered.load(), 3000);
    state->entryRelease.release();
    state->finalRelease.release();
    // The worker thread may finish before its queued terminal callback runs.
    // It must still reserve the request slot until that callback releases it.
    QVERIFY(!worker.startStream(request, {}, {}));
    QCOMPARE(state->calls.load(), 1);
    QTRY_COMPARE_WITH_TIMEOUT(firstFinalCallbacks, 1, 3000);
    QVERIFY(worker.startStream(request, {}, {}));
    state->entryRelease.release();
    state->finalRelease.release();
}

void LocalInferenceTest::workerCancellationPropagatesToGatedStreamClient() {
    using namespace sentinel::test;
    auto state = std::make_shared<StreamClientState>();
    state->chunks = {{1, QStringLiteral("never delivered"), false, false, {}}};
    state->result.status = LocalInferenceStreamStatus::Completed;
    QObject context;
    sentinel::core::LocalInferenceWorker worker(
        nullptr, std::make_unique<GatedInferenceStreamClient>(state), &context, true, true);
    int finalCallbacks = 0;
    LocalInferenceStreamStatus finalStatus = LocalInferenceStreamStatus::NotStarted;
    LocalInferenceRequest request;
    request.id = QStringLiteral("stream-cancel-1");
    request.prompt = QStringLiteral("test prompt");
    request.options.model = QStringLiteral("test-model");
    QVERIFY(worker.startStream(
        request, {},
        [&finalCallbacks, &finalStatus](const QString&,
                                        const sentinel::core::LocalInferenceStreamResult& result) {
            ++finalCallbacks;
            finalStatus = result.status;
        }));
    QTRY_VERIFY_WITH_TIMEOUT(state->entered.load(), 3000);
    worker.cancel(request.id);
    state->entryRelease.release();
    state->chunkRelease.release();
    state->finalRelease.release();
    QTRY_COMPARE_WITH_TIMEOUT(finalCallbacks, 1, 3000);
    QVERIFY(state->cancelled.load());
    QCOMPARE(state->chunkCallbacks.load(), 0);
    QCOMPARE(finalStatus, LocalInferenceStreamStatus::Cancelled);
}

void LocalInferenceTest::workerCompletesWithFakeClient() {
    sentinel::test::LocalInferenceWorkerFixture fixture;
    QVERIFY(fixture.start());
    fixture.state->release.release();
    QTRY_COMPARE_WITH_TIMEOUT(fixture.callbacks, 1, 3000);
    QCOMPARE(fixture.state->calls.load(), 1);
    QCOMPARE(fixture.response.status, LocalInferenceStatus::Succeeded);
    QCOMPARE(fixture.response.text, QStringLiteral("worker result"));
    QCOMPARE(fixture.completedId, QStringLiteral("worker-1"));
    QVERIFY(fixture.state->exited.load());
}

void LocalInferenceTest::workerReportsClientFailure() {
    sentinel::test::LocalInferenceWorkerFixture fixture;
    fixture.state->outcome = LocalInferenceStatus::Error;
    QVERIFY(fixture.start());
    fixture.state->release.release();
    QTRY_COMPARE_WITH_TIMEOUT(fixture.callbacks, 1, 3000);
    QCOMPARE(fixture.response.status, LocalInferenceStatus::Error);
    QCOMPARE(fixture.response.error, LocalInferenceError::ClientUnavailable);
    QVERIFY(fixture.response.text.isEmpty());
}

void LocalInferenceTest::workerRejectsConcurrentInference() {
    sentinel::test::LocalInferenceWorkerFixture fixture;
    QVERIFY(fixture.start());
    QTRY_VERIFY_WITH_TIMEOUT(fixture.state->entered.load(), 3000);
    QVERIFY(!fixture.start(QStringLiteral("duplicate")));
    QCOMPARE(fixture.state->calls.load(), 1);
    fixture.state->release.release();
    QTRY_COMPARE_WITH_TIMEOUT(fixture.callbacks, 1, 3000);
    QVERIFY(fixture.start(QStringLiteral("worker-2")));
    fixture.state->release.release();
    QTRY_COMPARE_WITH_TIMEOUT(fixture.callbacks, 2, 3000);
    QCOMPARE(fixture.completedId, QStringLiteral("worker-2"));
}

void LocalInferenceTest::workerPropagatesCancellationToInferenceClient() {
    sentinel::test::LocalInferenceWorkerFixture fixture;
    QVERIFY(fixture.start());
    QTRY_VERIFY_WITH_TIMEOUT(fixture.state->entered.load(), 3000);
    fixture.worker->cancel(QStringLiteral("worker-1"));
    fixture.state->release.release();
    QTRY_COMPARE_WITH_TIMEOUT(fixture.callbacks, 1, 3000);
    QVERIFY(fixture.state->cancelled.load());
    QCOMPARE(fixture.response.status, LocalInferenceStatus::Succeeded);
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

void LocalInferenceTest::completedReasoningOnlyIsInvalid() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    connect(&server, &QTcpServer::newConnection, &server, [&] {
        auto* socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, socket, [socket] {
            socket->readAll();
            const QByteArray body =
                R"({"choices":[{"finish_reason":"stop","message":{"content":"","reasoning_content":"Inspect current state","tool_calls":[]}}],"usage":{"completion_tokens_details":{"reasoning_tokens":12}}})";
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " +
                          QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
            socket->disconnectFromHost();
        });
    });
    LMStudioConfig config;
    config.endpoint = QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort()));
    LMStudioLocalInferenceClient client(config);
    LocalInferenceRequest request;
    request.prompt = QStringLiteral("Classify");
    request.options.model = QStringLiteral("fixture");
    const auto response = client.infer(request);
    QCOMPARE(response.status, LocalInferenceStatus::Error);
    QCOMPARE(response.error, LocalInferenceError::InvalidResponse);
    QVERIFY(response.text.isEmpty());
}

void LocalInferenceTest::zeroTimeoutCancelsOutstandingRequest() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    auto token = std::make_shared<std::atomic_bool>(false);
    bool received = false;
    connect(&server, &QTcpServer::newConnection, &server, [&] {
        auto* socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
            socket->readAll();
            received = true;
            QTimer::singleShot(50, &server, [token] { token->store(true); });
        });
    });
    // Test watchdog, not a provider or production timeout.
    QTimer::singleShot(3000, &server, [token] { token->store(true); });
    LMStudioConfig config;
    config.endpoint = QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort()));
    LMStudioLocalInferenceClient client(config, 0);
    LocalInferenceRequest request;
    request.prompt = QStringLiteral("Classify");
    request.options.model = QStringLiteral("fixture");
    request.options.timeoutMs = 0;
    request.options.cancellationToken = token;
    QElapsedTimer elapsed;
    elapsed.start();
    const auto response = client.infer(request);
    QVERIFY(received);
    QVERIFY(elapsed.elapsed() < 1000);
    QCOMPARE(response.providerErrorCategory,
             static_cast<int>(sentinel::core::ChatProviderErrorCategory::Cancelled));
    QVERIFY(response.text.isEmpty());
}

#include "test_local_inference.moc"
