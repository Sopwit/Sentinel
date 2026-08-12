// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/chat/ChatMessage.h"
#include "sentinel/core/app/IClock.h"

#include <QList>
#include <QString>
#include <memory>

namespace sentinel::core {

class ChatSession final {
public:
    explicit ChatSession(std::unique_ptr<IClock> clock);

    const QList<ChatMessage>& messages() const;
    ChatMessage appendSystemMessage(const QString& content, ChatMessageStatus status);
    ChatMessage appendUserMessage(const QString& content);
    ChatMessage appendAssistantMessage(const QString& content, ChatMessageStatus status);
    void loadMessages(QList<ChatMessage> messages);
    void clear();

private:
    ChatMessage append(ChatRole role, QString content, ChatMessageStatus status);

    std::unique_ptr<IClock> clock_;
    QList<ChatMessage> messages_;
    int nextId_ = 1;
};

} // namespace sentinel::core
