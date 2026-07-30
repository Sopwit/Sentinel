#include "sentinel/core/plugin/PluginDependencyResolver.h"
#include <QQueue>
#include <QSet>

namespace sentinel::core::plugin {

ResolutionResult PluginDependencyResolver::resolve(const QList<PluginManifest>& manifests) {
    ResolutionResult result;
    QMap<QString, PluginManifest> manifestMap;
    for (const auto& m : manifests) {
        manifestMap.insert(m.id, m);
    }

    // Filter dependencies to plugin-to-plugin dependencies (excluding dev.sentinel.core)
    QMap<QString, QSet<QString>> inEdges;  // plugin -> set of plugins depending on it
    QMap<QString, int> inDegree;          // plugin -> count of unfulfilled plugin dependencies

    for (const auto& m : manifests) {
        inDegree[m.id] = 0;
    }

    for (const auto& m : manifests) {
        for (auto it = m.dependencies.begin(); it != m.dependencies.end(); ++it) {
            const QString depId = it.key();
            const QString reqVer = it.value();

            if (depId == QStringLiteral("dev.sentinel.core")) {
                continue;
            }

            if (!manifestMap.contains(depId)) {
                result.success = false;
                result.errorMessage = QStringLiteral("Missing plugin dependency '%1' required by '%2'").arg(depId, m.id);
                return result;
            }

            const auto& depManifest = manifestMap[depId];
            if (!checkVersionRequirement(depManifest.version, reqVer)) {
                result.success = false;
                result.errorMessage = QStringLiteral("Plugin '%1' requires '%2' version '%3', but found version '%4'")
                                          .arg(m.id, depId, reqVer, depManifest.version);
                return result;
            }

            inEdges[depId].insert(m.id);
            inDegree[m.id]++;
        }
    }

    // Kahn's algorithm for Topological Sort
    QQueue<QString> readyQueue;
    for (auto it = inDegree.begin(); it != inDegree.end(); ++it) {
        if (it.value() == 0) {
            readyQueue.enqueue(it.key());
        }
    }

    QList<QString> order;
    while (!readyQueue.isEmpty()) {
        QString current = readyQueue.dequeue();
        order.append(current);

        for (const QString& dependent : inEdges.value(current)) {
            inDegree[dependent]--;
            if (inDegree[dependent] == 0) {
                readyQueue.enqueue(dependent);
            }
        }
    }

    if (order.size() != manifests.size()) {
        result.success = false;
        result.errorMessage = QStringLiteral("Circular dependency detected among plugins");
        return result;
    }

    result.success = true;
    result.loadOrder = order;
    return result;
}

} // namespace sentinel::core::plugin
