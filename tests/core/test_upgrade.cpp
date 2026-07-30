// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/memory/JsonSettingsStore.h"
#include "sentinel/core/platform/DpapiEncryptedSettingsStore.h"
#include "sentinel/core/chat/SQLiteChatHistoryStore.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

using sentinel::core::JsonSettingsStore;
using sentinel::core::DpapiEncryptedSettingsStore;

class UpgradeTest final : public QObject {
    Q_OBJECT

private slots:
    void settingsBackwardCompatible();
    void settingsForwardCompatible();
    void chatHistorySchemaMigration();
};

void UpgradeTest::settingsBackwardCompatible() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto filePath = dir.filePath(QStringLiteral("settings.json"));

    // Write an old-format settings file (plain JSON, no $dpapi$ prefix)
    QJsonObject oldSettings;
    oldSettings.insert(QStringLiteral("themeName"), QStringLiteral("Sentinel Dark"));
    oldSettings.insert(QStringLiteral("routingMode"), QStringLiteral("Balanced"));
    oldSettings.insert(QStringLiteral("openAiApiKey"), QStringLiteral("sk-old-key-format"));
    oldSettings.insert(QStringLiteral("ollamaEndpoint"), QStringLiteral("http://localhost:11434"));
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QJsonDocument(oldSettings).toJson());
    }

    // Load with current store chain (DpapiEncryptedSettingsStore + JsonSettingsStore)
    DpapiEncryptedSettingsStore store(
        std::make_unique<JsonSettingsStore>(filePath));

    // Non-secret keys should read normally
    QCOMPARE(store.value(QStringLiteral("themeName")), QStringLiteral("Sentinel Dark"));
    QCOMPARE(store.value(QStringLiteral("routingMode")), QStringLiteral("Balanced"));
    QCOMPARE(store.value(QStringLiteral("ollamaEndpoint")),
             QStringLiteral("http://localhost:11434"));

    // Secret keys stored as plain text (no $dpapi$ prefix) should still return
    // their raw value — the decrypt path is only taken when prefix is present.
    QCOMPARE(store.value(QStringLiteral("openAiApiKey"), QStringLiteral("fallback")),
             QStringLiteral("sk-old-key-format"));
}

void UpgradeTest::settingsForwardCompatible() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto filePath = dir.filePath(QStringLiteral("settings.json"));

    // Write settings using the current encrypted store
    {
        DpapiEncryptedSettingsStore store(
            std::make_unique<JsonSettingsStore>(filePath));
        store.setValue(QStringLiteral("themeName"), QStringLiteral("Sentinel Light"));
        store.setValue(QStringLiteral("openAiApiKey"), QStringLiteral("sk-new-key"));
        store.setValue(QStringLiteral("ollamaEndpoint"), QStringLiteral("http://localhost:11434"));
    }

    // Verify the JSON file is valid and non-secret keys are readable with plain JsonSettingsStore
    JsonSettingsStore plainStore(filePath);
    QCOMPARE(plainStore.value(QStringLiteral("themeName")), QStringLiteral("Sentinel Light"));
    QCOMPARE(plainStore.value(QStringLiteral("ollamaEndpoint")),
             QStringLiteral("http://localhost:11434"));

    // The API key should be $dpapi$ prefixed and base64 encoded in the raw JSON
    const QString rawApiKey = plainStore.value(QStringLiteral("openAiApiKey"));
    QVERIFY(rawApiKey.startsWith(QStringLiteral("$dpapi$")));

    // Verify encrypted store can decrypt it
    DpapiEncryptedSettingsStore encryptedStore(
        std::make_unique<JsonSettingsStore>(filePath));
    QCOMPARE(encryptedStore.value(QStringLiteral("openAiApiKey")), QStringLiteral("sk-new-key"));
}

void UpgradeTest::chatHistorySchemaMigration() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto dbPath = dir.filePath(QStringLiteral("chat_history_old.sqlite3"));

    {
        // Create a SQLite database with an old schema (missing the newer columns)
        // Use a unique connection name to avoid conflicts with the store below
        const QString connName = QStringLiteral("migration_test_") + QUuid::createUuid().toString(QUuid::Id128);
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
            db.setDatabaseName(dbPath);
            QVERIFY2(db.open(), qPrintable(db.lastError().text()));

            QSqlQuery query(db);
            // Old schema: only the essential columns
            QVERIFY(query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS chat_messages("
                                               "id INTEGER PRIMARY KEY NOT NULL,"
                                               "role TEXT NOT NULL,"
                                               "content TEXT NOT NULL,"
                                               "timestamp TEXT NOT NULL,"
                                               "status TEXT NOT NULL)")));

            // Insert a sample message
            query.prepare(QStringLiteral("INSERT INTO chat_messages(role, content, timestamp, status) "
                                          "VALUES(?, ?, ?, ?)"));
            query.addBindValue(QStringLiteral("user"));
            query.addBindValue(QStringLiteral("Hello from old version"));
            query.addBindValue(QStringLiteral("2025-01-01T00:00:00"));
            query.addBindValue(QStringLiteral("sent"));
            QVERIFY(query.exec());

            db.close();
        }
        QSqlDatabase::removeDatabase(connName);
    }

    // Open with current store — constructor opens and migrates schema
    // (need a unique connection name in case other tests ran first)
    {
        sentinel::core::SQLiteChatHistoryStore store(dbPath);
        QVERIFY(store.isAvailable());
        QVERIFY(store.schemaVersion() > 0);
        QVERIFY(store.lastError().isEmpty());
    }
}

QTEST_MAIN(UpgradeTest)

#include "test_upgrade.moc"
