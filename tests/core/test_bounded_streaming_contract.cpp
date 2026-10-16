// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef Q_MOC_RUN
#include "../support/ProviderHttpFixture.h"
#endif
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/chat/LocalEchoProvider.h"
#include "sentinel/core/chat/OllamaChatProvider.h"
#include "sentinel/core/interfaces/IChatProvider.h"
#include "sentinel/core/memory/InMemorySettingsStore.h"
#include "sentinel/core/model/ModelService.h"
#include "sentinel/core/network/NetworkPolicyService.h"
#include "sentinel/core/runtime/LocalInference.h"
#include "sentinel/core/security/CredentialStore.h"
#include <QJsonDocument>
#include <QtTest>

#include <atomic>
#include <memory>

using namespace sentinel::core;
using sentinel::test::ProviderHttpFixture;

namespace {

// Loopback-only Ollama provider with a caller-chosen provider-level timeout.
// A provider timeout of 0 proves the bounded deadline came from the request.
std::shared_ptr<IChatProvider> ollamaProvider(const ProviderHttpFixture& server,
                                              int providerTimeoutMs) {
    auto config = OllamaConfig::fromEndpoint(server.endpoint().toString());
    config.modelDiscoveryEnabled = false;
    auto provider = std::make_shared<OllamaChatProvider>(config, providerTimeoutMs);
    provider->setSelectedModel(QStringLiteral("fixture"));
    return provider;
}

ChatRequestOptions boundedOptions(int deadlineMs, bool requireBounded = true) {
    ChatRequestOptions options;
    options.deadlineMs = deadlineMs;
    options.requireBoundedStreaming = requireBounded;
    return options;
}

// Streaming provider that only implements the legacy entry point. It must be an
// explicit capability-negative for the bounded contract.
class LegacyStreamingProvider final : public IChatProvider {
public:
    using IChatProvider::sendMessageStreaming;
    QString name() const override {
        return QStringLiteral("legacy-streaming");
    }
    ChatProviderStatus status() const override {
        return ChatProviderStatus::Ready;
    }
    bool supportsStreaming() const override {
        return true;
    }
    ChatProviderReply sendMessage(const QString&) override {
        ChatProviderReply reply;
        reply.success = true;
        reply.message = QStringLiteral("legacy-final");
        reply.lifecycle = ChatRequestLifecycle::Completed;
        return reply;
    }
    ChatProviderReply sendMessageStreaming(const QString&,
                                           const std::function<void(const QString&)>& onDelta,
                                           const std::shared_ptr<std::atomic_bool>&) override {
        if (onDelta)
            onDelta(QStringLiteral("legacy-delta"));
        ChatProviderReply reply;
        reply.success = true;
        reply.message = QStringLiteral("legacy-delta");
        reply.lifecycle = ChatRequestLifecycle::Completed;
        return reply;
    }
};

} // namespace

class BoundedStreamingContractTest final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void boundedStreamingCompletesWithOrderedDeltas();
    void boundedStreamingDeadlineReachesTransport();
    void boundedStreamingTimeoutBeforeAnyDelta();
    void boundedStreamingTimeoutAfterPartialOutput();
    void boundedStreamingCancellationAfterFirstDelta();
    void boundedStreamingCancellationBeforeFirstDelta();
    void boundedStreamingMissingTerminalMarker();
    void boundedStreamingMalformedResponse();
    void boundedStreamingRecoversFromMalformedChunk();
    void boundedStreamingRejectsAbsentDeadline();
    void boundedStreamingRejectsStructuredAndNative();
    void transportRefusesBoundedRequestWithoutDeadline();
    void boundedStreamingRedactsUpstreamErrors();
    void legacyStreamingAndNonStreamingUnchanged();
    void unsupportedProviderRejectsBoundedStreaming();
    void selectedEndpointBoundedStreamingAndCapability();
};

void BoundedStreamingContractTest::initTestCase() {
    // Every request is loopback or rejected before transport. No OS credential store.
    NetworkPolicyService::instance().setMode(NetworkMode::LocalOnly);
}

