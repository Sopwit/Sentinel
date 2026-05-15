import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: chatPanel
    required property var viewModel
    property bool compact: width < 520

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: chatPanel.compact ? SentinelTheme.spaceMd : SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceSm

        SectionTitle {
            title: "AI Bridge"
            subtitle: chatPanel.viewModel.providerName + " is " + chatPanel.viewModel.providerStatus + ". Chat history: " + chatPanel.viewModel.chatHistoryStatus + "."
        }

        ListView {
            id: messageList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: SentinelTheme.spaceSm
            model: chatPanel.viewModel.chatMessages

            delegate: Rectangle {
                id: messageDelegate
                required property string messageRole
                required property string messageStatus
                required property string content

                width: ListView.view.width
                radius: SentinelTheme.radiusMd
                color: messageDelegate.messageRole === "user" ? SentinelTheme.userMessageSurface : messageDelegate.messageStatus === "error" ? SentinelTheme.errorSurface : SentinelTheme.surface
                border.color: messageDelegate.messageRole === "user" ? SentinelTheme.accentBorder : messageDelegate.messageStatus === "error" ? SentinelTheme.errorBorder : SentinelTheme.successBorder
                implicitHeight: messageText.implicitHeight + 22

                Text {
                    id: messageText
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.spaceSm
                    text: (messageDelegate.messageRole === "user" ? "You" : "Sentinel") + ": " + messageDelegate.content
                    color: SentinelTheme.textPrimary
                    wrapMode: Text.WordWrap
                    font.pixelSize: SentinelTheme.fontBody
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
            columnSpacing: SentinelTheme.spaceSm
            rowSpacing: SentinelTheme.spaceSm

            SentinelTextField {
                id: chatInput
                Layout.fillWidth: true
                Layout.columnSpan: chatPanel.compact ? 2 : 1
                placeholderText: "Send a local test prompt"
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
