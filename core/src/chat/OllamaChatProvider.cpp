// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/chat/OllamaChatProvider.h"

#include "sentinel/core/runtime/LocalInference.h"

namespace sentinel::core {

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

    const auto models = client.installedModels();
    if (models.isEmpty()) {
        return ChatProviderStatus::Unavailable;
    }

    return ChatProviderStatus::Ready;
}

ChatProviderReply OllamaChatProvider::sendMessage(const QString& message) {
    const auto trimmed = message.trimmed();
    if (trimmed.isEmpty()) {
        return {false, {}, QStringLiteral("Prompt is blank.")};
    }

    OllamaHttpRuntimeClient runtimeClient(config_, std::min(timeoutMs_, 750));
    const auto health = runtimeClient.healthCheck();
    if (health.healthStatus != OllamaHealthStatus::Healthy) {
        return {false, {}, QStringLiteral("Ollama is not running or unreachable at %1.")
                            .arg(config_.endpoint.toString())};
    }

    auto model = selectedModel_.trimmed();
    if (model.isEmpty()) {
        const auto models = runtimeClient.installedModels();
        if (models.isEmpty()) {
            return {false, {}, QStringLiteral("No Ollama model is installed yet. "
                                              "Run 'sentinel-cli model pull <name>' to install one.")};
        }
        model = models.first().name;
    }

    OllamaLocalInferenceClient inferenceClient(config_, timeoutMs_);
    LocalInferenceRequest request;
    request.id = QStringLiteral("ollama-chat-provider-request");
    request.prompt = trimmed;
    request.options.model = model;
    request.options.timeoutMs = timeoutMs_;
    request.options.temperature = 0.7;
    request.options.topP = 0.9;
    request.options.maxTokens = 2048;

    const auto response = inferenceClient.infer(request);
    if (response.status == LocalInferenceStatus::Succeeded) {
        return {true, response.text, {}};
    }

    return {false, response.text, response.summary};
}

} // namespace sentinel::core
