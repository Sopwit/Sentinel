import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ShellPanel {
    id: chatPanel
    required property var viewModel
    property bool compact: width < 520
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property bool modelReady: viewModel.selectedLocalModelStatus === "Available"
                                      || viewModel.selectedLocalModelStatus === "Fallback"
    readonly property bool chatReady: viewModel.localChatInferenceEnabled && modelReady
    readonly property int contentPadding: compact ? SentinelTheme.spaceMd : SentinelTheme.spaceLg
    readonly property string uiSelfCheck: "conversation-menu-themed no-message-duplication disabled-actions-muted"
    property string renameStatusText: ""

    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.046)
    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: chatPanel.contentPadding
        spacing: SentinelTheme.spaceMd

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            Rectangle {
                Layout.preferredWidth: 6
                Layout.preferredHeight: 6
                radius: 3
                color: chatPanel.chatReady ? SentinelTheme.success : SentinelTheme.textMuted
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceXs

                Label {
                    Layout.fillWidth: true
                    text: "AI BRIDGE"
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontTiny
                    font.letterSpacing: 2.4
                }

                Label {
                    Layout.fillWidth: true
                    text: "Local provider status and conversations"
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                }
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            StatusChip {
                label: "Provider"
                value: "Local Ollama"
                accent: chatPanel.chatReady ? SentinelTheme.success : SentinelTheme.textMuted
                muted: !chatPanel.chatReady
            }

            StatusChip {
                label: "Model"
                value: chatPanel.viewModel.selectedLocalModelStatus
                accent: chatPanel.modelReady ? SentinelTheme.success : SentinelTheme.textMuted
                muted: !chatPanel.modelReady
            }

            StatusChip {
                label: "Cloud"
                value: "inactive"
                accent: SentinelTheme.textMuted
                muted: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            ColumnLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceXs

                Label {
                    Layout.fillWidth: true
                    text: "Conversations"
                    color: SentinelTheme.textPrimary
                    font.pixelSize: SentinelTheme.fontCard
                }

                Label {
                    Layout.fillWidth: true
                    text: chatPanel.viewModel.conversationStoreConversationCount
                          + (chatPanel.viewModel.conversationStoreConversationCount === 1
                             ? " conversation"
                             : " conversations")
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                }
            }

            SentinelButton {
                text: "New"
                Layout.preferredWidth: 78
                onClicked: {
                    var nextNumber = chatPanel.viewModel.conversationStoreConversationCount + 1
                    chatPanel.viewModel.createConversation("Conversation " + nextNumber)
                }
            }
        }

        ListView {
            id: conversationList
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 220
            clip: true
            spacing: SentinelTheme.spaceSm
            model: chatPanel.viewModel.conversationIds.length
            boundsBehavior: Flickable.StopAtBounds
            boundsMovement: Flickable.StopAtBounds
            maximumFlickVelocity: 2200
            flickDeceleration: 5200
            bottomMargin: SentinelTheme.spaceMd
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 4
                    radius: 2
                    color: SentinelTheme.withAlpha(chatPanel.modeAccent, parent.active ? 0.34 : 0.18)
                }
                background: Rectangle {
                    color: "transparent"
                }
            }

            delegate: Rectangle {
                id: conversationItem
                required property int index
                readonly property string conversationId: chatPanel.viewModel.conversationIds[index]
                readonly property bool active: conversationId === chatPanel.viewModel.activeConversationId
                readonly property bool archived: chatPanel.viewModel.conversationArchivedSummaries[index] === "Archived"
                readonly property bool hovered: conversationClick.containsMouse

                width: ListView.view.width
                radius: SentinelTheme.radiusMd
                color: active ? SentinelTheme.withAlpha(chatPanel.modeAccent, 0.11)
                              : hovered ? SentinelTheme.withAlpha(chatPanel.modeAccent, 0.060)
                              : archived ? SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.030)
                                         : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.024)
                border.color: active ? SentinelTheme.withAlpha(chatPanel.modeAccent, 0.22)
                                     : hovered ? SentinelTheme.withAlpha(chatPanel.modeAccent, 0.14)
                                               : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.055)
                opacity: archived && !active ? 0.70 : 1.0
                implicitHeight: conversationRow.implicitHeight + SentinelTheme.spaceMd
                scale: hovered && !active ? InteractionTokens.cardHoverLift
                                          : active ? InteractionTokens.focusScale : 1.0

                Behavior on color {
                    ColorAnimation {
                        duration: MotionTokens.fast
                        easing.type: MotionTokens.standard
                    }
                }

                Behavior on border.color {
                    ColorAnimation {
                        duration: MotionTokens.fast
                        easing.type: MotionTokens.standard
                    }
                }

                Behavior on scale {
                    NumberAnimation {
                        duration: MotionTokens.fast
                        easing.type: MotionTokens.enter
                    }
                }

                MouseArea {
                    id: conversationClick
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (!conversationItem.active)
                            chatPanel.viewModel.switchConversation(conversationItem.conversationId)
                    }
                }

                Rectangle {
                    width: 3
                    height: conversationItem.active ? parent.height - SentinelTheme.spaceMd : 0
                    radius: 2
                    anchors.left: parent.left
                    anchors.leftMargin: SentinelTheme.spaceXs
                    anchors.verticalCenter: parent.verticalCenter
                    color: chatPanel.modeAccent
                    opacity: conversationItem.active ? 0.86 : 0.0

                    Behavior on height {
                        NumberAnimation {
                            duration: MotionTokens.duration(MotionTokens.fast, chatPanel.viewModel.currentModeName)
                            easing.type: MotionTokens.enter
                        }
                    }

                    Behavior on opacity {
                        NumberAnimation {
                            duration: MotionTokens.duration(MotionTokens.fast, chatPanel.viewModel.currentModeName)
                            easing.type: MotionTokens.standard
                        }
                    }
                }

                RowLayout {
                    id: conversationRow
                    x: SentinelTheme.spaceSm
                    y: SentinelTheme.spaceXs
                    width: parent.width - SentinelTheme.spaceSm * 2
                    spacing: SentinelTheme.spaceSm

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            Layout.fillWidth: true
                            text: chatPanel.viewModel.conversationTitles[index]
                            color: conversationItem.archived && !conversationItem.active
                                   ? SentinelTheme.textMuted
                                   : SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontSmall
                            font.bold: conversationItem.active
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: (conversationItem.active ? "Current" : "Tap to open")
                                  + " / "
                                  + chatPanel.viewModel.conversationMessageCountSummaries[index]
                                  + " / "
                                  + chatPanel.viewModel.conversationArchivedSummaries[index]
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontTiny
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                        }
                    }

                    Button {
                        id: overflowButton
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 30
                        text: "\u22ef"
                        hoverEnabled: true
                        focusPolicy: Qt.StrongFocus
                        onClicked: overflowMenu.open()

                        contentItem: Text {
                            text: overflowButton.text
                            color: SentinelTheme.textPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: SentinelTheme.fontControl
                        }

                        background: Rectangle {
                            radius: 15
                            color: InteractionTokens.surfaceColor(overflowButton.hovered, overflowButton.down,
                                                                   overflowMenu.visible,
                                                                   chatPanel.modeAccent)
                            border.color: InteractionTokens.borderColor(overflowButton.activeFocus,
                                                                         overflowButton.hovered,
                                                                         overflowMenu.visible,
                                                                         chatPanel.modeAccent)

                            Behavior on color {
                                ColorAnimation {
                                    duration: MotionTokens.fast
                                    easing.type: MotionTokens.standard
                                }
                            }
                        }

                        Menu {
                            id: overflowMenu
                            width: 180
                            x: overflowButton.width - width
                            y: overflowButton.height + SentinelTheme.spaceXs
                            padding: SentinelTheme.spaceXs
                            modal: true
                            dim: false

                            enter: Transition {
                                NumberAnimation {
                                    property: "opacity"
                                    from: 0.0
                                    to: 1.0
                                    duration: MotionTokens.menu
                                    easing.type: MotionTokens.enter
                                }
                            }

                            exit: Transition {
                                NumberAnimation {
                                    property: "opacity"
                                    to: 0.0
                                    duration: MotionTokens.fast
                                    easing.type: MotionTokens.exit
                                }
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusLg
                                color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                border.color: InteractionTokens.borderColor(false, true, false,
                                                                             chatPanel.modeAccent)
                            }

                            MenuItem {
                                id: renameMenuItem
                                text: "Rename"
                                onTriggered: {
                                    if (!conversationItem.active)
                                        chatPanel.viewModel.switchConversation(conversationItem.conversationId)
                                    renameInput.forceActiveFocus()
                                }

                                contentItem: Text {
                                    text: renameMenuItem.text
                                    color: renameMenuItem.enabled ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: InteractionTokens.surfaceColor(renameMenuItem.highlighted, false, false,
                                                                           chatPanel.modeAccent)
                                }
                            }

                            MenuItem {
                                id: archiveMenuItem
                                text: conversationItem.archived ? "Unarchive" : "Archive"
                                onTriggered: {
                                    if (conversationItem.archived)
                                        chatPanel.viewModel.unarchiveConversation(conversationItem.conversationId)
                                    else
                                        chatPanel.viewModel.archiveConversation(conversationItem.conversationId)
                                }

                                contentItem: Text {
                                    text: archiveMenuItem.text
                                    color: archiveMenuItem.enabled ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: InteractionTokens.surfaceColor(archiveMenuItem.highlighted, false, false,
                                                                           chatPanel.modeAccent)
                                }
                            }

                            MenuItem {
                                id: pinMenuItem
                                text: "Pin unavailable"
                                enabled: false

                                contentItem: Text {
                                    text: pinMenuItem.text
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.024)
                                }
                            }

                            MenuItem {
                                id: deleteMenuItem
                                text: "Delete unavailable"
                                enabled: false

                                contentItem: Text {
                                    text: deleteMenuItem.text
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.024)
                                }
                            }
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: chatPanel.viewModel.conversationBrowserEmptyStateVisible
            text: chatPanel.viewModel.conversationBrowserEmptyStateSummary
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            radius: SentinelTheme.radiusMd
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.028)
            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
            implicitHeight: selectedStatusColumn.implicitHeight + SentinelTheme.spaceMd

            ColumnLayout {
                id: selectedStatusColumn
                x: SentinelTheme.spaceSm
                y: SentinelTheme.spaceXs
                width: parent.width - SentinelTheme.spaceSm * 2
                spacing: SentinelTheme.spaceXs

                InfoRow {
                    compact: true
                    label: "Selected"
                    value: chatPanel.viewModel.activeConversationSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: true
                    label: "State"
                    value: chatPanel.viewModel.activeConversationStateSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: true
                    label: "Delete"
                    value: chatPanel.viewModel.conversationDeleteReadinessStatus
                           + " / metadata-only"
                    Layout.fillWidth: true
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            SentinelTextField {
                id: renameInput
                Layout.fillWidth: true
                placeholderText: "Rename selected conversation"
                enabled: chatPanel.viewModel.activeConversationId.length > 0
                onAccepted: {
                    if (renameButton.enabled)
                        renameButton.clicked()
                }
            }

            SentinelButton {
                id: renameButton
                text: "Rename"
                enabled: renameInput.text.trim().length > 0
                Layout.preferredWidth: 92
                onClicked: {
                    var renamed = chatPanel.viewModel.renameConversation(
                        chatPanel.viewModel.activeConversationId,
                        renameInput.text)
                    chatPanel.renameStatusText = renamed ? "Rename saved." : "Rename refused."
                    if (renamed)
                        renameInput.clear()
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: chatPanel.renameStatusText.length > 0
            text: chatPanel.renameStatusText
            color: chatPanel.renameStatusText === "Rename saved."
                   ? SentinelTheme.success
                   : SentinelTheme.warning
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }
    }
}
