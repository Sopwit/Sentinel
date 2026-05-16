#include "sentinel/core/OllamaRuntime.h"

#include <QtTest>

using sentinel::core::NullOllamaRuntimeClient;
using sentinel::core::OllamaConfig;
using sentinel::core::OllamaConnectionStatus;
using sentinel::core::OllamaEndpoint;
using sentinel::core::OllamaHealthStatus;

class OllamaRuntimeTest final : public QObject {
    Q_OBJECT

private slots:
    void defaultEndpointIsLocalLoopback();
    void normalizesInvalidEndpointToSafeDefault();
    void acceptsLocalhostEndpointOnly();
    void nullClientIsDeterministicallyUnavailable();
};

void OllamaRuntimeTest::defaultEndpointIsLocalLoopback() {
    const auto endpoint = OllamaEndpoint::defaultEndpoint();

    QVERIFY(endpoint.valid);
    QVERIFY(endpoint.isLoopbackHttp());
    QCOMPARE(endpoint.toString(), QStringLiteral("http://127.0.0.1:11434"));
}

void OllamaRuntimeTest::normalizesInvalidEndpointToSafeDefault() {
    const auto cloudEndpoint = OllamaEndpoint::fromUserInput(QStringLiteral("https://example.com"));
    const auto fileEndpoint = OllamaEndpoint::fromUserInput(QStringLiteral("file:///tmp/socket"));

    QVERIFY(!cloudEndpoint.valid);
    QVERIFY(cloudEndpoint.normalizedFromInvalid);
    QCOMPARE(cloudEndpoint.toString(), QStringLiteral("http://127.0.0.1:11434"));

    QVERIFY(!fileEndpoint.valid);
    QVERIFY(fileEndpoint.normalizedFromInvalid);
    QCOMPARE(fileEndpoint.toString(), QStringLiteral("http://127.0.0.1:11434"));
}

void OllamaRuntimeTest::acceptsLocalhostEndpointOnly() {
    const auto endpoint = OllamaEndpoint::fromUserInput(QStringLiteral("http://localhost:11434/"));
    const auto endpointWithPath =
        OllamaEndpoint::fromUserInput(QStringLiteral("http://localhost:11434/api/generate"));
    const auto nonLoopback = OllamaEndpoint::fromUserInput(QStringLiteral("http://192.168.1.10"));

    QVERIFY(endpoint.valid);
    QVERIFY(endpoint.isLoopbackHttp());
    QCOMPARE(endpoint.toString(), QStringLiteral("http://localhost:11434"));
    QVERIFY(endpointWithPath.valid);
    QCOMPARE(endpointWithPath.toString(), QStringLiteral("http://localhost:11434"));

    QVERIFY(!nonLoopback.valid);
    QCOMPARE(nonLoopback.toString(), QStringLiteral("http://127.0.0.1:11434"));
}

void OllamaRuntimeTest::nullClientIsDeterministicallyUnavailable() {
    const NullOllamaRuntimeClient client{OllamaConfig::fromEndpoint(QStringLiteral("bad"))};
    const auto health = client.healthCheck();

    QCOMPARE(client.config().endpoint.toString(), QStringLiteral("http://127.0.0.1:11434"));
    QCOMPARE(health.connectionStatus, OllamaConnectionStatus::Unavailable);
    QCOMPARE(health.healthStatus, OllamaHealthStatus::Unavailable);
    QCOMPARE(health.endpoint, QStringLiteral("http://127.0.0.1:11434"));
    QVERIFY(health.summary.contains(QStringLiteral("no local health check")));
    QVERIFY(client.installedModels().isEmpty());
}

QTEST_MAIN(OllamaRuntimeTest)

#include "test_ollama_runtime.moc"
