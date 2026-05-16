#pragma once

#include "sentinel/core/MemoryMetadata.h"

#include <QList>

namespace sentinel::core {

class IMemoryCatalog {
public:
    virtual ~IMemoryCatalog() = default;

    virtual QList<MemoryShardDescriptor> shards() const = 0;
};

} // namespace sentinel::core
