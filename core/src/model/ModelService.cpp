// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/model/ModelService.h"

#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/chat/OllamaChatProvider.h"
#include "sentinel/core/memory/JsonSettingsStore.h"
#include "sentinel/core/model/IModelRouter.h"
#include "sentinel/core/runtime/LocalInference.h"
#include "sentinel/core/runtime/ProviderRequestRuntime.h"

#include <QDir>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QStandardPaths>

#include <utility>
#include <algorithm>
#include <atomic>
#include <mutex>

namespace sentinel::core {

struct ProviderHealthRegistry {
    struct Entry {
        ProviderHealth health = ProviderHealth::Unknown;
        quint64 sequence = 0;
        QString source;
    };
    std::mutex mutex;
    std::atomic<quint64> nextSequence{0};
    QHash<QString, Entry> entries;
    OllamaModelDiscoveryResult ollamaDiscovery;
    QList<OllamaModelSummary> ollamaModels;
    quint64 ollamaDiscoverySequence = 0;
    struct Catalog {
        QList<OllamaModelSummary> models;
        ProviderDiscoveryOutcome outcome;
        quint64 sequence = 0;
        bool observed = false;
    };
    QHash<QString, Catalog> catalogs;
};

QString providerHealthName(ProviderHealth health) {
    switch (health) {
    case ProviderHealth::Unknown: return QStringLiteral("Unknown");
    case ProviderHealth::Available: return QStringLiteral("Available");
    case ProviderHealth::Degraded: return QStringLiteral("Degraded");
    case ProviderHealth::Unavailable: return QStringLiteral("Unavailable");
    }
    return QStringLiteral("Unknown");
}

QString providerCatalogStateName(ProviderCatalogState state) {
    switch (state) {
    case ProviderCatalogState::Pending: return QStringLiteral("Pending");
    case ProviderCatalogState::Available: return QStringLiteral("Available");
    case ProviderCatalogState::Empty: return QStringLiteral("Empty catalog");
    case ProviderCatalogState::Failed: return QStringLiteral("Discovery failed");
    case ProviderCatalogState::AuthenticationRequired: return QStringLiteral("Authentication required");
    case ProviderCatalogState::EndpointUnavailable: return QStringLiteral("Endpoint unavailable");
    case ProviderCatalogState::Stale: return QStringLiteral("Stale catalog");
    case ProviderCatalogState::ConfiguredModelOnly: return QStringLiteral("Configured model only");
    case ProviderCatalogState::Unverified: return QStringLiteral("Unverified");
    }
    return QStringLiteral("Unverified");
}

namespace {

class HealthTrackedProvider final : public IChatProvider {
public:
    HealthTrackedProvider(QString id, std::shared_ptr<IChatProvider> delegate,
                          std::shared_ptr<ProviderHealthRegistry> registry)
        : id_(id.trimmed().toLower()), delegate_(std::move(delegate)),
          registry_(std::move(registry)) {}
    QString name() const override { return delegate_->name(); }
    ChatProviderStatus status() const override { return delegate_->status(); }
    ChatProviderConcurrency concurrency() const override { return delegate_->concurrency(); }
    bool supportsStreaming() const override { return delegate_->supportsStreaming(); }
    ChatProviderReply sendMessage(const QString& message) override {
        const auto sequence = ++registry_->nextSequence;
        auto reply = delegate_->sendMessage(message);
        update(sequence, reply, QStringLiteral("generation"));
        return reply;
    }
    ChatProviderReply sendRequest(const QString& message,
                                  const ChatRequestOptions& options) override {
        const auto sequence = ++registry_->nextSequence;
        auto reply = delegate_->sendRequest(message, options);
        update(sequence, reply, QStringLiteral("generation"));
        return reply;
    }
    ChatProviderReply sendMessageStreaming(
        const QString& message, const std::function<void(const QString&)>& onDelta,
        const std::shared_ptr<std::atomic_bool>& cancellationToken) override {
        const auto sequence = ++registry_->nextSequence;
        auto reply = delegate_->sendMessageStreaming(message, onDelta, cancellationToken);
        update(sequence, reply, QStringLiteral("stream"));
        return reply;
    }
private:
    void update(quint64 sequence, const ChatProviderReply& reply, const QString& source) {
        std::lock_guard lock(registry_->mutex);
        auto& entry = registry_->entries[id_];
        if (sequence < entry.sequence) return;
        entry.sequence = sequence;
        entry.source = source;
        if (reply.success) entry.health = ProviderHealth::Available;
        else if (reply.category == ChatProviderErrorCategory::RateLimited ||
                 reply.category == ChatProviderErrorCategory::ConnectionFailed ||
                 reply.category == ChatProviderErrorCategory::Timeout ||
                 reply.category == ChatProviderErrorCategory::MalformedResponse)
            entry.health = ProviderHealth::Degraded;
        else if (reply.category == ChatProviderErrorCategory::ProviderUnavailable ||
                 reply.category == ChatProviderErrorCategory::AuthenticationRequired)
            entry.health = ProviderHealth::Unavailable;
    }
    QString id_;
    std::shared_ptr<IChatProvider> delegate_;
    std::shared_ptr<ProviderHealthRegistry> registry_;
};

QString nativeFunctionName(const QString& toolId) {
    static const QRegularExpression valid(QStringLiteral("^[A-Za-z0-9_-]{1,64}$"));
    if (valid.match(toolId).hasMatch())
        return toolId;
    return QStringLiteral("tool_%1").arg(QString::fromLatin1(
        QCryptographicHash::hash(toolId.toUtf8(), QCryptographicHash::Sha256).toHex().left(48)));
}

QJsonObject nativeToolDefinition(const ToolDescriptor& tool) {
    QJsonObject schema = tool.inputSchema;
    if (schema.isEmpty()) {
        QJsonObject properties;
        QJsonArray required;
        for (const auto& parameter : tool.parameters) {
            properties.insert(parameter.id,
                              QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                                          {QStringLiteral("description"), parameter.description}});
            if (parameter.required)
                required.append(parameter.id);
        }
        schema = {{QStringLiteral("type"), QStringLiteral("object")},
                  {QStringLiteral("properties"), properties},
                  {QStringLiteral("required"), required}};
    }
    return {{QStringLiteral("type"), QStringLiteral("function")},
            {QStringLiteral("function"),
             QJsonObject{{QStringLiteral("name"), nativeFunctionName(tool.id)},
                         {QStringLiteral("description"), tool.description.left(1024)},
                         {QStringLiteral("parameters"), schema}}}};
}

ChatProviderErrorCategory categoryFromLocalInference(LocalInferenceError error,
                                                     LocalInferenceStatus status) {
    if (error == LocalInferenceError::Timeout)
        return ChatProviderErrorCategory::Timeout;
    if (error == LocalInferenceError::MissingModel ||
        error == LocalInferenceError::ModelUnavailable ||
        status == LocalInferenceStatus::ModelUnavailable)
        return ChatProviderErrorCategory::ModelNotFound;
    if (error == LocalInferenceError::PermissionDenied)
        return ChatProviderErrorCategory::AuthenticationRequired;
    if (error == LocalInferenceError::InvalidResponse)
        return ChatProviderErrorCategory::MalformedResponse;
    if (error == LocalInferenceError::EndpointUnreachable ||
        error == LocalInferenceError::OllamaNotRunning)
        return ChatProviderErrorCategory::ConnectionFailed;
    if (error == LocalInferenceError::BlankPrompt ||
        error == LocalInferenceError::EndpointBlocked ||
        error == LocalInferenceError::SafetyBlocked)
        return ChatProviderErrorCategory::RequestRejected;
    return ChatProviderErrorCategory::ProviderUnavailable;
}

ChatRequestLifecycle lifecycleForCategory(ChatProviderErrorCategory category) {
    if (category == ChatProviderErrorCategory::Timeout)
        return ChatRequestLifecycle::TimedOut;
    if (category == ChatProviderErrorCategory::RateLimited)
        return ChatRequestLifecycle::RateLimited;
    if (category == ChatProviderErrorCategory::Cancelled)
        return ChatRequestLifecycle::Cancelled;
    return ChatRequestLifecycle::Failed;
}

ChatProviderReply providerFailure(QString detail, ChatProviderErrorCategory category,
                                  ChatProviderReply::Error error =
                                      ChatProviderReply::Error::ProviderFailure,
                                  int httpStatus = 0, int attempts = 1,
                                  QString retrySummary = {}) {
    ChatProviderReply reply;
    reply.success = false;
    reply.errorMessage = std::move(detail);
    reply.error = error;
    reply.category = category;
    reply.lifecycle = lifecycleForCategory(category);
    reply.httpStatus = httpStatus;
    reply.attempts = qMax(1, attempts);
    reply.retrySummary = std::move(retrySummary);
    return reply;
}

ChatProviderReply nativeEndpointReply(
    LMStudioLocalInferenceClient::NativeProtocol protocol, const ModelBinding& binding,
    const LMStudioConfig& config, int timeoutMs, const QString& message,
    const ChatRequestOptions& options) {
    const bool claude = protocol == LMStudioLocalInferenceClient::NativeProtocol::Claude;
    const auto malformed = [](const QString& detail) {
        return providerFailure(detail, ChatProviderErrorCategory::MalformedResponse,
                               ChatProviderReply::Error::InvalidResponse);
    };
    if (options.structuredOutput && options.structuredSchema.isEmpty())
        return malformed(QStringLiteral("Native structured schema is missing."));
    if (options.priorToolCalls.size() != options.toolResults.size())
        return malformed(QStringLiteral("Native tool results are incomplete."));
    for (const auto& call : options.priorToolCalls)
        if (std::none_of(options.tools.cbegin(), options.tools.cend(),
                         [&](const ToolDescriptor& tool) {
                             return tool.enabled && tool.exposedToModel && tool.id == call.toolId;
                         }))
            return malformed(QStringLiteral("Native tool continuation is not registered."));

    QJsonObject body{{QStringLiteral("model"), binding.modelId}};
    QJsonArray initialParts{QJsonObject{{QStringLiteral("text"), message}}};
    if (claude) {
        QJsonArray messages{QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                        {QStringLiteral("content"), message}}};
        if (!options.priorToolCalls.isEmpty()) {
            QJsonParseError historyError;
            const auto history = QJsonDocument::fromJson(
                options.priorToolCalls.first().providerContinuation.toUtf8(), &historyError);
            if (historyError.error != QJsonParseError::NoError || !history.isArray() ||
                history.array().isEmpty())
                return malformed(QStringLiteral("Claude tool continuation history is missing."));
            for (const auto& call : options.priorToolCalls)
                if (call.providerContinuation != options.priorToolCalls.first().providerContinuation)
                    return malformed(QStringLiteral("Claude tool continuation histories differ."));
            messages = history.array();
            QJsonArray results;
            for (int i = 0; i < options.priorToolCalls.size(); ++i) {
                const auto& call = options.priorToolCalls.at(i);
                const auto& result = options.toolResults.at(i);
                if (call.callId.isEmpty() || call.callId != result.callId)
                    return malformed(QStringLiteral("Native tool result ID mismatch."));
                results.append(QJsonObject{{QStringLiteral("type"), QStringLiteral("tool_result")},
                                           {QStringLiteral("tool_use_id"), call.callId},
                                           {QStringLiteral("content"), result.content.left(6800)}});
            }
            messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                        {QStringLiteral("content"), results}});
        }
        body.insert(QStringLiteral("messages"), messages);
        body.insert(QStringLiteral("max_tokens"), 2048);
        if (options.nativeToolCalling) {
            QJsonArray tools;
            for (const auto& tool : options.tools) {
                if (!tool.enabled || !tool.exposedToModel) continue;
                const auto function = nativeToolDefinition(tool)
                                          .value(QStringLiteral("function")).toObject();
                tools.append(QJsonObject{{QStringLiteral("name"), function.value(QStringLiteral("name"))},
                                         {QStringLiteral("description"), function.value(QStringLiteral("description"))},
                                         {QStringLiteral("input_schema"), function.value(QStringLiteral("parameters"))}});
            }
            if (!tools.isEmpty()) body.insert(QStringLiteral("tools"), tools);
        }
        if (options.structuredOutput)
            body.insert(QStringLiteral("output_config"),
                        QJsonObject{{QStringLiteral("format"),
                                     QJsonObject{{QStringLiteral("type"), QStringLiteral("json_schema")},
                                                 {QStringLiteral("schema"), options.structuredSchema}}}});
    } else {
        QJsonArray contents{QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                        {QStringLiteral("parts"), initialParts}}};
        if (!options.priorToolCalls.isEmpty()) {
            QJsonParseError historyError;
            const auto history = QJsonDocument::fromJson(
                options.priorToolCalls.first().providerContinuation.toUtf8(), &historyError);
            if (historyError.error != QJsonParseError::NoError || !history.isArray() ||
                history.array().isEmpty())
                return malformed(QStringLiteral("Gemini function continuation history is missing."));
            for (const auto& call : options.priorToolCalls)
                if (call.providerContinuation != options.priorToolCalls.first().providerContinuation)
                    return malformed(QStringLiteral("Gemini function continuation histories differ."));
            contents = history.array();
            QJsonArray results;
            for (int i = 0; i < options.priorToolCalls.size(); ++i) {
                const auto& call = options.priorToolCalls.at(i);
                const auto& result = options.toolResults.at(i);
                if (call.callId.isEmpty() || call.callId != result.callId)
                    return malformed(QStringLiteral("Native function result ID mismatch."));
                QJsonObject functionResponse{{QStringLiteral("name"), nativeFunctionName(call.toolId)},
                                             {QStringLiteral("response"),
                                              QJsonObject{{QStringLiteral("output"), result.content.left(6800)}}}};
                if (!call.callId.startsWith(QLatin1String("sentinel-gemini-"))) {
                    functionResponse.insert(QStringLiteral("id"), call.callId);
                }
                results.append(QJsonObject{{QStringLiteral("functionResponse"), functionResponse}});
            }
            contents.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                        {QStringLiteral("parts"), results}});
        }
        body.insert(QStringLiteral("contents"), contents);
        if (options.nativeToolCalling) {
            QJsonArray declarations;
            for (const auto& tool : options.tools) {
                if (!tool.enabled || !tool.exposedToModel) continue;
                const auto function = nativeToolDefinition(tool)
                                          .value(QStringLiteral("function")).toObject();
                declarations.append(QJsonObject{{QStringLiteral("name"), function.value(QStringLiteral("name"))},
                                                 {QStringLiteral("description"), function.value(QStringLiteral("description"))},
                                                 {QStringLiteral("parameters"), function.value(QStringLiteral("parameters"))}});
            }
            if (!declarations.isEmpty())
                body.insert(QStringLiteral("tools"),
                            QJsonArray{QJsonObject{{QStringLiteral("functionDeclarations"), declarations}}});
        }
        if (options.structuredOutput)
            body.insert(QStringLiteral("generationConfig"),
                        QJsonObject{{QStringLiteral("responseMimeType"), QStringLiteral("application/json")},
                                    {QStringLiteral("responseSchema"), options.structuredSchema}});
    }

    LMStudioLocalInferenceClient client(config, timeoutMs);
    const auto completion = client.completeNativeChat(protocol, body, options.cancellationToken);
    if (!completion.ok) {
        const auto classified = completion.cancelled ? ChatProviderErrorCategory::Cancelled
            : completion.malformed ? ChatProviderErrorCategory::MalformedResponse
            : completion.httpStatus == 400 || completion.httpStatus == 422 || completion.httpStatus == 501
                ? ChatProviderErrorCategory::CapabilityUnsupported
            : completion.providerErrorCategory > 0
                ? static_cast<ChatProviderErrorCategory>(completion.providerErrorCategory)
                : ProviderRequestRuntime::classify(completion.httpStatus, completion.networkError,
                                                   completion.timedOut, completion.cancelled);
        const auto category = classified == ChatProviderErrorCategory::None
            ? ChatProviderErrorCategory::ProviderUnavailable : classified;
        auto reply = providerFailure(completion.error, category,
                                     category == ChatProviderErrorCategory::CapabilityUnsupported
                                         ? ChatProviderReply::Error::CapabilityRejected
                                         : ChatProviderReply::Error::ProviderFailure,
                                     completion.httpStatus, completion.attempts,
                                     completion.retrySummary);
        reply.requestId = completion.requestId;
        return reply;
    }
    ChatProviderReply result;
    result.success = true;
    result.lifecycle = ChatRequestLifecycle::Completed;
    result.httpStatus = completion.httpStatus;
    result.attempts = completion.attempts;
    result.retrySummary = completion.retrySummary;
    result.requestId = completion.requestId;
    const auto invalid = [&](const QString& detail) {
        auto reply = malformed(detail);
        reply.httpStatus = completion.httpStatus;
        reply.attempts = completion.attempts;
        reply.retrySummary = completion.retrySummary;
        reply.requestId = completion.requestId;
        return reply;
    };
    QJsonArray parts;
    QString claudeContinuation;
    QString geminiContinuation;
    if (claude) {
        parts = completion.body.value(QStringLiteral("content")).toArray();
        auto history = body.value(QStringLiteral("messages")).toArray();
        history.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("assistant")},
                                   {QStringLiteral("content"), parts}});
        claudeContinuation = QString::fromUtf8(
            QJsonDocument(history).toJson(QJsonDocument::Compact));
    } else {
        const auto candidates = completion.body.value(QStringLiteral("candidates")).toArray();
        if (candidates.isEmpty() &&
            !completion.body.value(QStringLiteral("promptFeedback"))
                 .toObject().value(QStringLiteral("blockReason")).toString().isEmpty()) {
            auto reply = providerFailure(QStringLiteral("Gemini blocked the request."),
                                         ChatProviderErrorCategory::RequestRejected,
                                         ChatProviderReply::Error::ProviderFailure,
                                         completion.httpStatus, completion.attempts,
                                         completion.retrySummary);
            reply.requestId = completion.requestId;
            return reply;
        }
        if (!candidates.isEmpty()) {
            const auto candidate = candidates.first().toObject();
            const auto finishReason = candidate.value(QStringLiteral("finishReason")).toString();
            if (finishReason == QLatin1String("SAFETY") ||
                finishReason == QLatin1String("PROHIBITED_CONTENT")) {
                auto reply = providerFailure(QStringLiteral("Gemini rejected the response."),
                                             ChatProviderErrorCategory::RequestRejected,
                                             ChatProviderReply::Error::ProviderFailure,
                                             completion.httpStatus, completion.attempts,
                                             completion.retrySummary);
                reply.requestId = completion.requestId;
                return reply;
            }
            const auto modelContent = candidate.value(QStringLiteral("content")).toObject();
            parts = modelContent.value(QStringLiteral("parts")).toArray();
            auto history = body.value(QStringLiteral("contents")).toArray();
            history.append(modelContent);
            geminiContinuation = QString::fromUtf8(
                QJsonDocument(history).toJson(QJsonDocument::Compact));
        }
    }
    if (parts.isEmpty()) return invalid(QStringLiteral("Native provider response has no content."));
    int generatedId = 0;
    for (const auto& value : parts) {
        const auto part = value.toObject();
        if (claude) {
            const auto type = part.value(QStringLiteral("type")).toString();
            if (type == QLatin1String("text")) {
                result.message += part.value(QStringLiteral("text")).toString();
                continue;
            }
            if (type != QLatin1String("tool_use")) continue;
        } else if (part.contains(QStringLiteral("text"))) {
            result.message += part.value(QStringLiteral("text")).toString();
            continue;
        } else if (!part.contains(QStringLiteral("functionCall"))) {
            continue;
        }
        const auto call = claude ? part : part.value(QStringLiteral("functionCall")).toObject();
        const auto name = call.value(QStringLiteral("name")).toString();
        const auto arguments = call.value(claude ? QStringLiteral("input")
                                               : QStringLiteral("args"));
        const auto callId = claude ? call.value(QStringLiteral("id")).toString()
            : call.value(QStringLiteral("id")).toString().isEmpty()
                ? QStringLiteral("sentinel-gemini-%1-%2").arg(completion.requestId).arg(++generatedId)
                : call.value(QStringLiteral("id")).toString();
        QString toolId;
        for (const auto& tool : options.tools)
            if (tool.enabled && tool.exposedToModel && nativeFunctionName(tool.id) == name) {
                toolId = tool.id;
                break;
            }
        if (!options.nativeToolCalling || toolId.isEmpty() || callId.isEmpty() ||
            !arguments.isObject())
            return invalid(QStringLiteral("Invalid or unregistered native function call."));
        const auto signature = claude ? QString{}
            : part.value(QStringLiteral("thoughtSignature")).toString();
        if (!claude && binding.modelId.startsWith(QLatin1String("gemini-3")) &&
            result.toolCalls.isEmpty() && signature.isEmpty())
            return invalid(QStringLiteral("Gemini function response lacks a continuation signature."));
        result.toolCalls.append({callId, toolId, arguments.toObject(),
                                 claude ? claudeContinuation : geminiContinuation});
    }
    if (options.structuredOutput && result.toolCalls.isEmpty()) {
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(result.message.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
            return invalid(QStringLiteral("Native structured response is malformed."));
        result.structuredResult = document.object();
    }
    if (result.message.isEmpty() && result.toolCalls.isEmpty())
        return invalid(QStringLiteral("Native provider response has no text or function calls."));
    return result;
}

