// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/desktop/ExtensionsHubPresentation.h"
#include <QCoreApplication>
#include <QSet>
namespace sentinel::desktop {
namespace {
QString text(const char* value) {
    return QCoreApplication::translate("ExtensionsHub", value);
}
QVariantMap phase(const QString& state, const QString& message) {
    return {{"phase", state},
            {"message", message},
            {"entries", QVariantList{}},
            {"tools", QStringList{}},
            {"policy", QStringList{}}};
}
} // namespace
QVariantMap ExtensionsHubPresentation::inventory(bool connected, bool loaded,
                                                 const QVariantMap& settings,
                                                 const QStringList& toolIds,
                                                 const QStringList& policy) {
    if (!connected)
        return phase("disconnected",
                     text("Daemon disconnected. Extension and tool state is unavailable."));
    if (!loaded || settings.isEmpty())
        return phase("loading", text("Loading extension metadata from the daemon…"));
    if (settings.contains("code"))
        return phase("error", text("Extension metadata could not be loaded. Retry or review daemon "
                                   "connection details."));
    if (settings.value("value").metaType().id() != QMetaType::QVariantList)
        return phase("unavailable", text("The daemon did not supply an extension inventory."));
    QVariantList entries;
    QSet<QString> seen;
    for (const auto& value : settings.value("value").toList()) {
        const auto row = value.toMap();
        const auto key = row.value("id").toString();
        if (!key.startsWith("extensions."))
            continue;
        const auto id = key.mid(11);
        QString type;
        // These namespaces are published by the existing ExtensionService settings rows.
        if (id.startsWith("plugin:"))
            type = "plugins";
        else if (id.startsWith("mcp:"))
            type = "mcp";
        else if (id.startsWith("skill:"))
            type = "skills";
        else
            continue; // Unknown backend types must not be misclassified.
        if (seen.contains(id))
            continue;
        seen.insert(id);
        const auto keywords = row.value("keywords").toStringList();
        const bool known = row.value("value").metaType().id() == QMetaType::Bool;
        entries.append(
            QVariantMap{{"id", id},
                        {"type", type},
                        {"name", keywords.size() > 1 ? keywords.at(1) : id},
                        {"enabledKnown", known},
                        {"enabled", known && row.value("value").toBool()},
                        {"scope", row.value("source").toString()},
                        {"failure", row.value("unavailableReason").toString().left(160)}});
    }
    auto result = phase("ready", text("Read-only daemon metadata. Discovery and enablement do not "
                                      "grant permission to execute."));
    result["entries"] = entries;
    result["tools"] = toolIds;
    result["policy"] = policy;
    return result;
}
QVariantMap ExtensionsHubPresentation::detail(bool connected, const QString& id,
                                              const QVariantMap& response, bool answered) {
    if (!connected)
        return {{"phase", "disconnected"}};
    if (response.isEmpty())
        return {{"phase", answered ? "unavailable" : "loading"}};
    if (response.contains("code"))
        return {{"phase", "error"}};
    if (response.value("id").toString() != id)
        return {{"phase", "unavailable"}};
    // Expose only safe known fields. Credentials/source/unknown payloads stay outside the hub.
    QVariantMap result{{"phase", "ready"}, {"id", id}};
    for (const auto& field : {"globalEnabled", "effectiveEnabled", "available"})
        if (response.value(field).metaType().id() == QMetaType::Bool)
            result[field] = response.value(field);
    result["version"] = response.value("version").toString();
    result["workspacePreference"] = response.value("workspacePreference").toString();
    result["requirements"] = response.value("requirements").toStringList();
    return result;
}
} // namespace sentinel::desktop
