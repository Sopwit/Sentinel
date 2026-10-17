// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/provider/protocols/gemini_generate_content/GeminiGenerateContentProtocol.h"

#include <QUrlQuery>

namespace sentinel::core::provider {

QUrl geminiGenerateContentUrl(const QString& model, const QString& apiKey) {
    const auto percentModel = QString::fromLatin1(QUrl::toPercentEncoding(model));
    QString urlStr = QStringLiteral("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent")
                         .arg(percentModel);
    QUrl url(urlStr);
    if (!apiKey.isEmpty()) {
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("key"), apiKey);
        url.setQuery(query);
    }
    return url;
}

QUrl geminiStreamGenerateContentUrl(const QString& model, const QString& apiKey) {
    const auto percentModel = QString::fromLatin1(QUrl::toPercentEncoding(model));
    QString urlStr = QStringLiteral(
                         "https://generativelanguage.googleapis.com/v1beta/models/%1:streamGenerateContent")
                         .arg(percentModel);
    QUrl url(urlStr);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("alt"), QStringLiteral("sse"));
    if (!apiKey.isEmpty()) {
        query.addQueryItem(QStringLiteral("key"), apiKey);
    }
    url.setQuery(query);
    return url;
}

QJsonObject buildGeminiGenerateContentBody(const QJsonArray& contents,
                                          int maxOutputTokens,
                                          std::optional<double> temperature,
                                          std::optional<double> topP) {
    QJsonObject body;
    body.insert(QStringLiteral("contents"), contents);

    QJsonObject generationConfig;
    if (temperature.has_value()) {
        generationConfig.insert(QStringLiteral("temperature"), *temperature);
    }
    if (topP.has_value()) {
        generationConfig.insert(QStringLiteral("topP"), *topP);
    }
    generationConfig.insert(QStringLiteral("maxOutputTokens"),
                            maxOutputTokens > 0 ? maxOutputTokens : 2048);
    body.insert(QStringLiteral("generationConfig"), generationConfig);
    return body;
}

std::optional<QString>
parseGeminiGenerateContentText(const QJsonObject& body,
                               GeminiGenerateContentTextError* error) {
    if (error) {
        *error = GeminiGenerateContentTextError::None;
    }
    const auto candidatesVal = body.value(QStringLiteral("candidates"));
    if (!candidatesVal.isArray()) {
        if (error) {
            *error = GeminiGenerateContentTextError::MissingCandidates;
        }
        return std::nullopt;
    }
    const auto candidates = candidatesVal.toArray();
    if (candidates.isEmpty()) {
        if (error) {
            *error = GeminiGenerateContentTextError::MissingCandidates;
        }
        return std::nullopt;
    }
    const auto firstCandidate = candidates.first().toObject();
    const auto contentVal = firstCandidate.value(QStringLiteral("content"));
    if (!contentVal.isObject()) {
        if (error) {
            *error = GeminiGenerateContentTextError::MissingContent;
        }
        return std::nullopt;
    }
    const auto partsVal = contentVal.toObject().value(QStringLiteral("parts"));
    if (!partsVal.isArray()) {
        if (error) {
            *error = GeminiGenerateContentTextError::MissingText;
        }
        return std::nullopt;
    }
    const auto parts = partsVal.toArray();
    QString text;
    for (const auto& part : parts) {
        if (part.isObject()) {
            text += part.toObject().value(QStringLiteral("text")).toString();
        }
    }
    if (text.isEmpty()) {
        if (error) {
            *error = GeminiGenerateContentTextError::MissingText;
        }
        return std::nullopt;
    }
    return text;
}

QJsonObject buildGeminiNativeRequestBody(const QString& model, const QJsonArray& contents,
                                         int maxOutputTokens,
                                         const QJsonArray& functionDeclarations,
                                         const QJsonObject& structuredSchema) {
    QJsonObject body;
    body.insert(QStringLiteral("model"), model);
    body.insert(QStringLiteral("contents"), contents);
    if (!functionDeclarations.isEmpty()) {
        body.insert(QStringLiteral("tools"),
                    QJsonArray{QJsonObject{
                        {QStringLiteral("functionDeclarations"), functionDeclarations}}});
    }
    QJsonObject generationConfig;
    if (maxOutputTokens > 0) {
        generationConfig.insert(QStringLiteral("maxOutputTokens"), maxOutputTokens);
    }
    if (!structuredSchema.isEmpty()) {
        generationConfig.insert(QStringLiteral("responseMimeType"),
                                QStringLiteral("application/json"));
        generationConfig.insert(QStringLiteral("responseSchema"), structuredSchema);
    }
    if (!generationConfig.isEmpty()) {
        body.insert(QStringLiteral("generationConfig"), generationConfig);
    }
    return body;
}

QJsonArray geminiFunctionResponseParts(const QList<GeminiFunctionResult>& results,
                                       int maxContentLength) {
    QJsonArray parts;
    for (const auto& result : results) {
        QJsonObject functionResponse{
            {QStringLiteral("name"), result.functionName},
            {QStringLiteral("response"),
             QJsonObject{{QStringLiteral("output"), result.content.left(maxContentLength)}}}};
        if (!result.callId.startsWith(QLatin1String("sentinel-gemini-"))) {
            functionResponse.insert(QStringLiteral("id"), result.callId);
        }
        parts.append(QJsonObject{{QStringLiteral("functionResponse"), functionResponse}});
    }
    return parts;
}

} // namespace sentinel::core::provider