// IChatProvider implementation for every non-Ollama provider id. The binding
// fixes the model for the whole request/run; configuration comes from the
// persisted endpoint and credential settings resolved by ModelService.
class SelectedEndpointChatProvider final : public IChatProvider {
public:
    SelectedEndpointChatProvider(ModelBinding binding, LMStudioConfig config, int timeoutMs)
        : binding_(std::move(binding)), config_(std::move(config)), timeoutMs_(timeoutMs) {}

    QString name() const override {
        return binding_.providerId;
    }
    ChatProviderStatus status() const override {
        return config_.isAllowedEndpoint() ? ChatProviderStatus::Ready
                                           : ChatProviderStatus::Unavailable;
    }
    ChatProviderConcurrency concurrency() const override {
        return ChatProviderConcurrency::Supported;
    }
    bool supportsStreaming() const override {
        return binding_.capabilities.streaming == CapabilitySupport::Supported;
    }
    ChatProviderReply sendMessage(const QString& message) override {
        return sendMessageWithToken(message, {});
    }
    ChatProviderReply sendMessageWithToken(
        const QString& message,
        const std::shared_ptr<std::atomic_bool>& cancellationToken) {
        if (!config_.isAllowedEndpoint())
            return providerFailure(
                QStringLiteral("Selected provider '%1' is unavailable or not configured.")
                    .arg(binding_.providerId),
                ChatProviderErrorCategory::ProviderUnavailable);
        LocalInferenceRequest request;
        request.prompt = message;
        request.options.model = binding_.modelId;
        request.options.timeoutMs = timeoutMs_;
        request.options.cancellationToken = cancellationToken;
        LMStudioLocalInferenceClient client(config_, timeoutMs_);
        const auto result = client.infer(request);
        if (result.status == LocalInferenceStatus::Succeeded) {
            ChatProviderReply reply{true, result.text, {}};
            reply.lifecycle = ChatRequestLifecycle::Completed;
            reply.attempts = result.attempts;
            reply.retrySummary = result.retrySummary;
            reply.requestId = result.requestId;
            return reply;
        }
        if (result.error == LocalInferenceError::ModelUnavailable)
            return providerFailure(
                QStringLiteral("Selected model '%1' is unavailable from provider '%2'.")
                    .arg(binding_.modelId, binding_.providerId),
                ChatProviderErrorCategory::ModelNotFound);
        auto reply = providerFailure(QStringLiteral("Provider '%1': %2")
                                   .arg(binding_.providerId, result.summary),
                               result.providerErrorCategory > 0
                                   ? static_cast<ChatProviderErrorCategory>(result.providerErrorCategory)
                                   : categoryFromLocalInference(result.error, result.status),
                               ChatProviderReply::Error::ProviderFailure, result.httpStatus,
                               result.attempts, result.retrySummary);
        reply.requestId = result.requestId;
        return reply;
    }

