// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/mcp/IMcpResourceService.h"
#include <QObject>
#include <QList>

namespace sentinel::core {

class McpResourceService : public QObject, public IMcpResourceService {
    Q_OBJECT
public:
    explicit McpResourceService(QObject* parent = nullptr);
    ~McpResourceService() override;

    QList<McpResource> listResources() const override;
    QString readResource(const QString& uri) const override;
    QList<McpResource> resourcesByServer(const QString& serverName) const override;
    bool subscribeToResource(const QString& uri) override;
    void unsubscribeFromResource(const QString& uri) override;

private:
    QList<McpResource> m_resources;
    QSet<QString> m_subscriptions;
};

} // namespace sentinel::core
