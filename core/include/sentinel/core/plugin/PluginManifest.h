#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include "sentinel/core/plugin/PluginPermissions.h"

namespace sentinel::core::plugin {

struct PluginManifest {
    QString id;
    QString name;
    QString version;
    QString apiVersion;
    QString vendor;
    QString description;
    QString category;
    QString entryPoint;
    PluginPermissions permissions;
    QMap<QString, QString> dependencies;

    bool isValid(QString* errorOut = nullptr) const;
    bool isCompatibleWithCore(const QString& currentCoreVersion) const;

    static PluginManifest parseJson(const QJsonObject& json, QString* errorOut = nullptr);
    static PluginManifest parseFile(const QString& filePath, QString* errorOut = nullptr);
    QJsonObject toJson() const;
};

bool checkVersionRequirement(const QString& actualVersion, const QString& constraint);

} // namespace sentinel::core::plugin