void BoundedStreamingContractTest::boundedStreamingCompletesWithOrderedDeltas() {
    ProviderHttpFixture server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    server.respond = [](QTcpSocket* socket, const auto&) {
        ProviderHttpFixture::reply(
            socket,
            "{\"response\":\"alpha \"}\n{\"response\":\"beta \"}\n{\"response\":\"gamma\"}\n"
            "{\"done\":true}\n",
            200, "application/x-ndjson");
    };
    auto provider = ollamaProvider(server, 0);
    QVERIFY(provider->supportsStreaming());
    QVERIFY(provider->supportsBoundedStreaming());

    QString deltas;
    const auto reply = provider->sendMessageStreaming(
        "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; },
        std::shared_ptr<std::atomic_bool>{}, boundedOptions(1000));
    QVERIFY(reply.success);
    QCOMPARE(reply.lifecycle, ChatRequestLifecycle::Completed);
    QCOMPARE(reply.category, ChatProviderErrorCategory::None);
    // Delta order and complete accumulation match.
    QCOMPARE(deltas, QStringLiteral("alpha beta gamma"));
    QCOMPARE(reply.message, deltas);
    QCOMPARE(server.requests.size(), 1);
    QCOMPARE(server.requests.first().target, QByteArray("/api/generate"));
    QVERIFY(server.requests.first().body.value("stream").toBool());
    QCOMPARE(server.requests.first().body.value("model").toString(), QStringLiteral("fixture"));
}

void BoundedStreamingContractTest::boundedStreamingDeadlineReachesTransport() {
    ProviderHttpFixture server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    server.respond = [](QTcpSocket* socket, const auto&) {
        ProviderHttpFixture::reply(socket, "{\"response\":\"ok\"}\n{\"done\":true}\n", 200,
                                   "application/x-ndjson");
    };
    // Provider-level timeout is 0; only the request deadline can bound the transport.
    auto provider = ollamaProvider(server, 0);
    QString deltas;
    const auto reply = provider->sendMessageStreaming(
        "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; },
        std::shared_ptr<std::atomic_bool>{}, boundedOptions(700));
    QVERIFY(reply.success);
    QCOMPARE(deltas, QStringLiteral("ok"));
    // The resolved transport deadline equals the requested finite deadline.
    QCOMPARE(reply.diagnostics.value(QStringLiteral("deadline_ms")).toInt(), 700);
    QCOMPARE(reply.diagnostics.value(QStringLiteral("bounded_streaming")).toBool(), true);
    QVERIFY(!QJsonDocument(reply.diagnostics).toJson().contains("synthetic"));
}

void BoundedStreamingContractTest::boundedStreamingTimeoutBeforeAnyDelta() {
    ProviderHttpFixture server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    server.respond = [](QTcpSocket* socket, const auto&) {
        // Retain the connection with headers only; the request deadline must fire.
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/x-ndjson\r\n"
                      "Connection: close\r\n\r\n");
    };
    auto provider = ollamaProvider(server, 1000);
    QString deltas;
    const auto reply = provider->sendMessageStreaming(
        "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; },
        std::shared_ptr<std::atomic_bool>{}, boundedOptions(150));
    QVERIFY(!reply.success);
    QCOMPARE(reply.lifecycle, ChatRequestLifecycle::TimedOut);
    QCOMPARE(reply.category, ChatProviderErrorCategory::Timeout);
    QVERIFY(deltas.isEmpty());
    QCOMPARE(server.requests.size(), 1);
    QCOMPARE(reply.diagnostics.value(QStringLiteral("deadline_ms")).toInt(), 150);
}

void BoundedStreamingContractTest::boundedStreamingTimeoutAfterPartialOutput() {
    ProviderHttpFixture server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    server.respond = [](QTcpSocket* socket, const auto&) {
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/x-ndjson\r\n"
                      "Connection: close\r\n\r\n");
        socket->write("{\"response\":\"partial\"}\n");
        // Hold open without a terminal marker; the deadline must fire.
    };
    auto provider = ollamaProvider(server, 1000);
    QString deltas;
    const auto reply = provider->sendMessageStreaming(
        "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; },
        std::shared_ptr<std::atomic_bool>{}, boundedOptions(150));
    QVERIFY(!reply.success);
    QCOMPARE(reply.lifecycle, ChatRequestLifecycle::TimedOut);
    QCOMPARE(reply.category, ChatProviderErrorCategory::Timeout);
    // Bounded output was delivered, but the partial response is rejected as a whole.
    QCOMPARE(deltas, QStringLiteral("partial"));
    QVERIFY(reply.lifecycle != ChatRequestLifecycle::Completed);
}

