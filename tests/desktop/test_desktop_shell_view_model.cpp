#include "sentinel/desktop/DesktopShellViewModel.h"

#include "sentinel/core/AppSettings.h"
#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/InMemorySettingsStore.h"
#include "sentinel/core/InMemoryStore.h"
#include "sentinel/core/LocalEchoProvider.h"
#include "sentinel/core/ModeManager.h"

#include <QSignalSpy>
#include <QtTest>

#include <memory>

using sentinel::core::ApplicationController;
using sentinel::core::AppSettings;
using sentinel::core::InMemorySettingsStore;
using sentinel::core::InMemoryStore;
using sentinel::core::LocalEchoProvider;
using sentinel::core::ModeManager;
using sentinel::desktop::DesktopShellViewModel;

class DesktopShellViewModelTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesInitialShellState();
    void forwardsChatActions();
    void ignoresBlankChatActions();
    void forwardsModeChanges();
    void forwardsMemoryWrites();
    void forwardsSettingsChanges();
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

void DesktopShellViewModelTest::exposesInitialShellState() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.viewModel.providerName(), QStringLiteral("LocalEchoProvider"));
    QCOMPARE(fixture.viewModel.providerStatus(), QStringLiteral("Ready"));
    QCOMPARE(fixture.viewModel.currentModeName(), QStringLiteral("Companion Mode"));
    QVERIFY(fixture.viewModel.availableModes().contains(QStringLiteral("Tactical Mode")));
    QCOMPARE(fixture.viewModel.themeName(), QStringLiteral("Sentinel Dark"));
    QCOMPARE(fixture.viewModel.configurationProfile(), QStringLiteral("Desktop Alpha"));
    QCOMPARE(fixture.viewModel.currentPage(), QStringLiteral("Dashboard"));
    QCOMPARE(fixture.viewModel.availablePages(),
             QStringList({QStringLiteral("Dashboard"), QStringLiteral("Memory"),
                          QStringLiteral("Settings")}));
}

void DesktopShellViewModelTest::forwardsChatActions() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::chatMessagesChanged);

    const auto sent = fixture.viewModel.sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(fixture.viewModel.chatMessages().size(), 3);
    QCOMPARE(fixture.viewModel.chatMessages().last(),
             QStringLiteral("Sentinel: Sentinel Core online. Local chat pipeline is active."));
    QCOMPARE(spy.count(), 1);
}

void DesktopShellViewModelTest::ignoresBlankChatActions() {
    ViewModelFixture fixture;
    QSignalSpy spy(&fixture.viewModel, &DesktopShellViewModel::chatMessagesChanged);

    const auto sent = fixture.viewModel.sendMessage(QStringLiteral("   "));

    QVERIFY(!sent);
    QCOMPARE(fixture.viewModel.chatMessages().size(), 1);
    QCOMPARE(spy.count(), 0);
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
