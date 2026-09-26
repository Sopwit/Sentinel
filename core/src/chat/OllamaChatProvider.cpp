// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/chat/OllamaChatProvider.h"

#include "sentinel/core/runtime/LocalInference.h"

#include <QUuid>

#include <utility>

namespace sentinel::core {
namespace {

ChatProviderErrorCategory categoryFromLocalInference(LocalInferenceError error,
                                                     LocalInferenceStatus status) {
    if (error == LocalInferenceError::Timeout)
        return ChatProviderErrorCategory::Timeout;
    if (error == LocalInferenceError::OllamaNotRunning ||
        error == LocalInferenceError::ClientUnavailable)
        return ChatProviderErrorCategory::ProviderUnavailable;
    if (error == LocalInferenceError::EndpointUnreachable)
        return ChatProviderErrorCategory::ConnectionFailed;
    if (error == LocalInferenceError::MissingModel ||
        error == LocalInferenceError::ModelUnavailable ||
        status == LocalInferenceStatus::ModelUnavailable)
        return ChatProviderErrorCategory::ModelNotFound;
    if (error == LocalInferenceError::InvalidResponse ||
        error == LocalInferenceError::StreamInterrupted)
        return ChatProviderErrorCategory::MalformedResponse;
    if (error == LocalInferenceError::BlankPrompt)
        return ChatProviderErrorCategory::RequestRejected;
    return ChatProviderErrorCategory::ProviderUnavailable;
}

ChatProviderReply failureReply(QString detail, ChatProviderErrorCategory category,
                              ChatProviderReply::Error error = ChatProviderReply::Error::ProviderFailure) {
    ChatProviderReply reply;
    reply.success = false;
    reply.errorMessage = std::move(detail);
    reply.error = error;
    reply.category = category;
    reply.lifecycle = category == ChatProviderErrorCategory::Timeout
                          ? ChatRequestLifecycle::TimedOut
                      : category == ChatProviderErrorCategory::Cancelled
                          ? ChatRequestLifecycle::Cancelled
                      : category == ChatProviderErrorCategory::RateLimited
                          ? ChatRequestLifecycle::RateLimited
                          : ChatRequestLifecycle::Failed;
    return reply;
}

} // namespace

OllamaChatProvider::OllamaChatProvider(OllamaConfig config, int timeoutMs)
    : config_(std::move(config)), timeoutMs_(timeoutMs) {}

QString OllamaChatProvider::name() const {
    return QStringLiteral("OllamaChatProvider");
}

QString OllamaChatProvider::endpoint() const {
    return config_.endpoint.toString();
}

QString OllamaChatProvider::selectedModel() const {
    return selectedModel_;
}

void OllamaChatProvider::setSelectedModel(const QString& model) {
    selectedModel_ = model.trimmed();
}

ChatProviderStatus OllamaChatProvider::status() const {
    if (!config_.endpoint.isLoopbackHttp()) {
        return ChatProviderStatus::Unavailable;
    }

    OllamaHttpRuntimeClient client(config_, std::min(timeoutMs_, 750));
    const auto health = client.healthCheck();
    if (health.healthStatus != OllamaHealthStatus::Healthy) {
        return ChatProviderStatus::Unavailable;
    }

    const auto discovery = discoverySnapshot_.has_value() ? *discoverySnapshot_
                                                          : client.discoverModels();
    if (!discovery.succeeded()) {
        return ChatProviderStatus::Unavailable;
    }

    return ChatProviderStatus::Ready;
}

ChatProviderReply OllamaChatProvider::sendMessage(const QString& message) {
    return sendMessageWithToken(message, {});
}

ChatProviderReply OllamaChatProvider::sendRequest(const QString& message,
                                                  const ChatRequestOptions& options) {
    if (options.nativeToolCalling || options.structuredOutput)
        return IChatProvider::sendRequest(message, options);
    return sendMessageWithToken(message, options.cancellationToken);
}

ChatProviderReply OllamaChatProvider::sendMessageWithToken(
    const QString& message,
    const std::shared_ptr<std::atomic_bool>& cancellationToken) {
    const auto trimmed = message.trimmed();
    if (trimmed.isEmpty()) {
        return failureReply(QStringLiteral("Prompt is blank."),
                            ChatProviderErrorCategory::RequestRejected);
    }

    auto model = selectedModel_.trimmed();
    if (model.isEmpty()) {
        OllamaHttpRuntimeClient runtimeClient(config_, std::min(timeoutMs_, 750));
        const auto discovery = discoverySnapshot_.has_value()
                                   ? *discoverySnapshot_ : runtimeClient.discoverModels(cancellationToken);
        if (!discovery.succeeded()) {
            auto reply = failureReply(discovery.safeDetail, discovery.errorCategory);
            reply.httpStatus = discovery.httpStatus;
            reply.attempts = discovery.attemptCount;
            reply.requestId = discovery.requestId;
            return reply;
        }
        if (discovery.models.isEmpty()) {
            return failureReply(
                QStringLiteral("No Ollama model is installed yet. "
                               "Run 'sentinel-cli model pull <name>' to install one."),
                ChatProviderErrorCategory::ModelNotFound);
        }
        model = discovery.models.first().name;
    }

    OllamaLocalInferenceClient inferenceClient(config_, timeoutMs_);
    LocalInferenceRequest request;
    request.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.prompt = trimmed;
    request.options.model = model;
    request.options.discoverySnapshot = discoverySnapshot_;
    request.options.timeoutMs = timeoutMs_;
    request.options.cancellationToken = cancellationToken;
    request.options.temperature = 0.7;
    request.options.topP = 0.9;
    request.options.maxTokens = 2048;

    const auto response = inferenceClient.infer(request);
    if (response.status == LocalInferenceStatus::Succeeded) {
        ChatProviderReply reply{true, response.text, {}};
        reply.lifecycle = ChatRequestLifecycle::Completed;
        reply.attempts = response.attempts;
        reply.retrySummary = response.retrySummary;
        reply.requestId = response.requestId;
        return reply;
    }

    auto reply = failureReply(response.summary,
                              response.providerErrorCategory > 0
                                  ? static_cast<ChatProviderErrorCategory>(response.providerErrorCategory)
                                  : categoryFromLocalInference(response.error, response.status));
    reply.message = response.text;
    reply.httpStatus = response.httpStatus;
    reply.attempts = response.attempts;
    reply.retrySummary = response.retrySummary;
    reply.requestId = response.requestId;
    return reply;
}

ChatProviderReply OllamaChatProvider::sendMessageStreaming(
    const QString& message, const std::function<void(const QString&)>& onDelta,
    const std::shared_ptr<std::atomic_bool>& cancellationToken) {
    const auto trimmed = message.trimmed();
    if (trimmed.isEmpty())
        return failureReply(QStringLiteral("Prompt is blank."),
                            ChatProviderErrorCategory::RequestRejected);

    auto model = selectedModel_.trimmed();
    if (model.isEmpty()) {
        OllamaHttpRuntimeClient runtimeClient(config_, std::min(timeoutMs_, 750));
        const auto discovery = discoverySnapshot_.has_value()
                                   ? *discoverySnapshot_ : runtimeClient.discoverModels(cancellationToken);
        if (!discovery.succeeded()) {
            auto reply = failureReply(discovery.safeDetail, discovery.errorCategory);
            reply.httpStatus = discovery.httpStatus;
            reply.attempts = discovery.attemptCount;
            reply.requestId = discovery.requestId;
            return reply;
        }
        if (discovery.models.isEmpty()) {
            return failureReply(
                QStringLiteral("No Ollama model is installed yet. "
                               "Run 'sentinel-cli model pull <name>' to install one."),
                ChatProviderErrorCategory::ModelNotFound);
        }
        model = discovery.models.first().name;
    }

    LocalInferenceRequest request;
    request.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.prompt = trimmed;
    request.options.model = model;
    request.options.timeoutMs = timeoutMs_;
    request.options.temperature = 0.7;
    request.options.topP = 0.9;
    request.options.maxTokens = 2048;
    request.options.streamingRequested = true;
    request.options.cancellationToken = cancellationToken;

    OllamaLocalInferenceStreamClient client(config_, timeoutMs_);
    const auto result = client.startStream(request, [&](const LocalInferenceStreamChunk& chunk) {
        if (!chunk.malformed && !chunk.text.isEmpty() &&
            !(cancellationToken && cancellationToken->load()) && onDelta) {
            onDelta(chunk.text);
        }
    });
    if (result.status == LocalInferenceStreamStatus::Completed) {
        ChatProviderReply reply{true, result.accumulatedText, {}};
        reply.lifecycle = ChatRequestLifecycle::Completed;
        reply.httpStatus = result.httpStatus;
        reply.attempts = result.attempts;
        reply.retrySummary = result.retrySummary;
        reply.requestId = result.requestId;
        return reply;
    }
    auto reply = failureReply(
        result.summary,
        result.providerErrorCategory > 0
            ? static_cast<ChatProviderErrorCategory>(result.providerErrorCategory)
            : result.status == LocalInferenceStreamStatus::Cancelled
            ? ChatProviderErrorCategory::Cancelled
            : categoryFromLocalInference(result.error, LocalInferenceStatus::Error));
    reply.message = result.accumulatedText;
    reply.httpStatus = result.httpStatus;
    reply.attempts = result.attempts;
    reply.retrySummary = result.retrySummary;
    reply.requestId = result.requestId;
    return reply;
}

} // namespace sentinel::core
