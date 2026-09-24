// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/mcp/McpService.h"
#include "sentinel/core/runtime/IToolRegistry.h"
#include <QObject>
#include <QSet>
#include <memory>

namespace sentinel::core {

class McpToolProvider final : public QObject {
    Q_OBJECT
public:
    McpToolProvider(std::shared_ptr<IMcpService> service, IToolRegistry& registry,
                    QObject* parent = nullptr);
    ~McpToolProvider() override;
    bool refresh(const QString& serverName);
    void disconnectServer(const QString& serverName);

private:
    std::shared_ptr<IMcpService> service_;
    IToolRegistry& registry_;
    QSet<QString> registeredServers_;
};

} // namespace sentinel::core
