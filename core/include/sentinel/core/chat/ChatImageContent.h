// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/interfaces/IChatProvider.h"
#include <QJsonArray>
namespace sentinel::core {
inline QJsonValue chatImageContent(const QString& text, const QList<ChatImage>& images, bool anthropic = false) {
    if (images.isEmpty()) return text;
    QJsonArray parts{QJsonObject{{"type", "text"}, {"text", text}}};
    for (const auto& image : images) {
        const auto base64 = QString::fromLatin1(image.bytes.toBase64());
        if (anthropic) parts.append(QJsonObject{{"type", "image"}, {"source", QJsonObject{{"type", "base64"}, {"media_type", image.mimeType}, {"data", base64}}}});
        else parts.append(QJsonObject{{"type", "image_url"}, {"image_url", QJsonObject{{"url", "data:" + image.mimeType + ";base64," + base64}}}});
    }
    return parts;
}
inline void appendGeminiImageParts(QJsonArray& parts, const QList<ChatImage>& images) {
    for (const auto& image : images) parts.append(QJsonObject{{"inlineData", QJsonObject{{"mimeType", image.mimeType}, {"data", QString::fromLatin1(image.bytes.toBase64())}}}});
}
}
