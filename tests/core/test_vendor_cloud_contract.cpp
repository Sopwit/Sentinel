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
QStringList vendors() {
    return {"openai", "deepseek", "groq", "mistral", "openai-compatible"};
}
constexpr char key[] = "synthetic-vendor-key-not-real";
constexpr char answer[] =
    R"({"choices":[{"finish_reason":"stop","message":{"role":"assistant","content":"fixture answer"}}]})";
constexpr char delta[] = "data: {\"choices\":[{\"delta\":{\"content\":\"first\"}}]}\n\n";
constexpr char done[] = "data: [DONE]\n\n";

QString hostFor(const QString& vendor) {
    if (vendor == "deepseek") {
        return "api.deepseek.com";
    }
    if (vendor == "groq") {
        return "api.groq.com";
    }
    if (vendor == "mistral") {
        return "api.mistral.ai";
    }
    return "api.openai.com";
}
QString pathFor(const QString& vendor) {
    return vendor == "groq" ? "/openai/v1/chat/completions" : "/v1/chat/completions";
}
void vendorData() {
    QTest::addColumn<QString>("vendor");
    for (const auto& vendor : vendors()) {
        QTest::newRow(qPrintable(vendor)) << vendor;
    }
}
ToolDescriptor tool() {
    ToolDescriptor result;
    result.id = "read-file";
    result.name = "Read File";
    result.inputSchema = {{"type", "object"},
                          {"properties", QJsonObject{{"path", QJsonObject{{"type", "string"}}}}},
                          {"required", QJsonArray{"path"}}};
    return result;
}
ChatRequestOptions options() {
    ChatRequestOptions result;
    result.deadlineMs = 150;
    result.maxOutputTokens = 32;
    return result;
}
struct Harness {
    AppSettings settings{std::make_unique<InMemorySettingsStore>(), inMemoryTestCredentialStore()};
    ModelService service{&settings};
    std::shared_ptr<ScriptedTransportState> state = std::make_shared<ScriptedTransportState>();
    explicit Harness(bool credentials = true) {
        settings.setNetworkMode(QStringLiteral("LocalOnly"));
        if (credentials) {
            settings.setOpenAiApiKey(key);
            settings.setClaudeApiKey(key);
            settings.setGeminiApiKey(key);
            settings.setDeepseekApiKey(key);
            settings.setGroqApiKey(key);
            settings.setMistralApiKey(key);
        }
        service.setProviderNetworkFactoryForTesting(
            std::make_shared<ScriptedNetworkFactory>(state));
        respond(ScriptedHttpResponse::body(answer));
    }
    void respond(const ScriptedHttpResponse& response) {
        state->responder = [response](const auto&) { return response; };
    }
    ModelBinding binding(const QString& vendor) {
        ModelBinding result;
        result.providerId = vendor;
        result.modelId = "synthetic-model-" + vendor;
        result.capabilities = service.capabilities(vendor, result.modelId);
        return result;
    }
    QJsonObject body(int index = 0) const {
        return QJsonDocument::fromJson(state->requests.at(index).body).object();
    }
};
} // namespace

class VendorCloudContractTest final : public QObject {
    Q_OBJECT
private slots:
    void init() {
        NetworkPolicyService::instance().setMode(NetworkMode::LocalOnly);
    }
    void constructionAndFrozenBinding_data() {
        vendorData();
    }
    void constructionAndFrozenBinding();
    void toolAndStructuredWire_data();
    void toolAndStructuredWire();
    void unsupportedCapabilities_data();
    void unsupportedCapabilities();
    void missingCredentialsNoFallback_data() {
        vendorData();
    }
    void missingCredentialsNoFallback();
    void httpErrors_data();
    void httpErrors();
    void nativeMalformedPayload_data();
    void nativeMalformedPayload();
    void malformedBodies_data();
    void malformedBodies();
    void interruption_data();
    void interruption();
    void streaming_data();
    void streaming();
    void networkPolicy_data();
    void networkPolicy();
    void cloudAlias_data() {
        vendorData();
    }
    void cloudAlias();
    void configurableEndpoint_data();
    void configurableEndpoint();
};

