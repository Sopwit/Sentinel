// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/provider/ProviderMessageNormalizer.h"

namespace sentinel::core {

NormalizedMessage ProviderMessageNormalizer::normalize(const QJsonObject& message,
                                                       const QString& provider) const {
    NormalizedMessage nm;
    nm.provider = provider;
    nm.original = message;
    nm.role = message["role"].toString();
    nm.content = extractText(message);
    nm.toolCalls = extractToolCalls(message);
    return nm;
}

QJsonObject ProviderMessageNormalizer::denormalize(const NormalizedMessage& message) const {
    return message.original;
}

QList<NormalizedMessage>
ProviderMessageNormalizer::normalizeConversation(const QJsonArray& messages,
                                                 const QString& provider) const {
    QList<NormalizedMessage> result;
    for (const auto& msg : messages) {
        result.append(normalize(msg.toObject(), provider));
    }
    return result;
}

QString ProviderMessageNormalizer::extractText(const QJsonObject& message) const {
    if (message["content"].isString()) {
        return message["content"].toString();
    }
    if (message["content"].isArray()) {
        QString text;
        for (const auto& part : message["content"].toArray()) {
            QJsonObject p = part.toObject();
            if (p["type"].toString() == "text") {
                text += p["text"].toString();
            }
        }
        return text;
    }
    return {};
}

QList<QJsonObject> ProviderMessageNormalizer::extractToolCalls(const QJsonObject& message) const {
    QList<QJsonObject> calls;
    if (message.contains("tool_calls")) {
        for (const auto& tc : message["tool_calls"].toArray()) {
            calls.append(tc.toObject());
        }
    }
    return calls;
}

QJsonObject ProviderMessageNormalizer::injectToolResults(const QJsonObject& message,
                                                         const QJsonArray& results) const {
    QJsonObject msg = message;
    msg["tool_results"] = results;
    return msg;
}

} // namespace sentinel::core
