#include "sentinel/core/LocalEchoProvider.h"

#include <QtTest>

using sentinel::core::ChatProviderStatus;
using sentinel::core::chatProviderStatusName;
using sentinel::core::LocalEchoProvider;

class LocalEchoProviderTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesNameAndReadyStatus();
    void returnsDeterministicResponse();
    void statusNamesAreStable();
};

void LocalEchoProviderTest::exposesNameAndReadyStatus() {
    LocalEchoProvider provider;

    QCOMPARE(provider.name(), QStringLiteral("LocalEchoProvider"));
    QCOMPARE(provider.status(), ChatProviderStatus::Ready);
}

void LocalEchoProviderTest::returnsDeterministicResponse() {
    LocalEchoProvider provider;

    const auto first = provider.sendMessage(QStringLiteral("status"));
    const auto second = provider.sendMessage(QStringLiteral("different prompt"));

    QVERIFY(first.success);
    QVERIFY(second.success);
    QCOMPARE(first.message, QStringLiteral("Sentinel Core online. Local chat pipeline is active."));
    QCOMPARE(second.message, first.message);
    QVERIFY(first.errorMessage.isEmpty());
}

void LocalEchoProviderTest::statusNamesAreStable() {
    QCOMPARE(chatProviderStatusName(ChatProviderStatus::Ready), QStringLiteral("Ready"));
    QCOMPARE(chatProviderStatusName(ChatProviderStatus::Unavailable),
             QStringLiteral("Unavailable"));
    QCOMPARE(chatProviderStatusName(ChatProviderStatus::Error), QStringLiteral("Error"));
}

QTEST_MAIN(LocalEchoProviderTest)

#include "test_local_echo_provider.moc"
