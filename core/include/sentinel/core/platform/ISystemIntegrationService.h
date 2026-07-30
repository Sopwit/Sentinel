#pragma once

namespace sentinel::core {

class ISystemIntegrationService {
public:
    virtual ~ISystemIntegrationService() = default;

    virtual bool isAvailable() const {
        return false;
    }
};

} // namespace sentinel::core
