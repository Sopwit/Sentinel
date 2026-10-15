// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop
ApplicationWindow {
    id: window
    width: 1100; height: 780
    visible: true
    color: SentinelTheme.backgroundBase
    title: "Extensions — synthetic UI fixture, no extensions executed"
    property string currentPage: "Extensions"
    property string settingsCategory: ""
    Binding { target: SentinelTheme; property: "activeTheme"; value: shellViewModel.themeName }
    QtObject {
        id: fixture
        objectName: "extensionsFixture"
        property bool daemonConnected: false
        property var data: ({phase: "disconnected", message: "Daemon disconnected — synthetic UI fixture", entries: [], tools: [], policy: []})
        property var availability: ({phase: "ready", available: false, version: "fixture", requirements: ["Fixture permission pending"]})
        signal runtimeProjectionChanged()
        property var actionData: ({phase: "unavailable"})
        property int actionCalls: 0
        onActionDataChanged: runtimeProjectionChanged()
        function extensionsHubActionState() { return actionData }
        function performExtensionsHubAction(id, action) {
            if (actionData.phase === "pending") return false
            actionCalls += 1
            actionData = {phase: "pending", id: id, action: action}
            return true
        }
        function extensionsHubSnapshot() { return data }
        function extensionsHubDetail(id) { return daemonConnected ? availability : {phase: "disconnected"} }
        function refreshExtensionsHub() { runtimeProjectionChanged() }
        onDataChanged: runtimeProjectionChanged()
        onDaemonConnectedChanged: runtimeProjectionChanged()
    }
    NavigationRail {
        id: rail
        anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
        currentPage: window.currentPage
        connected: fixture.daemonConnected
        onPageRequested: function(page) {
            window.currentPage = page
            if (page === "Tools" || page === "Extensions") hub.openSection(page === "Tools" ? "tools" : "plugins")
        }
    }
    ExtensionsHubPage {
        id: hub
        anchors.fill: parent
        anchors.leftMargin: rail.width + 24
        anchors.margins: 24
        viewModel: fixture
        visible: window.currentPage === "Extensions" || window.currentPage === "Tools"
        onSettingsRequested: function(category) { window.settingsCategory = category; window.currentPage = "Settings" }
    }
}
