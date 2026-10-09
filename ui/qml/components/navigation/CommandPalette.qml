// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import Sentinel.Desktop

SentinelOverlayModal {
    id: palette
    required property var viewModel
    property string query: ""
    property string actionStatus: ""
    signal openSettingsRequested(string category)
    signal openUpdateRequested()
    signal focusChatRequested()
    readonly property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property var actions: [
        { "title": qsTr("Ask Sentinel"), "subtitle": qsTr("Focus the fixed chat composer"), "kind": qsTr("Chat"), "action": "ask", "enabled": true },
        { "title": qsTr("New Chat"), "subtitle": qsTr("Create a local conversation"), "kind": qsTr("Chat"), "action": "new-chat", "enabled": true },
        { "title": qsTr("Search Chats"), "subtitle": qsTr("Conversation search is not available in this palette"), "kind": qsTr("Search"), "action": "search-chats", "enabled": false },
        { "title": qsTr("Open Workspace"), "subtitle": qsTr("Open workspace controls in Settings"), "kind": qsTr("Workspace"), "category": "Memory", "action": "settings", "enabled": true },
        { "title": qsTr("Open Settings"), "subtitle": qsTr("Open the full Settings page"), "kind": qsTr("Settings"), "action": "settings", "enabled": true },
        { "title": qsTr("Check Updates"), "subtitle": qsTr("Check for updates in the update dialog"), "kind": qsTr("Updates"), "action": "updates", "enabled": true },
        { "title": qsTr("Open Updates"), "subtitle": qsTr("Open the update status dialog"), "kind": qsTr("Updates"), "action": "updates", "enabled": true },
        { "title": qsTr("Open Notifications"), "subtitle": qsTr("Open notification center controls"), "kind": qsTr("Notifications"), "action": "notifications", "enabled": true },
        { "title": qsTr("Export Current Chat"), "subtitle": qsTr("Save Markdown in the controlled export directory"), "kind": qsTr("Export"), "action": "export-md", "enabled": true },
        { "title": qsTr("Export Data"), "subtitle": qsTr("Prepare export preview for local data"), "kind": qsTr("Export"), "action": "export-preview", "enabled": true },
        { "title": qsTr("Change Theme"), "subtitle": qsTr("Cycle through all available theme presets"), "kind": qsTr("Appearance"), "action": "theme", "enabled": true },
        { "title": qsTr("Switch Model"), "subtitle": qsTr("Open Models settings"), "kind": qsTr("Models"), "category": "AI", "action": "settings", "enabled": true },
        { "title": qsTr("Universal Search"), "subtitle": qsTr("Only command filtering is available; global search is not implemented"), "kind": qsTr("Search"), "action": "universal-search", "enabled": false }
    ]
    readonly property var filteredActions: {
        var normalized = query.trim().toLowerCase()
        var result = []
        for (var i = 0; i < actions.length; ++i) {
            var action = actions[i]
            var haystack = (action.title + " " + action.subtitle + " " + action.kind).toLowerCase()
            if (normalized.length === 0 || haystack.indexOf(normalized) >= 0)
                result.push(action)
        }
        return result
    }

    accent: modeAccent
    modeName: viewModel.currentModeName
    preferredWidth: 640
    preferredHeight: Math.min(520, Math.max(360, (parent ? parent.height : 720) - SentinelTheme.space4Xl))
    onOpened: {
        query = ""
        actionStatus = qsTr("Select an action or search commands.")
        searchField.forceActiveFocus()
    }

    function openPalette() {
        open()
    }

    function runAction(action) {
        if (!action.enabled) return
        if (action.page && action.page.length > 0) {
            viewModel.currentPage = action.page
            close()
            return
        }
        if (action.action === "ask") {
            close()
            palette.focusChatRequested()
        } else if (action.action === "new-chat") {
            viewModel.createConversation(qsTr("New Chat"))
            close()
            palette.focusChatRequested()
        } else if (action.action === "settings") {
            close()
            palette.openSettingsRequested(action.category || "Interface")
        } else if (action.action === "notifications") {
            close()
            viewModel.notificationCenterVisible = true
        } else if (action.action === "updates") {
            close()
            palette.openUpdateRequested()
        } else if (action.action === "export-md") {
            viewModel.exportTranscript("markdown")
            actionStatus = viewModel.conversationExportLastResultSummary
        } else if (action.action === "export-preview") {
            viewModel.prepareExportPreview("conversations", "Markdown")
            actionStatus = viewModel.exportPreviewSummaries.join(" / ")
        } else if (action.action === "theme") {
            var choices = viewModel.availableThemes
            if (!choices.length) return
            var next = (choices.indexOf(viewModel.themeName) + 1) % choices.length
            viewModel.themeName = choices[next]
            actionStatus = qsTr("Theme changed to %1.").arg(SentinelTheme.localizedThemeName(viewModel.themeName))

        } else {
            actionStatus = qsTr("%1 is unavailable.").arg(action.title)
        }
    }

    contentItem: ColumnLayout {
        spacing: SentinelTheme.spaceMd

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#000000"
            shadowOpacity: 0.10
            shadowBlur: 0.10
            shadowHorizontalOffset: 2
            shadowVerticalOffset: 2
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: SentinelTheme.spaceLg
            Layout.bottomMargin: 0
            spacing: SentinelTheme.spaceSm

            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceSm

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Command Palette")
                    color: SentinelTheme.textPrimary
                    font.pixelSize: SentinelTheme.fontCard
                    font.bold: true
                    maximumLineCount: 1
                    elide: Text.ElideRight
                }

                Label {
                    text: "Ctrl/Cmd K"
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontTiny
                    leftPadding: SentinelTheme.spaceSm
                    rightPadding: SentinelTheme.spaceSm
                    topPadding: 3
                    bottomPadding: 3
                    background: Rectangle {
                        radius: SentinelTheme.radiusSm
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.050)
                        border.color: SentinelTheme.withAlpha(palette.modeAccent, 0.14)
                    }
                }
            }

            SentinelTextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search local commands")
                Accessible.name: placeholderText
                text: palette.query
                onTextChanged: palette.query = text
                Keys.onDownPressed: actionList.forceActiveFocus()
                Keys.onReturnPressed: {
                    if (palette.filteredActions.length > 0)
                        palette.runAction(palette.filteredActions[0])
                }
            }
        }

        ListView {
            id: actionList
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: SentinelTheme.spaceLg
            Layout.rightMargin: SentinelTheme.spaceLg
            clip: true
            keyNavigationWraps: true
            boundsBehavior: Flickable.StopAtBounds
            boundsMovement: Flickable.StopAtBounds
            maximumFlickVelocity: 2200
            flickDeceleration: 5200
            spacing: SentinelTheme.spaceSm
            model: palette.filteredActions
            currentIndex: model.length > 0 ? 0 : -1
            ScrollBar.vertical: ScrollBar {
                id: actionListScrollBar
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 4
                    radius: 2
                    color: SentinelTheme.withAlpha(palette.modeAccent, actionListScrollBar.active ? 0.34 : 0.18)
                }
                background: Rectangle {
                    color: "transparent"
                }
            }
            Keys.onUpPressed: {
                if (currentIndex <= 0)
                    searchField.forceActiveFocus()
                else
                    decrementCurrentIndex()
            }
            Keys.onReturnPressed: {
                if (currentIndex >= 0 && currentIndex < palette.filteredActions.length)
                    palette.runAction(palette.filteredActions[currentIndex])
            }

            delegate: Button {
                id: actionButton
                required property var modelData
                required property int index
                readonly property bool active: ListView.isCurrentItem
                width: ListView.view.width
                height: Math.max(58, actionText.implicitHeight + SentinelTheme.spaceMd)
                hoverEnabled: true
                focusPolicy: Qt.StrongFocus
                enabled: modelData.enabled
                Accessible.name: modelData.title
                Accessible.description: modelData.subtitle
                onClicked: palette.runAction(modelData)

                contentItem: RowLayout {
                    id: actionText
                    spacing: SentinelTheme.spaceSm

                    Rectangle {
                        Layout.preferredWidth: 7
                        Layout.preferredHeight: 7
                        radius: 4
                        color: actionButton.modelData.enabled ? palette.modeAccent : SentinelTheme.textMuted
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            text: actionButton.modelData.title
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontBody
                            maximumLineCount: 1
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: actionButton.modelData.subtitle
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                        }
                    }

                    Label {
                        text: actionButton.modelData.kind
                        color: actionButton.modelData.enabled ? SentinelTheme.textMuted : SentinelTheme.warning
                        font.pixelSize: SentinelTheme.fontTiny
                    }
                }

                background: Rectangle {
                    radius: SentinelTheme.radiusMd
                    color: InteractionTokens.surfaceColor(actionButton.hovered, actionButton.down,
                                                           actionButton.active,
                                                           palette.modeAccent)
                    border.color: InteractionTokens.borderColor(actionButton.activeFocus,
                                                                 actionButton.hovered,
                                                                 actionButton.active,
                                                                 palette.modeAccent)

                    layer.enabled: actionButton.hovered
                    layer.effect: MultiEffect {
                        shadowEnabled: true
                        shadowColor: "#000000"
                        shadowOpacity: 0.12
                        shadowBlur: 0.08
                        shadowHorizontalOffset: 1
                        shadowVerticalOffset: 1
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: SentinelTheme.spaceLg
            Layout.rightMargin: SentinelTheme.spaceLg
            Layout.bottomMargin: SentinelTheme.spaceLg
            text: palette.actionStatus
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }
    }
}
