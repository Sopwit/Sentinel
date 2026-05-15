#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/FakeProvider.h"
#include "sentinel/core/InMemoryStore.h"

#include <QSignalSpy>
#include <QtTest>

#include <memory>

using sentinel::core::ApplicationController;
using sentinel::core::FakeProvider;
using sentinel::core::InMemoryStore;

class ApplicationControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesProviderNameAndInitialSystemMessage();
    void sendsMessageThroughProvider();
    void ignoresBlankChatMessages();
    void storesRuntimeMemoryEntries();
    void rejectsBlankMemoryKeys();
    void overwritesMemoryEntriesThroughStoreBackend();
};

static std::unique_ptr<ApplicationController> makeController() {
    return std::make_unique<ApplicationController>(std::make_unique<FakeProvider>(),
                                                   std::make_unique<InMemoryStore>());
}

void ApplicationControllerTest::exposesProviderNameAndInitialSystemMessage() {
    const auto controller = makeController();

    QCOMPARE(controller->providerName(), QStringLiteral("FakeProvider"));
    QCOMPARE(controller->chatMessages().size(), 1);
    QCOMPARE(controller->chatMessages().first(), QStringLiteral("Sentinel: Sentinel Core online."));
    QVERIFY(controller->memoryEntries().isEmpty());
}

void ApplicationControllerTest::sendsMessageThroughProvider() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    controller->sendMessage(QStringLiteral("status"));

    const auto messages = controller->chatMessages();
    QCOMPARE(messages.size(), 3);
    QCOMPARE(messages.at(1), QStringLiteral("You: status"));
    QCOMPARE(messages.at(2),
             QStringLiteral("Sentinel: Sentinel Core online. Provider bridge is active."));
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::ignoresBlankChatMessages() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    controller->sendMessage(QStringLiteral("   "));

    QCOMPARE(controller->chatMessages().size(), 1);
    QCOMPARE(spy.count(), 0);
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
    controller->remember(QStringLiteral("provider"), QStringLiteral("FakeProvider"));
    controller->remember(QStringLiteral("mode"), QStringLiteral("Tactical"));

    const QStringList expected{
        QStringLiteral("mode: Tactical"),
        QStringLiteral("provider: FakeProvider"),
    };

    QCOMPARE(controller->memoryEntries(), expected);
    QCOMPARE(spy.count(), 3);
}

QTEST_MAIN(ApplicationControllerTest)

#include "test_application_controller.moc"
