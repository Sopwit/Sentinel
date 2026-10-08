// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/model/HuggingFaceModelSource.h"
#include "sentinel/core/model/ModelCategory.h"
#include "sentinel/core/network/NetworkPolicyService.h"
#include "sentinel/core/runtime/OllamaRuntime.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QTcpServer>
#include <QTcpSocket>
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
    void taskCategoriesAndMixedCapabilities();
    void parsesLmStudioCloudAndDownloadMetadata();
    void explicitRefreshBypassesFreshCache();
    void llamaCppLoadedTemplateCapabilities_data();
    void llamaCppLoadedTemplateCapabilities();
};

void OllamaRuntimeTest::llamaCppLoadedTemplateCapabilities_data() {
    QTest::addColumn<QString>("alias");
    QTest::addColumn<QJsonObject>("caps");
    QTest::addColumn<int>("modelCount");
    QTest::addColumn<int>("propsStatus");
    QTest::addColumn<int>("expected");
    using sentinel::core::CapabilitySupport;
    const QJsonObject supported{{"supports_tools", true}, {"supports_tool_calls", true}};
    QTest::newRow("matching-loaded-model")
        << QString("loaded") << supported << 1 << 200 << int(CapabilitySupport::Supported);
    QTest::newRow("wrong-identity")
        << QString("other") << supported << 1 << 200 << int(CapabilitySupport::Unknown);
    QTest::newRow("router-catalog")
        << QString("loaded") << supported << 2 << 200 << int(CapabilitySupport::Unknown);
    QTest::newRow("unsupported-template")
        << QString("loaded")
        << QJsonObject{{"supports_tools", true}, {"supports_tool_calls", false}} << 1 << 200
        << int(CapabilitySupport::Unsupported);
    QTest::newRow("missing-metadata")
        << QString("loaded") << QJsonObject{} << 1 << 200 << int(CapabilitySupport::Unknown);
    QTest::newRow("props-unavailable")
        << QString("loaded") << supported << 1 << 404 << int(CapabilitySupport::Unknown);
}

void OllamaRuntimeTest::llamaCppLoadedTemplateCapabilities() {
    QFETCH(QString, alias);
    QFETCH(QJsonObject, caps);
    QFETCH(int, modelCount);
    QFETCH(int, propsStatus);
    QFETCH(int, expected);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    int propsRequests = 0;
    connect(&server, &QTcpServer::newConnection, &server, [&] {
        auto* socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
            const auto request = socket->readAll();
            const bool props = request.startsWith("GET /props ");
            QJsonArray models{QJsonObject{{"id", "loaded"}}};
            if (modelCount > 1)
                models.append(QJsonObject{{"id", "other"}});
            if (props)
                ++propsRequests;
            const QJsonObject body =
                props ? QJsonObject{{"model_alias", alias},
                                    {"chat_template_caps", caps},
                                    {"default_generation_settings", QJsonObject{{"n_ctx", 8192}}}}
                      : QJsonObject{{"data", models}};
            const auto bytes = QJsonDocument(body).toJson(QJsonDocument::Compact);
            socket->write(QByteArray("HTTP/1.1 ") + QByteArray::number(props ? propsStatus : 200) +
                          " OK\r\nContent-Type: application/json\r\nContent-Length: " +
                          QByteArray::number(bytes.size()) + "\r\nConnection: close\r\n\r\n" +
                          bytes);
            socket->disconnectFromHost();
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    });
    sentinel::core::ProviderDiscoveryOutcome outcome;
    const auto models = sentinel::core::fetchLlamaCppModels(
        QUrl(QString("http://127.0.0.1:%1/v1/models").arg(server.serverPort())), 1000, {},
        &outcome);
    QVERIFY(outcome.completed);
    QCOMPARE(models.size(), modelCount);
    QCOMPARE(int(models.front().capabilities.nativeToolCalling), expected);
    QCOMPARE(propsRequests, modelCount == 1 ? 1 : 0);
    if (alias == "loaded" && modelCount == 1 && propsStatus == 200)
        QCOMPARE(models.front().capabilities.contextWindow.value_or(0), 8192);
}

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
    QCOMPARE(model.value(QStringLiteral("size")).toString(), QStringLiteral("—"));
    QCOMPARE(model.value(QStringLiteral("popularity")).toString(), QStringLiteral("1,234 pulls"));
    QCOMPARE(model.value(QStringLiteral("description")).toString(),
             QStringLiteral("This is a description of test-model-1."));
    QCOMPARE(model.value(QStringLiteral("badge")).toString(), QStringLiteral("tools"));
    QCOMPARE(model.value(QStringLiteral("badgeColor")).toString(), QStringLiteral("#10b981"));

    const auto tags = model.value(QStringLiteral("tags")).toStringList();
    QVERIFY(tags.contains(QStringLiteral("tools")));
    QVERIFY(tags.contains(QStringLiteral("thinking")));
    QVERIFY(tags.contains(QStringLiteral("2 days ago")));
}