void VendorCloudContractTest::constructionAndFrozenBinding() {
    QFETCH(QString, vendor);
    Harness h;
    QVERIFY(h.service.isKnownProvider(vendor));
    auto binding = h.binding(vendor);
    const auto model = binding.modelId;
    auto provider = h.service.constructProvider(binding);
    binding.modelId = "mutated-caller-model";
    h.service.setSelectedModel({"ollama", "different-selection"});
    h.settings.setOpenAiApiKey("synthetic-replacement-key");
    h.settings.setDeepseekApiKey("synthetic-replacement-key");
    h.settings.setGroqApiKey("synthetic-replacement-key");
    h.settings.setMistralApiKey("synthetic-replacement-key");
    ScopedOnlineNetworkMode online;
    const auto reply = provider->sendRequest("synthetic prompt", options());
    QVERIFY2(reply.success, qPrintable(reply.errorMessage));
    QCOMPARE(reply.message, QString("fixture answer"));
    QCOMPARE(reply.lifecycle, ChatRequestLifecycle::Completed);
    QCOMPARE(h.state->requests.size(), 1);
    const auto request = h.state->requests.first();
    QCOMPARE(request.url.scheme(), QString("https"));
    QCOMPARE(request.url.host(), hostFor(vendor));
    QCOMPARE(request.url.path(), pathFor(vendor));
    QCOMPARE(request.method, QByteArray("POST"));
    QCOMPARE(request.headers.value("authorization"), QByteArray("Bearer ") + QByteArray(key));
    QVERIFY(request.headers.value("content-type").startsWith("application/json"));
    QCOMPARE(h.body().value("model").toString(), model);
    QCOMPARE(h.body().value("stream").toBool(), false);
    QCOMPARE(h.body().value("max_tokens").toInt(), 32);
    QCOMPARE(h.body().value("temperature").toDouble(), 0.7);
    QCOMPARE(h.body().value("messages").toArray().first().toObject().value("content").toString(),
             QString("synthetic prompt"));
    QVERIFY(!request.body.contains(QByteArray(key)));
    QCOMPARE(h.service.selectedModel().providerId, QString("ollama"));
}

