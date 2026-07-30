#pragma once

#include <QString>
#include <QList>
#include <QMap>
#include "sentinel/core/plugin/PluginManifest.h"

namespace sentinel::core::plugin {

struct ResolutionResult {
    bool success{false};
    QList<QString> loadOrder;
    QString errorMessage;
};

class PluginDependencyResolver {
public:
    PluginDependencyResolver() = default;

    static ResolutionResult resolve(const QList<PluginManifest>& manifests);
};

} // namespace sentinel::core::plugin