void OllamaRuntimeTest::taskCategoriesAndMixedCapabilities() {
    using sentinel::core::modelCategory;
    QCOMPARE(modelCategory("phi4", {}), QString("LLM"));
    QCOMPARE(modelCategory("encoder", {"feature-extraction", "vision"}), QString("Embedding"));
    QCOMPARE(modelCategory("mixed", {"vision", "thinking"}), QString("Vision"));
    QCOMPARE(modelCategory("video", {"image-to-video"}), QString("Video"));
    QCOMPARE(modelCategory("speech", {"text-to-speech"}), QString("TTS"));
    QCOMPARE(modelCategory("speech", {"automatic-speech-recognition"}), QString("STT"));
}
void OllamaRuntimeTest::parsesLmStudioCloudAndDownloadMetadata() {
    LMStudioLibraryFetcher fetcher;
    fetcher.parseHtml(QStringLiteral(
        "<a href=\"/models/cloud-model\"><div class=\"text-lg font-medium\">Cloud model</div>"
        "<div class=\"text-muted-foreground\">Multimodal vision and reasoning model&#39;s "
        "card</div>"
        "<div class=\"font-mono text-xs\"><svg><path d=\"M0 "
        "0\"></path></svg><span>Cloud</span></div>"
        "Available in LM Studio Cloud<span class=\"font-medium\">123</span></a>"));
    QCOMPARE(fetcher.models().size(), 1);
    const auto model = fetcher.models().first().toMap();
    QCOMPARE(model.value("category").toString(), QString("Vision"));
    QVERIFY(model.value("cloudOnly").toBool());
    QVERIFY(model.value("description").toString().contains("model's card"));
    QCOMPARE(model.value("size").toString(), QString("—"));
    QVERIFY(model.value("tags").toStringList().contains("reasoning"));
    QVERIFY(model.value("tags").toStringList().contains("Cloud"));
    for (const auto& tag : model.value("tags").toStringList())
        QVERIFY(!tag.contains('<'));
    fetcher.parseHtml("<html>No model cards because markup changed</html>");
    QCOMPARE(fetcher.models().size(), 1);
    QVERIFY(!fetcher.errorText().isEmpty());
}
void OllamaRuntimeTest::explicitRefreshBypassesFreshCache() {
    QTemporaryDir directory;
    QFile file(directory.filePath("cache.json"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(QJsonObject{{"query", "GGUF"},
                                         {"fetchedAt", QDateTime::currentDateTimeUtc().toString(
                                                           Qt::ISODateWithMs)},
                                         {"models", QJsonArray{QJsonObject{{"id", "test/model"}}}}})
                   .toJson());
    file.close();
    sentinel::core::HuggingFaceModelSource source(file.fileName());
    auto& policy = sentinel::core::NetworkPolicyService::instance();
    const auto previous = policy.mode();
    policy.setMode(sentinel::core::NetworkMode::Offline);
    QSignalSpy finished(&source, &sentinel::core::HuggingFaceModelSource::requestFinished);
    source.search("GGUF");
    const bool cacheHit = finished.takeFirst().first().toBool();
    source.search("GGUF", true);
    const bool refreshSucceeded = finished.takeFirst().first().toBool();
    policy.setMode(previous);
    QVERIFY(cacheHit);
    QVERIFY(!refreshSucceeded);
    QCOMPARE(source.catalogState(), sentinel::core::HuggingFaceCatalogState::Offline);
    QVERIFY(!source.entries().isEmpty());
}

QTEST_MAIN(OllamaRuntimeTest)

#include "test_ollama_runtime.moc"
