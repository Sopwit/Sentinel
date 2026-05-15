#include "sentinel/desktop/DesktopShellViewModel.h"

#include "sentinel/core/AppSettings.h"
#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/IChatHistoryStore.h"
#include "sentinel/core/InMemorySettingsStore.h"
#include "sentinel/core/InMemoryStore.h"
#include "sentinel/core/LocalEchoProvider.h"
#include "sentinel/core/ModeManager.h"

#include <QSignalSpy>
#include <QtTest>

#include <memory>

using sentinel::core::ApplicationController;
using sentinel::core::AppSettings;
using sentinel::core::IChatHistoryStore;
using sentinel::core::InMemorySettingsStore;
using sentinel::core::InMemoryStore;
using sentinel::core::LocalEchoProvider;
using sentinel::core::ModeManager;
using sentinel::desktop::ChatMessageListModel;
using sentinel::desktop::DesktopShellViewModel;

class DesktopShellViewModelTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesInitialShellState();
    void exposesChatHistoryStatus();
    void exposesMaintenanceStatuses();
    void exposesStartupLoadedMessages();
    void forwardsChatActions();
    void ignoresBlankChatActions();
    void clearsMemoryActions();
    void clearsChatActions();
    void reportsRuntimeOnlyChatMaintenanceWhenStoreUnavailable();
    void forwardsModeChanges();
    void forwardsMemoryWrites();
    void forwardsSettingsChanges();
    void keepsSettingsSeparateFromClearActions();
    void tracksNavigationState();
    void ignoresRepeatedAndUnknownNavigationChanges();
};

class ViewModelFixture {
public:
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
};

class StaticChatHistoryStore final : public IChatHistoryStore {
public:
    explicit StaticChatHistoryStore(QList<sentinel::core::ChatMessage> messages = {},
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
            messages_.clear();
        }
    }

    bool isAvailable() const override {
        return available_;
    }

private:
    QList<sentinel::core::ChatMessage> messages_;
    bool available_ = true;
};

void DesktopShellViewModelTest::exposesInitialShellState() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.providerName(), QStringLiteral("LocalEchoProvider"));
    QCOMPARE(fixture.viewModel.providerStatus(), QStringLiteral("Ready"));
    QCOMPARE(fixture.viewModel.memoryStatus(), QStringLiteral("Available"));
    QCOMPARE(fixture.viewModel.chatHistoryStatus(), QStringLiteral("Runtime Only"));
    QCOMPARE(fixture.viewModel.currentModeName(), QStringLiteral("Companion Mode"));
    QVERIFY(fixture.viewModel.availableModes().contains(QStringLiteral("Tactical Mode")));
    QCOMPARE(fixture.viewModel.themeName(), QStringLiteral("Sentinel Dark"));
    QCOMPARE(fixture.viewModel.configurationProfile(), QStringLiteral("Desktop Alpha"));
    QCOMPARE(fixture.viewModel.currentPage(), QStringLiteral("Dashboard"));
    QCOMPARE(fixture.viewModel.availablePages(),
             QStringList({QStringLiteral("Dashboard"), QStringLiteral("Memory"),
                          QStringLiteral("Settings")}));
}

void DesktopShellViewModelTest::exposesChatHistoryStatus() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr,
                                     std::make_unique<StaticChatHistoryStore>()};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};

    QCOMPARE(viewModel.chatHistoryStatus(), QStringLiteral("Available"));
}

void DesktopShellViewModelTest::exposesMaintenanceStatuses() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.memoryMaintenanceStatus(), QStringLiteral("Ready"));
    QCOMPARE(fixture.viewModel.chatMaintenanceStatus(), QStringLiteral("Ready"));
}

