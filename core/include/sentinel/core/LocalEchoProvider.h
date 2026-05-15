#pragma once

#include "sentinel/core/IChatProvider.h"

namespace sentinel::core {

class LocalEchoProvider final : public IChatProvider {
public:
    QString name() const override;
    ChatProviderStatus status() const override;
    ChatProviderReply sendMessage(const QString& message) override;
};

} // namespace sentinel::core
