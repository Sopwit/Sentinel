// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/app/StorageMigration.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtTest>

using sentinel::core::StorageMigration;

namespace {

constexpr auto kLegacyApplicationName = "Sentinel Desktop";
constexpr auto kCurrentApplicationName = "Sentinel";

struct Roots {
    QString config;
    QString data;
    QString localData;

    QStringList distinct() const {
        QStringList roots;
        for (const QString& root : {config, data, localData}) {
            if (!root.isEmpty() && !roots.contains(root)) {
                roots.append(root);
            }
        }
        return roots;
    }
};

Roots rootsForApplicationName(const QString& applicationName) {
    const QString previousApplicationName = QCoreApplication::applicationName();
    QCoreApplication::setApplicationName(applicationName);
    const Roots roots{
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation),
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation),
    };
    QCoreApplication::setApplicationName(previousApplicationName);
    return roots;
}

bool writeTextFile(const QString& path, const QByteArray& contents) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return file.write(contents) == contents.size();
}

QByteArray readTextFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

} // namespace

class StorageMigrationTest final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void movesLegacyRootsOntoCurrentRoots();
    void keepsCurrentEntriesAndAdoptsMissingOnes();
    void repointsStoredPathsInMovedJsonFiles();
    void doesNothingWhenLegacyStorageIsAbsent();

private:
    QString m_originalOrganization;
    QString m_originalApplication;
};

void StorageMigrationTest::initTestCase() {
    QStandardPaths::setTestModeEnabled(true);
    m_originalOrganization = QCoreApplication::organizationName();
    m_originalApplication = QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName(QStringLiteral("SentinelStorageMigrationTests"));
}

void StorageMigrationTest::init() {
    for (const QString& root : rootsForApplicationName(kLegacyApplicationName).distinct() +
                                   rootsForApplicationName(kCurrentApplicationName).distinct()) {
        QDir(root).removeRecursively();
    }
}

void StorageMigrationTest::cleanupTestCase() {
    QCoreApplication::setOrganizationName(m_originalOrganization);
    QCoreApplication::setApplicationName(m_originalApplication);
    QStandardPaths::setTestModeEnabled(false);
}

void StorageMigrationTest::movesLegacyRootsOntoCurrentRoots() {
    const Roots legacyRoots = rootsForApplicationName(kLegacyApplicationName);
    for (const QString& root : legacyRoots.distinct()) {
        QVERIFY(writeTextFile(root + QStringLiteral("/settings.json"), "legacy"));
        QVERIFY(QDir().mkpath(root + QStringLiteral("/nested")));
        QVERIFY(writeTextFile(root + QStringLiteral("/nested/model.bin"), "weights"));
    }

    const int migrated = StorageMigration::migrateLegacyApplicationStorage(kLegacyApplicationName,
                                                                           kCurrentApplicationName);
    QCOMPARE(migrated, int(legacyRoots.distinct().size()));

    const Roots currentRoots = rootsForApplicationName(kCurrentApplicationName);
    QCOMPARE(int(currentRoots.distinct().size()), int(legacyRoots.distinct().size()));
    for (const QString& root : currentRoots.distinct()) {
        QCOMPARE(readTextFile(root + QStringLiteral("/settings.json")), QByteArray("legacy"));
        QCOMPARE(readTextFile(root + QStringLiteral("/nested/model.bin")), QByteArray("weights"));
    }

    for (const QString& legacyRoot : legacyRoots.distinct()) {
        QVERIFY(!QFileInfo::exists(legacyRoot));
    }
}

