// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ApplicationWindow {
    id: window
    width: 1100
    height: 780
    visible: true
    title: "Sentinel — disposable certification profile"
    color: SentinelTheme.backgroundBase
    property int surface: 0
    Binding { target: SentinelTheme; property: "activeTheme"; value: shellViewModel.themeName }
    Binding { target: SentinelTheme; property: "highContrast"; value: shellViewModel.highContrastEnabled }
    Binding { target: SentinelTheme; property: "reducedTransparency"; value: shellViewModel.reducedTransparencyEnabled }
    Binding { target: MotionTokens; property: "reducedMotion"; value: shellViewModel.reducedMotionEnabled }
    property color settingsBackground: SentinelTheme.backgroundBase
    property color settingsText: SentinelTheme.textPrimary
    property color settingsMuted: SentinelTheme.textMuted
    property color settingsSurface: SentinelTheme.surface
    property int settingsMotionDuration: MotionTokens.normal
    property color settingsPanel: SentinelTheme.panel

    NavigationRail {
        id: rail
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        currentPage: surface === 0 ? "Settings" : "Dashboard"
        onPageRequested: function(page) { surface = page === "Settings" ? 0 : 2 }
    }
    Timer {
        interval: 1000
        running: navigationSmoke
        onTriggered: {
            window.width = 780
            window.height = 640
            rail.pageRequested("Dashboard")
            if (surface !== 2 || rail.width !== 76) { Qt.exit(20); return }
            rail.pageRequested("Settings")
            settingsPage.sidebarExpanded = false
            settingsPage.jumpTo("Voice")
            settingsPage.searchQuery = "zzzz-no-match"
            if (surface !== 0 || settingsPage.activeCategory !== "Voice"
                || settingsPage.filteredSidebarItems.length !== 0) { Qt.exit(21); return }
            settingsPage.searchQuery = ""
            settingsPage.sidebarExpanded = true
            window.width = 1320
            Qt.callLater(function() {
                if (settingsPage.width <= 0 || settingsPage.height <= 0) { Qt.exit(22); return }
                console.info("Navigation and settings smoke passed at compact and wide sizes")
                Qt.exit(0)
            })
        }
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: rail.width + 16
        RowLayout {
            Button { text: "Settings"; onClicked: surface = 0 }
            Button { text: "Onboarding"; onClicked: { shellViewModel.reopenOnboarding(); surface = 1; onboarding.active = true } }
            Button { text: "Chat"; onClicked: surface = 2 }
            Button { text: "780 px"; onClicked: { window.width = 780; window.height = 640 } }
        }
        SettingsPage { id: settingsPage; viewModel: shellViewModel; visible: surface === 0; Layout.fillWidth: true; Layout.fillHeight: true }
        ModelsPage { objectName: "modelsPage"; viewModel: shellViewModel; visible: surface === 3; Layout.fillWidth: true; Layout.fillHeight: true }
        HomeChatSurface { viewModel: shellViewModel; visible: surface === 2; Layout.fillWidth: true; Layout.fillHeight: true }
    }
    QtObject {
        id: quickFixture
        objectName: "quickPanelFixture"
        property string error: ""
        property int submissions: 0
        property bool lastAgent: false
        property var state: ({ready: false, busy: false, connection: "Disconnected", state: "idle", voiceState: "idle", voiceAvailable: false, voiceTranscript: "", approvalCount: 0, approval: ({}), preview: ""})
        function ask(text) { if (!text.trim()) return false; submissions++; lastAgent = false; return true }
        function agent(text) { if (!text.trim()) return false; submissions++; lastAgent = true; return true }
        function cancel() { return true }
        function voiceAction(action) { return false }
        function approve(allow) { return false }
        function continueConversation() {}
        function openApproval() {}
    }
    TrayCompanionWindow { id: quickWindow; viewModel: shellViewModel; controller: quickFixture }
    OnboardingScreen {
        id: onboarding
        anchors.fill: parent
        viewModel: shellViewModel
        active: !shellViewModel.onboardingComplete || surface === 1
        onFinished: surface = 0
    }
}
