#pragma once

#include "sentinel/core/IProvider.h"

namespace sentinel::core {

class FakeProvider final : public IProvider {
public:
    QString name() const override;
    QString generateReply(const QString& prompt) override;
};

} // namespace sentinel::core
