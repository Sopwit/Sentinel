#include "sentinel/core/InMemoryStore.h"

#include <QtTest>

using sentinel::core::InMemoryStore;

class InMemoryStoreTest final : public QObject {
    Q_OBJECT

private slots:
    void returnsEmptyStringForMissingKeys();
    void storesAndRetrievesValues();
    void overwritesExistingKeys();
    void handlesEmptyKeysAsStorageData();
    void handlesRepeatedOverwrites();
    void keepsMultipleKeysIndependent();
    void exposesAllEntriesDeterministically();
};

void InMemoryStoreTest::returnsEmptyStringForMissingKeys() {
    InMemoryStore store;

    QVERIFY(store.get(QStringLiteral("missing")).isEmpty());
    QVERIFY(store.entries().isEmpty());
}

void InMemoryStoreTest::storesAndRetrievesValues() {
    InMemoryStore store;

    store.put(QStringLiteral("callsign"), QStringLiteral("Sentinel"));

    QCOMPARE(store.get(QStringLiteral("callsign")), QStringLiteral("Sentinel"));
    QCOMPARE(store.entries().size(), 1);
}

void InMemoryStoreTest::overwritesExistingKeys() {
    InMemoryStore store;

    store.put(QStringLiteral("mode"), QStringLiteral("Companion"));
    store.put(QStringLiteral("mode"), QStringLiteral("Tactical"));

    QCOMPARE(store.get(QStringLiteral("mode")), QStringLiteral("Tactical"));
    QCOMPARE(store.entries().size(), 1);
}

void InMemoryStoreTest::handlesEmptyKeysAsStorageData() {
    InMemoryStore store;

    store.put(QString(), QStringLiteral("empty-key-value"));

    QCOMPARE(store.get(QString()), QStringLiteral("empty-key-value"));
    QCOMPARE(store.entries().size(), 1);
    QCOMPARE(store.entries().first().first, QString());
}

void InMemoryStoreTest::handlesRepeatedOverwrites() {
    InMemoryStore store;

    store.put(QStringLiteral("status"), QStringLiteral("one"));
    store.put(QStringLiteral("status"), QStringLiteral("two"));
    store.put(QStringLiteral("status"), QStringLiteral("three"));

    QCOMPARE(store.get(QStringLiteral("status")), QStringLiteral("three"));
    QCOMPARE(store.entries().size(), 1);
}

void InMemoryStoreTest::keepsMultipleKeysIndependent() {
    InMemoryStore store;

    store.put(QStringLiteral("mode"), QStringLiteral("Companion"));
    store.put(QStringLiteral("provider"), QStringLiteral("FakeProvider"));
    store.put(QStringLiteral("mode"), QStringLiteral("Mission"));

    QCOMPARE(store.get(QStringLiteral("mode")), QStringLiteral("Mission"));
    QCOMPARE(store.get(QStringLiteral("provider")), QStringLiteral("FakeProvider"));
    QCOMPARE(store.entries().size(), 2);
}

void InMemoryStoreTest::exposesAllEntriesDeterministically() {
    InMemoryStore store;

    store.put(QStringLiteral("zulu"), QStringLiteral("last"));
    store.put(QStringLiteral("alpha"), QStringLiteral("first"));

    const auto entries = store.entries();

    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.at(0).first, QStringLiteral("alpha"));
    QCOMPARE(entries.at(0).second, QStringLiteral("first"));
    QCOMPARE(entries.at(1).first, QStringLiteral("zulu"));
    QCOMPARE(entries.at(1).second, QStringLiteral("last"));
}

QTEST_MAIN(InMemoryStoreTest)

#include "test_in_memory_store.moc"
