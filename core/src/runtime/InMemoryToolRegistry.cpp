// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/ToolArgumentValidator.h"

#include <QReadLocker>
#include <QSet>
#include <QWriteLocker>

namespace sentinel::core {

bool InMemoryToolRegistry::registerTool(ToolDescriptor descriptor) {
    return registerTool(Registration{std::move(descriptor), {}});
}

bool InMemoryToolRegistry::registerTool(Registration registration) {
    const auto id = registration.descriptor.id.trimmed();
    if (!ToolArgumentValidator::validateSchema(registration.descriptor.inputSchema).isEmpty())
        return false;
    if (registration.descriptor.source == ToolSource::Plugin &&
        registration.descriptor.inputSchema.isEmpty())
        return false;
    QWriteLocker lock(&mutex_);
    if (id.isEmpty() || toolsById_.contains(id)) {
        return false;
    }
    // Dynamic tools must never become planner-visible without an execution target.
    if ((registration.descriptor.source == ToolSource::MCP ||
         registration.descriptor.source == ToolSource::Plugin) &&
        (!registration.handler || registration.descriptor.providerId.trimmed().isEmpty()))
        return false;
    if (registration.descriptor.source == ToolSource::MCP && !id.startsWith(QStringLiteral("mcp.")))
        return false;
    if (registration.descriptor.source == ToolSource::Plugin &&
        !id.startsWith(QStringLiteral("plugin.")))
        return false;
    registration.descriptor.id = id;
    toolsById_.insert(id, std::move(registration));
    return true;
}

bool InMemoryToolRegistry::unregisterTool(const QString& id) {
    QWriteLocker lock(&mutex_);
    return toolsById_.remove(id.trimmed()) != 0;
}

bool InMemoryToolRegistry::setEnabled(const QString& id, bool enabled) {
    QWriteLocker lock(&mutex_);
    auto it = toolsById_.find(id.trimmed());
    if (it == toolsById_.end())
        return false;
    it->descriptor.enabled = enabled;
    return true;
}

int InMemoryToolRegistry::unregisterProvider(ToolSource source, const QString& providerId) {
    if (providerId.isEmpty())
        return 0;
    QWriteLocker lock(&mutex_);
    int removed = 0;
    for (auto it = toolsById_.begin(); it != toolsById_.end();) {
        if (it->descriptor.source == source && it->descriptor.providerId == providerId) {
            it = toolsById_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    return removed;
}

bool InMemoryToolRegistry::replaceProvider(ToolSource source, const QString& providerId,
                                           QList<Registration> registrations) {
    if (providerId.trimmed().isEmpty())
        return false;
    QWriteLocker lock(&mutex_);
    QSet<QString> ids;
    for (auto& registration : registrations) {
        auto& descriptor = registration.descriptor;
        descriptor.id = descriptor.id.trimmed();
        if (!ToolArgumentValidator::validateSchema(descriptor.inputSchema).isEmpty() ||
            (source == ToolSource::Plugin && descriptor.inputSchema.isEmpty()))
            return false;
        if (descriptor.source != source || descriptor.providerId != providerId ||
            descriptor.id.isEmpty() || !registration.handler || ids.contains(descriptor.id))
            return false;
        if ((source == ToolSource::MCP && !descriptor.id.startsWith(QStringLiteral("mcp."))) ||
            (source == ToolSource::Plugin && !descriptor.id.startsWith(QStringLiteral("plugin."))))
            return false;
        const auto existing = toolsById_.constFind(descriptor.id);
        if (existing != toolsById_.cend() && (existing->descriptor.source != source ||
                                              existing->descriptor.providerId != providerId))
            return false;
        ids.insert(descriptor.id);
    }
    for (auto it = toolsById_.begin(); it != toolsById_.end();) {
        if (it->descriptor.source == source && it->descriptor.providerId == providerId)
            it = toolsById_.erase(it);
        else
            ++it;
    }
    for (auto& registration : registrations)
        toolsById_.insert(registration.descriptor.id, std::move(registration));
    return true;
}

std::optional<IToolRegistry::Registration>
InMemoryToolRegistry::findRegistration(const QString& id) const {
    QReadLocker lock(&mutex_);
    const auto it = toolsById_.constFind(id.trimmed());
    if (it == toolsById_.cend())
        return std::nullopt;
    return it.value();
}

QList<ToolDescriptor> InMemoryToolRegistry::listTools() const {
    QReadLocker lock(&mutex_);
    QList<ToolDescriptor> tools;
    for (const auto& registration : toolsById_)
        tools.append(registration.descriptor);
    return tools;
}

QList<ToolDescriptor> InMemoryToolRegistry::enabledTools() const {
    QReadLocker lock(&mutex_);
    QList<ToolDescriptor> tools;
    for (const auto& registration : toolsById_)
        if (registration.descriptor.enabled && registration.descriptor.exposedToModel &&
            registration.handler)
            tools.append(registration.descriptor);
    return tools;
}

std::optional<ToolDescriptor> InMemoryToolRegistry::findToolById(const QString& id) const {
    const auto registration = findRegistration(id);
    return registration ? std::optional<ToolDescriptor>{registration->descriptor} : std::nullopt;
}

} // namespace sentinel::core
