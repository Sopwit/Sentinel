// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef Q_MOC_RUN
#include "../support/ScriptedProviderTransport.h"
#endif
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/memory/InMemorySettingsStore.h"
#include "sentinel/core/model/ModelService.h"
#include "sentinel/core/runtime/LocalInference.h"
#include "sentinel/core/security/CredentialStore.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QtTest>
using namespace sentinel::core;
using namespace sentinel::test;
namespace {
constexpr char key[] = "synthetic-security-key";
QStringList providers() {
    return {"openai", "deepseek",          "groq",   "mistral",          "claude",
            "gemini", "openai-compatible", "ollama", "llama-cpp-server", "lm-studio"};
}
LMStudioConfig configFor(const QString& url, const std::shared_ptr<ScriptedTransportState>& state) {
    LMStudioConfig config;
    config.endpoint = QUrl(url);
    config.apiKey = key;
    config.timeoutMs = 100;
    config.networkFactory = std::make_shared<ScriptedNetworkFactory>(state);
    return config;
}
} // namespace
class EndpointSecurityCapabilityTest final : public QObject {
    Q_OBJECT
private slots:
    void endpointValidation_data();
    void endpointValidation();
    void localWithoutCredentials_data();
    void localWithoutCredentials();
    void pathAndHostIdentity_data();
    void pathAndHostIdentity();
    void redirectRejected_data();
    void redirectRejected();
    void capabilityDisclosure_data();
    void capabilityDisclosure();
    void unknownCapabilityRejected_data();
    void unknownCapabilityRejected();
    void cloudDiscoveryEndpoint_data();
    void cloudDiscoveryEndpoint();
    void discovery_data();
    void discovery();
    void staleCatalogPreserved_data();
    void staleCatalogPreserved();
    void frozenEndpointAndFixedVendors();
};
void EndpointSecurityCapabilityTest::endpointValidation_data() {
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("allowed");
    QTest::addColumn<bool>("stream");
    const QList<QPair<QString, bool>> cases{
        {"https://fixture.invalid", true},
        {"http://fixture.invalid", false},
        {"ftp://fixture.invalid", false},
        {"file:///fixture", false},
        {"https:///", false},
        {"https://", false},
        {"https://bad host/", false},
        {"https://synthetic-user:synthetic-password@fixture.invalid", false},
        {"http://synthetic-user@localhost:1234", false},
        {"http://127.0.0.1:1234", true},
        {"http://localhost:1234", true},
        {"http://[::1]:1234", true},
        {"http://127.0.0.2:1234", true},
        {"http://192.168.1.2:1234", false},
        {"http://[fd00::1]:1234", false},
        {"https://192.168.1.2", true},
        {"https://[fd00::1]", true},
        {"https://localhost", true},
        {"https://[::1]", true},
        {"https://fixture.invalid/proxy/v1?synthetic-secret=discard#fragment", true}};
    for (int i = 0; i < cases.size(); ++i) {
        for (bool stream : {false, true}) {
            QTest::newRow(qPrintable(QString("endpoint-%1-stream-%2").arg(i).arg(stream)))
                << cases[i].first << cases[i].second << stream;
        }
    }
}
void EndpointSecurityCapabilityTest::endpointValidation() {
    QFETCH(QString, url);
    QFETCH(bool, allowed);
    QFETCH(bool, stream);
    auto state = std::make_shared<ScriptedTransportState>();
    state->responder = [stream](const auto&) {
        return ScriptedHttpResponse::body(
            stream
                ? QByteArray(
                      "data: {\"choices\":[{\"delta\":{\"content\":\"ok\"}}]}\n\ndata: [DONE]\n\n")
                : QByteArray(R"({"choices":[{"message":{"content":"ok"}}]})"));
    };
    auto config = configFor(url, state);
    LocalInferenceRequest request;
    request.prompt = "synthetic";
    request.options.model = "fixture";
    request.options.timeoutMs = 100;
    request.options.requireBoundedStreaming = true;
    ScopedOnlineNetworkMode online;
    if (stream) {
        const auto reply =
            LMStudioLocalInferenceStreamClient(config).startStream(request, [](const auto&) {});
        QVERIFY(!reply.endpoint.contains("synthetic-password"));
        QVERIFY(!reply.endpoint.contains("synthetic-secret"));
        QCOMPARE(reply.status == LocalInferenceStreamStatus::Completed, allowed);
    } else {
        const auto reply = LMStudioLocalInferenceClient(config).infer(request);
        QVERIFY(!reply.endpoint.contains("synthetic-password"));
        QVERIFY(!reply.endpoint.contains("synthetic-secret"));
        QCOMPARE(reply.status == LocalInferenceStatus::Succeeded, allowed);
    }
    QCOMPARE(state->requests.size(), allowed ? 1 : 0);
}

