#pragma once

#include <QString>

namespace sentinel::core {

class IPlugin {
public:
    virtual ~IPlugin() = default;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual void initialize() = 0;
    virtual void shutdown() = 0;
};

} // namespace sentinel::core
