#include "sentinel/core/ApplicationController.h"

namespace sentinel::core {

ApplicationController::ApplicationController(std::unique_ptr<IChatProvider> provider,
                                             std::unique_ptr<IMemoryStore> memoryStore,
                                             QObject* parent)
    : QObject(parent), provider_(std::move(provider)), memoryStore_(std::move(memoryStore)) {
    chatMessages_.append(QStringLiteral("Sentinel: Sentinel Core online."));
}

QString ApplicationController::providerName() const {
    return provider_ ? provider_->name() : QStringLiteral("No Provider");
}

QString ApplicationController::providerStatus() const {
    return provider_ ? chatProviderStatusName(provider_->status()) : QStringLiteral("Unavailable");
}

QStringList ApplicationController::chatMessages() const {
    return chatMessages_;
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

    chatMessages_.append(QStringLiteral("You: %1").arg(trimmed));

    if (!provider_ || provider_->status() != ChatProviderStatus::Ready) {
        chatMessages_.append(
            QStringLiteral("Sentinel: Provider unavailable. Status: %1").arg(providerStatus()));
        emit chatMessagesChanged();
        return false;
    }

    const auto reply = provider_->sendMessage(trimmed);
    chatMessages_.append(
        QStringLiteral("Sentinel: %1")
            .arg(reply.success ? reply.message
                               : QStringLiteral("Provider error: %1").arg(reply.errorMessage)));
    emit chatMessagesChanged();
    return reply.success;
}

void ApplicationController::remember(const QString& key, const QString& value) {
    if (!memoryStore_ || key.trimmed().isEmpty()) {
        return;
    }

    memoryStore_->put(key.trimmed(), value.trimmed());
    emit memoryEntriesChanged();
}

} // namespace sentinel::core