    ChatProviderReply sendRequest(const QString& message,
                                  const ChatRequestOptions& options) override {
        if ((options.nativeToolCalling &&
             binding_.capabilities.nativeToolCalling != CapabilitySupport::Supported) ||
            (options.structuredOutput &&
             binding_.capabilities.structuredOutput != CapabilitySupport::Supported) ||
            (options.nativeToolCalling && options.structuredOutput &&
             binding_.capabilities.combinedToolsAndStructuredOutput != CapabilitySupport::Supported))
            return providerFailure(QStringLiteral("Selected model does not support the requested native capability."),
                                   ChatProviderErrorCategory::CapabilityUnsupported,
                                   ChatProviderReply::Error::CapabilityRejected);
        if (!options.nativeToolCalling && !options.structuredOutput)
            return sendMessageWithToken(message, options.cancellationToken);
        if (!config_.isAllowedEndpoint())
            return providerFailure(QStringLiteral("Selected endpoint is unavailable."),
                                   ChatProviderErrorCategory::ProviderUnavailable);
        const auto host = config_.endpoint.host().toLower();
        if (host == QLatin1String("api.anthropic.com"))
            return nativeEndpointReply(LMStudioLocalInferenceClient::NativeProtocol::Claude,
                                       binding_, config_, timeoutMs_, message, options);
        if (host == QLatin1String("generativelanguage.googleapis.com"))
            return nativeEndpointReply(LMStudioLocalInferenceClient::NativeProtocol::Gemini,
                                       binding_, config_, timeoutMs_, message, options);
        QJsonArray messages{
            QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                        {QStringLiteral("content"), message}}};
        if (!options.priorToolCalls.isEmpty()) {
            if (options.priorToolCalls.size() != options.toolResults.size())
                return providerFailure(QStringLiteral("Native tool results are incomplete."),
                                       ChatProviderErrorCategory::MalformedResponse,
                                       ChatProviderReply::Error::InvalidResponse);
            QJsonArray calls;
            for (const auto& call : options.priorToolCalls) {
                calls.append(QJsonObject{
                    {QStringLiteral("id"), call.callId},
                    {QStringLiteral("type"), QStringLiteral("function")},
                    {QStringLiteral("function"), QJsonObject{
                        {QStringLiteral("name"), nativeFunctionName(call.toolId)},
                        {QStringLiteral("arguments"), QString::fromUtf8(
                            QJsonDocument(call.arguments).toJson(QJsonDocument::Compact))}}}});
            }
            messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("assistant")},
                                        {QStringLiteral("tool_calls"), calls}});
            for (int i = 0; i < options.toolResults.size(); ++i) {
                const auto& result = options.toolResults.at(i);
                if (result.callId != options.priorToolCalls.at(i).callId)
                    return providerFailure(QStringLiteral("Native tool result IDs do not match."),
                                           ChatProviderErrorCategory::MalformedResponse,
                                           ChatProviderReply::Error::InvalidResponse);
                messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("tool")},
                                            {QStringLiteral("tool_call_id"), result.callId},
                                            {QStringLiteral("content"), result.content.left(6800)}});
            }
        }
        QJsonObject body{{QStringLiteral("model"), binding_.modelId},
                         {QStringLiteral("messages"), messages},
                         {QStringLiteral("stream"), false}};
        if (options.nativeToolCalling) {
            QJsonArray tools;
            for (const auto& tool : options.tools)
                if (tool.enabled && tool.exposedToModel)
                    tools.append(nativeToolDefinition(tool));
            if (!tools.isEmpty()) {
                body.insert(QStringLiteral("tools"), tools);
                body.insert(QStringLiteral("tool_choice"), QStringLiteral("auto"));
            }
        }
        if (options.structuredOutput) {
            if (options.structuredSchemaName.isEmpty() || options.structuredSchema.isEmpty())
                return providerFailure(QStringLiteral("Structured planner schema is missing."),
                                       ChatProviderErrorCategory::MalformedResponse,
                                       ChatProviderReply::Error::InvalidResponse);
            body.insert(QStringLiteral("response_format"),
                        QJsonObject{
                            {QStringLiteral("type"), QStringLiteral("json_schema")},
                            {QStringLiteral("json_schema"), QJsonObject{
                                {QStringLiteral("name"), options.structuredSchemaName},
                                {QStringLiteral("strict"), options.strictStructuredOutput},
                                {QStringLiteral("schema"), options.structuredSchema}}}});
        }
        LMStudioLocalInferenceClient client(config_, timeoutMs_);
        const auto completion = client.completeOpenAiChat(body, options.cancellationToken);
        if (!completion.ok) {
            const bool rejected = (options.nativeToolCalling || options.structuredOutput) &&
                (completion.httpStatus == 400 || completion.httpStatus == 422 ||
                 completion.httpStatus == 501);
            auto reply = providerFailure(
                completion.error,
                completion.cancelled ? ChatProviderErrorCategory::Cancelled
                         : rejected ? ChatProviderErrorCategory::CapabilityUnsupported
                         : completion.malformed ? ChatProviderErrorCategory::MalformedResponse
                         : completion.providerErrorCategory > 0
                             ? static_cast<ChatProviderErrorCategory>(completion.providerErrorCategory)
                             : ProviderRequestRuntime::classify(
                                   completion.httpStatus, completion.networkError,
                                   completion.timedOut, completion.cancelled),
                rejected ? ChatProviderReply::Error::CapabilityRejected
                         : ChatProviderReply::Error::ProviderFailure,
                completion.httpStatus, completion.attempts, completion.retrySummary);
            reply.requestId = completion.requestId;
            return reply;
        }
        const auto choices = completion.body.value(QStringLiteral("choices")).toArray();
        if (choices.isEmpty()) {
            auto reply = providerFailure(QStringLiteral("Chat completion has no choices."),
                                   ChatProviderErrorCategory::MalformedResponse,
                                   ChatProviderReply::Error::InvalidResponse, completion.httpStatus,
                                   completion.attempts, completion.retrySummary);
            reply.requestId = completion.requestId;
            return reply;
        }
        const auto replyMessage = choices.first().toObject().value(QStringLiteral("message")).toObject();
        ChatProviderReply result;
        result.success = true;
        result.lifecycle = ChatRequestLifecycle::Completed;
        result.attempts = completion.attempts;
        result.retrySummary = completion.retrySummary;
        result.requestId = completion.requestId;
        result.message = replyMessage.value(QStringLiteral("content")).toString();
        const auto malformed = [&](const QString& detail) {
            auto reply = providerFailure(detail, ChatProviderErrorCategory::MalformedResponse,
                                         ChatProviderReply::Error::InvalidResponse,
                                         completion.httpStatus, completion.attempts,
                                         completion.retrySummary);
            reply.requestId = completion.requestId;
            return reply;
        };
        for (const auto& value : replyMessage.value(QStringLiteral("tool_calls")).toArray()) {
            const auto call = value.toObject();
            const auto function = call.value(QStringLiteral("function")).toObject();
            const auto name = function.value(QStringLiteral("name")).toString();
            QString toolId;
            for (const auto& tool : options.tools)
                if (nativeFunctionName(tool.id) == name) {
                    toolId = tool.id;
                    break;
                }
            QJsonParseError parseError;
            const auto arguments = QJsonDocument::fromJson(
                function.value(QStringLiteral("arguments")).toString().toUtf8(), &parseError);
            const auto callId = call.value(QStringLiteral("id")).toString();
            if (toolId.isEmpty() || callId.isEmpty() || !arguments.isObject() ||
                parseError.error != QJsonParseError::NoError)
                return malformed(QStringLiteral("Invalid or unregistered native tool call."));
            result.toolCalls.append({callId, toolId, arguments.object()});
        }
        if (options.structuredOutput && result.toolCalls.isEmpty()) {
            QJsonParseError parseError;
            const auto document = QJsonDocument::fromJson(result.message.toUtf8(), &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject())
                return malformed(QStringLiteral("Invalid native structured response."));
            result.structuredResult = document.object();
        }
        if (result.message.isEmpty() && result.toolCalls.isEmpty())
            return malformed(QStringLiteral("Chat completion returned no content or tool calls."));
        return result;
    }

    ChatProviderReply sendMessageStreaming(
        const QString& message, const std::function<void(const QString&)>& onDelta,
        const std::shared_ptr<std::atomic_bool>& cancellationToken) override {
        if (binding_.capabilities.streaming != CapabilitySupport::Supported)
            return providerFailure(QStringLiteral("Selected model does not support streaming."),
                                   ChatProviderErrorCategory::CapabilityUnsupported,
                                   ChatProviderReply::Error::CapabilityRejected);
        LocalInferenceRequest request;
        request.prompt = message;
        request.options.model = binding_.modelId;
        request.options.timeoutMs = timeoutMs_;
        request.options.cancellationToken = cancellationToken;
        LMStudioLocalInferenceStreamClient client(config_, timeoutMs_);
        const auto result = client.startStream(request, [&](const LocalInferenceStreamChunk& chunk) {
            if (!chunk.text.isEmpty() && onDelta) onDelta(chunk.text);
        });
        if (result.status == LocalInferenceStreamStatus::Completed) {
            ChatProviderReply reply{true, result.accumulatedText, {}};
            reply.lifecycle = ChatRequestLifecycle::Completed;
            reply.requestId = result.requestId;
            return reply;
        }
        auto reply = providerFailure(result.summary,
                                     result.providerErrorCategory > 0
                                         ? static_cast<ChatProviderErrorCategory>(result.providerErrorCategory)
                                         : result.cancelled ? ChatProviderErrorCategory::Cancelled
                                         : categoryFromLocalInference(result.error, LocalInferenceStatus::Error),
                                     ChatProviderReply::Error::ProviderFailure,
                                     result.httpStatus, result.attempts, result.retrySummary);
        reply.lifecycle = result.lifecycle;
        reply.requestId = result.requestId;
        return reply;
    }

