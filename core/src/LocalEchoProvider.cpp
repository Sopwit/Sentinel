#include "sentinel/core/LocalEchoProvider.h"

namespace sentinel::core {

QString LocalEchoProvider::name() const {
    return QStringLiteral("LocalEchoProvider");
}

ChatProviderStatus LocalEchoProvider::status() const {
    return ChatProviderStatus::Ready;
}

ChatProviderReply LocalEchoProvider::sendMessage(const QString& message) {
    Q_UNUSED(message);
    return {
        true,
        QStringLiteral("Sentinel Core online. Local chat pipeline is active."),
        {},
    };
}

} // namespace sentinel::core
