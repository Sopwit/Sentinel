#include "sentinel/core/StaticProviderCatalog.h"

#include <QtTest>

using sentinel::core::CatalogAvailability;
using sentinel::core::catalogAvailabilityName;
using sentinel::core::CatalogPrivacyLevel;
using sentinel::core::catalogPrivacyLevelName;
using sentinel::core::ProviderKind;
using sentinel::core::StaticProviderCatalog;

class StaticProviderCatalogTest final : public QObject {
    Q_OBJECT

private slots:
    void namesCatalogMetadata();
    void exposesDeterministicPlaceholderEntries();
    void classifiesLocalAndCloudMetadata();
    void keepsCloudPlaceholdersNotConfigured();
    void exposesOnlyAvailableProviderAndModelDescriptors();
};

void StaticProviderCatalogTest::namesCatalogMetadata() {
    QCOMPARE(catalogAvailabilityName(CatalogAvailability::Available), QStringLiteral("Available"));
    QCOMPARE(catalogAvailabilityName(CatalogAvailability::NotConfigured),
             QStringLiteral("Not Configured"));
    QCOMPARE(catalogPrivacyLevelName(CatalogPrivacyLevel::CloudMetadataOnly),
             QStringLiteral("Cloud Metadata Only"));
}

void StaticProviderCatalogTest::exposesDeterministicPlaceholderEntries() {
    const StaticProviderCatalog catalog;
    const auto entries = catalog.entries();

    QCOMPARE(entries.size(), 4);
    QCOMPARE(entries.at(0).descriptor.id, QStringLiteral("anthropic-cloud"));
    QCOMPARE(entries.at(1).descriptor.id, QStringLiteral("local-placeholder"));
    QCOMPARE(entries.at(2).descriptor.id, QStringLiteral("ollama-local"));
    QCOMPARE(entries.at(3).descriptor.id, QStringLiteral("openai-cloud"));
    QCOMPARE(entries.at(1).descriptor.name, QStringLiteral("Local Metadata Provider"));
    QCOMPARE(entries.at(1).models.first().descriptor.name,
             QStringLiteral("Sentinel Local Placeholder"));
}

void StaticProviderCatalogTest::classifiesLocalAndCloudMetadata() {
    const StaticProviderCatalog catalog;
    const auto entries = catalog.entries();

    QCOMPARE(entries.at(1).descriptor.kind, ProviderKind::Local);
    QCOMPARE(entries.at(1).privacyLevel, CatalogPrivacyLevel::LocalOnly);
    QVERIFY(entries.at(1).descriptor.capabilityProfile.local);
    QVERIFY(!entries.at(1).descriptor.capabilityProfile.cloud);

    QCOMPARE(entries.at(0).descriptor.kind, ProviderKind::Cloud);
    QCOMPARE(entries.at(0).privacyLevel, CatalogPrivacyLevel::CloudMetadataOnly);
    QVERIFY(!entries.at(0).descriptor.capabilityProfile.local);
    QVERIFY(entries.at(0).descriptor.capabilityProfile.cloud);
}

void StaticProviderCatalogTest::keepsCloudPlaceholdersNotConfigured() {
    const StaticProviderCatalog catalog;

    for (const auto& entry : catalog.entries()) {
        if (entry.descriptor.kind != ProviderKind::Cloud) {
            continue;
        }

        QCOMPARE(entry.availability, CatalogAvailability::NotConfigured);
        QCOMPARE(entry.privacyLevel, CatalogPrivacyLevel::CloudMetadataOnly);
        QVERIFY(!entry.models.isEmpty());
        QCOMPARE(entry.models.first().availability, CatalogAvailability::NotConfigured);
        QVERIFY(!entry.models.first().descriptor.installed);
        QVERIFY(!entry.models.first().descriptor.localOnly);
    }
}

void StaticProviderCatalogTest::exposesOnlyAvailableProviderAndModelDescriptors() {
    const StaticProviderCatalog catalog;

    const auto providers = catalog.availableProviders();
    const auto models = catalog.availableModels();

    QCOMPARE(providers.size(), 1);
    QCOMPARE(models.size(), 1);
    QCOMPARE(providers.first().id, QStringLiteral("local-placeholder"));
    QCOMPARE(models.first().id, QStringLiteral("sentinel-local-placeholder"));
    QCOMPARE(catalog.providerSummaries().size(), 4);
}

QTEST_MAIN(StaticProviderCatalogTest)

#include "test_static_provider_catalog.moc"
