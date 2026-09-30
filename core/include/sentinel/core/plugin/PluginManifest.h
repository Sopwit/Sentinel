// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/plugin/PluginPermissions.h"
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QList>

namespace sentinel::core::plugin {

struct PluginCredentialDeclaration {
    QString id;
    QString labelId;
    QString kind;
    bool required = false;
    QStringList allowedHosts;
};

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
    QStringList hostCapabilities;
    QMap<QString, QString> dependencies;
    QList<PluginCredentialDeclaration> credentials;

    bool isValid(QString* errorOut = nullptr) const;
    bool isCompatibleWithCore(const QString& currentCoreVersion) const;

    static PluginManifest parseJson(const QJsonObject& json, QString* errorOut = nullptr);
    static PluginManifest parseFile(const QString& filePath, QString* errorOut = nullptr);
    QJsonObject toJson() const;
};

bool checkVersionRequirement(const QString& actualVersion, const QString& constraint);

} // namespace sentinel::core::plugin
