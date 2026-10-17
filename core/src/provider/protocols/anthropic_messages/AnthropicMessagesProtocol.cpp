// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/provider/protocols/anthropic_messages/AnthropicMessagesProtocol.h"

namespace sentinel::core::provider {

QUrl anthropicMessagesUrl() {
    return QUrl(QStringLiteral("https://api.anthropic.com/v1/messages"));
}

QJsonObject buildAnthropicMessagesBody(const QString& model,
                                       const QJsonArray& messages,
                                       int maxTokens,
                                       bool stream,
                                       const QString& systemPrompt) {
    QJsonObject body;
    body.insert(QStringLiteral("model"), model);
    if (!systemPrompt.isEmpty()) {
        body.insert(QStringLiteral("system"), systemPrompt);
    }
    body.insert(QStringLiteral("messages"), messages);
    body.insert(QStringLiteral("max_tokens"), maxTokens > 0 ? maxTokens : 2048);
    body.insert(QStringLiteral("stream"), stream);
    return body;
}

std::optional<QString> parseAnthropicMessagesText(const QJsonObject& body,
                                                 AnthropicMessagesTextError* error) {
    if (error) {
        *error = AnthropicMessagesTextError::None;
    }
    const auto contentVal = body.value(QStringLiteral("content"));
    if (!contentVal.isArray()) {
        if (error) {
            *error = AnthropicMessagesTextError::MissingContent;
        }
        return std::nullopt;
    }
    const auto contentParts = contentVal.toArray();
    QString text;
    for (const auto& part : contentParts) {
        if (!part.isObject()) {
            continue;
        }
        const auto partObj = part.toObject();
        if (partObj.value(QStringLiteral("type")).toString() == QStringLiteral("text")) {
            text += partObj.value(QStringLiteral("text")).toString();
        }
    }
    if (text.isEmpty()) {
        if (error) {
            *error = AnthropicMessagesTextError::MissingText;
        }
        return std::nullopt;
    }
    return text;
}

QJsonObject anthropicToolDeclaration(const QJsonObject& nativeToolDefinition) {
    const auto function = nativeToolDefinition.value(QStringLiteral("function")).toObject();
    return QJsonObject{
        {QStringLiteral("name"), function.value(QStringLiteral("name"))},
        {QStringLiteral("description"), function.value(QStringLiteral("description"))},
        {QStringLiteral("input_schema"), function.value(QStringLiteral("parameters"))}};
}

QJsonObject buildAnthropicNativeRequestBody(const QString& model, const QJsonArray& messages,
                                            int maxTokens, const QJsonArray& tools,
                                            const QJsonObject& structuredSchema) {
    QJsonObject body;
    body.insert(QStringLiteral("model"), model);
    body.insert(QStringLiteral("messages"), messages);
    body.insert(QStringLiteral("max_tokens"), maxTokens > 0 ? maxTokens : 2048);
    if (!tools.isEmpty()) {
        body.insert(QStringLiteral("tools"), tools);
    }
    if (!structuredSchema.isEmpty()) {
        body.insert(
            QStringLiteral("output_config"),
            QJsonObject{{QStringLiteral("format"),
                         QJsonObject{{QStringLiteral("type"), QStringLiteral("json_schema")},
                                     {QStringLiteral("schema"), structuredSchema}}}});
    }
    return body;
}

QJsonArray anthropicToolResultBlocks(const QList<AnthropicToolResult>& results,
                                     int maxContentLength) {
    QJsonArray blocks;
    for (const auto& result : results) {
        blocks.append(
            QJsonObject{{QStringLiteral("type"), QStringLiteral("tool_result")},
                        {QStringLiteral("tool_use_id"), result.callId},
                        {QStringLiteral("content"), result.content.left(maxContentLength)}});
    }
    return blocks;
}

} // namespace sentinel::core::provider