void BoundedStreamingContractTest::boundedStreamingCancellationAfterFirstDelta() {
    ProviderHttpFixture server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    auto token = std::make_shared<std::atomic_bool>(false);
    server.respond = [](QTcpSocket* socket, const auto&) {
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/x-ndjson\r\n"
                      "Connection: close\r\n\r\n");
        // Late frames are already on the wire; the provider must fence them.
        socket->write("{\"response\":\"first\"}\n{\"response\":\"late-one\"}\n"
                      "{\"response\":\"late-two\"}\n");
    };
    auto provider = ollamaProvider(server, 1000);
    QString deltas;
    int deltaCount = 0;
    const auto reply = provider->sendMessageStreaming(
        "synthetic bounded prompt",
        [&](const QString& delta) {
            deltas += delta;
            ++deltaCount;
            if (delta == QStringLiteral("first"))
                token->store(true);
        },
        token, boundedOptions(1000));
    QCoreApplication::processEvents();
    QVERIFY(!reply.success);
    QCOMPARE(reply.lifecycle, ChatRequestLifecycle::Cancelled);
    QCOMPARE(reply.category, ChatProviderErrorCategory::Cancelled);
    QCOMPARE(deltas, QStringLiteral("first"));
    QCOMPARE(deltaCount, 1);
}

void BoundedStreamingContractTest::boundedStreamingCancellationBeforeFirstDelta() {
    ProviderHttpFixture server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    auto token = std::make_shared<std::atomic_bool>(false);
    server.respond = [&](QTcpSocket* socket, const auto&) {
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/x-ndjson\r\n"
                      "Connection: close\r\n\r\n");
        // Arrives after the request but before any delta.
        token->store(true);
    };
    auto provider = ollamaProvider(server, 1000);
    QString deltas;
    const auto reply = provider->sendMessageStreaming(
        "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; }, token,
        boundedOptions(1000));
    QVERIFY(!reply.success);
    QCOMPARE(reply.lifecycle, ChatRequestLifecycle::Cancelled);
    QCOMPARE(reply.category, ChatProviderErrorCategory::Cancelled);
    QVERIFY(deltas.isEmpty());
    QCOMPARE(server.requests.size(), 1);

    // A token cancelled before dispatch opens no transport at all.
    auto preCancelled = std::make_shared<std::atomic_bool>(true);
    const auto early = provider->sendMessageStreaming(
        "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; }, preCancelled,
        boundedOptions(1000));
    QVERIFY(!early.success);
    QCOMPARE(early.lifecycle, ChatRequestLifecycle::Cancelled);
    QCOMPARE(server.requests.size(), 1);
    QVERIFY(deltas.isEmpty());
}

void BoundedStreamingContractTest::boundedStreamingMissingTerminalMarker() {
    ProviderHttpFixture server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    server.respond = [](QTcpSocket* socket, const auto&) {
        ProviderHttpFixture::reply(socket, "{\"response\":\"half\"}\n", 200,
                                   "application/x-ndjson");
    };
    auto provider = ollamaProvider(server, 1000);
    QString deltas;
    const auto reply = provider->sendMessageStreaming(
        "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; },
        std::shared_ptr<std::atomic_bool>{}, boundedOptions(1000));
    QVERIFY(!reply.success);
    QCOMPARE(reply.lifecycle, ChatRequestLifecycle::Failed);
    QCOMPARE(reply.category, ChatProviderErrorCategory::MalformedResponse);
    // Partial output was delivered as a delta but is not accepted as a final response.
    QCOMPARE(deltas, QStringLiteral("half"));
    QVERIFY(reply.lifecycle != ChatRequestLifecycle::Completed);
}

