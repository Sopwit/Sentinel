#pragma once

#include "sentinel/core/IMemoryCatalog.h"

namespace sentinel::core {

class StaticMemoryCatalog final : public IMemoryCatalog {
public:
    StaticMemoryCatalog();
    explicit StaticMemoryCatalog(QList<MemoryShardDescriptor> shards);

    QList<MemoryShardDescriptor> shards() const override;
    QStringList shardSummaries() const;

private:
    QList<MemoryShardDescriptor> shards_;
};

} // namespace sentinel::core