void DesktopShellViewModelTest::exposesStartupLoadedMessages() {
    QList<sentinel::core::ChatMessage> persisted{
        {7, sentinel::core::ChatRole::System, QStringLiteral("previous system"),
         QDateTime::fromString(QStringLiteral("2026-05-15T12:00:00.000Z"), Qt::ISODateWithMs),
         sentinel::core::ChatMessageStatus::Received},
        {8, sentinel::core::ChatRole::User, QStringLiteral("previous user"),
         QDateTime::fromString(QStringLiteral("2026-05-15T12:01:00.000Z"), Qt::ISODateWithMs),
         sentinel::core::ChatMessageStatus::Sent},
    };
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr,
                                     std::make_unique<StaticChatHistoryStore>(persisted)};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};

    QCOMPARE(viewModel.chatMessages()->rowCount(), 2);
    const auto firstIndex = viewModel.chatMessages()->index(0, 0);
    const auto secondIndex = viewModel.chatMessages()->index(1, 0);
    QCOMPARE(viewModel.chatMessages()->data(firstIndex, ChatMessageListModel::ContentRole),
             QStringLiteral("previous system"));
    QCOMPARE(viewModel.chatMessages()->data(secondIndex, ChatMessageListModel::ContentRole),
             QStringLiteral("previous user"));
    QCOMPARE(viewModel.chatMessages()->data(secondIndex, ChatMessageListModel::StatusRole),
             QStringLiteral("sent"));
}

void DesktopShellViewModelTest::forwardsChatActions() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::chatMessagesChanged);

    const auto sent = fixture.viewModel.sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(fixture.viewModel.chatMessages()->rowCount(), 3);
    const auto lastIndex = fixture.viewModel.chatMessages()->index(2, 0);
    QCOMPARE(fixture.viewModel.chatMessages()->data(lastIndex, ChatMessageListModel::ContentRole),
             QStringLiteral("Sentinel Core online. Local chat pipeline is active."));
    QCOMPARE(fixture.viewModel.chatMessages()->data(lastIndex, ChatMessageListModel::StatusRole),
             QStringLiteral("received"));
    QCOMPARE(spy.count(), 1);
}

void DesktopShellViewModelTest::ignoresBlankChatActions() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::chatMessagesChanged);

    const auto sent = fixture.viewModel.sendMessage(QStringLiteral("   "));

    QVERIFY(!sent);
    QCOMPARE(fixture.viewModel.chatMessages()->rowCount(), 1);
    QCOMPARE(spy.count(), 0);
}

void DesktopShellViewModelTest::clearsChatActions() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::chatMessagesChanged);

    fixture.viewModel.sendMessage(QStringLiteral("status"));
    fixture.viewModel.clearChat();

    QCOMPARE(fixture.viewModel.chatMessages()->rowCount(), 1);
    QCOMPARE(spy.count(), 2);
}

void DesktopShellViewModelTest::clearsMemoryActions() {
    ViewModelFixture fixture;
    QSignalSpy memorySpy(&fixture.viewModel, &DesktopShellViewModel::memoryEntriesChanged);
    QSignalSpy maintenanceSpy(&fixture.viewModel, &DesktopShellViewModel::maintenanceStatusChanged);
    fixture.viewModel.remember(QStringLiteral("mode"), QStringLiteral("Companion"));

    const auto cleared = fixture.viewModel.clearMemory();

    QVERIFY(cleared);
    QVERIFY(fixture.viewModel.memoryEntries().isEmpty());
    QCOMPARE(fixture.viewModel.memoryMaintenanceStatus(), QStringLiteral("Clear completed"));
    QCOMPARE(memorySpy.count(), 2);
    QCOMPARE(maintenanceSpy.count(), 1);
}

void DesktopShellViewModelTest::reportsRuntimeOnlyChatMaintenanceWhenStoreUnavailable() {
    ApplicationController controller{
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr,
        std::make_unique<StaticChatHistoryStore>(QList<sentinel::core::ChatMessage>{}, false)};
    ModeManager modeManager;
    AppSettings settings{std::make_unique<InMemorySettingsStore>()};
    DesktopShellViewModel viewModel{controller, modeManager, settings};
    QSignalSpy maintenanceSpy(&viewModel, &DesktopShellViewModel::maintenanceStatusChanged);

    viewModel.sendMessage(QStringLiteral("status"));
    const auto cleared = viewModel.clearChat();

    QVERIFY(!cleared);
    QCOMPARE(viewModel.chatMaintenanceStatus(), QStringLiteral("Runtime Only"));
    QCOMPARE(viewModel.chatMessages()->rowCount(), 1);
    QCOMPARE(maintenanceSpy.count(), 1);
}

