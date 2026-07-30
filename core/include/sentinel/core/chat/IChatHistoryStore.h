// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/chat/ChatMessage.h"

#include <QList>
#include <QString>

namespace sentinel::core {

class IChatHistoryStore {
public:
    virtual ~IChatHistoryStore() = default;

    virtual QList<ChatMessage> loadMessages() const = 0;
    virtual void appendMessage(const ChatMessage& message) = 0;
    virtual void clear() = 0;
    virtual bool isAvailable() const {
        return true;
    }
    virtual QString lastError() const {
        return {};
    }
};

} // namespace sentinel::core
