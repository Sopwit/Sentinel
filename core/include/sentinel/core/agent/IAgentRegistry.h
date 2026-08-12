// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/AgentMetadata.h"

namespace sentinel::core {

class IAgentRegistry {
public:
    virtual ~IAgentRegistry() = default;

    virtual QList<AgentDescriptor> agents() const = 0;
    virtual AgentDescriptor agentById(const QString& id) const = 0;
};

} // namespace sentinel::core
