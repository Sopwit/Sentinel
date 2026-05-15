#pragma once

#include "sentinel/core/IToolRegistry.h"

#include <QMap>

namespace sentinel::core {

class InMemoryToolRegistry final : public IToolRegistry {
public:
    bool registerTool(ToolDescriptor descriptor) override;
    QList<ToolDescriptor> listTools() const override;
    std::optional<ToolDescriptor> findToolById(const QString& id) const override;

private:
    QMap<QString, ToolDescriptor> toolsById_;
};

} // namespace sentinel::core