void VendorCloudContractTest::toolAndStructuredWire_data() {
    QTest::addColumn<QString>("vendor");
    QTest::addColumn<bool>("structured");
    for (const auto& vendor : vendors()) {
        for (bool structured : {false, true}) {
            QTest::newRow(qPrintable(vendor + (structured ? "-schema" : "-tool")))
                << vendor << structured;
        }
    }
}
void VendorCloudContractTest::toolAndStructuredWire() {
    QFETCH(QString, vendor);
    QFETCH(bool, structured);
    Harness h;
    auto binding = h.binding(vendor);
    // Model-specific support is explicitly supplied, never inferred from vendor compatibility.
    binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
    binding.capabilities.structuredOutput = CapabilitySupport::Supported;
    auto provider = h.service.constructProvider(binding);
    auto requestOptions = options();
    requestOptions.nativeToolCalling = !structured;
    requestOptions.structuredOutput = structured;
    requestOptions.tools = {tool()};
    requestOptions.structuredSchemaName = "answer";
    requestOptions.structuredSchema = tool().inputSchema;
    requestOptions.strictStructuredOutput = true;
    h.respond(ScriptedHttpResponse::body(
        structured
            ? QByteArray(
                  R"({"choices":[{"message":{"content":"{\"path\":\"fixture.txt\"}"},"finish_reason":"stop"}]})")
            : QByteArray(
                  R"({"choices":[{"message":{"tool_calls":[{"id":"fixture-call","type":"function","function":{"name":"read-file","arguments":"{\"path\":\"fixture.txt\"}"}}]},"finish_reason":"tool_calls"}]})")));
    ScopedOnlineNetworkMode online;
    const auto reply = provider->sendRequest("synthetic", requestOptions);
    QVERIFY2(reply.success, qPrintable(reply.errorMessage));
    QCOMPARE(h.state->requests.size(), 1);
    QCOMPARE(h.state->requests.first().url.path(), pathFor(vendor));
    QCOMPARE(h.state->requests.first().headers.value("authorization"),
             QByteArray("Bearer ") + QByteArray(key));
    if (structured) {
        QVERIFY(reply.structuredResult.has_value());
        QCOMPARE(reply.structuredResult.value_or(QJsonObject{}).value("path").toString(),
                 QString("fixture.txt"));
        const auto format = h.body().value("response_format").toObject();
        QCOMPARE(format.value("type").toString(), QString("json_schema"));
        QCOMPARE(format.value("json_schema").toObject().value("schema").toObject(),
                 tool().inputSchema);
        QVERIFY(format.value("json_schema").toObject().value("strict").toBool());
    } else {
        QCOMPARE(reply.toolCalls.size(), 1);
        QCOMPARE(reply.toolCalls.first().callId, QString("fixture-call"));
        QCOMPARE(reply.toolCalls.first().toolId, QString("read-file"));
        QCOMPARE(reply.toolCalls.first().arguments.value("path").toString(),
                 QString("fixture.txt"));
        QCOMPARE(h.body()
                     .value("tools")
                     .toArray()
                     .first()
                     .toObject()
                     .value("function")
                     .toObject()
                     .value("parameters")
                     .toObject(),
                 tool().inputSchema);
        requestOptions.priorToolCalls = reply.toolCalls;
        requestOptions.toolResults = {{"fixture-call", "synthetic observation"}};
        h.respond(ScriptedHttpResponse::body(answer));
        const auto continued = provider->sendRequest("continue", requestOptions);
        QVERIFY(continued.success);
        QCOMPARE(h.state->requests.size(), 2);
        const auto messages = h.body(1).value("messages").toArray();
        QCOMPARE(messages.last().toObject().value("role").toString(), QString("tool"));
        QCOMPARE(messages.last().toObject().value("tool_call_id").toString(),
                 QString("fixture-call"));
        QCOMPARE(messages.last().toObject().value("content").toString(),
                 QString("synthetic observation"));
    }
}

void VendorCloudContractTest::unsupportedCapabilities_data() {
    QTest::addColumn<QString>("vendor");
    QTest::addColumn<QString>("capability");
    for (const auto& vendor : vendors()) {
        for (const auto& capability :
             QStringList{"tools", "schema", "vision", "stream", "combined", "bounded-tools",
                         "bounded-schema", "no-deadline"}) {
            QTest::newRow(qPrintable(vendor + "-" + capability)) << vendor << capability;
        }
    }
}
void VendorCloudContractTest::unsupportedCapabilities() {
    QFETCH(QString, vendor);
    QFETCH(QString, capability);
    Harness h;
    auto binding = h.binding(vendor);
    QCOMPARE(binding.capabilities.nativeToolCalling, CapabilitySupport::Unsupported);
    QCOMPARE(binding.capabilities.structuredOutput, CapabilitySupport::Unsupported);
    QCOMPARE(binding.capabilities.visionInput, CapabilitySupport::Unknown);
    auto requestOptions = options();
    if (capability == "tools") {
        requestOptions.nativeToolCalling = true;
    }
    if (capability == "schema") {
        requestOptions.structuredOutput = true;
    }
    if (capability == "vision") {
        binding.capabilities.visionInput = CapabilitySupport::Unsupported;
        requestOptions.images = {{"image/png", QByteArray("synthetic-image")}};
    }
    if (capability == "stream") {
        binding.capabilities.streaming = CapabilitySupport::Unsupported;
    }
    if (capability == "combined") {
        binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
        binding.capabilities.structuredOutput = CapabilitySupport::Supported;
        requestOptions.nativeToolCalling = true;
        requestOptions.structuredOutput = true;
    }
    const bool stream =
        capability == "stream" || capability.startsWith("bounded-") || capability == "no-deadline";
    if (capability == "bounded-tools") {
        requestOptions.nativeToolCalling = true;
    }
    if (capability == "bounded-schema") {
        requestOptions.structuredOutput = true;
    }
    if (capability == "no-deadline") {
        requestOptions.deadlineMs = 0;
    }
    requestOptions.requireBoundedStreaming = stream;
    auto provider = h.service.constructProvider(binding);
    ScopedOnlineNetworkMode online;
    const auto reply = stream ? provider->sendMessageStreaming(
                                    "synthetic", [](const auto&) {}, {}, requestOptions)
                              : provider->sendRequest("synthetic", requestOptions);
    QVERIFY(!reply.success);
    QCOMPARE(reply.category, capability == "no-deadline"
                                 ? ChatProviderErrorCategory::RequestRejected
                                 : ChatProviderErrorCategory::CapabilityUnsupported);
    QCOMPARE(h.state->requests.size(), 0);
}

