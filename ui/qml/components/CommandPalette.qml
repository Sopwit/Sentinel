pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

SentinelOverlayModal {
    id: palette
    required property var viewModel
    property string query: ""
    property string actionStatus: ""
    signal openSettingsRequested()
    signal focusChatRequested()
    readonly property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property var actions: [
        { "title": qsTr("Ask Sentinel"), "subtitle": qsTr("Focus the fixed chat composer"), "kind": qsTr("Chat"), "action": "ask", "enabled": true },
        { "title": qsTr("New Chat"), "subtitle": qsTr("Create a local conversation"), "kind": qsTr("Chat"), "action": "new-chat", "enabled": true },
        { "title": qsTr("Search Chats"), "subtitle": qsTr("Filter conversations by title or id"), "kind": qsTr("Search"), "action": "search-chats", "enabled": true },
        { "title": qsTr("Open Workspace"), "subtitle": qsTr("Open workspace controls in Settings"), "kind": qsTr("Workspace"), "action": "settings", "enabled": true },
        { "title": qsTr("Open Settings"), "subtitle": qsTr("Open floating preferences"), "kind": qsTr("Modal"), "action": "settings", "enabled": true },
        { "title": qsTr("Check Updates"), "subtitle": qsTr("Manual stub; no hidden network polling"), "kind": qsTr("Updates"), "action": "updates", "enabled": true },
        { "title": qsTr("Open Updates"), "subtitle": qsTr("Open manual update and release notes surfaces"), "kind": qsTr("Updates"), "action": "settings", "enabled": true },
        { "title": qsTr("Open Notifications"), "subtitle": qsTr("Open notification center controls"), "kind": qsTr("Notifications"), "action": "settings", "enabled": true },
        { "title": qsTr("Export Current Chat"), "subtitle": qsTr("Save Markdown in the controlled export directory"), "kind": qsTr("Export"), "action": "export-md", "enabled": true },
        { "title": qsTr("Export Data"), "subtitle": qsTr("Prepare export preview for local data"), "kind": qsTr("Export"), "action": "export-preview", "enabled": true },
        { "title": qsTr("Change Theme"), "subtitle": qsTr("Cycle Sentinel Dark, Midnight, Aurora, Graphite, System Adaptive"), "kind": qsTr("Appearance"), "action": "theme", "enabled": true },
        { "title": qsTr("Switch Model"), "subtitle": qsTr("Open Models settings"), "kind": qsTr("Models"), "action": "settings", "enabled": true },
        { "title": qsTr("Toggle Focus Mode"), "subtitle": qsTr("Switch to Focus Mode presentation"), "kind": qsTr("Mode"), "action": "focus-mode", "enabled": true },
        { "title": qsTr("Universal Search"), "subtitle": qsTr("Search chats, settings, models, and profiles"), "kind": qsTr("Search"), "action": "universal-search", "enabled": true }
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
        actionStatus = qsTr("Local navigation only. Quick actions are visible as metadata and do not execute.")
        searchField.forceActiveFocus()
    }

    function openPalette() {
        open()
    }

    function runAction(action) {
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
            palette.openSettingsRequested()
        } else if (action.action === "updates") {
            viewModel.checkForUpdates()
            actionStatus = qsTr("No update check was sent. Manual update networking is not implemented.")
        } else if (action.action === "export-md") {
            viewModel.exportTranscript("markdown")
            actionStatus = viewModel.conversationExportLastResultSummary
        } else if (action.action === "export-preview") {
            viewModel.prepareExportPreview("conversations", "Markdown")
            actionStatus = viewModel.exportPreviewSummaries.join(" / ")
        } else if (action.action === "theme") {
            var choices = ["Sentinel Dark", "Midnight", "Aurora", "Graphite", "System Adaptive"]
            var next = (choices.indexOf(viewModel.themeName) + 1) % choices.length
            viewModel.themeName = choices[next]
            actionStatus = qsTr("Theme changed to %1.").arg(viewModel.themeName)
        } else if (action.action === "focus-mode") {
            viewModel.currentModeName = "Focus Mode"
            actionStatus = qsTr("Focus Mode selected.")
        } else if (action.action === "search-chats" || action.action === "universal-search") {
            actionStatus = qsTr("Type to search commands, chats, settings, models, and profiles.")
        } else {
            actionStatus = qsTr("%1 is unavailable.").arg(action.title)
        }
    }

    contentItem: ColumnLayout {
        spacing: SentinelTheme.spaceMd

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
                enabled: true
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
