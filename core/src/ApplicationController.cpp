#include "sentinel/core/ApplicationController.h"

namespace sentinel::core {

ApplicationController::ApplicationController(std::unique_ptr<IChatProvider> provider,
                                             std::unique_ptr<IMemoryStore> memoryStore,
                                             std::unique_ptr<ChatSession> chatSession,
                                             std::unique_ptr<IChatHistoryStore> chatHistoryStore,
                                             QObject* parent)
    : QObject(parent), provider_(std::move(provider)), memoryStore_(std::move(memoryStore)),
      chatSession_(chatSession ? std::move(chatSession)
                               : std::make_unique<ChatSession>(std::make_unique<SystemClock>())),
      chatHistoryStore_(std::move(chatHistoryStore)) {
    const auto persistedMessages =
        chatHistoryStore_ && chatHistoryStore_->isAvailable() ? chatHistoryStore_->loadMessages()
                                                              : QList<ChatMessage>{};
    if (!persistedMessages.isEmpty()) {
        chatSession_->loadMessages(persistedMessages);
    } else {
        const auto message = chatSession_->appendSystemMessage(QStringLiteral("Sentinel Core online."),
                                                               ChatMessageStatus::Received);
        if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
            chatHistoryStore_->appendMessage(message);
        }
    }
}

QString ApplicationController::providerName() const {
    return provider_ ? provider_->name() : QStringLiteral("No Provider");
}

QString ApplicationController::providerStatus() const {
    return provider_ ? chatProviderStatusName(provider_->status()) : QStringLiteral("Unavailable");
}

QString ApplicationController::memoryStatus() const {
    return memoryStore_ && memoryStore_->isAvailable() ? QStringLiteral("Available")
                                                       : QStringLiteral("Unavailable");
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

    const auto userMessage = chatSession_->appendUserMessage(trimmed);
    if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
        chatHistoryStore_->appendMessage(userMessage);
    }

    if (!provider_ || provider_->status() != ChatProviderStatus::Ready) {
        const auto errorMessage = chatSession_->appendAssistantMessage(
            QStringLiteral("Provider unavailable. Status: %1").arg(providerStatus()),
            ChatMessageStatus::Error);
        if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
            chatHistoryStore_->appendMessage(errorMessage);
        }
        emit chatMessagesChanged();
        return false;
    }

    const auto reply = provider_->sendMessage(trimmed);
    const auto assistantMessage = chatSession_->appendAssistantMessage(
        reply.success ? reply.message
                      : QStringLiteral("Provider error: %1").arg(reply.errorMessage),
        reply.success ? ChatMessageStatus::Received : ChatMessageStatus::Error);
    if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
        chatHistoryStore_->appendMessage(assistantMessage);
    }
    emit chatMessagesChanged();
    return reply.success;
}

void ApplicationController::clearChat() {
    if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
        chatHistoryStore_->clear();
    }
    chatSession_->clear();
    const auto message = chatSession_->appendSystemMessage(QStringLiteral("Sentinel Core online."),
                                                           ChatMessageStatus::Received);
    if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
        chatHistoryStore_->appendMessage(message);
    }
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