void VendorCloudContractTest::missingCredentialsNoFallback() {
    QFETCH(QString, vendor);
    Harness h(false);
    int fallback = 0;
    h.service.registerProvider("ollama",
                               [&](const ModelBinding&) -> std::shared_ptr<IChatProvider> {
                                   ++fallback;
                                   return {};
                               });
    h.service.setSelectedModel({vendor, "synthetic-model"});
    ScopedOnlineNetworkMode online;
    const auto resolution = h.service.resolve(h.service.selectedModel());
    QVERIFY(!resolution.ok());
    QCOMPARE(resolution.error, ModelBindingError::AuthenticationRequired);
    QCOMPARE(fallback, 0);
    QCOMPARE(h.state->requests.size(), 0);
    QCOMPARE(h.service.selectedModel().providerId, vendor);
}

void VendorCloudContractTest::httpErrors_data() {
    QTest::addColumn<QString>("vendor");
    QTest::addColumn<int>("status");
    QTest::addColumn<bool>("native");
    for (const auto& vendor : vendors()) {
        for (int status : {400, 401, 403, 404, 408, 429, 500, 501}) {
            for (bool native : {false, true}) {
                QTest::newRow(qPrintable(vendor + QString("-%1-%2").arg(status).arg(native)))
                    << vendor << status << native;
            }
        }
    }
}
void VendorCloudContractTest::httpErrors() {
    QFETCH(QString, vendor);
    QFETCH(int, status);
    QFETCH(bool, native);
    Harness h;
    h.respond(ScriptedHttpResponse::body(
        QJsonDocument(
            QJsonObject{{"error", QJsonObject{{"message", QString::fromLatin1(key) +
                                                              " PRIVATE_UPSTREAM_ECHO"}}}})
            .toJson(),
        status));
    auto binding = h.binding(vendor);
    binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
    auto provider = h.service.constructProvider(binding);
    auto requestOptions = options();
    requestOptions.nativeToolCalling = native;
    requestOptions.tools = {tool()};
    ScopedOnlineNetworkMode online;
    const auto reply = provider->sendRequest("synthetic prompt", requestOptions);
    QVERIFY(!reply.success);
    const auto expected = status == 400 ? (native ? ChatProviderErrorCategory::CapabilityUnsupported
                                                  : ChatProviderErrorCategory::RequestRejected)
                          : status == 401 || status == 403
                              ? ChatProviderErrorCategory::AuthenticationRequired
                          : status == 404 ? ChatProviderErrorCategory::ModelNotFound
                          : status == 408 ? ChatProviderErrorCategory::Timeout
                          : status == 429 ? ChatProviderErrorCategory::RateLimited
                          : status == 501 ? ChatProviderErrorCategory::CapabilityUnsupported
                                          : ChatProviderErrorCategory::ProviderUnavailable;
    QCOMPARE(reply.category, expected);
    QCOMPARE(reply.httpStatus, status);
    QCOMPARE(reply.attempts, 1);
    QVERIFY(reply.message.isEmpty());
    QVERIFY(!reply.errorMessage.contains(key));
    QVERIFY(!reply.errorMessage.contains("PRIVATE_"));
    QVERIFY(reply.errorMessage.size() < 512);
    const auto diagnostics = QJsonDocument(reply.diagnostics).toJson();
    QVERIFY(!diagnostics.contains(QByteArray(key)));
    QVERIFY(!diagnostics.contains("PRIVATE_"));
    QVERIFY(!diagnostics.contains("synthetic prompt"));
    QCOMPARE(h.state->requests.size(), 1);
    QCOMPARE(h.state->requests.first().url.host(), hostFor(vendor));
}

