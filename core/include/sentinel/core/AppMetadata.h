#pragma once

#include <QString>

namespace sentinel::core {

struct AppMetadata {
    static QString appId();
    static QString displayName();
    static QString version();
    static QString organizationName();
};

} // namespace sentinel::core
