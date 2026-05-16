#include "sentinel/core/StaticMemoryCatalog.h"

#include <QSet>
#include <QtTest>

#include <algorithm>

using sentinel::core::MemoryPrivacyLevel;
using sentinel::core::memoryPrivacyLevelName;
using sentinel::core::MemoryRetentionPolicy;
using sentinel::core::memoryRetentionPolicyName;
using sentinel::core::MemoryShardStatus;
using sentinel::core::memoryShardStatusName;
using sentinel::core::MemoryType;
using sentinel::core::memoryTypeName;
using sentinel::core::StaticMemoryCatalog;

class StaticMemoryCatalogTest final : public QObject {
    Q_OBJECT

private slots:
    void namesMemoryMetadata();
    void exposesDeterministicOrderedShards();
    void keepsMemoryTypeIdsUnique();
    void preservesRetentionAndPrivacyMetadata();
    void exposesMetadataOnlySummaries();
};

void StaticMemoryCatalogTest::namesMemoryMetadata() {
    QCOMPARE(memoryTypeName(MemoryType::Episodic), QStringLiteral("Episodic"));
    QCOMPARE(memoryShardStatusName(MemoryShardStatus::Available), QStringLiteral("Available"));
    QCOMPARE(memoryRetentionPolicyName(MemoryRetentionPolicy::UserControlled),
             QStringLiteral("User Controlled"));
    QCOMPARE(memoryPrivacyLevelName(MemoryPrivacyLevel::Sensitive), QStringLiteral("Sensitive"));
}

void StaticMemoryCatalogTest::exposesDeterministicOrderedShards() {
    const StaticMemoryCatalog catalog;
    const auto shards = catalog.shards();

    QCOMPARE(shards.size(), 5);
    QCOMPARE(shards.at(0).id, QStringLiteral("ambient"));
    QCOMPARE(shards.at(1).id, QStringLiteral("episodic"));
    QCOMPARE(shards.at(2).id, QStringLiteral("procedural"));
    QCOMPARE(shards.at(3).id, QStringLiteral("reflective"));
    QCOMPARE(shards.at(4).id, QStringLiteral("semantic"));
}

void StaticMemoryCatalogTest::keepsMemoryTypeIdsUnique() {
    const StaticMemoryCatalog catalog;
    QSet<QString> ids;
    QSet<QString> types;

    for (const auto& shard : catalog.shards()) {
        QVERIFY2(!ids.contains(shard.id), qPrintable(shard.id));
        ids.insert(shard.id);
        QVERIFY2(!types.contains(memoryTypeName(shard.type)), qPrintable(shard.displayName));
        types.insert(memoryTypeName(shard.type));
    }

    QCOMPARE(ids.size(), 5);
    QCOMPARE(types.size(), 5);
}

void StaticMemoryCatalogTest::preservesRetentionAndPrivacyMetadata() {
    const StaticMemoryCatalog catalog;
    const auto shards = catalog.shards();

    const auto reflective = std::find_if(shards.cbegin(), shards.cend(),
                                         [](const sentinel::core::MemoryShardDescriptor& shard) {
                                             return shard.id == QStringLiteral("reflective");
                                         });
    QVERIFY(reflective != shards.cend());
    QCOMPARE(reflective->retentionPolicy, MemoryRetentionPolicy::UserControlled);
    QCOMPARE(reflective->privacyLevel, MemoryPrivacyLevel::Sensitive);
    QCOMPARE(reflective->status, MemoryShardStatus::Available);
    QVERIFY(!reflective->affinities.isEmpty());
}

void StaticMemoryCatalogTest::exposesMetadataOnlySummaries() {
    const StaticMemoryCatalog catalog;
    const auto summaries = catalog.shardSummaries();

    QCOMPARE(summaries.size(), 5);
    QVERIFY(summaries.contains(QStringLiteral("Semantic (Available, Local Only, Durable)")));
    QVERIFY(
        summaries.contains(QStringLiteral("Reflective (Available, Sensitive, User Controlled)")));
}

QTEST_MAIN(StaticMemoryCatalogTest)

#include "test_static_memory_catalog.moc"