void BoundedStreamingContractTest::boundedStreamingMalformedResponse() {
    ProviderHttpFixture server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    server.respond = [](QTcpSocket* socket, const auto&) {
        ProviderHttpFixture::reply(socket, "{this-is-not-json\n", 200, "application/x-ndjson");
    };
    auto provider = ollamaProvider(server, 1000);
    QString deltas;
    const auto reply = provider->sendMessageStreaming(
        "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; },
        std::shared_ptr<std::atomic_bool>{}, boundedOptions(1000));
    QVERIFY(!reply.success);
    QCOMPARE(reply.category, ChatProviderErrorCategory::MalformedResponse);
    QVERIFY(deltas.isEmpty());
}

void BoundedStreamingContractTest::boundedStreamingRecoversFromMalformedChunk() {
    ProviderHttpFixture server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    server.respond = [](QTcpSocket* socket, const auto&) {
        ProviderHttpFixture::reply(socket, "{broken\n{\"response\":\"ok\"}\n{\"done\":true}\n", 200,
                                   "application/x-ndjson");
    };
    auto provider = ollamaProvider(server, 1000);
    QString deltas;
    const auto reply = provider->sendMessageStreaming(
        "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; },
        std::shared_ptr<std::atomic_bool>{}, boundedOptions(1000));
    QVERIFY(reply.success);
    QCOMPARE(reply.lifecycle, ChatRequestLifecycle::Completed);
    // Malformed frames are never delivered as deltas and do not corrupt order.
    QCOMPARE(deltas, QStringLiteral("ok"));
    QCOMPARE(reply.message, QStringLiteral("ok"));
}

void BoundedStreamingContractTest::boundedStreamingRejectsAbsentDeadline() {
    ProviderHttpFixture server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    server.respond = [](QTcpSocket* socket, const auto&) {
        ProviderHttpFixture::reply(socket, "{\"done\":true}\n", 200, "application/x-ndjson");
    };
    // A finite provider-level timeout must not silently satisfy a bounded request
    // that carries no deadline.
    auto provider = ollamaProvider(server, 500);
    QString deltas;
    const auto reply = provider->sendMessageStreaming(
        "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; },
        std::shared_ptr<std::atomic_bool>{}, boundedOptions(0));
    QVERIFY(!reply.success);
    QCOMPARE(reply.category, ChatProviderErrorCategory::RequestRejected);
    QCOMPARE(reply.lifecycle, ChatRequestLifecycle::Failed);
    QCOMPARE(reply.diagnostics.value(QStringLiteral("deadline_ms")).toInt(), 0);
    QCOMPARE(server.requests.size(), 0);
    QVERIFY(deltas.isEmpty());
}

void BoundedStreamingContractTest::boundedStreamingRejectsStructuredAndNative() {
    ProviderHttpFixture server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    auto provider = ollamaProvider(server, 1000);
    for (int mode = 0; mode < 2; ++mode) {
        auto options = boundedOptions(1000);
        options.structuredOutput = mode == 0;
        options.nativeToolCalling = mode == 1;
        QString deltas;
        const auto reply = provider->sendMessageStreaming(
            "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; },
            std::shared_ptr<std::atomic_bool>{}, options);
        QVERIFY(!reply.success);
        QCOMPARE(reply.category, ChatProviderErrorCategory::CapabilityUnsupported);
        QCOMPARE(reply.error, ChatProviderReply::Error::CapabilityRejected);
        QVERIFY(deltas.isEmpty());
    }
    QCOMPARE(server.requests.size(), 0);
}

