pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

SentinelOverlayModal {
    id: palette
    required property var viewModel
    property string query: ""
    property string actionStatus: ""
    readonly property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property var actions: [
        { "title": "Home", "subtitle": "Open the main local chat workspace", "kind": "Navigate", "page": "Dashboard", "enabled": true },
        { "title": "Runtime/Memory", "subtitle": "Open local memory and runtime metadata", "kind": "Navigate", "page": "Memory", "enabled": true },
        { "title": "Agents", "subtitle": "Open metadata-only agent readiness", "kind": "Navigate", "page": "Agents", "enabled": true },
        { "title": "Settings", "subtitle": "Open desktop preferences", "kind": "Navigate", "page": "Settings", "enabled": true },
        { "title": "Clear Chat History", "subtitle": "Prepared only; no command-palette mutation", "kind": "Quick action", "page": "", "enabled": false },
        { "title": "Export Markdown", "subtitle": "Prepared only; no filesystem access from palette", "kind": "Quick action", "page": "", "enabled": false },
        { "title": "Export JSON", "subtitle": "Prepared only; no filesystem access from palette", "kind": "Quick action", "page": "", "enabled": false }
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
        actionStatus = "Local navigation only. Quick actions are visible as metadata and do not execute."
        searchField.forceActiveFocus()
    }

    function openPalette() {
        open()
    }

    function runAction(action) {
        if (action.enabled && action.page.length > 0) {
            viewModel.currentPage = action.page
            close()
            return
        }
        actionStatus = action.title + " is metadata-only in this shell."
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
                    text: "Command Palette"
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
                placeholderText: "Search local commands"
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