void VendorCloudContractTest::nativeMalformedPayload_data() {
    QTest::addColumn<QString>("vendor");
    QTest::addColumn<QString>("fault");
    for (const auto& vendor : vendors()) {
        for (const auto& fault : QStringList{"unknown-tool", "missing-id", "array-args",
                                             "broken-args", "schema-text", "schema-array"}) {
            QTest::newRow(qPrintable(vendor + "-" + fault)) << vendor << fault;
        }
    }
}
void VendorCloudContractTest::nativeMalformedPayload() {
    QFETCH(QString, vendor);
    QFETCH(QString, fault);
    Harness h;
    auto binding = h.binding(vendor);
    binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
    binding.capabilities.structuredOutput = CapabilitySupport::Supported;
    auto requestOptions = options();
    requestOptions.nativeToolCalling = !fault.startsWith("schema-");
    requestOptions.structuredOutput = fault.startsWith("schema-");
    requestOptions.tools = {tool()};
    requestOptions.structuredSchemaName = "answer";
    requestOptions.structuredSchema = tool().inputSchema;
    QJsonObject message;
    if (requestOptions.structuredOutput) {
        message.insert("content", fault == "schema-text" ? "not JSON" : "[]");
    } else {
        const QJsonObject call{
            {"id", fault == "missing-id" ? "" : "fixture-call"},
            {"type", "function"},
            {"function", QJsonObject{{"name", fault == "unknown-tool" ? "unknown" : "read-file"},
                                     {"arguments", fault == "array-args"    ? "[]"
                                                   : fault == "broken-args" ? "{broken"
                                                                            : "{}"}}}};
        message.insert("tool_calls", QJsonArray{call});
    }
    h.respond(ScriptedHttpResponse::body(
        QJsonDocument(QJsonObject{{"choices", QJsonArray{QJsonObject{{"message", message}}}}})
            .toJson()));
    auto provider = h.service.constructProvider(binding);
    ScopedOnlineNetworkMode online;
    const auto reply = provider->sendRequest("synthetic", requestOptions);
    QVERIFY(!reply.success);
    QCOMPARE(reply.category, ChatProviderErrorCategory::MalformedResponse);
    QVERIFY(reply.toolCalls.isEmpty());
    QVERIFY(!reply.structuredResult.has_value());
    QCOMPARE(h.state->requests.size(), 1);
}

