#include "sentinel/core/config/RuntimeFeatureFlags.h"

#include <QProcessEnvironment>

namespace sentinel::core {

bool RuntimeFeatureFlags::enabled(const QString& name, bool defaultValue) const {
    const auto overrideIt = overrides_.constFind(name);
    if (overrideIt != overrides_.constEnd()) {
        return overrideIt.value();
    }

    const QString raw = value(name);
    if (raw.isEmpty()) {
        return defaultValue;
    }
    return raw.compare(QStringLiteral("1"), Qt::CaseInsensitive) == 0 ||
           raw.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 ||
           raw.compare(QStringLiteral("yes"), Qt::CaseInsensitive) == 0 ||
           raw.compare(QStringLiteral("on"), Qt::CaseInsensitive) == 0;
}

QString RuntimeFeatureFlags::value(const QString& name, const QString& defaultValue) const {
    return QProcessEnvironment::systemEnvironment().value(name, defaultValue);
}

bool RuntimeFeatureFlags::experimental() const {
    return enabled(QStringLiteral("SENTINEL_EXPERIMENTAL"));
}

void RuntimeFeatureFlags::setOverride(const QString& name, bool enabledValue) {
    overrides_.insert(name, enabledValue);
}

void RuntimeFeatureFlags::clearOverrides() {
    overrides_.clear();
}

} // namespace sentinel::core
