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
    void parsesOllamaLibraryHtml();
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

void OllamaRuntimeTest::parsesOllamaLibraryHtml() {
    const QString sampleHtml = QStringLiteral(
        "<ul>"
        "  <li x-test-model class=\"flex items-baseline py-6\">"
        "    <a href=\"/library/test-model-1\" class=\"group\">"
        "      <div class=\"flex flex-col\">"
        "        <h2>"
        "          <span class=\"group-hover:underline truncate\">test-model-1</span>"
        "        </h2>"
        "        <p class=\"max-w-lg break-words text-neutral-800 text-md\">"
        "          This is a description of test-model-1."
        "        </p>"
        "      </div>"
        "      <div class=\"flex flex-col\">"
        "        <div class=\"flex flex-wrap\">"
        "          <span x-test-capability>tools</span>"
        "          <span x-test-capability>thinking</span>"
        "        </div>"
        "        <p class=\"my-4 text-neutral-500\">"
        "          <span>"
        "            <span x-test-pull-count>1,234</span> pulls"
        "          </span>"
        "          <span>"
        "            <span x-test-updated>2 days ago</span>"
        "          </span>"
        "        </p>"
        "      </div>"
        "    </a>"
        "  </li>"
        "</ul>");

    OllamaLibraryFetcher fetcher;
    fetcher.parseHtml(sampleHtml);

    const auto models = fetcher.models();
    QCOMPARE(models.size(), 1);

    const auto model = models.first().toMap();
    QCOMPARE(model.value(QStringLiteral("id")).toString(), QStringLiteral("test-model-1"));
    QCOMPARE(model.value(QStringLiteral("ollamaId")).toString(), QStringLiteral("test-model-1"));
    QCOMPARE(model.value(QStringLiteral("category")).toString(), QStringLiteral("Think"));
    QCOMPARE(model.value(QStringLiteral("name")).toString(), QStringLiteral("test-model-1"));
    QCOMPARE(model.value(QStringLiteral("provider")).toString(), QStringLiteral("Ollama Library"));
    QCOMPARE(model.value(QStringLiteral("size")).toString(), QStringLiteral("1,234 pulls"));
    QCOMPARE(model.value(QStringLiteral("description")).toString(),
             QStringLiteral("This is a description of test-model-1."));
    QCOMPARE(model.value(QStringLiteral("badge")).toString(), QStringLiteral("tools"));
    QCOMPARE(model.value(QStringLiteral("badgeColor")).toString(), QStringLiteral("#10b981"));

    const auto tags = model.value(QStringLiteral("tags")).toStringList();
    QVERIFY(tags.contains(QStringLiteral("tools")));
    QVERIFY(tags.contains(QStringLiteral("thinking")));
    QVERIFY(tags.contains(QStringLiteral("2 days ago")));
}

QTEST_MAIN(OllamaRuntimeTest)

#include "test_ollama_runtime.moc"
