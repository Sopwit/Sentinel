#include "sentinel/core/config/ConfigResolver.h"

#include "sentinel/core/config/JsoncParser.h"

#include <QDir>
#include <QFileInfo>

namespace sentinel::core {

QJsonObject ConfigResolver::deepMerge(const QJsonObject& base, const QJsonObject& overlay) {
    QJsonObject result = base;
    for (auto it = overlay.constBegin(); it != overlay.constEnd(); ++it) {
        if (it.value().isObject() && result.value(it.key()).isObject()) {
            result.insert(it.key(),
                          deepMerge(result.value(it.key()).toObject(), it.value().toObject()));
        } else {
            result.insert(it.key(), it.value());
        }
    }
    return result;
}

ResolvedConfig ConfigResolver::resolve(const QString& projectDirectory,
                                       const QJsonObject& globalConfig,
                                       const QJsonObject& managedConfig) {
    ResolvedConfig resolved{globalConfig, {}, {}};
    if (!globalConfig.isEmpty())
        resolved.sources.append(QStringLiteral("global"));

    QStringList directories;
    QDir current(projectDirectory);
    if (!current.exists()) {
        resolved.errors.append(
            QStringLiteral("Project directory does not exist: %1").arg(projectDirectory));
        return resolved;
    }
    while (true) {
        directories.prepend(current.absolutePath());
        const QString parent = QFileInfo(current.absolutePath()).dir().absolutePath();
        if (parent == current.absolutePath())
            break;
        current.setPath(parent);
    }

    for (const QString& directory : directories) {
        for (const QString& filename :
             {QStringLiteral("sentinel.json"), QStringLiteral("sentinel.jsonc")}) {
            const QString path = QDir(directory).filePath(filename);
            if (!QFileInfo::exists(path))
                continue;
            QString error;
            const QJsonObject layer = JsoncParser::parseFile(path, error);
            if (!error.isEmpty()) {
                resolved.errors.append(QStringLiteral("%1: %2").arg(path, error));
            } else {
                resolved.value = deepMerge(resolved.value, layer);
                resolved.sources.append(path);
            }
        }
    }
    if (!managedConfig.isEmpty()) {
        resolved.value = deepMerge(resolved.value, managedConfig);
        resolved.sources.append(QStringLiteral("managed"));
    }
    return resolved;
}

} // namespace sentinel::core