void VendorCloudContractTest::malformedBodies_data() {
    QTest::addColumn<QString>("vendor");
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<bool>("native");
    const QList<QByteArray> bodies{"{broken", "{}", R"({"choices":[]})",
                                   R"({"choices":[{"message":{"content":null}}]})"};
    for (const auto& vendor : vendors()) {
        for (int i = 0; i < bodies.size(); ++i) {
            for (bool native : {false, true}) {
                QTest::newRow(qPrintable(vendor + QString("-%1-%2").arg(i).arg(native)))
                    << vendor << bodies[i] << native;
            }
        }
    }
}
void VendorCloudContractTest::malformedBodies() {
    QFETCH(QString, vendor);
    QFETCH(QByteArray, body);
    QFETCH(bool, native);
    Harness h;
    h.respond(ScriptedHttpResponse::body(body));
    auto binding = h.binding(vendor);
    binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
    auto provider = h.service.constructProvider(binding);
    auto requestOptions = options();
    requestOptions.nativeToolCalling = native;
    requestOptions.tools = {tool()};
    ScopedOnlineNetworkMode online;
    const auto reply = provider->sendRequest("synthetic", requestOptions);
    QVERIFY(!reply.success);
    QCOMPARE(reply.category, ChatProviderErrorCategory::MalformedResponse);
    QVERIFY(reply.message.isEmpty());
    QCOMPARE(h.state->requests.size(), 1);
}

void VendorCloudContractTest::interruption_data() {
    QTest::addColumn<QString>("vendor");
    QTest::addColumn<bool>("native");
    QTest::addColumn<QString>("fault");
    for (const auto& vendor : vendors()) {
        for (bool native : {false, true}) {
            for (const auto& fault :
                 QStringList{"cancel-before", "cancel-dispatched", "deadline"}) {
                QTest::newRow(qPrintable(vendor + QString("-%1-%2").arg(native).arg(fault)))
                    << vendor << native << fault;
            }
        }
    }
}
void VendorCloudContractTest::interruption() {
    QFETCH(QString, vendor);
    QFETCH(bool, native);
    QFETCH(QString, fault);
    Harness h;
    auto binding = h.binding(vendor);
    binding.capabilities.nativeToolCalling = CapabilitySupport::Supported;
    auto provider = h.service.constructProvider(binding);
    auto requestOptions = options();
    requestOptions.nativeToolCalling = native;
    requestOptions.tools = {tool()};
    requestOptions.cancellationToken = std::make_shared<std::atomic_bool>(fault == "cancel-before");
    h.state->responder = [&](const auto&) {
        if (fault == "cancel-dispatched") {
            requestOptions.cancellationToken->store(true);
        }
        return ScriptedHttpResponse::streaming({}, false);
    };
    ScopedOnlineNetworkMode online;
    const auto reply = provider->sendRequest("synthetic", requestOptions);
    QVERIFY(!reply.success);
    QCOMPARE(reply.category, fault == "deadline" ? ChatProviderErrorCategory::Timeout
                                                 : ChatProviderErrorCategory::Cancelled);
    QCOMPARE(reply.lifecycle, fault == "deadline" ? ChatRequestLifecycle::TimedOut
                                                  : ChatRequestLifecycle::Cancelled);
    QCOMPARE(h.state->requests.size(), fault == "cancel-before" ? 0 : 1);
    QVERIFY(reply.message.isEmpty());
}