void DesktopShellViewModelTest::forwardsModeChanges() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::currentModeChanged);

    fixture.viewModel.setModeByName(QStringLiteral("Mission Mode"));

    QCOMPARE(fixture.viewModel.currentModeName(), QStringLiteral("Mission Mode"));
    QCOMPARE(spy.count(), 1);
}

void DesktopShellViewModelTest::forwardsMemoryWrites() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::memoryEntriesChanged);

    fixture.viewModel.remember(QStringLiteral("mode"), QStringLiteral("Companion"));

    QCOMPARE(fixture.viewModel.memoryEntries(), QStringList{QStringLiteral("mode: Companion")});
    QCOMPARE(spy.count(), 1);
}

void DesktopShellViewModelTest::forwardsSettingsChanges() {
    ViewModelFixture fixture;
    QSignalSpy themeSpy(&fixture.viewModel, &DesktopShellViewModel::themeNameChanged);
    QSignalSpy profileSpy(&fixture.viewModel, &DesktopShellViewModel::configurationProfileChanged);

    fixture.viewModel.setThemeName(QStringLiteral("Sentinel Light"));
    fixture.viewModel.setConfigurationProfile(QStringLiteral("Phase 2 Shell"));

    QCOMPARE(fixture.viewModel.themeName(), QStringLiteral("Sentinel Light"));
    QCOMPARE(fixture.viewModel.configurationProfile(), QStringLiteral("Phase 2 Shell"));
    QCOMPARE(themeSpy.count(), 1);
    QCOMPARE(profileSpy.count(), 1);
}

void DesktopShellViewModelTest::keepsSettingsSeparateFromClearActions() {
    ViewModelFixture fixture;
    fixture.viewModel.setThemeName(QStringLiteral("Sentinel Light"));
    fixture.viewModel.setConfigurationProfile(QStringLiteral("Desktop Stable"));
    fixture.viewModel.remember(QStringLiteral("mode"), QStringLiteral("Companion"));
    fixture.viewModel.sendMessage(QStringLiteral("status"));

    fixture.viewModel.clearMemory();
    fixture.viewModel.clearChat();

    QCOMPARE(fixture.viewModel.themeName(), QStringLiteral("Sentinel Light"));
    QCOMPARE(fixture.viewModel.configurationProfile(), QStringLiteral("Desktop Stable"));
}

void DesktopShellViewModelTest::tracksNavigationState() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::currentPageChanged);

    fixture.viewModel.setCurrentPage(QStringLiteral("Memory"));

    QCOMPARE(fixture.viewModel.currentPage(), QStringLiteral("Memory"));
    QCOMPARE(spy.count(), 1);

    fixture.viewModel.setCurrentPage(QStringLiteral("Settings"));
    QCOMPARE(fixture.viewModel.currentPage(), QStringLiteral("Settings"));
    QCOMPARE(spy.count(), 2);
}

void DesktopShellViewModelTest::ignoresRepeatedAndUnknownNavigationChanges() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::currentPageChanged);

    fixture.viewModel.setCurrentPage(QStringLiteral("Dashboard"));
    QCOMPARE(spy.count(), 0);

    fixture.viewModel.setCurrentPage(QStringLiteral("Unknown"));
    QCOMPARE(fixture.viewModel.currentPage(), QStringLiteral("Dashboard"));
    QCOMPARE(spy.count(), 0);
}

QTEST_MAIN(DesktopShellViewModelTest)

#include "test_desktop_shell_view_model.moc"