void EndpointSecurityCapabilityTest::localWithoutCredentials_data() {
    QTest::addColumn<QString>("url");
    for (const auto& url :
         QStringList{"http://localhost:1234", "http://127.0.0.1:1234", "http://127.0.0.2:1234",
                     "http://[::1]:1234", "https://fixture.invalid"}) {
        QTest::newRow(qPrintable(url)) << url;
    }
}
void EndpointSecurityCapabilityTest::localWithoutCredentials() {
    QFETCH(QString, url);
    auto state = std::make_shared<ScriptedTransportState>();
    state->responder = [](const auto&) {
        return ScriptedHttpResponse::body(R"({"choices":[{"message":{"content":"ok"}}]})");
    };
    auto config = configFor(url, state);
    config.apiKey.clear();
    const bool local = config.isLoopbackHttp();
    ScopedOnlineNetworkMode online;
    const auto reply =
        LMStudioLocalInferenceClient(config).completeOpenAiChat({{"model", "fixture"}});
    QCOMPARE(reply.ok, local);
    QCOMPARE(state->requests.size(), local ? 1 : 0);
    if (local) {
        QVERIFY(!state->requests.first().headers.contains("authorization"));
    }
}
void EndpointSecurityCapabilityTest::pathAndHostIdentity_data() {
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("stream");
    const QList<QPair<QString, QString>> cases{
        {"https://fixture.invalid/v1/", "/v1/chat/completions"},
        {"https://fixture.invalid/notv1", "/notv1/v1/chat/completions"},
        {"https://fixture.invalid/proxy/v1", "/proxy/v1/chat/completions"},
        {"https://fixture.invalid/other?synthetic-secret=x#fragment", "/other/v1/chat/completions"},
        {"https://api.anthropic.com.fixture.invalid", "/v1/chat/completions"},
        {"https://generativelanguage.googleapis.com.fixture.invalid", "/v1/chat/completions"}};
    for (int i = 0; i < cases.size(); ++i) {
        for (bool stream : {false, true}) {
            QTest::newRow(qPrintable(QString("identity-%1-%2").arg(i).arg(stream)))
                << cases[i].first << cases[i].second << stream;
        }
    }
}
void EndpointSecurityCapabilityTest::pathAndHostIdentity() {
    QFETCH(QString, url);
    QFETCH(QString, path);
    QFETCH(bool, stream);
    auto state = std::make_shared<ScriptedTransportState>();
    state->responder = [stream](const auto&) {
        return ScriptedHttpResponse::body(
            stream ? QByteArray(R"(data: {"choices":[{"delta":{"content":"ok"}}]}

data: [DONE]

)")
                   : QByteArray(R"({"choices":[{"message":{"content":"ok"}}]})"));
    };
    auto config = configFor(url, state);
    LocalInferenceRequest request;
    request.prompt = "synthetic";
    request.options.model = "fixture";
    request.options.timeoutMs = 100;
    ScopedOnlineNetworkMode online;
    if (stream) {
        const auto reply =
            LMStudioLocalInferenceStreamClient(config).startStream(request, [](const auto&) {});
        QCOMPARE(reply.status, LocalInferenceStreamStatus::Completed);
    } else {
        const auto reply = LMStudioLocalInferenceClient(config).infer(request);
        QCOMPARE(reply.status, LocalInferenceStatus::Succeeded);
    }
    QCOMPARE(state->requests.size(), 1);
    QCOMPARE(state->requests.first().url.host(), QUrl(url).host());
    QCOMPARE(state->requests.first().url.path(), path);
    QVERIFY(state->requests.first().url.query().isEmpty());
    QVERIFY(state->requests.first().url.fragment().isEmpty());
}
void EndpointSecurityCapabilityTest::redirectRejected_data() {
    QTest::addColumn<int>("status");
    QTest::addColumn<QString>("destination");
    QTest::addColumn<bool>("stream");
    for (int status : {301, 302, 307, 308}) {
        for (const auto& destination :
             QStringList{"https://other.invalid", "http://192.168.1.2", "http://[::1]"}) {
            for (bool stream : {false, true}) {
                QTest::newRow(qPrintable(
                    QString("redirect-%1-%2-%3").arg(status).arg(destination).arg(stream)))
                    << status << destination << stream;
            }
        }
    }
}
void EndpointSecurityCapabilityTest::redirectRejected() {
    QFETCH(int, status);
    QFETCH(QString, destination);
    QFETCH(bool, stream);
    auto state = std::make_shared<ScriptedTransportState>();
    state->responder = [=](const auto&) {
        auto response = ScriptedHttpResponse::body(
            R"({"choices":[{"message":{"content":"must not succeed"}}]})", status);
        response.rawHeaders.insert("Location", destination.toUtf8());
        return response;
    };
    auto config = configFor("https://fixture.invalid", state);
    LocalInferenceRequest request;
    request.prompt = "synthetic";
    request.options.model = "fixture";
    request.options.timeoutMs = 100;
    ScopedOnlineNetworkMode online;
    if (stream) {
        const auto reply =
            LMStudioLocalInferenceStreamClient(config).startStream(request, [](const auto&) {});
        QVERIFY(reply.status != LocalInferenceStreamStatus::Completed);
        QCOMPARE(reply.providerErrorCategory, int(ChatProviderErrorCategory::RequestRejected));
    } else {
        const auto reply = LMStudioLocalInferenceClient(config).infer(request);
        QVERIFY(reply.status != LocalInferenceStatus::Succeeded);
    }
    QCOMPARE(state->requests.size(), 1);
    QCOMPARE(state->requests.first().redirectPolicy, int(QNetworkRequest::ManualRedirectPolicy));
}
void EndpointSecurityCapabilityTest::capabilityDisclosure_data() {
    QTest::addColumn<QString>("provider");
    for (const auto& provider : providers()) {
        QTest::newRow(qPrintable(provider)) << provider;
    }
}
void EndpointSecurityCapabilityTest::capabilityDisclosure() {
    QFETCH(QString, provider);
    AppSettings settings(std::make_unique<InMemorySettingsStore>(), inMemoryTestCredentialStore());
    ModelService service(&settings);
    const auto capability = service.capabilities(provider, "unobserved-model");
    QCOMPARE(capability.streaming, CapabilitySupport::Supported);
    const bool native = provider == "claude" || provider == "gemini";
    QCOMPARE(capability.nativeToolCalling,
             native ? CapabilitySupport::Supported : CapabilitySupport::Unsupported);
    QCOMPARE(capability.structuredOutput,
             native ? CapabilitySupport::Unknown : CapabilitySupport::Unsupported);
    QCOMPARE(capability.visionInput, CapabilitySupport::Unknown);
    QCOMPARE(capability.audioInput, CapabilitySupport::Unknown);
    QCOMPARE(capability.combinedToolsAndStructuredOutput, CapabilitySupport::Unknown);
    QVERIFY(!capability.contextWindow.has_value());
    QCOMPARE(service.providerStatusSnapshot(provider).health, ProviderHealth::Unknown);
    ModelBinding binding;
    binding.providerId = provider;
    binding.modelId = "unobserved-model";
    binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
    // A synthetic binding never changes the authoritative service metadata.
    auto leased = service.constructProvider(binding);
    QVERIFY(leased);
    QCOMPARE(service.capabilities(provider, "unobserved-model").visionInput,
             CapabilitySupport::Unknown);
    QCOMPARE(service.capabilities(provider, "unobserved-model").nativeToolCalling,
             capability.nativeToolCalling);
}
void EndpointSecurityCapabilityTest::unknownCapabilityRejected_data() {
    QTest::addColumn<QString>("provider");
    QTest::addColumn<QString>("operation");
    auto ids = providers();
    ids.removeAll("ollama");
    for (const auto& provider : ids) {
        for (const auto& operation :
             QStringList{"tools", "schema", "stream", "vision-unsupported", "vision-stream"}) {
            QTest::newRow(qPrintable(provider + "-" + operation)) << provider << operation;
        }
    }
}
void EndpointSecurityCapabilityTest::unknownCapabilityRejected() {
    QFETCH(QString, provider);
    QFETCH(QString, operation);
    AppSettings settings(std::make_unique<InMemorySettingsStore>(), inMemoryTestCredentialStore());
    settings.setNetworkMode("LocalOnly");
    settings.setOpenAiApiKey(key);
    settings.setDeepseekApiKey(key);
    settings.setGroqApiKey(key);
    settings.setMistralApiKey(key);
    settings.setClaudeApiKey(key);
    settings.setGeminiApiKey(key);
    ModelService service(&settings);
    auto state = std::make_shared<ScriptedTransportState>();
    service.setProviderNetworkFactoryForTesting(std::make_shared<ScriptedNetworkFactory>(state));
    ModelBinding binding;
    binding.providerId = provider;
    binding.modelId = "fixture";
    ChatRequestOptions options;
    options.deadlineMs = 100;
    options.requireBoundedStreaming = true;
    options.nativeToolCalling = operation == "tools";
    options.structuredOutput = operation == "schema";
    if (operation == "vision-unsupported" || operation == "vision-stream") {
        if (operation == "vision-stream") {
            binding.capabilities.streaming = CapabilitySupport::Supported;
        }
        binding.capabilities.visionInput = CapabilitySupport::Unsupported;
        options.images = {{"image/png", QByteArray("synthetic")}};
    }
    auto leased = service.constructProvider(binding);
    ScopedOnlineNetworkMode online;
    const auto reply = (operation == "stream" || operation == "vision-stream")
                           ? leased->sendMessageStreaming(
                                 "synthetic", [](const auto&) {}, {}, options)
                           : leased->sendRequest("synthetic", options);
    QVERIFY(!reply.success);
    QCOMPARE(state->requests.size(), 0);
    QCOMPARE(reply.category, ChatProviderErrorCategory::CapabilityUnsupported);
}
void EndpointSecurityCapabilityTest::cloudDiscoveryEndpoint_data() {
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("allowed");
    const QList<QPair<QString, bool>> cases{
        {"https://fixture.invalid/v1/models", true},
        {"http://fixture.invalid/v1/models", false},
        {"http://127.0.0.1/v1/models", false},
        {"http://[::1]/v1/models", false},
        {"https://synthetic-user:synthetic-password@fixture.invalid/v1/models", false},
        {"https:///v1/models", false},
        {"ftp://fixture.invalid/v1/models", false},
        {"https://192.168.1.2/v1/models", true},
        {"https://[fd00::1]/v1/models", true},
        {"https://localhost/v1/models", true}};
    for (int i = 0; i < cases.size(); ++i) {
        QTest::newRow(qPrintable(QString("cloud-discovery-%1").arg(i)))
            << cases[i].first << cases[i].second;
    }
}
void EndpointSecurityCapabilityTest::cloudDiscoveryEndpoint() {
    QFETCH(QString, url);
    QFETCH(bool, allowed);
    auto state = std::make_shared<ScriptedTransportState>();
    state->responder = [](const auto&) { return ScriptedHttpResponse::body(R"({"data":[]})"); };
    auto factory = std::make_shared<ScriptedNetworkFactory>(state);
    ProviderDiscoveryOutcome outcome;
    QString error;
    ScopedOnlineNetworkMode online;
    const auto models = fetchOpenAiCloudModels(QUrl(url), key, 100, &error, {}, &outcome, factory);
    QVERIFY(models.isEmpty());
    QCOMPARE(outcome.completed, allowed);
    QCOMPARE(state->requests.size(), allowed ? 1 : 0);
    QVERIFY(!error.contains("synthetic-password"));
    QVERIFY(!error.contains(key));
}