void VendorCloudContractTest::streaming_data() {
    QTest::addColumn<QString>("vendor");
    QTest::addColumn<QString>("fault");
    for (const auto& vendor : vendors()) {
        for (const auto& fault :
             QStringList{"done", "finish-reason", "unterminated", "malformed", "error-event",
                         "cancel", "deadline", "http400", "http401", "http403", "http404",
                         "http408", "http429", "http500", "http501"}) {
            QTest::newRow(qPrintable(vendor + "-" + fault)) << vendor << fault;
        }
    }
}
void VendorCloudContractTest::streaming() {
    QFETCH(QString, vendor);
    QFETCH(QString, fault);
    Harness h;
    auto binding = h.binding(vendor);
    auto provider = h.service.constructProvider(binding);
    auto requestOptions = options();
    requestOptions.requireBoundedStreaming = true;
    auto token = std::make_shared<std::atomic_bool>(false);
    if (fault == "done") {
        h.respond(ScriptedHttpResponse::streaming({delta, done}));
    } else if (fault == "finish-reason") {
        h.respond(ScriptedHttpResponse::streaming(
            {delta, "data: {\"choices\":[{\"delta\":{},\"finish_reason\":\"stop\"}]}\n\n"}));
    } else if (fault == "malformed") {
        h.respond(ScriptedHttpResponse::streaming({"data: {broken\n\n"}));
    } else if (fault == "error-event") {
        h.respond(ScriptedHttpResponse::streaming(
            {delta, "data: {\"error\":{\"message\":\"PRIVATE_UPSTREAM_ECHO\"}}\n\n"}));
    } else if (fault.startsWith("http")) {
        h.respond(ScriptedHttpResponse::body("PRIVATE_UPSTREAM_ECHO", fault.mid(4).toInt()));
    } else {
        h.respond(ScriptedHttpResponse::streaming({delta}, fault == "unterminated"));
    }
    QString deltas;
    ScopedOnlineNetworkMode online;
    const auto reply = provider->sendMessageStreaming(
        "synthetic stream",
        [&](const QString& text) {
            deltas += text;
            if (fault == "cancel") {
                token->store(true);
            }
        },
        token, requestOptions);
    const bool success = fault == "done" || fault == "finish-reason";
    QCOMPARE(reply.success, success);
    QCOMPARE(h.state->requests.size(), 1);
    const auto recorded = h.state->requests.first();
    QCOMPARE(recorded.url.host(), hostFor(vendor));
    QCOMPARE(recorded.url.path(), pathFor(vendor));
    QCOMPARE(recorded.method, QByteArray("POST"));
    QCOMPARE(recorded.headers.value("authorization"), QByteArray("Bearer ") + QByteArray(key));
    QVERIFY(h.body().value("stream").toBool());
    QCOMPARE(h.body().value("model").toString(), binding.modelId);
    QCOMPARE(h.body().value("max_tokens").toInt(), 32);
    if (success) {
        QCOMPARE(reply.message, QString("first"));
        QCOMPARE(deltas, reply.message);
    } else {
        QVERIFY(reply.lifecycle != ChatRequestLifecycle::Completed);
        QVERIFY(reply.message.isEmpty());
        if (fault == "cancel") {
            QCOMPARE(reply.category, ChatProviderErrorCategory::Cancelled);
        }
        if (fault == "deadline") {
            QCOMPARE(reply.category, ChatProviderErrorCategory::Timeout);
        }
        if (fault.startsWith("http")) {
            const int status = fault.mid(4).toInt();
            const auto expected = status == 400 ? ChatProviderErrorCategory::RequestRejected
                                  : status == 401 || status == 403
                                      ? ChatProviderErrorCategory::AuthenticationRequired
                                  : status == 404 ? ChatProviderErrorCategory::ModelNotFound
                                  : status == 408 ? ChatProviderErrorCategory::Timeout
                                  : status == 429 ? ChatProviderErrorCategory::RateLimited
                                  : status == 501 ? ChatProviderErrorCategory::CapabilityUnsupported
                                                  : ChatProviderErrorCategory::ProviderUnavailable;
            QCOMPARE(reply.category, expected);
        }
        if (fault == "unterminated" || fault == "malformed") {
            QCOMPARE(reply.category, ChatProviderErrorCategory::MalformedResponse);
        }
        QVERIFY(!reply.errorMessage.contains("PRIVATE_"));
        QVERIFY(!reply.errorMessage.contains(key));
        QVERIFY(!QJsonDocument(reply.diagnostics).toJson().contains("PRIVATE_"));
    }
    const auto captured = deltas;
    QCoreApplication::processEvents();
    QCOMPARE(deltas, captured);
}

