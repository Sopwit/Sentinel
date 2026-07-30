#include "sentinel/core/plugin/PluginPermissions.h"

namespace sentinel::core::plugin {

PluginPermissions::PluginPermissions(const QStringList& permissions) {
    for (const auto& perm : permissions) {
        if (!perm.trimmed().isEmpty()) {
            m_permissions.insert(perm.trimmed());
        }
    }
}

void PluginPermissions::grant(const QString& permission) {
    if (!permission.trimmed().isEmpty()) {
        m_permissions.insert(permission.trimmed());
    }
}

void PluginPermissions::revoke(const QString& permission) {
    m_permissions.remove(permission.trimmed());
}

bool PluginPermissions::has(const QString& permission) const {
    return m_permissions.contains(permission.trimmed());
}

QStringList PluginPermissions::toList() const {
    QStringList result;
    for (const auto& perm : m_permissions) {
        result.append(perm);
    }
    result.sort();
    return result;
}

QJsonArray PluginPermissions::toJsonArray() const {
    QJsonArray array;
    for (const auto& perm : toList()) {
        array.append(perm);
    }
    return array;
}

PluginPermissions PluginPermissions::fromJsonArray(const QJsonArray& array) {
    QStringList list;
    for (const auto& val : array) {
        if (val.isString()) {
            list.append(val.toString());
        }
    }
    return PluginPermissions(list);
}

} // namespace sentinel::core::plugin
