// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>
#include <QList>

namespace sentinel::core {

struct McpResource {
    QString uri;
    QString name;
    QString description;
    QString mimeType;
    QString serverName;
};

class IMcpResourceService {
public:
    virtual ~IMcpResourceService() = default;

    virtual QList<McpResource> listResources() const = 0;
    virtual QString readResource(const QString& uri) const = 0;
    virtual QList<McpResource> resourcesByServer(const QString& serverName) const = 0;
    virtual bool subscribeToResource(const QString& uri) = 0;
    virtual void unsubscribeFromResource(const QString& uri) = 0;
};

} // namespace sentinel::core
