#pragma once

#include <QString>

namespace sentinel::core {

class IIntegration {
public:
    virtual ~IIntegration() = default;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual bool isAvailable() const = 0;
};

} // namespace sentinel::core
