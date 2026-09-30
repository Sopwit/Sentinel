// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/plugin/PluginManifest.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>

namespace sentinel::core::plugin {

struct SemVer {
    int major{0};
    int minor{0};
    int patch{0};

    static SemVer parse(const QString& str) {
        SemVer ver;
        static const QRegularExpression regex(
            QStringLiteral(R"(^v?(\d+)(?:\.(\d+))?(?:\.(\d+))?)"));
        auto match = regex.match(str.trimmed());
        if (match.hasMatch()) {
            ver.major = match.captured(1).toInt();
            ver.minor = match.captured(2).isEmpty() ? 0 : match.captured(2).toInt();
            ver.patch = match.captured(3).isEmpty() ? 0 : match.captured(3).toInt();
        }
        return ver;
    }

    int compare(const SemVer& other) const {
        if (major != other.major)
            return major < other.major ? -1 : 1;
        if (minor != other.minor)
            return minor < other.minor ? -1 : 1;
        if (patch != other.patch)
            return patch < other.patch ? -1 : 1;
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

    if (op == QStringLiteral(">="))
        return cmp >= 0;
    if (op == QStringLiteral("<="))
        return cmp <= 0;
    if (op == QStringLiteral(">"))
        return cmp > 0;
    if (op == QStringLiteral("<"))
        return cmp < 0;
    if (op == QStringLiteral("=="))
        return cmp == 0;

    return cmp >= 0;
}

bool PluginManifest::isValid(QString* errorOut) const {
    static const QRegularExpression ownerId(QStringLiteral("^[A-Za-z0-9._-]{1,80}$"));
    if (!ownerId.match(id).hasMatch() || id != id.toLower() ||
        id == QStringLiteral(".") || id == QStringLiteral("..")) {
        if (errorOut)
            *errorOut = QStringLiteral("Plugin manifest is missing 'id'");
        return false;
    }
    if (name.trimmed().isEmpty()) {
        if (errorOut)
            *errorOut = QStringLiteral("Plugin manifest is missing 'name'");
        return false;
    }
    if (version.trimmed().isEmpty()) {
        if (errorOut)
            *errorOut = QStringLiteral("Plugin manifest is missing 'version'");
        return false;
    }
    static const QRegularExpression entryName(QStringLiteral("^[A-Za-z0-9._-]{1,128}$"));
    if (!entryName.match(entryPoint).hasMatch() ||
        entryPoint == QStringLiteral(".") || entryPoint == QStringLiteral("..")) {
        if (errorOut)
            *errorOut = QStringLiteral("Plugin manifest is missing 'entry_point'");
        return false;
    }
    QSet<QString> credentialIds;
    QSet<QString> capabilityIds;
    const QStringList knownCapabilities{QStringLiteral("FilesystemRead"),
        QStringLiteral("FilesystemWrite"), QStringLiteral("NetworkRequest"),
        QStringLiteral("ProcessExecute")};
    for (const auto& capability : hostCapabilities) {
        if (!knownCapabilities.contains(capability) || capabilityIds.contains(capability)) {
            if (errorOut) *errorOut = QStringLiteral("Invalid host capability declaration");
            return false;
        }
        capabilityIds.insert(capability);
    }
    static const QRegularExpression credentialId(QStringLiteral("^[A-Za-z0-9._-]{1,80}$"));
    for (const auto& credential : credentials) {
        if (!hostCapabilities.contains(QStringLiteral("NetworkRequest")) ||
            credential.allowedHosts.isEmpty() ||
            !credentialId.match(credential.id).hasMatch() ||
            !credentialId.match(credential.labelId).hasMatch() ||
            !QStringList{QStringLiteral("apiKey"), QStringLiteral("token")}.contains(credential.kind) ||
            credentialIds.contains(credential.id)) {
            if (errorOut) *errorOut = QStringLiteral("Invalid plugin credential declaration");
            return false;
        }
        credentialIds.insert(credential.id);
        QSet<QString> hosts;
        for (const auto& host : credential.allowedHosts) {
            const QUrl url(QStringLiteral("https://") + host);
            static const QRegularExpression hostName(QStringLiteral("^[a-z0-9.-]{1,253}$"));
            if (!hostName.match(host).hasMatch() || host.startsWith(QLatin1Char('.')) ||
                host.endsWith(QLatin1Char('.')) || host.contains(QStringLiteral("..")) ||
                host != host.toLower() || host.contains(QLatin1Char('/')) ||
                host.contains(QLatin1Char('@')) || url.host() != host || hosts.contains(host)) {
                if (errorOut) *errorOut = QStringLiteral("Invalid credential host binding");
                return false;
            }
            hosts.insert(host);
        }
    }
    return true;
}

bool PluginManifest::isCompatibleWithCore(const QString& currentCoreVersion) const {
    if (dependencies.contains(QStringLiteral("dev.sentinel.core"))) {
        return checkVersionRequirement(currentCoreVersion,
                                       dependencies.value(QStringLiteral("dev.sentinel.core")));
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
    if (json.contains(QStringLiteral("host_capabilities"))) {
        const auto capabilities = json.value(QStringLiteral("host_capabilities"));
        if (!capabilities.isArray() || capabilities.toArray().size() > 4) {
            if (errorOut) *errorOut = QStringLiteral("Invalid host capability declarations");
            return {};
        }
        for (const auto& value : capabilities.toArray()) {
            if (!value.isString()) {
                if (errorOut) *errorOut = QStringLiteral("Invalid host capability declaration");
                return {};
            }
            manifest.hostCapabilities.append(value.toString());
        }
    }
    if (json.contains(QStringLiteral("credentials"))) {
        if (json.value(QStringLiteral("credential_schema_version")).toInt(-1) != 1) {
            if (errorOut) *errorOut = QStringLiteral("Unsupported credential schema version");
            return {};
        }
        if (!json.value(QStringLiteral("credentials")).isArray() ||
            json.value(QStringLiteral("credentials")).toArray().size() > 20) {
            if (errorOut) *errorOut = QStringLiteral("Invalid credential declarations");
            return {};
        }
        for (const auto& value : json.value(QStringLiteral("credentials")).toArray()) {
            if (!value.isObject()) return {};
            const auto item = value.toObject();
            for (auto field = item.constBegin(); field != item.constEnd(); ++field) {
                if (!QStringList{QStringLiteral("id"), QStringLiteral("label_id"),
                                 QStringLiteral("kind"), QStringLiteral("required"),
                                 QStringLiteral("allowed_hosts")}.contains(field.key())) {
                    if (errorOut) *errorOut = QStringLiteral("Unsupported credential manifest field");
                    return {};
                }
            }
            if (!item.value(QStringLiteral("id")).isString() ||
                !item.value(QStringLiteral("label_id")).isString() ||
                !item.value(QStringLiteral("kind")).isString() ||
                (item.contains(QStringLiteral("required")) &&
                 !item.value(QStringLiteral("required")).isBool()) ||
                (item.contains(QStringLiteral("allowed_hosts")) &&
                 !item.value(QStringLiteral("allowed_hosts")).isArray()) ||
                item.contains(QStringLiteral("value")) || item.contains(QStringLiteral("secret"))) {
                if (errorOut) *errorOut = QStringLiteral("Credential values are forbidden in manifests");
                return {};
            }
            QStringList allowedHosts;
            for (const auto& host : item.value(QStringLiteral("allowed_hosts")).toArray()) {
                if (!host.isString() || allowedHosts.size() >= 16) {
                    if (errorOut) *errorOut = QStringLiteral("Invalid credential host binding");
                    return {};
                }
                allowedHosts.append(host.toString());
            }
            manifest.credentials.append({item.value(QStringLiteral("id")).toString(),
                item.value(QStringLiteral("label_id")).toString(),
                item.value(QStringLiteral("kind")).toString(),
                item.value(QStringLiteral("required")).toBool(), allowedHosts});
        }
    }

    if (json.contains(QStringLiteral("permissions")) &&
        json.value(QStringLiteral("permissions")).isArray()) {
        manifest.permissions =
            PluginPermissions::fromJsonArray(json.value(QStringLiteral("permissions")).toArray());
    }

    if (json.contains(QStringLiteral("dependencies")) &&
        json.value(QStringLiteral("dependencies")).isObject()) {
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
        if (errorOut)
            *errorOut = QStringLiteral("Failed to open file: %1").arg(filePath);
        return PluginManifest();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorOut)
            *errorOut = QStringLiteral("JSON parse error in %1: %2")
                            .arg(filePath, parseError.errorString());
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
    QJsonArray capabilitiesJson;
    for (const auto& capability : hostCapabilities) capabilitiesJson.append(capability);
    json[QStringLiteral("host_capabilities")] = capabilitiesJson;
    QJsonArray credentialsJson;
    for (const auto& credential : credentials)
        credentialsJson.append(QJsonObject{{QStringLiteral("id"), credential.id},
            {QStringLiteral("label_id"), credential.labelId},
            {QStringLiteral("kind"), credential.kind},
            {QStringLiteral("required"), credential.required},
            {QStringLiteral("allowed_hosts"), QJsonArray::fromStringList(credential.allowedHosts)}});
    json[QStringLiteral("credentials")] = credentialsJson;
    json[QStringLiteral("credential_schema_version")] = 1;

    QJsonObject deps;
    for (auto it = dependencies.begin(); it != dependencies.end(); ++it) {
        deps[it.key()] = it.value();
    }
    json[QStringLiteral("dependencies")] = deps;

    return json;
}

} // namespace sentinel::core::plugin
