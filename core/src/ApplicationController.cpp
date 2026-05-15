#include "sentinel/core/ApplicationController.h"

namespace sentinel::core {

ApplicationController::ApplicationController(std::unique_ptr<IChatProvider> provider,
                                             std::unique_ptr<IMemoryStore> memoryStore,
                                             std::unique_ptr<ChatSession> chatSession,
                                             QObject* parent)
    : QObject(parent), provider_(std::move(provider)), memoryStore_(std::move(memoryStore)),
      chatSession_(chatSession ? std::move(chatSession)
                               : std::make_unique<ChatSession>(std::make_unique<SystemClock>())) {
    chatSession_->appendSystemMessage(QStringLiteral("Sentinel Core online."),
                                      ChatMessageStatus::Received);
}

QString ApplicationController::providerName() const {
    return provider_ ? provider_->name() : QStringLiteral("No Provider");
}

QString ApplicationController::providerStatus() const {
    return provider_ ? chatProviderStatusName(provider_->status()) : QStringLiteral("Unavailable");
}

const QList<ChatMessage>& ApplicationController::chatHistory() const {
    return chatSession_->messages();
}

QStringList ApplicationController::chatMessages() const {
    QStringList result;
    for (const auto& message : chatSession_->messages()) {
        if (message.role == ChatRole::User) {
            result.append(QStringLiteral("You: %1").arg(message.content));
        } else {
            result.append(QStringLiteral("Sentinel: %1").arg(message.content));
        }
    }
    return result;
}

QStringList ApplicationController::memoryEntries() const {
    QStringList result;
    if (!memoryStore_) {
        return result;
    }

    for (const auto& [key, value] : memoryStore_->entries()) {
        result.append(QStringLiteral("%1: %2").arg(key, value));
    }

    return result;
}

bool ApplicationController::sendMessage(const QString& message) {
    const auto trimmed = message.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    chatSession_->appendUserMessage(trimmed);

    if (!provider_ || provider_->status() != ChatProviderStatus::Ready) {
        chatSession_->appendAssistantMessage(
            QStringLiteral("Provider unavailable. Status: %1").arg(providerStatus()),
            ChatMessageStatus::Error);
        emit chatMessagesChanged();
        return false;
    }

    const auto reply = provider_->sendMessage(trimmed);
    chatSession_->appendAssistantMessage(
        reply.success ? reply.message
                      : QStringLiteral("Provider error: %1").arg(reply.errorMessage),
        reply.success ? ChatMessageStatus::Received : ChatMessageStatus::Error);
    emit chatMessagesChanged();
    return reply.success;
}

void ApplicationController::clearChat() {
    chatSession_->clear();
    chatSession_->appendSystemMessage(QStringLiteral("Sentinel Core online."),
                                      ChatMessageStatus::Received);
    emit chatMessagesChanged();
}

void ApplicationController::remember(const QString& key, const QString& value) {
    if (!memoryStore_ || key.trimmed().isEmpty()) {
        return;
    }

    memoryStore_->put(key.trimmed(), value.trimmed());
    emit memoryEntriesChanged();
}

} // namespace sentinel::core
