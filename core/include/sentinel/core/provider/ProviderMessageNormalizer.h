// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace sentinel::core {

struct NormalizedMessage {
    QString role;
    QString content;
    QString provider;
    QJsonObject original;
    QList<QJsonObject> toolCalls;
};

class ProviderMessageNormalizer {
public:
    NormalizedMessage normalize(const QJsonObject& message, const QString& provider) const;
    QJsonObject denormalize(const NormalizedMessage& message) const;
    QList<NormalizedMessage> normalizeConversation(const QJsonArray& messages, const QString& provider) const;
    QString extractText(const QJsonObject& message) const;
    QList<QJsonObject> extractToolCalls(const QJsonObject& message) const;
    QJsonObject injectToolResults(const QJsonObject& message, const QJsonArray& results) const;
};

} // namespace sentinel::core
