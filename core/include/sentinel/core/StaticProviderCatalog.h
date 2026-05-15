#pragma once

#include "sentinel/core/IProviderCatalog.h"

namespace sentinel::core {

class StaticProviderCatalog final : public IProviderCatalog {
public:
    StaticProviderCatalog();
    explicit StaticProviderCatalog(QList<ProviderCatalogEntry> entries);

    QList<ProviderCatalogEntry> entries() const override;
    QList<ProviderDescriptor> availableProviders() const;
    QList<ModelDescriptor> availableModels() const;
    QStringList providerSummaries() const;

private:
    QList<ProviderCatalogEntry> entries_;
};

} // namespace sentinel::core
