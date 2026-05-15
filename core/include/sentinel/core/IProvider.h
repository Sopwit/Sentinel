#pragma once

#include <QString>

namespace sentinel::core {

class IProvider {
public:
    virtual ~IProvider() = default;

    virtual QString name() const = 0;
    virtual QString generateReply(const QString& prompt) = 0;
};

} // namespace sentinel::core
