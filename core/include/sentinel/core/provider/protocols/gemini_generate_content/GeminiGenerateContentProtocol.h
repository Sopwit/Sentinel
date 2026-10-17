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

// Builds the Gemini native generateContent request body used by the provider
// adapter. `contents` (role/parts, already built by the caller) is inserted
// verbatim. The native body keeps `model` so the transport can derive the URL and
// remove it before dispatch (unlike the infer/stream body, which carries a
// temperature and always emits maxOutputTokens). A non-empty
// `functionDeclarations` adds the functionDeclarations tool envelope; a non-empty
// `structuredSchema` adds responseMimeType/responseSchema to generationConfig.
QJsonObject buildGeminiNativeRequestBody(const QString& model, const QJsonArray& contents,
                                         int maxOutputTokens,
                                         const QJsonArray& functionDeclarations = {},
                                         const QJsonObject& structuredSchema = {});

// One Gemini functionResponse entry for a native function continuation round-trip.
struct GeminiFunctionResult {
    QString callId;
    QString functionName;
    QString content;
};

// Converts resolved function names and result text into Gemini functionResponse
// parts. Result text is truncated to `maxContentLength` (default 6800). Synthesized
// Sentinel ids ("sentinel-gemini-...") are internal bookkeeping and are never
// echoed to the provider; provider-supplied ids are forwarded verbatim.
QJsonArray geminiFunctionResponseParts(const QList<GeminiFunctionResult>& results,
                                       int maxContentLength = 6800);

} // namespace sentinel::core::provider
