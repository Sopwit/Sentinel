import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: chatPanel
    required property var viewModel

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        SectionTitle {
            title: "AI Bridge"
            subtitle: chatPanel.viewModel.providerName + " is " + chatPanel.viewModel.providerStatus + ". No API calls or network access."
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            model: chatPanel.viewModel.chatMessages

            delegate: Rectangle {
                id: messageDelegate
                required property string messageRole
                required property string messageStatus
                required property string content

                width: ListView.view.width
                radius: 14
                color: messageDelegate.messageRole === "user" ? "#173331" : messageDelegate.messageStatus === "error" ? "#33191a" : "#102326"
                border.color: messageDelegate.messageRole === "user" ? "#35f2c044" : messageDelegate.messageStatus === "error" ? "#d66b6b66" : "#7be8c733"
                implicitHeight: messageText.implicitHeight + 22

                Text {
                    id: messageText
                    anchors.fill: parent
                    anchors.margins: 11
                    text: (messageDelegate.messageRole === "user" ? "You" : "Sentinel") + ": " + messageDelegate.content
                    color: "#d9fff4"
                    wrapMode: Text.WordWrap
                    font.pixelSize: 13
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: chatInput
                Layout.fillWidth: true
                placeholderText: "Send a local test prompt"
                color: "#d9fff4"
                placeholderTextColor: "#648c83"
                onAccepted: sendButton.clicked()

                background: Rectangle {
                    radius: 14
                    color: "#081719"
                    border.color: "#35f2c044"
                }
            }

            Button {
                id: sendButton
                text: "Send"
                enabled: chatInput.text.trim().length > 0
                onClicked: {
                    chatPanel.viewModel.sendMessage(chatInput.text)
                    chatInput.clear()
                }
            }

            Button {
                text: "Clear"
                onClicked: chatPanel.viewModel.clearChat()
            }
        }
    }
}
