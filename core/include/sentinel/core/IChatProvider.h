#pragma once

#include <QString>

namespace sentinel::core {

enum class ChatProviderStatus {
    Unavailable,
    Ready,
    Error,
};

struct ChatProviderReply {
    bool success = false;
    QString message;
    QString errorMessage;
};

inline QString chatProviderStatusName(ChatProviderStatus status) {
    switch (status) {
    case ChatProviderStatus::Unavailable:
        return QStringLiteral("Unavailable");
    case ChatProviderStatus::Ready:
        return QStringLiteral("Ready");
    case ChatProviderStatus::Error:
        return QStringLiteral("Error");
    }

    return QStringLiteral("Unavailable");
}

class IChatProvider {
public:
    virtual ~IChatProvider() = default;

    virtual QString name() const = 0;
    virtual ChatProviderStatus status() const = 0;
    virtual ChatProviderReply sendMessage(const QString& message) = 0;
};

} // namespace sentinel::core