private:
    ModelBinding binding_;
    LMStudioConfig config_;
    int timeoutMs_ = 0;
};

QString normalizedProviderId(const QString& providerId) {
    return providerId.trimmed().toLower();
}

bool isCloudProviderId(const QString& providerId) {
    static const QStringList cloudIds{
        QStringLiteral("cloud-api"), QStringLiteral("openai"), QStringLiteral("openai-compatible"),
        QStringLiteral("claude"),    QStringLiteral("gemini"), QStringLiteral("deepseek"),
        QStringLiteral("groq"),      QStringLiteral("mistral")};
    return cloudIds.contains(providerId);
}

QString defaultProviderId() {
    return QStringLiteral("ollama");
}

ModelCapabilities mergedCapabilities(ModelCapabilities base, const ModelCapabilities& override) {
    auto merge = [](CapabilitySupport& target, CapabilitySupport value) {
        if (value != CapabilitySupport::Unknown)
            target = value;
    };
    merge(base.streaming, override.streaming);
    merge(base.structuredOutput, override.structuredOutput);
    merge(base.nativeToolCalling, override.nativeToolCalling);
    merge(base.combinedToolsAndStructuredOutput, override.combinedToolsAndStructuredOutput);
    merge(base.visionInput, override.visionInput);
    merge(base.audioInput, override.audioInput);
    merge(base.audioOutput, override.audioOutput);
    if (override.contextWindow && *override.contextWindow > 0)
        base.contextWindow = override.contextWindow;
    if (override.maxOutputTokens && *override.maxOutputTokens > 0)
        base.maxOutputTokens = override.maxOutputTokens;
    return base;
}

} // namespace

