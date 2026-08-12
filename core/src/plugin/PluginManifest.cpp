// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/plugin/PluginManifest.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

namespace sentinel::core::plugin {

struct SemVer {
    int major{0};
    int minor{0};
    int patch{0};

    static SemVer parse(const QString& str) {
        SemVer ver;
        static const QRegularExpression regex(QStringLiteral(R"(^v?(\d+)(?:\.(\d+))?(?:\.(\d+))?)"));
        auto match = regex.match(str.trimmed());
        if (match.hasMatch()) {
            ver.major = match.captured(1).toInt();
            ver.minor = match.captured(2).isEmpty() ? 0 : match.captured(2).toInt();
            ver.patch = match.captured(3).isEmpty() ? 0 : match.captured(3).toInt();
        }
        return ver;
    }

    int compare(const SemVer& other) const {
        if (major != other.major) return major < other.major ? -1 : 1;
        if (minor != other.minor) return minor < other.minor ? -1 : 1;
        if (patch != other.patch) return patch < other.patch ? -1 : 1;
        return 0;
    }
};

bool checkVersionRequirement(const QString& actualVersion, const QString& constraintStr) {
    if (constraintStr.isEmpty() || constraintStr == QStringLiteral("*")) {
        return true;
    }

    QString op = QStringLiteral("==");
    QString reqVerStr = constraintStr.trimmed();

    if (reqVerStr.startsWith(QStringLiteral(">="))) {
        op = QStringLiteral(">=");
        reqVerStr = reqVerStr.mid(2).trimmed();
    } else if (reqVerStr.startsWith(QStringLiteral("<="))) {
        op = QStringLiteral("<=");
        reqVerStr = reqVerStr.mid(2).trimmed();
    } else if (reqVerStr.startsWith(QStringLiteral(">"))) {
        op = QStringLiteral(">");
        reqVerStr = reqVerStr.mid(1).trimmed();
    } else if (reqVerStr.startsWith(QStringLiteral("<"))) {
        op = QStringLiteral("<");
        reqVerStr = reqVerStr.mid(1).trimmed();
    } else if (reqVerStr.startsWith(QStringLiteral("=="))) {
        op = QStringLiteral("==");
        reqVerStr = reqVerStr.mid(2).trimmed();
    }

    SemVer actual = SemVer::parse(actualVersion);
    SemVer required = SemVer::parse(reqVerStr);
    int cmp = actual.compare(required);

    if (op == QStringLiteral(">=")) return cmp >= 0;
    if (op == QStringLiteral("<=")) return cmp <= 0;
    if (op == QStringLiteral(">")) return cmp > 0;
    if (op == QStringLiteral("<")) return cmp < 0;
    if (op == QStringLiteral("==")) return cmp == 0;

    return cmp >= 0;
}

bool PluginManifest::isValid(QString* errorOut) const {
    if (id.trimmed().isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("Plugin manifest is missing 'id'");
        return false;
    }
    if (name.trimmed().isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("Plugin manifest is missing 'name'");
        return false;
    }
    if (version.trimmed().isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("Plugin manifest is missing 'version'");
        return false;
    }
    if (entryPoint.trimmed().isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("Plugin manifest is missing 'entry_point'");
        return false;
    }
    return true;
}

bool PluginManifest::isCompatibleWithCore(const QString& currentCoreVersion) const {
    if (dependencies.contains(QStringLiteral("dev.sentinel.core"))) {
        return checkVersionRequirement(currentCoreVersion, dependencies.value(QStringLiteral("dev.sentinel.core")));
    }
    return true;
}

PluginManifest PluginManifest::parseJson(const QJsonObject& json, QString* errorOut) {
    PluginManifest manifest;
    manifest.id = json.value(QStringLiteral("id")).toString();
    manifest.name = json.value(QStringLiteral("name")).toString();
    manifest.version = json.value(QStringLiteral("version")).toString();
    manifest.apiVersion = json.value(QStringLiteral("api_version")).toString();
    manifest.vendor = json.value(QStringLiteral("vendor")).toString();
    manifest.description = json.value(QStringLiteral("description")).toString();
    manifest.category = json.value(QStringLiteral("category")).toString();
    manifest.entryPoint = json.value(QStringLiteral("entry_point")).toString();

    if (json.contains(QStringLiteral("permissions")) && json.value(QStringLiteral("permissions")).isArray()) {
        manifest.permissions = PluginPermissions::fromJsonArray(json.value(QStringLiteral("permissions")).toArray());
    }

    if (json.contains(QStringLiteral("dependencies")) && json.value(QStringLiteral("dependencies")).isObject()) {
        QJsonObject deps = json.value(QStringLiteral("dependencies")).toObject();
        for (auto it = deps.begin(); it != deps.end(); ++it) {
            manifest.dependencies.insert(it.key(), it.value().toString());
        }
    }

    if (!manifest.isValid(errorOut)) {
        return PluginManifest();
    }
    return manifest;
}

PluginManifest PluginManifest::parseFile(const QString& filePath, QString* errorOut) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorOut) *errorOut = QStringLiteral("Failed to open file: %1").arg(filePath);
        return PluginManifest();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorOut) *errorOut = QStringLiteral("JSON parse error in %1: %2").arg(filePath, parseError.errorString());
        return PluginManifest();
    }

    return parseJson(doc.object(), errorOut);
}

QJsonObject PluginManifest::toJson() const {
    QJsonObject json;
    json[QStringLiteral("id")] = id;
    json[QStringLiteral("name")] = name;
    json[QStringLiteral("version")] = version;
    json[QStringLiteral("api_version")] = apiVersion;
    json[QStringLiteral("vendor")] = vendor;
    json[QStringLiteral("description")] = description;
    json[QStringLiteral("category")] = category;
    json[QStringLiteral("entry_point")] = entryPoint;
    json[QStringLiteral("permissions")] = permissions.toJsonArray();

    QJsonObject deps;
    for (auto it = dependencies.begin(); it != dependencies.end(); ++it) {
        deps[it.key()] = it.value();
    }
    json[QStringLiteral("dependencies")] = deps;

    return json;
}

} // namespace sentinel::core::plugin
