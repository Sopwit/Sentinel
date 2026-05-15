#pragma once

#include "sentinel/core/ModelRouting.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace sentinel::core {

enum class CatalogAvailability {
    Available,
    NotConfigured,
    Unavailable,
};

enum class CatalogPrivacyLevel {
    LocalOnly,
    CloudMetadataOnly,
};

inline QString catalogAvailabilityName(CatalogAvailability availability) {
    switch (availability) {
    case CatalogAvailability::Available:
        return QStringLiteral("Available");
    case CatalogAvailability::NotConfigured:
        return QStringLiteral("Not Configured");
    case CatalogAvailability::Unavailable:
        return QStringLiteral("Unavailable");
    }

    return QStringLiteral("Unavailable");
}

inline QString catalogPrivacyLevelName(CatalogPrivacyLevel privacyLevel) {
    switch (privacyLevel) {
    case CatalogPrivacyLevel::LocalOnly:
        return QStringLiteral("Local Only");
    case CatalogPrivacyLevel::CloudMetadataOnly:
        return QStringLiteral("Cloud Metadata Only");
    }

    return QStringLiteral("Local Only");
}

struct ModelCatalogEntry {
    ModelDescriptor descriptor;
    CatalogAvailability availability = CatalogAvailability::Unavailable;
    CatalogPrivacyLevel privacyLevel = CatalogPrivacyLevel::LocalOnly;
    int ramHintMb = 0;
    int diskHintMb = 0;
    QString summary;
};

struct ProviderCatalogEntry {
    ProviderDescriptor descriptor;
    CatalogAvailability availability = CatalogAvailability::Unavailable;
    CatalogPrivacyLevel privacyLevel = CatalogPrivacyLevel::LocalOnly;
    int ramHintMb = 0;
    int diskHintMb = 0;
    QString summary;
    QList<ModelCatalogEntry> models;
};

inline bool isCatalogEntryAvailable(CatalogAvailability availability) {
    return availability == CatalogAvailability::Available;
}

inline QString providerCatalogEntrySummary(const ProviderCatalogEntry& entry) {
    if (!entry.summary.isEmpty()) {
        return entry.summary;
    }

    return QStringLiteral("%1 (%2, %3)")
        .arg(entry.descriptor.name, providerKindName(entry.descriptor.kind),
             catalogAvailabilityName(entry.availability));
}

} // namespace sentinel::core