QString modelBindingErrorName(ModelBindingError error) {
    switch (error) {
    case ModelBindingError::None:
        return QStringLiteral("None");
    case ModelBindingError::ProviderNotFound:
        return QStringLiteral("ProviderNotFound");
    case ModelBindingError::ProviderUnavailable:
        return QStringLiteral("ProviderUnavailable");
    case ModelBindingError::ModelNotFound:
        return QStringLiteral("ModelNotFound");
    case ModelBindingError::ConfigurationInvalid:
        return QStringLiteral("ConfigurationInvalid");
    case ModelBindingError::AuthenticationRequired:
        return QStringLiteral("AuthenticationRequired");
    }

    return QStringLiteral("None");
}

QString modelBindingFailureSummary(const ModelBindingResolution& resolution) {
    const auto providerId = resolution.binding.providerId.isEmpty() ? QStringLiteral("<none>")
                                                                    : resolution.binding.providerId;
    const auto modelId = resolution.binding.modelId.isEmpty() ? QStringLiteral("<none>")
                                                              : resolution.binding.modelId;
    return QStringLiteral("Model binding failed for provider '%1' / model '%2': %3 (%4)")
        .arg(providerId, modelId, resolution.reason, modelBindingErrorName(resolution.error));
}

ModelService::ModelService(AppSettings* settings, QObject* parent)
    : QObject(parent), settings_(settings),
      ollamaEndpoint_(OllamaEndpoint::defaultEndpoint().toString()),
      lmStudioEndpoint_(QStringLiteral("http://127.0.0.1:1234")),
      llamaCppEndpoint_(QStringLiteral("http://127.0.0.1:8080")) {
    providerHealthRegistry_ = std::make_shared<ProviderHealthRegistry>();
    providerHealthRegistry_->ollamaDiscovery.lifecycle = ChatRequestLifecycle::Pending;
    providerHealthRegistry_->ollamaDiscovery.errorCategory = ChatProviderErrorCategory::None;
    providerHealthRegistry_->ollamaDiscovery.safeDetail =
        QStringLiteral("Ollama model discovery pending.");
    if (settings_) {
        selection_.providerId = normalizedProviderId(settings_->selectedRuntimeProvider());
        selection_.modelId = settings_->selectedModelForProvider(selection_.providerId).trimmed();
        connect(settings_, &AppSettings::cloudApiKeysChanged, this, [this] {
            {
                std::lock_guard lock(providerHealthRegistry_->mutex);
                for (const auto& id : {"cloud-api", "openai", "openai-compatible", "claude",
                                       "gemini", "deepseek", "groq", "mistral"})
                    providerHealthRegistry_->catalogs.remove(QString::fromLatin1(id));
            }
            emit providerRegistryChanged();
        });
        connect(settings_, &AppSettings::selectedCloudProviderChanged, this, [this] {
            {
                std::lock_guard lock(providerHealthRegistry_->mutex);
                providerHealthRegistry_->catalogs.remove(QStringLiteral("cloud-api"));
            }
            emit providerRegistryChanged();
            emit modelCapabilitiesChanged();
        });
    } else {
        selection_.providerId = defaultProviderId();
    }

    ModelCapabilities ollamaCapabilities;
    ollamaCapabilities.streaming = CapabilitySupport::Supported;
    ollamaCapabilities.structuredOutput = CapabilitySupport::Unsupported;
    ollamaCapabilities.nativeToolCalling = CapabilitySupport::Unsupported;
    registerProvider(defaultProviderId(), [this](const ModelBinding& binding) {
        auto provider = std::make_shared<OllamaChatProvider>(
            OllamaConfig::fromEndpoint(ollamaEndpoint_), localInferenceTimeoutMs_);
        provider->setSelectedModel(binding.modelId);
        provider->setDiscoverySnapshot(ollamaDiscovery());
        return std::shared_ptr<IChatProvider>(std::move(provider));
    }, ollamaCapabilities);
    static const QStringList endpointProviderIds{
        QStringLiteral("lm-studio"),        QStringLiteral("openai-compatible-local"),
        QStringLiteral("llama-cpp-server"), QStringLiteral("cloud-api"),
        QStringLiteral("openai"),           QStringLiteral("openai-compatible"),
        QStringLiteral("claude"),           QStringLiteral("gemini"),
        QStringLiteral("deepseek"),         QStringLiteral("groq"),
        QStringLiteral("mistral")};
    for (const auto& providerId : endpointProviderIds) {
        ModelCapabilities endpointCapabilities;
        endpointCapabilities.streaming = CapabilitySupport::Supported;
        endpointCapabilities.structuredOutput = CapabilitySupport::Unsupported;
        endpointCapabilities.nativeToolCalling = CapabilitySupport::Unsupported;
        if (providerId == QLatin1String("claude") || providerId == QLatin1String("gemini")) {
            endpointCapabilities.nativeToolCalling = CapabilitySupport::Supported;
            endpointCapabilities.structuredOutput = CapabilitySupport::Unknown;
        }
        registerProvider(providerId, [this](const ModelBinding& binding) {
            return std::make_shared<SelectedEndpointChatProvider>(binding, providerConfig(binding),
                                                                  localInferenceTimeoutMs_);
        }, endpointCapabilities);
    }
}

