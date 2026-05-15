#pragma once

#include "sentinel/core/ChatMessage.h"

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
