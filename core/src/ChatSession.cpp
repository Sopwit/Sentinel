#include "sentinel/core/ChatSession.h"

namespace sentinel::core {

ChatSession::ChatSession(std::unique_ptr<IClock> clock) : clock_(std::move(clock)) {}

const QList<ChatMessage>& ChatSession::messages() const {
    return messages_;
}

ChatMessage ChatSession::appendSystemMessage(const QString& content, ChatMessageStatus status) {
    return append(ChatRole::System, content, status);
}

ChatMessage ChatSession::appendUserMessage(const QString& content) {
    return append(ChatRole::User, content, ChatMessageStatus::Sent);
}

ChatMessage ChatSession::appendAssistantMessage(const QString& content, ChatMessageStatus status) {
    return append(ChatRole::Assistant, content, status);
}

void ChatSession::clear() {
    messages_.clear();
    nextId_ = 1;
}

ChatMessage ChatSession::append(ChatRole role, QString content, ChatMessageStatus status) {
    ChatMessage message{
        nextId_++, role, std::move(content), clock_ ? clock_->nowUtc() : QDateTime(), status,
    };

    messages_.append(message);
    return message;
}

} // namespace sentinel::core
