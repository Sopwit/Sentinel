#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/IChatHistoryStore.h"
#include "sentinel/core/InMemoryStore.h"
#include "sentinel/core/LocalEchoProvider.h"

#include <QSignalSpy>
#include <QtTest>

#include <memory>

using sentinel::core::ApplicationController;
using sentinel::core::ChatProviderReply;
using sentinel::core::ChatProviderStatus;
using sentinel::core::IChatHistoryStore;
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

class ErrorProvider final : public IChatProvider {
public:
    QString name() const override {
        return QStringLiteral("ErrorProvider");
    }

    ChatProviderStatus status() const override {
        return ChatProviderStatus::Ready;
    }

    ChatProviderReply sendMessage(const QString& message) override {
        Q_UNUSED(message);
        return {false, {}, QStringLiteral("deterministic failure")};
    }
};

class RecordingChatHistoryStore final : public IChatHistoryStore {
public:
    explicit RecordingChatHistoryStore(QList<sentinel::core::ChatMessage> messages = {},
                                       bool available = true)
        : messages_(std::move(messages)), available_(available) {}

    QList<sentinel::core::ChatMessage> loadMessages() const override {
        return available_ ? messages_ : QList<sentinel::core::ChatMessage>{};
    }

    void appendMessage(const sentinel::core::ChatMessage& message) override {
        if (available_) {
            messages_.append(message);
        }
    }

    void clear() override {
        if (available_) {
            wasCleared_ = true;
            messages_.clear();
        }
    }

    bool isAvailable() const override {
        return available_;
    }

    QString lastError() const override {
        return available_ ? QString() : QStringLiteral("unavailable");
    }

    QList<sentinel::core::ChatMessage> messages_;
    bool available_ = true;
    bool wasCleared_ = false;
};

class UnavailableMemoryStore final : public sentinel::core::IMemoryStore {
public:
    void put(QString key, QString value) override {
        Q_UNUSED(key);
        Q_UNUSED(value);
    }

    QString get(const QString& key) const override {
        Q_UNUSED(key);
        return {};
    }

    sentinel::core::MemoryEntries entries() const override {
        return {};
    }

    void clear() override {}

    bool isAvailable() const override {
        return false;
    }

    QString lastError() const override {
        return QStringLiteral("unavailable");
    }
};

class ApplicationControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesProviderNameAndInitialSystemMessage();
    void exposesProviderStatus();
    void exposesMemoryStatus();
    void sendsMessageThroughProvider();
    void ignoresBlankChatMessages();
    void handlesUnavailableProvider();
    void handlesProviderErrorReply();
    void clearsChatHistory();
    void loadsPersistedChatHistoryAtStartup();
    void appendsNewChatMessagesToHistoryStore();
    void clearsPersistentChatHistoryWhenAvailable();
    void keepsRuntimeChatWorkingWhenHistoryStoreUnavailable();
    void storesRuntimeMemoryEntries();
    void clearsRuntimeMemoryEntries();
    void failsSafeWhenMemoryStoreUnavailable();
    void rejectsBlankMemoryKeys();
    void overwritesMemoryEntriesThroughStoreBackend();
    void reportsRuntimeOnlyWhenChatStoreUnavailableOnClear();
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
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->chatHistory().first().role, sentinel::core::ChatRole::System);
    QVERIFY(controller->memoryEntries().isEmpty());
}

void ApplicationControllerTest::exposesProviderStatus() {
    const auto controller = makeController();

    QCOMPARE(controller->providerStatus(), QStringLiteral("Ready"));
}

void ApplicationControllerTest::exposesMemoryStatus() {
    const auto controller = makeController();

    QCOMPARE(controller->memoryStatus(), QStringLiteral("Available"));
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
    QCOMPARE(controller->chatHistory().at(1).status, sentinel::core::ChatMessageStatus::Sent);
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Received);
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
    QCOMPARE(controller->chatHistory().last().status, sentinel::core::ChatMessageStatus::Error);
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::handlesProviderErrorReply() {
    auto controller = std::make_unique<ApplicationController>(std::make_unique<ErrorProvider>(),
                                                              std::make_unique<InMemoryStore>());
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    const auto sent = controller->sendMessage(QStringLiteral("status"));

    QVERIFY(!sent);
    QCOMPARE(controller->chatMessages().size(), 3);
    QCOMPARE(controller->chatMessages().last(),
             QStringLiteral("Sentinel: Provider error: deterministic failure"));
    QCOMPARE(controller->chatHistory().last().status, sentinel::core::ChatMessageStatus::Error);
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::clearsChatHistory() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    controller->sendMessage(QStringLiteral("status"));
    controller->clearChat();

    QCOMPARE(controller->chatMessages(),
             QStringList{QStringLiteral("Sentinel: Sentinel Core online.")});
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->chatHistory().first().id, 1);
    QCOMPARE(spy.count(), 2);
}

