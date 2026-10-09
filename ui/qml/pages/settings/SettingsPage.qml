// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: settingsPage
    objectName: "settingsPage"
    LayoutMirroring.enabled: viewModel.appLanguage === "ar"
    LayoutMirroring.childrenInherit: true
    required property var viewModel
    property var soundManager: null
    property bool sidebarExpanded: true
    readonly property bool compact: width < 820
    readonly property int panelPadding: SentinelTheme.spaceLg

    signal openUpdateRequested()

    Accessible.name: qsTr("Settings Page")
    Accessible.role: Accessible.Pane
    Accessible.description: qsTr("Sentinel application preferences and settings interface")

    readonly property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property string uiSelfCheck: "page-ready collapsible-sidebar voice-path-wrap agent-runtime bottom-safe-scroll"

    function buildSidebarItems() {
        return [
            { key: "Interface", title: qsTr("General"), icon: "settings", keywords: ["language", "general"] },
            { key: "Appearance", title: qsTr("Appearance"), icon: "palette", keywords: ["theme", "accessibility", "motion", "contrast", "density"] },
            { key: "AI", title: qsTr("Models & Providers"), icon: "brain", keywords: ["ai", "models", "provider", "temperature", "tokens", "cloud", "api key"] },
            { key: "Voice", title: qsTr("Voice & Audio"), icon: "microphone", keywords: ["voice", "audio", "speech", "whisper"] },
            { key: "Memory", title: qsTr("Workspace & Memory"), icon: "folder", keywords: ["workspace", "memory", "rag", "knowledge", "files", "context"] },
            { key: "Security", title: qsTr("Privacy & Permissions"), icon: "shield", keywords: ["permissions", "tools", "agents", "policy", "sandbox", "proxy", "web search"] },
            { key: "Notifications", title: qsTr("Notifications"), icon: "bell", keywords: ["notifications", "alerts", "quiet", "channels"] },
            { key: "System", title: qsTr("System"), icon: "device-desktop", keywords: ["updates", "version", "shortcuts", "startup", "diagnostics"] }
        ]
    }

    property var sidebarItems: buildSidebarItems()
    property string activeCategory: "Interface"
    property string searchQuery: ""

    Connections {
        target: settingsPage.viewModel
        function onAppLanguageChanged() { settingsPage.sidebarItems = settingsPage.buildSidebarItems() }
    }

    readonly property var filteredSidebarItems: {
        if (searchQuery.trim() === "")
            return sidebarItems
        var q = searchQuery.toLowerCase()
        return sidebarItems.filter(function(item) {
            if (item.title.toLowerCase().indexOf(q) !== -1)
                return true
            if (item.keywords) {
                for (var i = 0; i < item.keywords.length; i++) {
                    if (item.keywords[i].toLowerCase().indexOf(q) !== -1)
                        return true
                }
            }
            return false
        })
    }

    function jumpTo(category) {
        activeCategory = category
        var flick = settingsFlick.contentItem
        if (flick && flick.hasOwnProperty("contentY"))
            flick.contentY = 0
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0
        Rectangle {
            objectName: "settingsCategorySidebar"
            visible: settingsPage.sidebarExpanded && !settingsPage.compact
            Layout.preferredWidth: settingsPage.width < 1050 ? 256 : 280
            Layout.fillHeight: true
            color: SentinelTheme.backgroundRaised
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: SentinelTheme.spaceLg
                spacing: SentinelTheme.spaceMd
                Label {
                    text: qsTr("Settings")
                    font.pixelSize: SentinelTheme.fontTitle
                    font.bold: true
                    color: SentinelTheme.textPrimary
                }
                SentinelTextField {
                    objectName: "settingsSearch"
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search settings")
                    Accessible.name: placeholderText
                    onTextChanged: settingsPage.searchQuery = text
                }
                Label {
                    visible: settingsPage.filteredSidebarItems.length === 0
                    text: qsTr("No matching settings")
                    color: SentinelTheme.textMuted
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                ListView {
                    id: sidebarList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 6
                    model: settingsPage.filteredSidebarItems
                    delegate: ItemDelegate {
                        id: navItem
                        required property var modelData
                        width: sidebarList.width
                        height: Math.max(44, contentItem.implicitHeight + 16)
                        hoverEnabled: true
                        focusPolicy: Qt.StrongFocus
                        Accessible.name: modelData.title
                        readonly property bool active: settingsPage.activeCategory === modelData.key
                        ToolTip.visible: hovered || activeFocus
                        ToolTip.text: modelData.title
                        onClicked: settingsPage.jumpTo(modelData.key)
                        contentItem: RowLayout {
                            spacing: 12
                            SentinelIcon { name: navItem.modelData.icon; source: "qrc:/icons/tabler/" + name + ".svg"; iconSize: 19; tint: SentinelTheme.textMuted }
                            Label {
                                Layout.fillWidth: true
                                text: navItem.modelData.title
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                                font.bold: navItem.active
                                wrapMode: Text.WordWrap
                            }
                        }
                        background: Rectangle {
                            radius: SentinelTheme.radiusMd
                            color: navItem.active || navItem.hovered ? SentinelTheme.surfaceHover : "transparent"
                            border.color: navItem.activeFocus ? settingsPage.modeAccent : "transparent"
                            border.width: 2
                        }
                    }
                }
            }
        }
        Rectangle { visible: settingsPage.sidebarExpanded && !settingsPage.compact; Layout.preferredWidth: 1; Layout.fillHeight: true; color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.12) }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0
            SentinelComboBox {
                visible: !settingsPage.sidebarExpanded || settingsPage.compact
                Layout.fillWidth: true
                Layout.margins: SentinelTheme.spaceMd
                model: settingsPage.sidebarItems
                textRole: "title"
                currentIndex: settingsPage.sidebarItems.findIndex(function(item) { return item.key === settingsPage.activeCategory })
                Accessible.name: qsTr("Settings category")
                onActivated: settingsPage.jumpTo(settingsPage.sidebarItems[currentIndex].key)
            }
            ScrollView {
                id: settingsFlick
                objectName: "settingsContentScroll"
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth
                Column {
                    width: settingsFlick.availableWidth
                    topPadding: settingsPage.compact ? 24 : 48
                    bottomPadding: 48
                    spacing: SentinelTheme.spaceLg
                    Label {
                        x: Math.max(24, (parent.width - 960) / 2 + 24)
                        width: parent.width - x - 24
                        text: settingsPage.sidebarItems.find(function(item) { return item.key === settingsPage.activeCategory }).title
                        color: SentinelTheme.textPrimary
                        font.pixelSize: 30
                        font.bold: true
                        wrapMode: Text.WordWrap
                    }
                    Column {
                        width: Math.min(parent.width, 960)
                        anchors.horizontalCenter: parent.horizontalCenter
                        AppearanceSettingsTab {
                            width: parent.width
                            visible: settingsPage.activeCategory === "Interface" || settingsPage.activeCategory === "Appearance"
                            showGeneral: settingsPage.activeCategory === "Interface"
                            viewModel: settingsPage.viewModel
                            compact: settingsPage.compact
                        }
                        ModelSettingsTab { width: parent.width; visible: settingsPage.activeCategory === "AI"; viewModel: settingsPage.viewModel; compact: settingsPage.compact; soundManager: settingsPage.soundManager }
                        VoiceSettingsTab { width: parent.width; visible: settingsPage.activeCategory === "Voice"; viewModel: settingsPage.viewModel; compact: settingsPage.compact; soundManager: settingsPage.soundManager }
                        WorkspaceSettingsTab { width: parent.width; visible: settingsPage.activeCategory === "Memory"; viewModel: settingsPage.viewModel; compact: settingsPage.compact }
                        SecuritySettingsTab { width: parent.width; visible: settingsPage.activeCategory === "Security"; viewModel: settingsPage.viewModel; compact: settingsPage.compact }
                        SystemSettingsTab {
                            width: parent.width
                            visible: settingsPage.activeCategory === "Interface" || settingsPage.activeCategory === "System" || settingsPage.activeCategory === "Notifications"
                            generalOnly: settingsPage.activeCategory === "Interface"
                            notificationsOnly: settingsPage.activeCategory === "Notifications"
                            viewModel: settingsPage.viewModel
                            compact: settingsPage.compact
                            soundManager: settingsPage.soundManager
                            onOpenUpdateRequested: settingsPage.openUpdateRequested()
                        }
                    }
                }
            }
        }
    }
}
