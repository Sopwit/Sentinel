#include "sentinel/core/platform/IPathProvider.h"
#include "sentinel/core/platform/StandardPathProvider.h"

#include <QDir>
#include <QStandardPaths>
#include <QtTest>

#include <memory>

using sentinel::core::IPathProvider;
using sentinel::core::StandardPathProvider;

class StandardPathProviderTest final : public QObject {
    Q_OBJECT

private slots:
    void returnsExpectedFileNamesInStandardLocations();
    void returnsExpectedPathsInPortableMode();
};

void StandardPathProviderTest::returnsExpectedFileNamesInStandardLocations() {
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("SentinelTests"));
    QCoreApplication::setApplicationName(QStringLiteral("SentinelDesktop"));

    StandardPathProvider provider(false);
    QCOMPARE(provider.isPortable(), false);

    const auto configDir = QDir::fromNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    const auto dataDir = QDir::fromNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
        QStringLiteral("/Sentinel"));
    const auto settingsPath = QDir::fromNativeSeparators(provider.settingsFilePath());
    const auto memoryPath = QDir::fromNativeSeparators(provider.memoryDatabasePath());
    const auto chatPath = QDir::fromNativeSeparators(provider.chatHistoryDatabasePath());
    const auto conversationPath = QDir::fromNativeSeparators(provider.conversationDatabasePath());
    const auto exportPath = QDir::fromNativeSeparators(provider.conversationExportDirectoryPath());

    QVERIFY(settingsPath.startsWith(configDir));
    QVERIFY(memoryPath.startsWith(dataDir));
    QVERIFY(chatPath.startsWith(dataDir));
    QVERIFY(conversationPath.startsWith(dataDir));
    QVERIFY(exportPath.startsWith(dataDir));
    QVERIFY(settingsPath.endsWith(QStringLiteral("/settings.json")));
    QVERIFY(memoryPath.endsWith(QStringLiteral("/memory.sqlite3")));
    QVERIFY(chatPath.endsWith(QStringLiteral("/chat_history.sqlite3")));
    QVERIFY(conversationPath.endsWith(QStringLiteral("/conversations.sqlite3")));
    QVERIFY(exportPath.endsWith(QStringLiteral("/exports")));
}

void StandardPathProviderTest::returnsExpectedPathsInPortableMode() {
    StandardPathProvider provider(true);
    QCOMPARE(provider.isPortable(), true);

    const auto settingsPath = QDir::fromNativeSeparators(provider.settingsFilePath());
    const auto memoryPath = QDir::fromNativeSeparators(provider.memoryDatabasePath());
    const auto chatPath = QDir::fromNativeSeparators(provider.chatHistoryDatabasePath());
    const auto conversationPath = QDir::fromNativeSeparators(provider.conversationDatabasePath());
    const auto exportPath = QDir::fromNativeSeparators(provider.conversationExportDirectoryPath());
    const auto ragPath = QDir::fromNativeSeparators(provider.localRagDatabasePath());
    const auto logPath = QDir::fromNativeSeparators(provider.logDirectoryPath());
    const auto crashPath = QDir::fromNativeSeparators(provider.crashDumpDirectoryPath());

    QVERIFY(settingsPath.endsWith(QStringLiteral("/settings.json")));
    QVERIFY(memoryPath.endsWith(QStringLiteral("/memory.sqlite3")));
    QVERIFY(chatPath.endsWith(QStringLiteral("/chat_history.sqlite3")));
    QVERIFY(conversationPath.endsWith(QStringLiteral("/conversations.sqlite3")));
    QVERIFY(exportPath.endsWith(QStringLiteral("/exports")));
    QVERIFY(ragPath.endsWith(QStringLiteral("/local_rag.sqlite3")));
    QVERIFY(logPath.endsWith(QStringLiteral("/Logs")));
    QVERIFY(crashPath.endsWith(QStringLiteral("/Crashes")));
}

QTEST_MAIN(StandardPathProviderTest)

#include "test_standard_path_provider.moc"
