import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: chatPanel
    required property var viewModel
    property bool compact: width < 520
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)

    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.038)
    border.color: SentinelTheme.withAlpha(modeAccent, 0.095)
    bracketColor: SentinelTheme.withAlpha(modeAccent, 0.22)
    bracketSize: 12

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: chatPanel.compact ? SentinelTheme.spaceMd : SentinelTheme.space2Xl
        spacing: SentinelTheme.spaceLg

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            Rectangle {
                width: 6
                height: 6
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
                    text: chatPanel.viewModel.providerName + " / " + chatPanel.viewModel.providerStatus + " / " + chatPanel.viewModel.chatHistoryStatus
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    elide: Text.ElideRight
                }
            }
        }

        ListView {
            id: messageList
            Layout.fillWidth: true
            Layout.fillHeight: true
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
                implicitHeight: messageColumn.implicitHeight + 26

                ColumnLayout {
                    id: messageColumn
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.spaceMd
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
                placeholderText: "Speak with Sentinel"
                onAccepted: sendButton.clicked()
            }

            SentinelButton {
                id: sendButton
                text: "Send"
                enabled: chatInput.text.trim().length > 0
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
