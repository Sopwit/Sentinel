#include "sentinel/core/LocalInference.h"

#include <QtTest>

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

#include "test_local_inference.moc"
