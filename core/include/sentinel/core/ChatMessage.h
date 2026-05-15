#pragma once

#include <QDateTime>
#include <QString>

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
};

struct ChatMessage {
    int id = 0;
    ChatRole role = ChatRole::System;
    QString content;
    QDateTime timestamp;
    ChatMessageStatus status = ChatMessageStatus::Received;
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
    }

    return QStringLiteral("received");
}

} // namespace sentinel::core