void StorageMigrationTest::keepsCurrentEntriesAndAdoptsMissingOnes() {
    const Roots legacyRoots = rootsForApplicationName(kLegacyApplicationName);
    const Roots currentRoots = rootsForApplicationName(kCurrentApplicationName);
    QVERIFY(writeTextFile(currentRoots.config + QStringLiteral("/settings.json"), "current"));
    QVERIFY(writeTextFile(legacyRoots.config + QStringLiteral("/settings.json"), "legacy"));
    QVERIFY(writeTextFile(legacyRoots.config + QStringLiteral("/legacy-only.json"), "legacy"));
    QVERIFY(writeTextFile(legacyRoots.data + QStringLiteral("/model.bin"), "weights"));

    const int migrated = StorageMigration::migrateLegacyApplicationStorage(kLegacyApplicationName,
                                                                           kCurrentApplicationName);
    QVERIFY(migrated >= 1);

    QCOMPARE(readTextFile(currentRoots.config + QStringLiteral("/settings.json")),
             QByteArray("current"));
    QCOMPARE(readTextFile(currentRoots.config + QStringLiteral("/legacy-only.json")),
             QByteArray("legacy"));
    QCOMPARE(readTextFile(currentRoots.data + QStringLiteral("/model.bin")), QByteArray("weights"));
    // A conflicting legacy copy is preserved untouched instead of being overwritten.
    QVERIFY(QFileInfo::exists(legacyRoots.config));
    QCOMPARE(readTextFile(legacyRoots.config + QStringLiteral("/settings.json")),
             QByteArray("legacy"));
    QVERIFY(!QFileInfo::exists(legacyRoots.config + QStringLiteral("/legacy-only.json")));
    QVERIFY(!QFileInfo::exists(legacyRoots.data));
}

void StorageMigrationTest::repointsStoredPathsInMovedJsonFiles() {
    const Roots legacyRoots = rootsForApplicationName(kLegacyApplicationName);
    const Roots currentRoots = rootsForApplicationName(kCurrentApplicationName);
    const QString legacyConfig = legacyRoots.config;
    const QString currentConfig = currentRoots.config;
    QVERIFY(legacyConfig != currentConfig);

    const QByteArray registry =
        QStringLiteral(R"({"activeRoot": "%1/models/huggingface",)"
                       R"( "knownRoots": ["%2"], "runtimeModelId": "llama"})")
            .arg(legacyConfig, QDir::toNativeSeparators(legacyConfig))
            .toUtf8();
    QVERIFY(writeTextFile(legacyConfig + QStringLiteral("/model-storage.json"), registry));
    QVERIFY(writeTextFile(legacyConfig + QStringLiteral("/plugins/local/config.json"),
                          QStringLiteral(R"({"root": "%1"})").arg(legacyConfig).toUtf8()));
    QVERIFY(writeTextFile(legacyConfig + QStringLiteral("/notes.txt"),
                          QStringLiteral("root=%1").arg(legacyConfig).toUtf8()));

    // Only the config root was populated, so exactly one root is relocated.
    QCOMPARE(StorageMigration::migrateLegacyApplicationStorage(kLegacyApplicationName,
                                                               kCurrentApplicationName),
             1);

    const QByteArray repointed =
        readTextFile(currentConfig + QStringLiteral("/model-storage.json"));
    QVERIFY(repointed.contains(currentConfig.toUtf8() + "/models/huggingface"));
    QVERIFY(repointed.contains(QDir::toNativeSeparators(currentConfig).toUtf8()));
    QVERIFY(repointed.contains(QByteArrayLiteral("\"runtimeModelId\": \"llama\"")));
    QVERIFY(!repointed.contains(legacyConfig.toUtf8()));

    const QByteArray nested =
        readTextFile(currentConfig + QStringLiteral("/plugins/local/config.json"));
    QVERIFY(nested.contains(currentConfig.toUtf8()));
    QVERIFY(!nested.contains(legacyConfig.toUtf8()));

    // Non-JSON payloads (for example SQLite databases) are moved untouched.
    QVERIFY(
        readTextFile(currentConfig + QStringLiteral("/notes.txt")).contains(legacyConfig.toUtf8()));
}

void StorageMigrationTest::doesNothingWhenLegacyStorageIsAbsent() {
    QCOMPARE(StorageMigration::migrateLegacyApplicationStorage(kLegacyApplicationName,
                                                               kCurrentApplicationName),
             0);
    QCOMPARE(StorageMigration::migrateLegacyApplicationStorage(kCurrentApplicationName,
                                                               kCurrentApplicationName),
             0);
}

QTEST_MAIN(StorageMigrationTest)

#include "test_storage_migration.moc"
