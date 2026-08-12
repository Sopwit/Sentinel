// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/IAgentRegistry.h"

namespace sentinel::core {

class StaticAgentRegistry final : public IAgentRegistry {
public:
    StaticAgentRegistry();
    explicit StaticAgentRegistry(QList<AgentDescriptor> agents);

    QList<AgentDescriptor> agents() const override;
    AgentDescriptor agentById(const QString& id) const override;

private:
    QList<AgentDescriptor> agents_;
};

} // namespace sentinel::core
