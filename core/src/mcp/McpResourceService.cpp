// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/mcp/McpResourceService.h"

namespace sentinel::core {

McpResourceService::McpResourceService(QObject* parent) : QObject(parent) {}
McpResourceService::~McpResourceService() = default;

QList<McpResource> McpResourceService::listResources() const { return m_resources; }

QString McpResourceService::readResource(const QString& uri) const {
    for (const auto& r : m_resources) {
        if (r.uri == uri) return r.name;
    }
    return {};
}

QList<McpResource> McpResourceService::resourcesByServer(const QString& serverName) const {
    QList<McpResource> result;
    for (const auto& r : m_resources) {
        if (r.serverName == serverName) result.append(r);
    }
    return result;
}

bool McpResourceService::subscribeToResource(const QString& uri) {
    m_subscriptions.insert(uri);
    return true;
}

void McpResourceService::unsubscribeFromResource(const QString& uri) {
    m_subscriptions.remove(uri);
}

} // namespace sentinel::core