void EndpointSecurityCapabilityTest::discovery_data() {
    QTest::addColumn<QString>("provider");
    QTest::addColumn<QString>("fault");
    for (const auto& provider :
         QStringList{"openai", "deepseek", "groq", "mistral", "claude", "gemini"}) {
        for (const auto& fault :
             QStringList{"valid", "empty", "malformed", "missing-array", "missing-id", "numeric-id",
                         "auth401", "auth403", "timeout", "cancel-before", "cancel-dispatched",
                         "redirect", "offline", "local-only", "missing-key"}) {
            QTest::newRow(qPrintable(provider + "-" + fault)) << provider << fault;
        }
    }
}
void EndpointSecurityCapabilityTest::discovery() {
    QFETCH(QString, provider);
    QFETCH(QString, fault);
    auto state = std::make_shared<ScriptedTransportState>();
    auto token = std::make_shared<std::atomic_bool>(fault == "cancel-before");
    const bool gemini = provider == "gemini";
    state->responder = [&](const auto&) {
        if (fault == "cancel-dispatched") {
            token->store(true);
        }
        if (fault == "timeout" || fault == "cancel-dispatched") {
            return ScriptedHttpResponse::streaming({}, false);
        }
        QByteArray payload =
            gemini ? R"({"models":[{"name":"models/fixture","inputTokenLimit":4096}]})"
                   : R"({"data":[{"id":"fixture","context_window":4096,"vision":"true"}]})";
        if (fault == "empty") {
            payload = gemini ? R"({"models":[]})" : R"({"data":[]})";
        }
        if (fault == "malformed") {
            payload = "{broken";
        }
        if (fault == "missing-array") {
            payload = "{}";
        }
        if (fault == "missing-id") {
            payload = gemini ? R"({"models":[{}]})" : R"({"data":[{}]})";
        }
        if (fault == "numeric-id") {
            payload = gemini ? R"({"models":[{"name":7}]})" : R"({"data":[{"id":7}]})";
        }
        auto response = ScriptedHttpResponse::body(payload, fault == "auth401"    ? 401
                                                            : fault == "auth403"  ? 403
                                                            : fault == "redirect" ? 302
                                                                                  : 200);
        if (fault == "redirect") {
            response.rawHeaders.insert("Location", "https://other.invalid?synthetic-secret=x");
        }
        return response;
    };
    auto factory = std::make_shared<ScriptedNetworkFactory>(state);
    QString host = provider == "deepseek"  ? "api.deepseek.com"
                   : provider == "groq"    ? "api.groq.com"
                   : provider == "mistral" ? "api.mistral.ai"
                                           : "api.openai.com";
    QString error;
    ProviderDiscoveryOutcome outcome;
    ScopedOnlineNetworkMode online;
    if (fault == "offline") {
        NetworkPolicyService::instance().setMode(NetworkMode::Offline);
    }
    if (fault == "local-only") {
        NetworkPolicyService::instance().setMode(NetworkMode::LocalOnly);
    }
    const QString credential = fault == "missing-key" ? QString() : QString::fromLatin1(key);
    QList<OllamaModelSummary> models;
    if (gemini) {
        models = fetchGeminiCloudModels(credential, 100, &error, token, &outcome, factory);
    } else if (provider == "claude") {
        models = fetchAnthropicCloudModels(credential, 100, &error, token, &outcome, factory);
    } else {
        models = fetchOpenAiCloudModels(
            QUrl("https://" + host + (provider == "groq" ? "/openai/v1/models" : "/v1/models")),
            credential, 100, &error, token, &outcome, factory);
    }
    const bool success = fault == "valid" || fault == "empty";
    QCOMPARE(outcome.completed, success);
    if (fault == "valid") {
        QCOMPARE(models.size(), 1);
        QCOMPARE(models.first().name, QString("fixture"));
        QCOMPARE(models.first().capabilities.contextWindow.value_or(0), 4096);
        QCOMPARE(models.first().capabilities.visionInput, CapabilitySupport::Unknown);
    } else {
        QVERIFY(models.isEmpty());
    }
    if (fault == "missing-array" || fault == "missing-id" || fault == "numeric-id" ||
        fault == "malformed") {
        QCOMPARE(outcome.category, ChatProviderErrorCategory::MalformedResponse);
    }
    if (fault.startsWith("auth") || fault == "missing-key") {
        QCOMPARE(outcome.category, ChatProviderErrorCategory::AuthenticationRequired);
    }
    if (fault == "timeout") {
        QCOMPARE(outcome.category, ChatProviderErrorCategory::Timeout);
    }
    if (fault.startsWith("cancel-")) {
        QCOMPARE(outcome.category, ChatProviderErrorCategory::Cancelled);
    }
    if (fault == "redirect") {
        QCOMPARE(outcome.category, ChatProviderErrorCategory::RequestRejected);
    }
    if (fault == "offline" || fault == "local-only") {
        QCOMPARE(outcome.category, ChatProviderErrorCategory::Offline);
    }
    const bool dispatched = fault != "offline" && fault != "local-only" &&
                            fault != "cancel-before" && fault != "missing-key";
    QCOMPARE(state->requests.size(), dispatched ? 1 : 0);
    QVERIFY(!error.contains(key));
    QVERIFY(!error.contains("synthetic-secret"));
    if (dispatched) {
        const auto request = state->requests.first();
        QCOMPARE(request.method, QByteArray("GET"));
        QVERIFY(request.body.isEmpty());
        QCOMPARE(request.redirectPolicy, int(QNetworkRequest::ManualRedirectPolicy));
        if (gemini) {
            QCOMPARE(request.url.host(), QString("generativelanguage.googleapis.com"));
            QCOMPARE(request.url.path(), QString("/v1beta/models"));
            QVERIFY(request.url.query().contains(key));
        } else if (provider == "claude") {
            QCOMPARE(request.url.host(), QString("api.anthropic.com"));
            QCOMPARE(request.headers.value("x-api-key"), QByteArray(key));
        } else {
            QCOMPARE(request.url.host(), host);
            QCOMPARE(request.headers.value("authorization"), QByteArray("Bearer ") + key);
        }
    }
}
void EndpointSecurityCapabilityTest::staleCatalogPreserved_data() {
    QTest::addColumn<QString>("provider");
    auto ids = providers();
    ids.removeAll("ollama");
    for (const auto& provider : ids) {
        QTest::newRow(qPrintable(provider)) << provider;
    }
}
void EndpointSecurityCapabilityTest::staleCatalogPreserved() {
    QFETCH(QString, provider);
    AppSettings settings(std::make_unique<InMemorySettingsStore>(), inMemoryTestCredentialStore());
    ModelService service(&settings);
    OllamaModelSummary model;
    model.name = "fixture";
    model.capabilities.contextWindow = 4096;
    service.acceptProviderDiscovery(provider, {model},
                                    {true, ChatProviderErrorCategory::None, 200, "new"}, 10);
    service.acceptProviderDiscovery(
        provider, {}, {false, ChatProviderErrorCategory::AuthenticationRequired, 401, "old"}, 9);
    QCOMPARE(service.providerStatusSnapshot(provider).catalog, ProviderCatalogState::Available);
    service.acceptProviderDiscovery(
        provider, {}, {false, ChatProviderErrorCategory::MalformedResponse, 200, "failure"}, 11);
    const auto status = service.providerStatusSnapshot(provider);
    QCOMPARE(status.catalog, ProviderCatalogState::Stale);
    QCOMPARE(status.modelIds, QStringList{"fixture"});
    QCOMPARE(service.capabilities(provider, "fixture").contextWindow.value_or(0), 4096);
    service.acceptProviderDiscovery(provider, {},
                                    {true, ChatProviderErrorCategory::None, 200, "empty"}, 12);
    QCOMPARE(service.providerStatusSnapshot(provider).catalog, ProviderCatalogState::Empty);
}
void EndpointSecurityCapabilityTest::frozenEndpointAndFixedVendors() {
    AppSettings settings(std::make_unique<InMemorySettingsStore>(), inMemoryTestCredentialStore());
    settings.setNetworkMode("LocalOnly");
    settings.setOpenAiApiKey(key);
    settings.setDeepseekApiKey(key);
    settings.setGroqApiKey(key);
    settings.setMistralApiKey(key);
    ModelService service(&settings);
    auto state = std::make_shared<ScriptedTransportState>();
    state->responder = [](const auto&) {
        return ScriptedHttpResponse::body(R"({"choices":[{"message":{"content":"ok"}}]})");
    };
    service.setProviderNetworkFactoryForTesting(std::make_shared<ScriptedNetworkFactory>(state));
    service.setLmStudioEndpoint("http://127.0.0.1:1234/proxy/v1?synthetic-secret=x");
    ModelBinding binding;
    binding.providerId = "lm-studio";
    binding.modelId = "frozen-model";
    auto leased = service.constructProvider(binding);
    service.setLmStudioEndpoint("http://192.168.1.2");
    service.setSelectedModel({"ollama", "future-model"});
    ScopedOnlineNetworkMode online;
    ChatRequestOptions options;
    options.deadlineMs = 100;
    QVERIFY(leased->sendRequest("synthetic", options).success);
    QCOMPARE(state->requests.size(), 1);
    QCOMPARE(state->requests.first().url.host(), QString("127.0.0.1"));
    QCOMPARE(state->requests.first().url.path(), QString("/proxy/v1/chat/completions"));
    for (const auto& id :
         QStringList{"openai", "deepseek", "groq", "mistral", "openai-compatible"}) {
        const auto config = service.providerConfig(ModelBinding{id, "fixture"});
        QCOMPARE(config.endpoint.scheme(), QString("https"));
        QVERIFY(config.endpoint.host() != QString("192.168.1.2"));
    }
}

QTEST_GUILESS_MAIN(EndpointSecurityCapabilityTest)
#include "test_endpoint_security_capability.moc"
