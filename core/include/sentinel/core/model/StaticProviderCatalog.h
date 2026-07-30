// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/model/IProviderCatalog.h"

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
