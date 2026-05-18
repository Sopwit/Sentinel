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
    readonly property int contentPadding: compact ? SentinelTheme.spaceMd : SentinelTheme.space2Xl
    readonly property int cardPadding: SentinelTheme.spaceMd
    readonly property string bridgeStatusText: "Model "
                                               + chatPanel.viewModel.selectedLocalModelStatus
                                               + " / Chat "
                                               + chatPanel.viewModel.localChatInferenceStatus
                                               + " / Stream "
                                               + chatPanel.viewModel.localInferenceStreamStatus
    readonly property string runtimeStatusText: chatPanel.viewModel.ollamaHealthStatus
                                                + " / "
                                                + chatPanel.viewModel.localInferenceRuntimeState
    property string renameStatusText: ""

    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.038)
    border.color: SentinelTheme.withAlpha(modeAccent, 0.095)
    bracketColor: SentinelTheme.withAlpha(modeAccent, 0.22)
    bracketSize: 12

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: chatPanel.contentPadding
        spacing: SentinelTheme.spaceLg

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            Rectangle {
                Layout.preferredWidth: 6
                Layout.preferredHeight: 6
                radius: 3
                color: chatPanel.modeAccent
                opacity: 0.9
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceXs

                Label {
                    Layout.fillWidth: true
                    text: "AI BRIDGE / LIVE SURFACE"
                    color: chatPanel.modeAccent
                    font.pixelSize: SentinelTheme.fontTiny
                    font.letterSpacing: 2.6
                }

                Label {
                    Layout.fillWidth: true
                    text: chatPanel.bridgeStatusText
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    elide: Text.ElideRight
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: SentinelTheme.radiusMd
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.026)
            border.color: SentinelTheme.withAlpha(chatPanel.modeAccent, 0.10)
            implicitHeight: conversationBrowserColumn.implicitHeight + SentinelTheme.spaceMd

            ColumnLayout {
                id: conversationBrowserColumn
                x: SentinelTheme.spaceSm
                y: SentinelTheme.spaceSm
                width: parent.width - SentinelTheme.spaceSm * 2
                spacing: SentinelTheme.spaceSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceSm

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceXs

                        Label {
                            Layout.fillWidth: true
                            text: "CONVERSATIONS"
                            color: chatPanel.modeAccent
                            font.pixelSize: SentinelTheme.fontTiny
                            font.letterSpacing: 2.2
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: chatPanel.viewModel.activeConversationSummary
                                  + " / "
                                  + chatPanel.viewModel.activeConversationStateSummary
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall
                            elide: Text.ElideRight
                        }
                    }

                    SentinelButton {
                        text: "New"
                        Layout.preferredWidth: 82
                        onClicked: {
                            var nextNumber = chatPanel.viewModel.conversationStoreConversationCount + 1
                            chatPanel.viewModel.createConversation("Conversation " + nextNumber)
                        }
                    }
                }

                Repeater {
                    model: chatPanel.viewModel.conversationIds.length

                    delegate: Rectangle {
                        required property int index
                        readonly property string conversationId: chatPanel.viewModel.conversationIds[index]
                        readonly property bool active: conversationId === chatPanel.viewModel.activeConversationId
                        readonly property bool archived: chatPanel.viewModel.conversationArchivedSummaries[index] === "Archived"
                        Layout.fillWidth: true
                        radius: SentinelTheme.radiusSm
                        color: active ? SentinelTheme.withAlpha(chatPanel.modeAccent, 0.12) : archived ? SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.035) : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.018)
                        border.color: active ? SentinelTheme.withAlpha(chatPanel.modeAccent, 0.38) : archived ? SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.09) : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.045)
                        opacity: archived && !active ? 0.72 : 1.0
                        implicitHeight: conversationRow.implicitHeight + SentinelTheme.spaceSm

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
                                    text: (active ? "CURRENT / " : "") + chatPanel.viewModel.conversationTitles[index]
                                    color: archived && !active ? SentinelTheme.textMuted : SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontSmall
                                    font.bold: active
                                    elide: Text.ElideRight
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: chatPanel.viewModel.conversationActiveSummaries[index]
                                          + " / "
                                          + chatPanel.viewModel.conversationLastUpdatedSummaries[index]
                                          + " / "
                                          + chatPanel.viewModel.conversationMessageCountSummaries[index]
                                          + " / "
                                          + chatPanel.viewModel.conversationArchivedSummaries[index]
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontTiny
                                    elide: Text.ElideRight
                                }
                            }

                            SentinelButton {
                                text: active ? "Open" : "Switch"
                                Layout.preferredWidth: 76
                                enabled: !active
                                onClicked: chatPanel.viewModel.switchConversation(conversationId)
                            }

                            SentinelButton {
                                text: chatPanel.viewModel.conversationArchivedSummaries[index] === "Archived" ? "Unarchive" : "Archive"
                                Layout.preferredWidth: 92
                                onClicked: {
                                    if (chatPanel.viewModel.conversationArchivedSummaries[index] === "Archived")
                                        chatPanel.viewModel.unarchiveConversation(conversationId)
                                    else
                                        chatPanel.viewModel.archiveConversation(conversationId)
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

                GridLayout {
                    Layout.fillWidth: true
                    columns: chatPanel.compact ? 1 : 3
                    columnSpacing: SentinelTheme.spaceSm
                    rowSpacing: SentinelTheme.spaceSm

                    SentinelTextField {
                        id: renameInput
                        Layout.fillWidth: true
                        Layout.columnSpan: chatPanel.compact ? 1 : 2
                        placeholderText: "Rename active conversation"
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
                        Layout.fillWidth: chatPanel.compact
                        onClicked: {
                            var renamed = chatPanel.viewModel.renameConversation(chatPanel.viewModel.activeConversationId, renameInput.text)
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
                    color: chatPanel.renameStatusText === "Rename saved." ? SentinelTheme.success : SentinelTheme.warning
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                }

                InfoRow {
                    compact: true
                    label: "Delete"
                    value: chatPanel.viewModel.conversationDeleteReadinessStatus
                           + " / "
                           + chatPanel.viewModel.conversationDeleteReadinessSummary
                    Layout.fillWidth: true
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: SentinelTheme.radiusMd
            color: SentinelTheme.withAlpha(chatPanel.modeAccent, 0.055)
            border.color: SentinelTheme.withAlpha(chatPanel.modeAccent, 0.12)
            implicitHeight: runtimeStatusColumn.implicitHeight + SentinelTheme.spaceMd

            ColumnLayout {
                id: runtimeStatusColumn
                x: SentinelTheme.spaceSm
                y: SentinelTheme.spaceSm
                width: parent.width - SentinelTheme.spaceSm * 2
                spacing: SentinelTheme.spaceXs

                InfoRow {
                    compact: true
                    label: "Selected"
                    value: chatPanel.viewModel.selectedLocalModelSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: true
                    label: "Runtime"
                    value: chatPanel.runtimeStatusText
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: true
                    label: "Session"
                    value: chatPanel.viewModel.conversationState
                           + " / "
                           + chatPanel.viewModel.conversationRuntimeActiveRoute
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: true
                    label: "History"
                    value: chatPanel.viewModel.conversationPersistenceStatus
                           + " / "
                           + chatPanel.viewModel.conversationHistoryMessageCount
                           + (chatPanel.viewModel.conversationHistoryMessageCount === 1 ? " message" : " messages")
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: true
                    label: "Saved"
                    value: chatPanel.viewModel.conversationLastSavedStatus
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: true
                    label: "Search"
                    value: chatPanel.viewModel.conversationSearchStatus
                           + " / "
                           + chatPanel.viewModel.conversationSearchSummaryText
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: true
                    label: "Export"
                    value: chatPanel.viewModel.conversationExportLastStatus
                           + " / "
                           + chatPanel.viewModel.conversationExportLastResultSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: true
                    label: "Inference"
                    value: chatPanel.viewModel.localInferenceSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: true
                    visible: chatPanel.viewModel.conversationRuntimeRequestId !== "None"
                    label: "Request"
                    value: chatPanel.viewModel.conversationRuntimeRequestId
                    Layout.fillWidth: true
                }

                Label {
                    Layout.fillWidth: true
                    visible: !chatPanel.modelReady
                    text: "Start Ollama and install/select a local model."
                    color: SentinelTheme.warning
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                }

                Label {
                    Layout.fillWidth: true
                    visible: chatPanel.viewModel.activeConversationArchived
                    text: chatPanel.viewModel.activeConversationStateSummary
                    color: SentinelTheme.warning
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                }
            }
        }

        ListView {
            id: messageList
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 160
            clip: true
            spacing: SentinelTheme.spaceLg
            model: chatPanel.viewModel.chatMessages

            delegate: Rectangle {
                id: messageDelegate
                required property string messageRole
                required property string messageStatus
                required property string content

                width: ListView.view.width
                radius: SentinelTheme.radiusLg
                color: messageDelegate.messageRole === "user" ? SentinelTheme.withAlpha(chatPanel.modeAccent, 0.075) : messageDelegate.messageStatus === "error" ? SentinelTheme.errorSurface : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.020)
                border.color: messageDelegate.messageRole === "user" ? SentinelTheme.withAlpha(chatPanel.modeAccent, 0.16) : messageDelegate.messageStatus === "error" ? SentinelTheme.errorBorder : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.035)
                implicitHeight: messageColumn.implicitHeight + chatPanel.cardPadding * 2

                ColumnLayout {
                    id: messageColumn
                    x: chatPanel.cardPadding
                    y: chatPanel.cardPadding
                    width: parent.width - chatPanel.cardPadding * 2
                    spacing: SentinelTheme.spaceXs

                    Text {
                        Layout.fillWidth: true
                        text: messageDelegate.messageRole === "user" ? "YOU" : "SENTINEL"
                        color: messageDelegate.messageRole === "user" ? chatPanel.modeAccent : SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        font.letterSpacing: 2.1
                    }

                    Text {
                        Layout.fillWidth: true
                        text: messageDelegate.content
                        color: SentinelTheme.textPrimary
                        wrapMode: Text.WordWrap
                        font.pixelSize: SentinelTheme.fontBody
                        lineHeight: 1.28
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            visible: chatPanel.viewModel.localInferenceBusy && chatPanel.viewModel.localInferenceStreamingText.length > 0
            radius: SentinelTheme.radiusLg
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.020)
            border.color: SentinelTheme.withAlpha(chatPanel.modeAccent, 0.14)
            implicitHeight: streamingColumn.implicitHeight + chatPanel.cardPadding * 2

            ColumnLayout {
                id: streamingColumn
                x: chatPanel.cardPadding
                y: chatPanel.cardPadding
                width: parent.width - chatPanel.cardPadding * 2
                spacing: SentinelTheme.spaceXs

                Text {
                    Layout.fillWidth: true
                    text: "SENTINEL / STREAMING"
                    color: chatPanel.modeAccent
                    font.pixelSize: SentinelTheme.fontTiny
                    font.letterSpacing: 2.1
                }

                Text {
                    Layout.fillWidth: true
                    text: chatPanel.viewModel.localInferenceStreamingText
                    color: SentinelTheme.textPrimary
                    wrapMode: Text.WordWrap
                    font.pixelSize: SentinelTheme.fontBody
                    lineHeight: 1.28
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: messageList.count === 0
            text: "No chat history yet."
            color: SentinelTheme.textMuted
            horizontalAlignment: Text.AlignHCenter
        }

        GridLayout {
            Layout.fillWidth: true
            columns: chatPanel.compact ? 2 : 3
            columnSpacing: SentinelTheme.spaceMd
            rowSpacing: SentinelTheme.spaceMd

            SentinelTextField {
                id: searchInput
                Layout.fillWidth: true
                Layout.columnSpan: chatPanel.compact ? 2 : 1
                placeholderText: "Search current transcript"
                onAccepted: {
                    if (searchButton.enabled)
                        searchButton.clicked()
                }
            }

            SentinelButton {
                id: searchButton
                text: "Search"
                enabled: searchInput.text.trim().length > 0
                Layout.fillWidth: chatPanel.compact
                onClicked: chatPanel.viewModel.searchConversation(searchInput.text)
            }

            SentinelButton {
                text: "Reset"
                enabled: chatPanel.viewModel.conversationSearchQueryText.length > 0
                Layout.fillWidth: chatPanel.compact
                onClicked: {
                    searchInput.clear()
                    chatPanel.viewModel.clearConversationSearch()
                }
            }

            SentinelButton {
                text: "Export Markdown"
                enabled: chatPanel.viewModel.conversationExportAvailable
                Layout.fillWidth: chatPanel.compact
                onClicked: chatPanel.viewModel.exportTranscript("markdown")
            }

            SentinelButton {
                text: "Export JSON"
                enabled: chatPanel.viewModel.conversationExportAvailable
                Layout.fillWidth: chatPanel.compact
                onClicked: chatPanel.viewModel.exportTranscript("json")
            }

            SentinelTextField {
                id: chatInput
                Layout.fillWidth: true
                Layout.columnSpan: chatPanel.compact ? 2 : 1
                placeholderText: chatPanel.viewModel.activeConversationArchived ? "Unarchive this conversation to send" : chatPanel.modelReady ? "Message Sentinel" : "Local model setup required for Ollama chat"
                enabled: !chatPanel.viewModel.localInferenceBusy && !chatPanel.viewModel.activeConversationArchived
                onAccepted: {
                    if (sendButton.enabled)
                        sendButton.clicked()
                }
            }

            SentinelButton {
                id: sendButton
                text: "Send"
                enabled: chatInput.text.trim().length > 0
                         && !chatPanel.viewModel.localInferenceBusy
                         && !chatPanel.viewModel.activeConversationArchived
                Layout.fillWidth: chatPanel.compact
                onClicked: {
                    chatPanel.viewModel.sendMessage(chatInput.text)
                    chatInput.clear()
                }
            }

            SentinelButton {
                id: clearButton
                text: "Clear"
                enabled: messageList.count > 1
                Layout.fillWidth: chatPanel.compact
                onClicked: clearChatDialog.open()
            }
        }
    }

    Dialog {
        id: clearChatDialog
        title: "Clear chat history?"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent

        Label {
            text: "This clears the visible transcript, cancels active request state, resets live streaming text, and clears persisted local chat history when available. Settings and memory are kept."
            color: SentinelTheme.textPrimary
            wrapMode: Text.WordWrap
            width: 360
        }

        onAccepted: chatPanel.viewModel.clearChat()
    }
}