void VendorCloudContractTest::networkPolicy_data() {
    QTest::addColumn<QString>("vendor");
    QTest::addColumn<bool>("offline");
    for (const auto& vendor : vendors() + QStringList{"claude", "gemini"}) {
        for (bool offline : {false, true}) {
            QTest::newRow(qPrintable(vendor + (offline ? "-offline" : "-local-only")))
                << vendor << offline;
        }
    }
}
void VendorCloudContractTest::networkPolicy() {
    QFETCH(QString, vendor);
    QFETCH(bool, offline);
    Harness h;
    NetworkPolicyService::instance().setMode(offline ? NetworkMode::Offline
                                                     : NetworkMode::LocalOnly);
    auto provider = h.service.constructProvider(h.binding(vendor));
    const auto reply = provider->sendRequest("synthetic", options());
    QCOMPARE(reply.category, ChatProviderErrorCategory::Offline);
    QVERIFY(!reply.success);
    auto streamingOptions = options();
    streamingOptions.requireBoundedStreaming = true;
    const auto streamReply =
        provider->sendMessageStreaming("synthetic", [](const auto&) {}, {}, streamingOptions);
    QVERIFY(!streamReply.success);
    QCOMPARE(h.state->requests.size(), 0);
    QCOMPARE(streamReply.category, ChatProviderErrorCategory::Offline);
}

void VendorCloudContractTest::cloudAlias() {
    QFETCH(QString, vendor);
    Harness h;
    h.settings.setSelectedCloudProvider(vendor == "openai-compatible" ? "openai" : vendor);
    auto binding = h.binding("cloud-api");
    auto provider = h.service.constructProvider(binding);
    ScopedOnlineNetworkMode online;
    const auto reply = provider->sendRequest("synthetic", options());
    QVERIFY(reply.success);
    QCOMPARE(h.state->requests.size(), 1);
    QCOMPARE(h.state->requests.first().url.host(), hostFor(vendor));
    QCOMPARE(h.state->requests.first().url.path(), pathFor(vendor));
}

void VendorCloudContractTest::configurableEndpoint_data() {
    QTest::addColumn<QString>("base");
    QTest::addColumn<QString>("path");
    QTest::newRow("root") << QString("https://fixture.invalid") << QString("/v1/chat/completions");
    QTest::newRow("v1") << QString("https://fixture.invalid/v1") << QString("/v1/chat/completions");
    QTest::newRow("prefix") << QString("https://fixture.invalid/proxy")
                            << QString("/proxy/v1/chat/completions");
    QTest::newRow("query-fragment")
        << QString("https://fixture.invalid/v1?synthetic=discard#fragment")
        << QString("/v1/chat/completions");
}
void VendorCloudContractTest::configurableEndpoint() {
    QFETCH(QString, base);
    QFETCH(QString, path);
    Harness h;
    // Lower-level config only; ModelService openai-compatible has no arbitrary URL setting.
    LMStudioConfig config;
    config.endpoint = QUrl(base);
    config.apiKey = key;
    config.networkFactory = std::make_shared<ScriptedNetworkFactory>(h.state);
    config.timeoutMs = 150;
    const QJsonObject body{
        {"model", "synthetic-generic"},
        {"messages", QJsonArray{QJsonObject{{"role", "user"}, {"content", "synthetic"}}}}};
    const auto blocked = LMStudioLocalInferenceClient(config).completeOpenAiChat(body);
    QVERIFY(!blocked.ok);
    QCOMPARE(h.state->requests.size(), 0);
    ScopedOnlineNetworkMode online;
    const auto reply = LMStudioLocalInferenceClient(config).completeOpenAiChat(body);
    QVERIFY(reply.ok);
    QCOMPARE(h.state->requests.size(), 1);
    QCOMPARE(h.state->requests.first().url.host(), QString("fixture.invalid"));
    QCOMPARE(h.state->requests.first().url.path(), path);
    QVERIFY(h.state->requests.first().url.query().isEmpty());
    QVERIFY(h.state->requests.first().url.fragment().isEmpty());
}

QTEST_GUILESS_MAIN(VendorCloudContractTest)
#include "test_vendor_cloud_contract.moc"
