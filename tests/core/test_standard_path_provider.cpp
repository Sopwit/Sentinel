#include "sentinel/core/IPathProvider.h"
#include "sentinel/core/StandardPathProvider.h"

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
};

void StandardPathProviderTest::returnsExpectedFileNamesInStandardLocations() {
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("SentinelTests"));
    QCoreApplication::setApplicationName(QStringLiteral("SentinelDesktop"));

    std::unique_ptr<IPathProvider> provider = std::make_unique<StandardPathProvider>();

    const auto configDir = QDir::fromNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    const auto dataDir = QDir::fromNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    const auto settingsPath = QDir::fromNativeSeparators(provider->settingsFilePath());
    const auto memoryPath = QDir::fromNativeSeparators(provider->memoryDatabasePath());
    const auto chatPath = QDir::fromNativeSeparators(provider->chatHistoryDatabasePath());
    const auto conversationPath = QDir::fromNativeSeparators(provider->conversationDatabasePath());
    const auto exportPath = QDir::fromNativeSeparators(provider->conversationExportDirectoryPath());

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

QTEST_MAIN(StandardPathProviderTest)

#include "test_standard_path_provider.moc"
