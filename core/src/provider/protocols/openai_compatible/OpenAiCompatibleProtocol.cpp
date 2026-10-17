// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.h"

#include <cmath>
#include <limits>
#include <utility>

namespace sentinel::core::provider {

QString openAiCompatibleChatCompletionsPath(const QUrl& endpoint) {
    return endpoint.adjusted(QUrl::StripTrailingSlash).path().endsWith(QLatin1String("/v1"))
               ? QStringLiteral("/chat/completions")
               : QStringLiteral("/v1/chat/completions");
}

QJsonObject buildOpenAiCompatibleChatBody(const QString& model, const QJsonArray& messages,
                                          int maxTokens, bool stream,
                                          std::optional<double> temperature) {
    QJsonObject body;
    body.insert(QStringLiteral("model"), model);
    body.insert(QStringLiteral("messages"), messages);
    body.insert(QStringLiteral("stream"), stream);
    if (temperature)
        body.insert(QStringLiteral("temperature"), *temperature);
    body.insert(QStringLiteral("max_tokens"), maxTokens);
    return body;
}

std::optional<QString> parseOpenAiCompatibleChatText(const QJsonObject& body,
                                                     OpenAiCompatibleChatTextError* error) {
    const auto choices = body.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        if (error)
            *error = OpenAiCompatibleChatTextError::MissingChoices;
        return std::nullopt;
    }
    auto text = choices.first()
                    .toObject()
                    .value(QStringLiteral("message"))
                    .toObject()
                    .value(QStringLiteral("content"))
                    .toString();
    if (text.isEmpty()) {
        if (error)
            *error = OpenAiCompatibleChatTextError::MissingText;
        return std::nullopt;
    }
    if (error)
        *error = OpenAiCompatibleChatTextError::None;
    return text;
}

QList<OpenAiCompatibleToolCall> parseOpenAiCompatibleToolCalls(const QJsonObject& body) {
    QList<OpenAiCompatibleToolCall> calls;
    const auto choices = body.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        return calls;
    }
    const auto message = choices.first().toObject().value(QStringLiteral("message")).toObject();
    for (const auto& value : message.value(QStringLiteral("tool_calls")).toArray()) {
        const auto call = value.toObject();
        const auto function = call.value(QStringLiteral("function")).toObject();
        calls.append(OpenAiCompatibleToolCall{call.value(QStringLiteral("id")).toString(),
                                              function.value(QStringLiteral("name")).toString(),
                                              function.value(QStringLiteral("arguments"))});
    }
    return calls;
}

OpenAiCompatibleCatalogParse parseOpenAiCompatibleModelCatalog(const QJsonObject& body) {
    OpenAiCompatibleCatalogParse parsed;
    const auto data = body.value(QStringLiteral("data"));
    if (!data.isArray()) {
        parsed.malformed = true;
        return parsed;
    }

    const auto positiveLimit = [](const QJsonObject& object,
                                  const QString& key) -> std::optional<int> {
        const auto field = object.value(key);
        if (!field.isDouble())
            return std::nullopt;
        const auto number = field.toDouble();
        if (number <= 0 || number > std::numeric_limits<int>::max() || number != std::floor(number))
            return std::nullopt;
        return static_cast<int>(number);
    };
    const auto reportedSupport = [](const QJsonObject& object, const QString& key) {
        const auto field = object.value(key);
        return field.isBool() ? (field.toBool() ? CapabilitySupport::Supported
                                                : CapabilitySupport::Unsupported)
                              : CapabilitySupport::Unknown;
    };

    const auto dataValues = data.toArray();
    for (const auto& value : dataValues) {
        const auto object = value.toObject();
        const auto id = object.value(QStringLiteral("id")).toString().trimmed();
        if (!value.isObject() || !object.value(QStringLiteral("id")).isString() || id.isEmpty()) {
            parsed.models.clear();
            parsed.malformed = true;
            return parsed;
        }

        QString createdStr;
        if (object.contains(QStringLiteral("created"))) {
            const auto createdVal = object.value(QStringLiteral("created"));
            if (createdVal.isDouble()) {
                createdStr = QString::number(createdVal.toDouble());
            } else {
                createdStr = createdVal.toString();
            }
        }

        OllamaModelSummary model{id, createdStr, 0};
        model.capabilities.contextWindow = positiveLimit(object, QStringLiteral("context_window"));
        if (!model.capabilities.contextWindow)
            model.capabilities.contextWindow =
                positiveLimit(object, QStringLiteral("max_context_length"));
        model.capabilities.maxOutputTokens =
            positiveLimit(object, QStringLiteral("max_output_tokens"));

        const auto reported = object.value(QStringLiteral("capabilities")).toObject();
        const auto booleanCapability = [&reported](const QString& key) {
            const auto field = reported.value(key);
            return field.isBool() ? (field.toBool() ? CapabilitySupport::Supported
                                                    : CapabilitySupport::Unsupported)
                                  : CapabilitySupport::Unknown;
        };
        model.capabilities.nativeToolCalling =
            booleanCapability(QStringLiteral("trained_for_tool_use"));
        if (model.capabilities.nativeToolCalling == CapabilitySupport::Unknown)
            model.capabilities.nativeToolCalling =
                reportedSupport(object, QStringLiteral("tool_calling"));
        model.capabilities.structuredOutput =
            reportedSupport(object, QStringLiteral("structured_output"));
        model.capabilities.visionInput = booleanCapability(QStringLiteral("vision"));
        if (model.capabilities.visionInput == CapabilitySupport::Unknown)
            model.capabilities.visionInput = reportedSupport(object, QStringLiteral("vision"));
        model.capabilities.audioInput = reportedSupport(object, QStringLiteral("audio_input"));
        model.capabilities.audioOutput = reportedSupport(object, QStringLiteral("audio_output"));
        model.capabilities.streaming = reportedSupport(object, QStringLiteral("streaming"));
        model.publisher = object.value(QStringLiteral("publisher")).toString();
        model.family = object.value(QStringLiteral("family")).toString();
        model.architecture = object.value(QStringLiteral("architecture")).toString();
        if (model.architecture.isEmpty())
            model.architecture = object.value(QStringLiteral("arch")).toString();
        parsed.models.append(std::move(model));
    }
    return parsed;
}

} // namespace sentinel::core::provider