ModelService::~ModelService() = default;

void ModelService::setModelRouter(IModelRouter* router) {
    router_ = router;
}

void ModelService::registerProvider(const QString& providerId, ModelProviderFactory factory,
                                    ModelCapabilities defaults) {
    const auto normalized = normalizedProviderId(providerId);
    if (normalized.isEmpty() || !factory)
        return;
    providerFactories_.insert(normalized, std::move(factory));
    providerCapabilities_.insert(normalized, std::move(defaults));
    emit providerRegistryChanged();
}

void ModelService::setModelCapabilities(const QString& providerId, const QString& modelId,
                                         ModelCapabilities capabilities) {
    const auto provider = normalizedProviderId(providerId);
    const auto model = modelId.trimmed();
    if (!provider.isEmpty() && !model.isEmpty()) {
        modelCapabilities_[provider].insert(model, std::move(capabilities));
        emit modelCapabilitiesChanged();
    }
}

ModelCapabilities ModelService::capabilityOverrides(const QString& providerId,
                                                     const QString& modelId) const {
    return settings_ ? settings_->modelCapabilitiesOverride(providerId, modelId)
                     : ModelCapabilities{};
}

void ModelService::setCapabilityOverrides(const QString& providerId, const QString& modelId,
                                           const ModelCapabilities& overrides) {
    if (!settings_ || providerId.trimmed().isEmpty() || modelId.trimmed().isEmpty())
        return;
    settings_->setModelCapabilitiesOverride(providerId, modelId, overrides);
    emit modelCapabilitiesChanged();
}

void ModelService::clearCapabilityOverrides(const QString& providerId, const QString& modelId) {
    setCapabilityOverrides(providerId, modelId, {});
}

ModelCapabilities ModelService::capabilities(const QString& providerId,
                                              const QString& modelId) const {
    const auto provider = normalizedProviderId(providerId);
    auto result = providerCapabilities_.value(provider);
    if (provider == QLatin1String("cloud-api")) {
        const auto host = providerConfig(ModelBinding{provider, modelId.trimmed()})
                              .endpoint.host().toLower();
        if (host == QLatin1String("api.anthropic.com") ||
            host == QLatin1String("generativelanguage.googleapis.com")) {
            result.nativeToolCalling = CapabilitySupport::Supported;
            result.structuredOutput = CapabilitySupport::Unknown;
        }
    }
    if (router_) {
        const auto route = router_->resolveSelection(ModelBinding{provider, modelId.trimmed()});
        if (route.status == ModelRoutingStatus::Routed) {
            if (route.provider.id == provider)
                result = mergedCapabilities(result, route.provider.modelCapabilities);
            if (route.model.providerId == provider && route.model.id == modelId.trimmed()) {
                result = mergedCapabilities(result, route.model.capabilities);
                if (route.model.contextWindowTokens > 0 && !result.contextWindow)
                    result.contextWindow = route.model.contextWindowTokens;
            }
        }
    }
    if (settings_)
        result = mergedCapabilities(result,
                                    settings_->modelCapabilitiesOverride(provider, modelId.trimmed()));
    return mergedCapabilities(result,
                              modelCapabilities_.value(provider).value(modelId.trimmed()));
}

bool ModelService::isKnownProvider(const QString& providerId) const {
    return providerFactories_.contains(normalizedProviderId(providerId));
}

QStringList ModelService::knownProviderIds() const {
    auto ids = providerFactories_.keys();
    ids.sort();
    return ids;
}

ModelSelection ModelService::selectedModel() const {
    return selection_;
}

void ModelService::setSelectedModel(const ModelSelection& selection) {
    setSelectedProviderId(selection.providerId);
    setSelectedModelId(selection.modelId);
}

void ModelService::setSelectedProviderId(const QString& providerId) {
    const auto normalized = normalizedProviderId(providerId);
    const auto selected =
        providerFactories_.contains(normalized) ? normalized : defaultProviderId();
    if (selected == selection_.providerId)
        return;
    selection_.providerId = selected;
    if (settings_)
        settings_->setSelectedRuntimeProvider(selected);
    emit selectedModelChanged();
}

void ModelService::setSelectedModelId(const QString& modelId) {
    const auto normalized = modelId.trimmed();
    if (normalized == selection_.modelId)
        return;
    selection_.modelId = normalized;
    if (settings_ && !selection_.providerId.isEmpty())
        settings_->setSelectedModelForProvider(selection_.providerId, normalized);
    emit selectedModelChanged();
}

ModelBindingResolution ModelService::resolve(const ModelSelection& selection) {
    return resolve(selection.providerId, selection.modelId);
}

