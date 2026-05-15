#include "sentinel/core/AppMetadata.h"

namespace sentinel::core {

QString AppMetadata::appId() {
    return QStringLiteral("dev.sentinel.Sentinel");
}

QString AppMetadata::displayName() {
    return QStringLiteral("Sentinel Desktop");
}

QString AppMetadata::version() {
    return QStringLiteral("0.1.0");
}

QString AppMetadata::organizationName() {
    return QStringLiteral("Sentinel");
}

} // namespace sentinel::core
