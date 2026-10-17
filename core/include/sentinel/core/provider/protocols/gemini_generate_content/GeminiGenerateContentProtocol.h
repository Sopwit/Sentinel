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

// Builds canonical Gemini generateContent endpoint URL for a given model.
// If apiKey is provided, it is appended as "?key=...".
QUrl geminiGenerateContentUrl(const QString& model, const QString& apiKey = QString());

// Builds canonical Gemini streaming endpoint URL for a given model.
// Appends "?alt=sse" and optionally "&key=...".
QUrl geminiStreamGenerateContentUrl(const QString& model, const QString& apiKey = QString());

// Builds a Gemini generateContent request body with contents array and optional generationConfig.
QJsonObject buildGeminiGenerateContentBody(const QJsonArray& contents,
                                          int maxOutputTokens = 2048,
                                          std::optional<double> temperature = std::nullopt,
                                          std::optional<double> topP = std::nullopt);

// Error classification when parsing Gemini generateContent response text.
enum class GeminiGenerateContentTextError {
    None,
    MissingCandidates,
    MissingContent,
    MissingText,
};

// Extracts concatenated text from candidates[0].content.parts blocks.
// Returns std::nullopt if candidates are missing or no text parts are found.
std::optional<QString>
parseGeminiGenerateContentText(const QJsonObject& body,
                               GeminiGenerateContentTextError* error = nullptr);

} // namespace sentinel::core::provider