ModelBindingResolution ModelService::resolve(const QString& providerId, const QString& modelId) {
    ModelBindingResolution resolution;
    const auto provider = normalizedProviderId(providerId);
    const auto model = modelId.trimmed();
    resolution.binding = ModelBinding{provider, model};
    resolution.selection = ModelSelection{provider, model};

    if (provider.isEmpty() || !providerFactories_.contains(provider)) {
        resolution.error = ModelBindingError::ProviderNotFound;
        resolution.reason = provider.isEmpty()
                                ? QStringLiteral("No provider is configured for this request.")
                                : QStringLiteral("Provider '%1' is not configured.").arg(provider);
        return resolution;
    }
    if (provider == QLatin1String("ollama")) {
        const auto discovery = ollamaDiscovery();
        if (discovery.lifecycle != ChatRequestLifecycle::Pending &&
            discovery.lifecycle != ChatRequestLifecycle::Running &&
            !discovery.succeeded()) {
            resolution.error = ModelBindingError::ProviderUnavailable;
            resolution.reason = discovery.safeDetail;
            return resolution;
        }
        if (discovery.succeeded() && discovery.models.isEmpty()) {
            resolution.error = ModelBindingError::ModelNotFound;
            resolution.reason = discovery.safeDetail;
            return resolution;
        }
        if (discovery.succeeded() && !model.isEmpty()) {
            const bool available = std::any_of(discovery.models.cbegin(), discovery.models.cend(),
                                               [&](const OllamaModelSummary& item) {
                                                   return item.name == model;
                                               });
            if (!available) {
                resolution.error = ModelBindingError::ModelNotFound;
                resolution.reason = QStringLiteral("Selected Ollama model is unavailable.");
                return resolution;
            }
        }
    }
    if (model.isEmpty()) {
        resolution.error = ModelBindingError::ModelNotFound;
        resolution.reason = QStringLiteral("Provider '%1' has no configured model.").arg(provider);
        return resolution;
    }
    if (router_) {
        const auto route = router_->resolveSelection(resolution.binding);
        if (route.status != ModelRoutingStatus::Routed) {
            resolution.error = ModelBindingError::ProviderUnavailable;
            resolution.reason =
                QStringLiteral("Provider '%1' has no available model route.").arg(provider);
            return resolution;
        }
    }
    resolution.binding.capabilities = capabilities(provider, model);
    resolution.binding.contextWindowTokens =
        resolution.binding.capabilities.contextWindow.value_or(0);
    if (provider != QLatin1String("ollama")) {
        const auto config = providerConfig(resolution.binding);
        if (isCloudProviderId(provider) && config.apiKey.trimmed().isEmpty()) {
            resolution.error = ModelBindingError::AuthenticationRequired;
            resolution.reason =
                QStringLiteral("Provider '%1' requires an API key that is not configured.")
                    .arg(provider);
            return resolution;
        }
        if (!config.isAllowedEndpoint()) {
            resolution.error = ModelBindingError::ConfigurationInvalid;
            resolution.reason =
                QStringLiteral("Provider '%1' is not configured with a usable endpoint.")
                    .arg(provider);
            return resolution;
        }
        const auto catalog = providerStatus(provider, model);
        if (catalog.catalog == ProviderCatalogState::AuthenticationRequired) {
            resolution.error = ModelBindingError::AuthenticationRequired;
            resolution.reason = catalog.safeDetail;
            return resolution;
        }
        if (catalog.catalog == ProviderCatalogState::Failed ||
            catalog.catalog == ProviderCatalogState::EndpointUnavailable) {
            resolution.error = ModelBindingError::ProviderUnavailable;
            resolution.reason = catalog.safeDetail;
            return resolution;
        }
        if (catalog.catalog == ProviderCatalogState::Empty ||
            (catalog.catalog == ProviderCatalogState::Available &&
             !catalog.modelIds.contains(model))) {
            resolution.error = ModelBindingError::ModelNotFound;
            resolution.reason = catalog.catalog == ProviderCatalogState::Empty
                ? catalog.safeDetail
                : QStringLiteral("Selected model is unavailable in the provider catalog.");
            return resolution;
        }
    }

    resolution.provider = constructProvider(resolution.binding);
    if (!resolution.provider) {
        resolution.error = ModelBindingError::ProviderUnavailable;
        resolution.reason = QStringLiteral("Provider '%1' could not be constructed.").arg(provider);
    }
    return resolution;
}

std::shared_ptr<IChatProvider> ModelService::constructProvider(const ModelBinding& binding) const {
    const auto factory = providerFactories_.value(binding.providerId);
    auto provider = factory ? factory(binding) : nullptr;
    return provider ? std::make_shared<HealthTrackedProvider>(
                          binding.providerId, std::move(provider), providerHealthRegistry_)
                    : nullptr;
}

ProviderHealth ModelService::providerHealth(const QString& providerId) const {
    std::lock_guard lock(providerHealthRegistry_->mutex);
    return providerHealthRegistry_->entries.value(normalizedProviderId(providerId)).health;
}

ProviderCompletenessStatus ModelService::providerStatus(const QString& providerId,
                                                        const QString& modelId) const {
    ProviderCompletenessStatus status;
    status.providerId = normalizedProviderId(providerId);
    status.health = providerHealth(status.providerId);
    const auto selected = modelId.trimmed().isEmpty() && selection_.providerId == status.providerId
        ? selection_.modelId : modelId.trimmed();
    status.capabilities = capabilities(status.providerId, selected);
    if (!isKnownProvider(status.providerId)) {
        status.catalog = ProviderCatalogState::Failed;
        status.errorCategory = ChatProviderErrorCategory::ProviderUnavailable;
        status.safeDetail = QStringLiteral("Provider is not registered.");
        return status;
    }
    if (status.providerId == QLatin1String("ollama")) {
        const auto discovery = ollamaDiscovery();
        status.errorCategory = discovery.errorCategory;
        status.safeDetail = discovery.safeDetail;
        if (discovery.succeeded()) {
            for (const auto& model : discovery.models) status.modelIds.append(model.name);
            status.catalog = status.modelIds.isEmpty() ? ProviderCatalogState::Empty
                                                        : ProviderCatalogState::Available;
        } else if (discovery.lifecycle == ChatRequestLifecycle::Pending ||
                   discovery.lifecycle == ChatRequestLifecycle::Running) {
            status.catalog = ProviderCatalogState::Pending;
        } else {
            for (const auto& model : discoveredOllamaModels())
                status.modelIds.append(model.name);
            status.catalog = !status.modelIds.isEmpty() ? ProviderCatalogState::Stale
                : discovery.errorCategory == ChatProviderErrorCategory::AuthenticationRequired
                    ? ProviderCatalogState::AuthenticationRequired
                : discovery.errorCategory == ChatProviderErrorCategory::ConnectionFailed
                    ? ProviderCatalogState::EndpointUnavailable
                    : ProviderCatalogState::Failed;
        }
        return status;
    }
    const ModelBinding binding{status.providerId, selected};
    const auto config = providerConfig(binding);
    if (config.isCloud() && config.apiKey.trimmed().isEmpty()) {
        status.catalog = ProviderCatalogState::AuthenticationRequired;
        status.errorCategory = ChatProviderErrorCategory::AuthenticationRequired;
        status.safeDetail = QStringLiteral("Provider API key is not configured.");
    } else if (!config.isAllowedEndpoint()) {
        status.catalog = ProviderCatalogState::EndpointUnavailable;
        status.errorCategory = ChatProviderErrorCategory::ProviderUnavailable;
        status.safeDetail = QStringLiteral("Provider endpoint is not configured or allowed.");
    } else {
        ProviderHealthRegistry::Catalog catalog;
        {
            std::lock_guard lock(providerHealthRegistry_->mutex);
            catalog = providerHealthRegistry_->catalogs.value(status.providerId);
        }
        if (catalog.observed) {
            status.errorCategory = catalog.outcome.category;
            for (const auto& model : catalog.models) status.modelIds.append(model.name);
            if (catalog.outcome.completed) {
                status.catalog = status.modelIds.isEmpty() ? ProviderCatalogState::Empty
                                                            : ProviderCatalogState::Available;
                status.safeDetail = status.modelIds.isEmpty()
                    ? QStringLiteral("Provider returned an empty model catalog.")
                    : QStringLiteral("Provider model catalog is available.");
            } else {
                if (config.isCloud() && catalog.models.isEmpty() &&
                    catalog.outcome.httpStatus == 404) {
                    status.catalog = ProviderCatalogState::ConfiguredModelOnly;
                    if (status.modelIds.isEmpty() && !selected.isEmpty())
                        status.modelIds.append(selected);
                    status.safeDetail = QStringLiteral(
                        "Provider model-list endpoint is unavailable; configured model is unverified.");
                    return status;
                }
                status.catalog = !status.modelIds.isEmpty() ? ProviderCatalogState::Stale
                    : catalog.outcome.category == ChatProviderErrorCategory::AuthenticationRequired
                        ? ProviderCatalogState::AuthenticationRequired
                    : catalog.outcome.category == ChatProviderErrorCategory::ConnectionFailed
                        ? ProviderCatalogState::EndpointUnavailable
                        : ProviderCatalogState::Failed;
                status.safeDetail = QStringLiteral("Provider model discovery failed (%1).")
                                        .arg(chatProviderErrorCategoryName(catalog.outcome.category));
            }
        } else if (config.isCloud()) {
            status.catalog = ProviderCatalogState::ConfiguredModelOnly;
            if (!selected.isEmpty()) status.modelIds.append(selected);
            status.safeDetail = QStringLiteral("Configured model; live provider catalog is unavailable.");
        } else {
            status.catalog = ProviderCatalogState::Unverified;
            status.safeDetail = QStringLiteral("Local endpoint catalog has not been observed yet.");
        }
    }
    return status;
}

