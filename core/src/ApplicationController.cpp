#include "sentinel/core/ApplicationController.h"

namespace sentinel::core {

ApplicationController::ApplicationController(std::unique_ptr<IProvider> provider,
                                             std::unique_ptr<IMemoryStore> memoryStore,
                                             QObject* parent)
    : QObject(parent), provider_(std::move(provider)), memoryStore_(std::move(memoryStore)) {
    chatMessages_.append(QStringLiteral("Sentinel: Sentinel Core online."));
}

QString ApplicationController::providerName() const {
    return provider_ ? provider_->name() : QStringLiteral("No Provider");
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

void ApplicationController::sendMessage(const QString& message) {
    const auto trimmed = message.trimmed();
    if (trimmed.isEmpty() || !provider_) {
        return;
    }

    chatMessages_.append(QStringLiteral("You: %1").arg(trimmed));
    chatMessages_.append(QStringLiteral("Sentinel: %1").arg(provider_->generateReply(trimmed)));
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
