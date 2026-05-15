#include "sentinel/core/JsonSettingsStore.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using sentinel::core::JsonSettingsStore;

class JsonSettingsStoreTest final : public QObject {
    Q_OBJECT

private slots:
    void returnsDefaultsForMissingFile();
    void persistsValuesAcrossInstances();
    void createsParentDirectories();
};

void JsonSettingsStoreTest::returnsDefaultsForMissingFile() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    JsonSettingsStore store(dir.filePath(QStringLiteral("missing/settings.json")));

    QCOMPARE(store.value(QStringLiteral("theme"), QStringLiteral("fallback")),
             QStringLiteral("fallback"));
}

void JsonSettingsStoreTest::persistsValuesAcrossInstances() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto filePath = dir.filePath(QStringLiteral("settings.json"));

    {
        JsonSettingsStore store(filePath);
        store.setValue(QStringLiteral("themeName"), QStringLiteral("Sentinel Light"));
        store.setValue(QStringLiteral("configurationProfile"), QStringLiteral("Phase 2.1"));
        store.setValue(QStringLiteral("routingMode"), QStringLiteral("Balanced"));
    }

    JsonSettingsStore reloaded(filePath);

    QCOMPARE(reloaded.value(QStringLiteral("themeName")), QStringLiteral("Sentinel Light"));
    QCOMPARE(reloaded.value(QStringLiteral("configurationProfile")), QStringLiteral("Phase 2.1"));
    QCOMPARE(reloaded.value(QStringLiteral("routingMode")), QStringLiteral("Balanced"));
}

void JsonSettingsStoreTest::createsParentDirectories() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto filePath = dir.filePath(QStringLiteral("nested/config/settings.json"));

    JsonSettingsStore store(filePath);
    store.setValue(QStringLiteral("themeName"), QStringLiteral("Sentinel Dark"));

    QVERIFY(QFile::exists(filePath));
}

QTEST_MAIN(JsonSettingsStoreTest)

#include "test_json_settings_store.moc"
