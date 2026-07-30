#pragma once

#include <QString>

namespace sentinel::core {

class IPlatformService {
public:
    virtual ~IPlatformService() = default;

    virtual QString platformName() const = 0;
};

} // namespace sentinel::core
