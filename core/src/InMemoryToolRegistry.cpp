#include "sentinel/core/InMemoryToolRegistry.h"

namespace sentinel::core {

bool InMemoryToolRegistry::registerTool(ToolDescriptor descriptor) {
    const auto id = descriptor.id.trimmed();
    if (id.isEmpty() || toolsById_.contains(id)) {
        return false;
    }

    descriptor.id = id;
    toolsById_.insert(id, descriptor);
    return true;
}

QList<ToolDescriptor> InMemoryToolRegistry::listTools() const {
    return toolsById_.values();
}

std::optional<ToolDescriptor> InMemoryToolRegistry::findToolById(const QString& id) const {
    const auto normalized = id.trimmed();
    if (normalized.isEmpty()) {
        return std::nullopt;
    }

    const auto found = toolsById_.find(normalized);
    if (found == toolsById_.end()) {
        return std::nullopt;
    }

    return found.value();
}

} // namespace sentinel::core
