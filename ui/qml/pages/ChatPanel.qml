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
                    label: "Inference"
                    value: chatPanel.viewModel.localInferenceSummary
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
                id: chatInput
                Layout.fillWidth: true
                Layout.columnSpan: chatPanel.compact ? 2 : 1
                placeholderText: chatPanel.modelReady ? "Message Sentinel" : "Local model setup required for Ollama chat"
                enabled: !chatPanel.viewModel.localInferenceBusy
                onAccepted: {
                    if (sendButton.enabled)
                        sendButton.clicked()
                }
            }

            SentinelButton {
                id: sendButton
                text: "Send"
                enabled: chatInput.text.trim().length > 0 && !chatPanel.viewModel.localInferenceBusy
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
            text: "This clears the current transcript and persisted local chat history."
            color: SentinelTheme.textPrimary
            wrapMode: Text.WordWrap
            width: 320
        }

        onAccepted: chatPanel.viewModel.clearChat()
    }
}
