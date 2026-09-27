// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QString>
#include "sentinel/core/interfaces/IChatProvider.h"

namespace sentinel::core {

enum class ChatRole {
    System,
    User,
    Assistant,
};

enum class ChatMessageStatus {
    Sent,
    Received,
    Error,
    Queued,
    Sending,
    Streaming,
    Completed,
    Failed,
    Cancelled,
    Interrupted,
};

struct ChatMessage {
    int id = 0;
    ChatRole role = ChatRole::System;
    QString content;
    QDateTime timestamp;
    ChatMessageStatus status = ChatMessageStatus::Received;
    QString providerUsed;
    QString modelUsed;
    QString roleUsed;
    qint64 responseDurationMs = -1;
    qint64 firstTokenLatencyMs = -1;
    double approximateTokensPerSecond = 0.0;
    int replyToMessageId = 0;
    int replacesMessageId = 0;
    bool partial = false;
    ChatProviderErrorCategory errorCategory = ChatProviderErrorCategory::None;
};

inline QString chatRoleName(ChatRole role) {
    switch (role) {
    case ChatRole::System:
        return QStringLiteral("system");
    case ChatRole::User:
        return QStringLiteral("user");
    case ChatRole::Assistant:
        return QStringLiteral("assistant");
    }

    return QStringLiteral("system");
}

inline QString chatMessageStatusName(ChatMessageStatus status) {
    switch (status) {
    case ChatMessageStatus::Sent:
        return QStringLiteral("sent");
    case ChatMessageStatus::Received:
        return QStringLiteral("received");
    case ChatMessageStatus::Error:
        return QStringLiteral("error");
    case ChatMessageStatus::Queued: return QStringLiteral("queued");
    case ChatMessageStatus::Sending: return QStringLiteral("sending");
    case ChatMessageStatus::Streaming: return QStringLiteral("streaming");
    case ChatMessageStatus::Completed: return QStringLiteral("completed");
    case ChatMessageStatus::Failed: return QStringLiteral("failed");
    case ChatMessageStatus::Cancelled: return QStringLiteral("cancelled");
    case ChatMessageStatus::Interrupted: return QStringLiteral("interrupted");
    }

    return QStringLiteral("received");
}

} // namespace sentinel::core