QList<OllamaModelSummary> ModelService::providerDiscoveredModels(const QString& providerId) const {
    const auto id = normalizedProviderId(providerId);
    std::lock_guard lock(providerHealthRegistry_->mutex);
    if (id == QLatin1String("ollama"))
        return providerHealthRegistry_->ollamaModels;
    return providerHealthRegistry_->catalogs.value(id).models;
}

void ModelService::acceptProviderDiscovery(const QString& providerId,
                                           const QList<OllamaModelSummary>& models,
                                           const ProviderDiscoveryOutcome& outcome,
                                           quint64 sequence) {
    std::lock_guard lock(providerHealthRegistry_->mutex);
    auto& catalog = providerHealthRegistry_->catalogs[normalizedProviderId(providerId)];
    if (sequence < catalog.sequence) return;
    catalog.sequence = sequence;
    catalog.observed = true;
    catalog.outcome = outcome;
    if (outcome.completed) catalog.models = models;
}

void ModelService::acceptOllamaDiscovery(const OllamaModelDiscoveryResult& result,
                                         quint64 sequence) {
    {
        std::lock_guard lock(providerHealthRegistry_->mutex);
        if (sequence < providerHealthRegistry_->ollamaDiscoverySequence) return;
        providerHealthRegistry_->ollamaDiscoverySequence = sequence;
        providerHealthRegistry_->ollamaDiscovery = result;
        if (result.succeeded()) providerHealthRegistry_->ollamaModels = result.models;
    }
    reportProviderDiscovery(QStringLiteral("ollama"), sequence, result.succeeded(),
                            result.errorCategory);
}

OllamaModelDiscoveryResult ModelService::ollamaDiscovery() const {
    std::lock_guard lock(providerHealthRegistry_->mutex);
    return providerHealthRegistry_->ollamaDiscovery;
}

QList<OllamaModelSummary> ModelService::discoveredOllamaModels() const {
    std::lock_guard lock(providerHealthRegistry_->mutex);
    return providerHealthRegistry_->ollamaModels;
}

quint64 ModelService::beginProviderHealthObservation() const {
    return ++providerHealthRegistry_->nextSequence;
}

void ModelService::reportProviderDiscovery(const QString& providerId, quint64 sequence,
                                            bool completed, ChatProviderErrorCategory category) {
    reportProviderRequest(providerId, sequence, completed, category,
                          QStringLiteral("discovery"));
}

void ModelService::reportProviderRequest(const QString& providerId, quint64 sequence,
                                          bool completed, ChatProviderErrorCategory category,
                                          const QString& source) {
    std::lock_guard lock(providerHealthRegistry_->mutex);
    auto& entry = providerHealthRegistry_->entries[normalizedProviderId(providerId)];
    if (sequence < entry.sequence) return;
    entry.sequence = sequence;
    entry.source = source;
    if (completed) entry.health = ProviderHealth::Available;
    else if (category == ChatProviderErrorCategory::RateLimited ||
             category == ChatProviderErrorCategory::ConnectionFailed ||
             category == ChatProviderErrorCategory::Timeout ||
             category == ChatProviderErrorCategory::ProviderUnavailable)
        entry.health = ProviderHealth::Degraded;
    else if (category == ChatProviderErrorCategory::AuthenticationRequired ||
             category == ChatProviderErrorCategory::RequestRejected ||
             category == ChatProviderErrorCategory::MalformedResponse)
        entry.health = ProviderHealth::Unavailable;
}

LMStudioConfig ModelService::providerConfig(const ModelBinding& binding) const {
    AppSettings* liveSettings = settings_;
    std::unique_ptr<AppSettings> fallbackSettings;
    if (!liveSettings) {
        const QString settingsPath =
            QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))
                .filePath(QStringLiteral("settings.json"));
        fallbackSettings =
            std::make_unique<AppSettings>(std::make_unique<JsonSettingsStore>(settingsPath));
        liveSettings = fallbackSettings.get();
    }

    LMStudioConfig config;
    const QString cloudProv = liveSettings->selectedCloudProvider().toLower().trimmed();

    if (binding.providerId == QStringLiteral("claude") ||
        (binding.providerId == QStringLiteral("cloud-api") &&
         cloudProv == QStringLiteral("claude"))) {
        config.endpoint = QUrl(QStringLiteral("https://api.anthropic.com"));
        config.apiKey = liveSettings->claudeApiKey();
    } else if (binding.providerId == QStringLiteral("gemini") ||
               (binding.providerId == QStringLiteral("cloud-api") &&
                cloudProv == QStringLiteral("gemini"))) {
        config.endpoint = QUrl(QStringLiteral("https://generativelanguage.googleapis.com"));
        config.apiKey = liveSettings->geminiApiKey();
    } else if (binding.providerId == QStringLiteral("deepseek") ||
               (binding.providerId == QStringLiteral("cloud-api") &&
                cloudProv == QStringLiteral("deepseek"))) {
        config.endpoint = QUrl(QStringLiteral("https://api.deepseek.com"));
        config.apiKey = liveSettings->deepseekApiKey();
    } else if (binding.providerId == QStringLiteral("groq") ||
               (binding.providerId == QStringLiteral("cloud-api") &&
                cloudProv == QStringLiteral("groq"))) {
        config.endpoint = QUrl(QStringLiteral("https://api.groq.com/openai"));
        config.apiKey = liveSettings->groqApiKey();
    } else if (binding.providerId == QStringLiteral("mistral") ||
               (binding.providerId == QStringLiteral("cloud-api") &&
                cloudProv == QStringLiteral("mistral"))) {
        config.endpoint = QUrl(QStringLiteral("https://api.mistral.ai"));
        config.apiKey = liveSettings->mistralApiKey();
    } else if (binding.providerId == QStringLiteral("openai") ||
               binding.providerId == QStringLiteral("openai-compatible") ||
               binding.providerId == QStringLiteral("cloud-api")) {
        config.endpoint = QUrl(QStringLiteral("https://api.openai.com"));
        config.apiKey = liveSettings->openAiApiKey();
    } else if (binding.providerId == QStringLiteral("llama-cpp-server")) {
        config.endpoint = QUrl(llamaCppEndpoint_);
    } else if (binding.providerId == QStringLiteral("openai-compatible-local")) {
        config.endpoint = QUrl(QStringLiteral("http://127.0.0.1:8000"));
    } else {
        config.endpoint = QUrl(lmStudioEndpoint_);
    }
    return config;
}

void ModelService::setOllamaEndpoint(const QString& endpoint) {
    if (ollamaEndpoint_ != endpoint) {
        std::lock_guard lock(providerHealthRegistry_->mutex);
        providerHealthRegistry_->ollamaModels.clear();
        providerHealthRegistry_->ollamaDiscovery = {};
        providerHealthRegistry_->ollamaDiscovery.lifecycle = ChatRequestLifecycle::Pending;
        providerHealthRegistry_->ollamaDiscovery.errorCategory = ChatProviderErrorCategory::None;
        providerHealthRegistry_->ollamaDiscovery.safeDetail =
            QStringLiteral("Ollama model discovery pending.");
        providerHealthRegistry_->entries.remove(QStringLiteral("ollama"));
    }
    ollamaEndpoint_ = endpoint;
}

void ModelService::setLmStudioEndpoint(const QString& endpoint) {
    if (lmStudioEndpoint_ != endpoint) {
        std::lock_guard lock(providerHealthRegistry_->mutex);
        providerHealthRegistry_->catalogs.remove(QStringLiteral("lm-studio"));
    }
    lmStudioEndpoint_ = endpoint;
}

void ModelService::setLlamaCppEndpoint(const QString& endpoint) {
    if (llamaCppEndpoint_ != endpoint) {
        std::lock_guard lock(providerHealthRegistry_->mutex);
        providerHealthRegistry_->catalogs.remove(QStringLiteral("llama-cpp-server"));
    }
    llamaCppEndpoint_ = endpoint;
}

void ModelService::setLocalInferenceTimeoutMs(int timeoutMs) {
    localInferenceTimeoutMs_ = timeoutMs;
}

} // namespace sentinel::core
