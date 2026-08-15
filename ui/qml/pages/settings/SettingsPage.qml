// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Shapes
import Sentinel.Desktop

Item {
    id: settingsPage
    required property var viewModel
    property var soundManager: null
    readonly property bool compact: width < 820
    readonly property int panelPadding: SentinelTheme.spaceLg

    signal openUpdateRequested()

    Accessible.name: qsTr("Settings Page")
    Accessible.role: Accessible.Pane
    Accessible.description: qsTr("Sentinel application preferences and settings interface")

    Shortcut {
        sequences: [StandardKey.Preferences]
        onActivated: {
            console.log("Native Preferences shortcut triggered")
        }
    }
    readonly property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property string uiSelfCheck: "modal-ready rail-scroll-sync voice-path-wrap agent-runtime bottom-safe-scroll"

    function buildSidebarItems() {
        return [
            { "key": "Interface", "title": qsTr("Interface"), "keywords": ["general", "appearance", "accessibility", "theme", "language", "density", "motion", "contrast"] },
            { "key": "AI", "title": qsTr("AI Settings"), "keywords": ["ai", "models", "chat", "profiles", "provider", "temperature", "tokens", "streaming"] },
            { "key": "Voice", "title": qsTr("Voice & Audio"), "keywords": ["voice", "audio", "speech", "tts", "stt", "kokoro", "piper", "whisper", "transcription"] },
            { "key": "Memory", "title": qsTr("Memory & Knowledge"), "keywords": ["brain", "workspace", "memory", "recall", "context", "rag", "knowledge", "files"] },
            { "key": "Security", "title": qsTr("Security & Agents"), "keywords": ["permissions", "tools", "agents", "boundary", "policy", "gateway", "sandbox", "proxy", "cloud", "api", "web search"] },
            { "key": "System", "title": qsTr("System"), "keywords": ["notifications", "updates", "policy", "version", "sound", "audio"] }
        ]
    }

    property var sidebarItems: buildSidebarItems()
    property string activeCategory: "Interface"
    property string searchQuery: ""

    Connections {
        target: shellViewModel
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
        settingsFlick.contentY = 0
    }

    RowLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        ShellPanel {
            Layout.preferredWidth: settingsPage.compact ? 196 : 278
            Layout.fillHeight: true
            color: SentinelTheme.backgroundBase
            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: SentinelTheme.spaceMd
                spacing: SentinelTheme.spaceMd

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: SentinelTheme.spaceMd
                    Layout.topMargin: SentinelTheme.spaceSm
                    Layout.bottomMargin: SentinelTheme.spaceXs

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Settings")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontTitle
                        font.bold: true
                        maximumLineCount: 1
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Preferences & system options")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        maximumLineCount: 1
                        elide: Text.ElideRight
                    }
                }

                ListView {
                    id: sidebarList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 4
                    model: settingsPage.filteredSidebarItems

                    delegate: ItemDelegate {
                        id: navItem
                        required property var modelData
                        width: sidebarList.width
                        implicitHeight: 40
                        hoverEnabled: true

                        readonly property bool active: settingsPage.activeCategory === modelData.key

                        onClicked: settingsPage.jumpTo(modelData.key)

                        contentItem: RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: SentinelTheme.spaceMd
                            anchors.rightMargin: SentinelTheme.spaceMd
                            spacing: SentinelTheme.spaceSm

                            Rectangle {
                                width: 4
                                height: 16
                                radius: 2
                                color: navItem.active ? settingsPage.modeAccent : "transparent"
                            }

                            Label {
                                Layout.fillWidth: true
                                text: navItem.modelData.title
                                color: navItem.active ? SentinelTheme.textPrimary : (navItem.hovered ? SentinelTheme.textPrimary : SentinelTheme.textMuted)
                                font.pixelSize: SentinelTheme.fontBody
                                font.bold: navItem.active
                                elide: Text.ElideRight
                            }
                        }

                        background: Rectangle {
                            radius: SentinelTheme.radiusMd
                            color: navItem.active
                                   ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.12)
                                   : (navItem.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04) : "transparent")
                        }
                    }
                }
            }
        }

        ScrollView {
            id: settingsFlick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Column {
                width: settingsFlick.availableWidth
                spacing: SentinelTheme.spaceLg

                AppearanceSettingsTab {
                    width: parent.width
                    visible: settingsPage.activeCategory === "Interface"
                    viewModel: settingsPage.viewModel
                    compact: settingsPage.compact
                    modeAccent: settingsPage.modeAccent
                }

                ModelSettingsTab {
                    width: parent.width
                    visible: settingsPage.activeCategory === "AI"
                    viewModel: settingsPage.viewModel
                    compact: settingsPage.compact
                    modeAccent: settingsPage.modeAccent
                    soundManager: settingsPage.soundManager
                }

                VoiceSettingsTab {
                    width: parent.width
                    visible: settingsPage.activeCategory === "Voice"
                    viewModel: settingsPage.viewModel
                    compact: settingsPage.compact
                    modeAccent: settingsPage.modeAccent
                    soundManager: settingsPage.soundManager
                }

                WorkspaceSettingsTab {
                    width: parent.width
                    visible: settingsPage.activeCategory === "Memory"
                    viewModel: settingsPage.viewModel
                    compact: settingsPage.compact
                    modeAccent: settingsPage.modeAccent
                }

                SecuritySettingsTab {
                    width: parent.width
                    visible: settingsPage.activeCategory === "Security"
                    viewModel: settingsPage.viewModel
                    compact: settingsPage.compact
                    modeAccent: settingsPage.modeAccent
                }

                SystemSettingsTab {
                    width: parent.width
                    visible: settingsPage.activeCategory === "System"
                    viewModel: settingsPage.viewModel
                    compact: settingsPage.compact
                    modeAccent: settingsPage.modeAccent
                    soundManager: settingsPage.soundManager
                    onOpenUpdateRequested: settingsPage.openUpdateRequested()
                }
            }
        }
    }
}
