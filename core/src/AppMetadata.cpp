#include "sentinel/core/AppMetadata.h"

#include "sentinel/core/AppBuildConfig.h"

#include <QSysInfo>

namespace sentinel::core {

QString AppMetadata::appId() {
    return QString::fromLatin1(SENTINEL_APP_ID);
}

QString AppMetadata::displayName() {
    return QString::fromLatin1(SENTINEL_DISPLAY_NAME);
}

QString AppMetadata::version() {
    return QString::fromLatin1(SENTINEL_APP_VERSION);
}

QString AppMetadata::projectVersion() {
    return QString::fromLatin1(SENTINEL_PROJECT_VERSION);
}

QString AppMetadata::buildNumber() {
    return QString::fromLatin1(SENTINEL_BUILD_NUMBER);
}

QString AppMetadata::gitCommit() {
    return QString::fromLatin1(SENTINEL_GIT_COMMIT);
}

QString AppMetadata::buildType() {
    const auto configured = QString::fromLatin1(SENTINEL_BUILD_TYPE);
    return configured.isEmpty() ? QStringLiteral("unknown") : configured;
}

QString AppMetadata::platform() {
    return QSysInfo::prettyProductName();
}

QString AppMetadata::architecture() {
    return QSysInfo::currentCpuArchitecture();
}

QString AppMetadata::organizationName() {
    return QString::fromLatin1(SENTINEL_ORGANIZATION_NAME);
}

QString AppMetadata::copyright() {
    return QString::fromLatin1(SENTINEL_COPYRIGHT);
}

QStringList AppMetadata::safeBuildSummaries() {
    return {
        QStringLiteral("Version: %1").arg(version()),
        QStringLiteral("Build number: %1").arg(buildNumber()),
        QStringLiteral("Git commit: %1").arg(gitCommit()),
        QStringLiteral("Build type: %1").arg(buildType()),
        QStringLiteral("Platform: %1").arg(platform()),
        QStringLiteral("Architecture: %1").arg(architecture()),
    };
}

} // namespace sentinel::core