void BoundedStreamingContractTest::transportRefusesBoundedRequestWithoutDeadline() {
    ProviderHttpFixture server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    server.respond = [](QTcpSocket* socket, const auto&) {
        ProviderHttpFixture::reply(socket, "{\"response\":\"ok\"}\n{\"done\":true}\n", 200,
                                   "application/x-ndjson");
    };
    auto ollamaConfig = OllamaConfig::fromEndpoint(server.endpoint().toString());
    ollamaConfig.modelDiscoveryEnabled = false;
    LocalInferenceRequest request;
    request.prompt = QStringLiteral("synthetic bounded prompt");
    request.options.model = QStringLiteral("fixture");
    request.options.streamingRequested = true;
    request.options.timeoutMs = 0;
    request.options.requireBoundedStreaming = true;
    const auto ollama = OllamaLocalInferenceStreamClient(ollamaConfig, 0).startStream(request, {});
    QCOMPARE(ollama.status, LocalInferenceStreamStatus::Refused);
    QCOMPARE(ollama.providerErrorCategory, int(ChatProviderErrorCategory::RequestRejected));
    QVERIFY(ollama.accumulatedText.isEmpty());

    LMStudioConfig lmConfig;
    lmConfig.endpoint = server.endpoint();
    const auto lm = LMStudioLocalInferenceStreamClient(lmConfig, 0).startStream(request, {});
    QCOMPARE(lm.status, LocalInferenceStreamStatus::Refused);
    QCOMPARE(lm.providerErrorCategory, int(ChatProviderErrorCategory::RequestRejected));
    // A bounded request without a deadline never reaches the transport.
    QCOMPARE(server.requests.size(), 0);
}

void BoundedStreamingContractTest::boundedStreamingRedactsUpstreamErrors() {
    const QByteArray marker = "PRIVATE_PLANNER_MARKER";
    for (int status : {200, 500}) {
        ProviderHttpFixture server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        server.respond = [=](QTcpSocket* socket, const auto&) {
            ProviderHttpFixture::reply(socket, "{\"error\":{\"message\":\"" + marker + "\"}}\n",
                                       status, "application/x-ndjson");
        };
        auto provider = ollamaProvider(server, 1000);
        QString deltas;
        const auto reply = provider->sendMessageStreaming(
            "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; },
            std::shared_ptr<std::atomic_bool>{}, boundedOptions(1000));
        QVERIFY(!reply.success);
        QVERIFY(!reply.errorMessage.contains("PRIVATE_"));
        QVERIFY(!QJsonDocument(reply.diagnostics).toJson().contains("PRIVATE_"));
        QVERIFY(!QJsonDocument(reply.diagnostics).toJson().contains(marker));
        QVERIFY(deltas.isEmpty());
    }
}

void BoundedStreamingContractTest::legacyStreamingAndNonStreamingUnchanged() {
    // Legacy streaming entry point keeps using the provider-level timeout.
    ProviderHttpFixture streamingServer;
    QVERIFY(streamingServer.listen(QHostAddress::LocalHost, 0));
    streamingServer.respond = [](QTcpSocket* socket, const auto&) {
        ProviderHttpFixture::reply(socket, "{\"response\":\"legacy-ok\"}\n{\"done\":true}\n", 200,
                                   "application/x-ndjson");
    };
    auto streamingProvider = ollamaProvider(streamingServer, 1000);
    QString deltas;
    const auto legacy = streamingProvider->sendMessageStreaming(
        "synthetic legacy prompt", [&](const QString& delta) { deltas += delta; },
        std::shared_ptr<std::atomic_bool>{});
    QVERIFY(legacy.success);
    QCOMPARE(legacy.lifecycle, ChatRequestLifecycle::Completed);
    QCOMPARE(deltas, QStringLiteral("legacy-ok"));
    QCOMPARE(streamingServer.requests.size(), 1);

    // Non-streaming requests keep the existing deadline-bearing path.
    ProviderHttpFixture generateServer;
    QVERIFY(generateServer.listen(QHostAddress::LocalHost, 0));
    generateServer.respond = [](QTcpSocket* socket, const auto&) {
        ProviderHttpFixture::reply(socket, R"({"response":"sync-ok"})");
    };
    auto generateProvider = ollamaProvider(generateServer, 1000);
    ChatRequestOptions options;
    options.deadlineMs = 1000;
    const auto sync = generateProvider->sendRequest("synthetic sync prompt", options);
    QVERIFY(sync.success);
    QCOMPARE(sync.message, QStringLiteral("sync-ok"));
    QCOMPARE(sync.lifecycle, ChatRequestLifecycle::Completed);
    QVERIFY(!generateServer.requests.first().body.value("stream").toBool());
}

