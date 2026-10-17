// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonArray>
#include <QJsonObject>
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

} // namespace sentinel::core::provider
