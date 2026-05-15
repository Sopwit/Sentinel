#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/InMemoryStore.h"
#include "sentinel/core/LocalEchoProvider.h"

#include <QSignalSpy>
#include <QtTest>

#include <memory>

using sentinel::core::ApplicationController;
using sentinel::core::ChatProviderReply;
using sentinel::core::ChatProviderStatus;
using sentinel::core::IChatProvider;
using sentinel::core::InMemoryStore;
using sentinel::core::LocalEchoProvider;

class UnavailableProvider final : public IChatProvider {
public:
    QString name() const override {
        return QStringLiteral("UnavailableProvider");
    }

    ChatProviderStatus status() const override {
        return ChatProviderStatus::Unavailable;
    }

    ChatProviderReply sendMessage(const QString& message) override {
        Q_UNUSED(message);
        return {false, {}, QStringLiteral("not available")};
    }
};

class ApplicationControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesProviderNameAndInitialSystemMessage();
    void exposesProviderStatus();
    void sendsMessageThroughProvider();
    void ignoresBlankChatMessages();
    void handlesUnavailableProvider();
    void storesRuntimeMemoryEntries();
    void rejectsBlankMemoryKeys();
    void overwritesMemoryEntriesThroughStoreBackend();
};

static std::unique_ptr<ApplicationController> makeController() {
    return std::make_unique<ApplicationController>(std::make_unique<LocalEchoProvider>(),
                                                   std::make_unique<InMemoryStore>());
}

void ApplicationControllerTest::exposesProviderNameAndInitialSystemMessage() {
    const auto controller = makeController();

    QCOMPARE(controller->providerName(), QStringLiteral("LocalEchoProvider"));
    QCOMPARE(controller->chatMessages().size(), 1);
    QCOMPARE(controller->chatMessages().first(), QStringLiteral("Sentinel: Sentinel Core online."));
    QVERIFY(controller->memoryEntries().isEmpty());
}

void ApplicationControllerTest::exposesProviderStatus() {
    const auto controller = makeController();

    QCOMPARE(controller->providerStatus(), QStringLiteral("Ready"));
}

void ApplicationControllerTest::sendsMessageThroughProvider() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    const auto sent = controller->sendMessage(QStringLiteral("status"));

    const auto messages = controller->chatMessages();
    QVERIFY(sent);
    QCOMPARE(messages.size(), 3);
    QCOMPARE(messages.at(1), QStringLiteral("You: status"));
    QCOMPARE(messages.at(2),
             QStringLiteral("Sentinel: Sentinel Core online. Local chat pipeline is active."));
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::ignoresBlankChatMessages() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    const auto sent = controller->sendMessage(QStringLiteral("   "));

    QVERIFY(!sent);
    QCOMPARE(controller->chatMessages().size(), 1);
    QCOMPARE(spy.count(), 0);
}

void ApplicationControllerTest::handlesUnavailableProvider() {
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<UnavailableProvider>(), std::make_unique<InMemoryStore>());
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    const auto sent = controller->sendMessage(QStringLiteral("status"));

    QVERIFY(!sent);
    QCOMPARE(controller->providerName(), QStringLiteral("UnavailableProvider"));
    QCOMPARE(controller->providerStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(controller->chatMessages().size(), 3);
    QCOMPARE(controller->chatMessages().last(),
             QStringLiteral("Sentinel: Provider unavailable. Status: Unavailable"));
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::storesRuntimeMemoryEntries() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::memoryEntriesChanged);

    controller->remember(QStringLiteral(" callsign "), QStringLiteral(" Sentinel "));

    QCOMPARE(controller->memoryEntries(), QStringList{QStringLiteral("callsign: Sentinel")});
    QCOMPARE(spy.count(), 1);

    controller->remember(QString(), QStringLiteral("ignored"));
    QCOMPARE(controller->memoryEntries(), QStringList{QStringLiteral("callsign: Sentinel")});
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::rejectsBlankMemoryKeys() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::memoryEntriesChanged);

    controller->remember(QStringLiteral("   "), QStringLiteral("ignored"));

    QVERIFY(controller->memoryEntries().isEmpty());
    QCOMPARE(spy.count(), 0);
}

void ApplicationControllerTest::overwritesMemoryEntriesThroughStoreBackend() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::memoryEntriesChanged);

    controller->remember(QStringLiteral("mode"), QStringLiteral("Companion"));
    controller->remember(QStringLiteral("provider"), QStringLiteral("LocalEchoProvider"));
    controller->remember(QStringLiteral("mode"), QStringLiteral("Tactical"));

    const QStringList expected{
        QStringLiteral("mode: Tactical"),
        QStringLiteral("provider: LocalEchoProvider"),
    };

    QCOMPARE(controller->memoryEntries(), expected);
    QCOMPARE(spy.count(), 3);
}

QTEST_MAIN(ApplicationControllerTest)

#include "test_application_controller.moc"
