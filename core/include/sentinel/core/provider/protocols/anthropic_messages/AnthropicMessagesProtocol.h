// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QUrl>

#include <optional>

namespace sentinel::core::provider {

// Canonical Anthropic Messages endpoint URL.
QUrl anthropicMessagesUrl();

// Builds an Anthropic /v1/messages request body.
// If maxTokens is <= 0, defaults to 2048.
// Messages array is inserted verbatim.
QJsonObject buildAnthropicMessagesBody(const QString& model,
                                       const QJsonArray& messages,
                                       int maxTokens,
                                       bool stream = false,
                                       const QString& systemPrompt = QString());

// Error classification when parsing Anthropic messages response text.
enum class AnthropicMessagesTextError {
    None,
    MissingContent,
    MissingText,
};

// Extracts and concatenates text from all blocks where type == "text".
// Returns std::nullopt if the "content" array is missing or contains no text.
std::optional<QString> parseAnthropicMessagesText(const QJsonObject& body,
                                                 AnthropicMessagesTextError* error = nullptr);

// Converts an OpenAI-style tool definition envelope
// ({type:"function", function:{name, description, parameters}}) into the flat
// Anthropic Messages tool declaration ({name, description, input_schema}).
// The Anthropic shape must not reuse the nested envelope.
QJsonObject anthropicToolDeclaration(const QJsonObject& nativeToolDefinition);

// Builds the Anthropic native /v1/messages request body. `messages` and `tools`
// (already Anthropic-shaped via anthropicToolDeclaration) are supplied by the
// caller. Unlike the streaming/infer body this never inserts a `stream` key, so
// the native non-streaming request bytes are preserved. A non-empty
// `structuredSchema` adds the output_config json_schema envelope.
QJsonObject buildAnthropicNativeRequestBody(const QString& model, const QJsonArray& messages,
                                            int maxTokens, const QJsonArray& tools = {},
                                            const QJsonObject& structuredSchema = {});

// One Anthropic tool_result entry for a native tool continuation round-trip.
struct AnthropicToolResult {
    QString callId;
    QString content;
};

// Converts provider tool-use ids and result text into Anthropic tool_result
// content blocks. Result text is truncated to `maxContentLength` (default 6800),
// matching the native continuation limit.
QJsonArray anthropicToolResultBlocks(const QList<AnthropicToolResult>& results,
                                     int maxContentLength = 6800);

} // namespace sentinel::core::provider