void ApplicationControllerTest::loadsPersistedChatHistoryAtStartup() {
    auto store = std::make_unique<RecordingChatHistoryStore>(QList<sentinel::core::ChatMessage>{
        {4, sentinel::core::ChatRole::System, QStringLiteral("previous system"),
         QDateTime::fromString(QStringLiteral("2026-05-15T12:00:00.000Z"), Qt::ISODateWithMs),
         sentinel::core::ChatMessageStatus::Received},
        {5, sentinel::core::ChatRole::User, QStringLiteral("previous user"),
         QDateTime::fromString(QStringLiteral("2026-05-15T12:01:00.000Z"), Qt::ISODateWithMs),
         sentinel::core::ChatMessageStatus::Sent},
    });
    const auto storePtr = store.get();

    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    QCOMPARE(controller.chatHistory().size(), 2);
    QCOMPARE(controller.chatHistory().first().id, 4);
    QCOMPARE(controller.chatMessages().first(), QStringLiteral("Sentinel: previous system"));
    QCOMPARE(storePtr->messages_.size(), 2);
}

void ApplicationControllerTest::appendsNewChatMessagesToHistoryStore() {
    auto store = std::make_unique<RecordingChatHistoryStore>();
    const auto storePtr = store.get();
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    const auto sent = controller.sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(storePtr->messages_.size(), 3);
    QCOMPARE(storePtr->messages_.at(0).role, sentinel::core::ChatRole::System);
    QCOMPARE(storePtr->messages_.at(1).role, sentinel::core::ChatRole::User);
    QCOMPARE(storePtr->messages_.at(1).content, QStringLiteral("status"));
    QCOMPARE(storePtr->messages_.at(2).role, sentinel::core::ChatRole::Assistant);
}

void ApplicationControllerTest::clearsPersistentChatHistoryWhenAvailable() {
    auto store = std::make_unique<RecordingChatHistoryStore>();
    const auto storePtr = store.get();
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    controller.sendMessage(QStringLiteral("status"));
    const auto cleared = controller.clearChat();

    QVERIFY(cleared);
    QVERIFY(storePtr->wasCleared_);
    QCOMPARE(storePtr->messages_.size(), 1);
    QCOMPARE(storePtr->messages_.first().id, 1);
    QCOMPARE(storePtr->messages_.first().role, sentinel::core::ChatRole::System);
    QCOMPARE(controller.chatHistory().size(), 1);
    QCOMPARE(controller.chatMaintenanceStatus(), QStringLiteral("Clear completed"));
}

void ApplicationControllerTest::keepsRuntimeChatWorkingWhenHistoryStoreUnavailable() {
    auto store =
        std::make_unique<RecordingChatHistoryStore>(QList<sentinel::core::ChatMessage>{}, false);
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    const auto sent = controller.sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(controller.chatHistory().size(), 3);
    QCOMPARE(controller.chatHistory().at(1).content, QStringLiteral("status"));
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

void ApplicationControllerTest::clearsRuntimeMemoryEntries() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::memoryEntriesChanged);
    QSignalSpy maintenanceSpy(controller.get(), &ApplicationController::maintenanceStatusChanged);
    controller->remember(QStringLiteral("mode"), QStringLiteral("Companion"));

    const auto cleared = controller->clearMemory();

    QVERIFY(cleared);
    QVERIFY(controller->memoryEntries().isEmpty());
    QCOMPARE(controller->memoryMaintenanceStatus(), QStringLiteral("Clear completed"));
    QCOMPARE(spy.count(), 2);
    QCOMPARE(maintenanceSpy.count(), 1);
}

void ApplicationControllerTest::failsSafeWhenMemoryStoreUnavailable() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<UnavailableMemoryStore>());
    QSignalSpy spy(&controller, &ApplicationController::memoryEntriesChanged);
    QSignalSpy maintenanceSpy(&controller, &ApplicationController::maintenanceStatusChanged);

    const auto cleared = controller.clearMemory();

    QVERIFY(!cleared);
    QCOMPARE(controller.memoryStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(controller.memoryMaintenanceStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(spy.count(), 0);
    QCOMPARE(maintenanceSpy.count(), 1);
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

void ApplicationControllerTest::reportsRuntimeOnlyWhenChatStoreUnavailableOnClear() {
    auto store =
        std::make_unique<RecordingChatHistoryStore>(QList<sentinel::core::ChatMessage>{}, false);
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));
    QSignalSpy maintenanceSpy(&controller, &ApplicationController::maintenanceStatusChanged);

    controller.sendMessage(QStringLiteral("status"));
    const auto cleared = controller.clearChat();

    QVERIFY(!cleared);
    QCOMPARE(controller.chatHistory().size(), 1);
    QCOMPARE(controller.chatMaintenanceStatus(), QStringLiteral("Runtime Only"));
    QCOMPARE(maintenanceSpy.count(), 1);
}

QTEST_MAIN(ApplicationControllerTest)

#include "test_application_controller.moc"