void BoundedStreamingContractTest::unsupportedProviderRejectsBoundedStreaming() {
    LegacyStreamingProvider provider;
    QVERIFY(provider.supportsStreaming());
    QVERIFY(!provider.supportsBoundedStreaming());
    QString deltas;
    const auto reply = provider.sendMessageStreaming(
        "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; },
        std::shared_ptr<std::atomic_bool>{}, boundedOptions(1000));
    QVERIFY(!reply.success);
    QCOMPARE(reply.category, ChatProviderErrorCategory::CapabilityUnsupported);
    QCOMPARE(reply.error, ChatProviderReply::Error::CapabilityRejected);
    QCOMPARE(reply.lifecycle, ChatRequestLifecycle::Failed);
    QVERIFY(deltas.isEmpty());

    // The legacy contract on the same provider is preserved.
    const auto legacy = provider.sendMessageStreaming(
        "synthetic legacy prompt", [&](const QString& delta) { deltas += delta; },
        std::shared_ptr<std::atomic_bool>{});
    QVERIFY(legacy.success);
    QCOMPARE(deltas, QStringLiteral("legacy-delta"));

    // The default provider is a capability-negative.
    LocalEchoProvider echo;
    QVERIFY(!echo.supportsBoundedStreaming());
}

void BoundedStreamingContractTest::selectedEndpointBoundedStreamingAndCapability() {
    ProviderHttpFixture server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    server.respond = [](QTcpSocket* socket, const auto&) {
        ProviderHttpFixture::reply(socket,
                                   "data: {\"choices\":[{\"delta\":{\"content\":\"a \"}}]}\n\n"
                                   "data: {\"choices\":[{\"delta\":{\"content\":\"b \"}}]}\n\n"
                                   "data: {\"choices\":[{\"delta\":{\"content\":\"c\"}}]}\n\n"
                                   "data: [DONE]\n\n",
                                   200, "text/event-stream");
    };
    AppSettings settings(std::make_unique<InMemorySettingsStore>(), inMemoryTestCredentialStore());
    settings.setNetworkMode(QStringLiteral("LocalOnly"));
    ModelService service(&settings);
    service.setLmStudioEndpoint(server.endpoint().toString());

    ModelBinding supported{"lm-studio", "fixture"};
    supported.capabilities.streaming = CapabilitySupport::Supported;
    auto provider = service.constructProvider(supported);
    QVERIFY(provider);
    QVERIFY(provider->supportsBoundedStreaming());
    QString deltas;
    const auto reply = provider->sendMessageStreaming(
        "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; },
        std::shared_ptr<std::atomic_bool>{}, boundedOptions(1000));
    QVERIFY(reply.success);
    QCOMPARE(reply.lifecycle, ChatRequestLifecycle::Completed);
    QCOMPARE(deltas, QStringLiteral("a b c"));
    QCOMPARE(reply.diagnostics.value(QStringLiteral("deadline_ms")).toInt(), 1000);
    QCOMPARE(server.requests.size(), 1);

    // A binding without streaming capability is an explicit capability-negative.
    ModelBinding unsupported{"lm-studio", "fixture"};
    unsupported.capabilities.streaming = CapabilitySupport::Unsupported;
    auto offProvider = service.constructProvider(unsupported);
    QVERIFY(offProvider);
    QVERIFY(!offProvider->supportsBoundedStreaming());
    deltas.clear();
    const auto off = offProvider->sendMessageStreaming(
        "synthetic bounded prompt", [&](const QString& delta) { deltas += delta; },
        std::shared_ptr<std::atomic_bool>{}, boundedOptions(1000));
    QVERIFY(!off.success);
    QCOMPARE(off.category, ChatProviderErrorCategory::CapabilityUnsupported);
    QCOMPARE(server.requests.size(), 1);
}

QTEST_GUILESS_MAIN(BoundedStreamingContractTest)
#include "test_bounded_streaming_contract.moc"
