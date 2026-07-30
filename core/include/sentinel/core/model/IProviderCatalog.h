#pragma once

#include "sentinel/core/model/ProviderCatalog.h"

namespace sentinel::core {

class IProviderCatalog {
public:
    virtual ~IProviderCatalog() = default;

    virtual QList<ProviderCatalogEntry> entries() const = 0;
};

} // namespace sentinel::core
