#pragma once

namespace sentinel::core {

class INotificationService {
public:
    virtual ~INotificationService() = default;

    virtual bool isAvailable() const {
        return false;
    }
};

} // namespace sentinel::core
