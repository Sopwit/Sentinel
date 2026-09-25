// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/model/ModelService.h"

#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/chat/OllamaChatProvider.h"
#include "sentinel/core/memory/JsonSettingsStore.h"
#include "sentinel/core/model/IModelRouter.h"
#include "sentinel/core/runtime/LocalInference.h"

#include <QDir>
#include <QStandardPaths>

#include <utility>

namespace sentinel::core {

namespace {

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
    ChatProviderReply sendMessage(const QString& message) override {
        if (!config_.isAllowedEndpoint())
            return {false,
                    {},
                    QStringLiteral("Selected provider '%1' is unavailable or not configured.")
                        .arg(binding_.providerId)};
        LocalInferenceRequest request;
        request.prompt = message;
        request.options.model = binding_.modelId;
        request.options.timeoutMs = timeoutMs_;
        LMStudioLocalInferenceClient client(config_, timeoutMs_);
        const auto result = client.infer(request);
        if (result.status == LocalInferenceStatus::Succeeded)
            return {true, result.text, {}};
        if (result.error == LocalInferenceError::ModelUnavailable)
            return {false,
                    {},
                    QStringLiteral("Selected model '%1' is unavailable from provider '%2'.")
                        .arg(binding_.modelId, binding_.providerId)};
        return {false,
                {},
                QStringLiteral("Provider '%1': %2").arg(binding_.providerId, result.summary)};
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
    if (settings_) {
        selection_.providerId = normalizedProviderId(settings_->selectedRuntimeProvider());
        selection_.modelId = settings_->selectedModelForProvider(selection_.providerId).trimmed();
    } else {
        selection_.providerId = defaultProviderId();
    }

    registerProvider(defaultProviderId(), [this](const ModelBinding& binding) {
        auto provider = std::make_shared<OllamaChatProvider>(
            OllamaConfig::fromEndpoint(ollamaEndpoint_), localInferenceTimeoutMs_);
        provider->setSelectedModel(binding.modelId);
        return std::shared_ptr<IChatProvider>(std::move(provider));
    });
    static const QStringList endpointProviderIds{
        QStringLiteral("lm-studio"),        QStringLiteral("openai-compatible-local"),
        QStringLiteral("llama-cpp-server"), QStringLiteral("cloud-api"),
        QStringLiteral("openai"),           QStringLiteral("openai-compatible"),
        QStringLiteral("claude"),           QStringLiteral("gemini"),
        QStringLiteral("deepseek"),         QStringLiteral("groq"),
        QStringLiteral("mistral")};
    for (const auto& providerId : endpointProviderIds) {
        registerProvider(providerId, [this](const ModelBinding& binding) {
            return std::make_shared<SelectedEndpointChatProvider>(binding, providerConfig(binding),
                                                                  localInferenceTimeoutMs_);
        });
    }
}

ModelService::~ModelService() = default;

void ModelService::setModelRouter(IModelRouter* router) {
    router_ = router;
}

void ModelService::registerProvider(const QString& providerId, ModelProviderFactory factory) {
    const auto normalized = normalizedProviderId(providerId);
    if (normalized.isEmpty() || !factory)
        return;
    providerFactories_.insert(normalized, std::move(factory));
    emit providerRegistryChanged();
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

    if (provider.isEmpty() || !providerFactories_.contains(provider)) {
        resolution.error = ModelBindingError::ProviderNotFound;
        resolution.reason = provider.isEmpty()
                                ? QStringLiteral("No provider is configured for this request.")
                                : QStringLiteral("Provider '%1' is not configured.").arg(provider);
        return resolution;
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
    return factory ? factory(binding) : nullptr;
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
    QString cloudProv = liveSettings->selectedCloudProvider().toLower().trimmed();

    // The generic cloud-api provider keeps its legacy credential disambiguation
    // by model-name prefix; explicit provider ids never depend on model names.
    const auto modelLower = binding.modelId.trimmed().toLower();
    if (modelLower.startsWith(QLatin1String("gemini"))) {
        cloudProv = QStringLiteral("gemini");
    } else if (modelLower.startsWith(QLatin1String("claude"))) {
        cloudProv = QStringLiteral("claude");
    } else if (modelLower.startsWith(QLatin1String("deepseek"))) {
        cloudProv = QStringLiteral("deepseek");
    } else if (modelLower.startsWith(QLatin1String("llama")) ||
               modelLower.startsWith(QLatin1String("mixtral"))) {
        cloudProv = QStringLiteral("groq");
    } else if (modelLower.startsWith(QLatin1String("mistral")) ||
               modelLower.startsWith(QLatin1String("pixtral")) ||
               modelLower.startsWith(QLatin1String("codestral"))) {
        cloudProv = QStringLiteral("mistral");
    } else if (modelLower.startsWith(QLatin1String("gpt")) ||
               modelLower.startsWith(QLatin1String("o1")) ||
               modelLower.startsWith(QLatin1String("o3"))) {
        cloudProv = QStringLiteral("openai");
    }

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
    } else {
        config.endpoint = QUrl(lmStudioEndpoint_);
    }
    return config;
}

void ModelService::setOllamaEndpoint(const QString& endpoint) {
    ollamaEndpoint_ = endpoint;
}

void ModelService::setLmStudioEndpoint(const QString& endpoint) {
    lmStudioEndpoint_ = endpoint;
}

void ModelService::setLlamaCppEndpoint(const QString& endpoint) {
    llamaCppEndpoint_ = endpoint;
}

void ModelService::setLocalInferenceTimeoutMs(int timeoutMs) {
    localInferenceTimeoutMs_ = timeoutMs;
}

} // namespace sentinel::core
