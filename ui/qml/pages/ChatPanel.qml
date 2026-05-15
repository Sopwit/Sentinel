import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        SectionTitle {
            title: "AI Bridge"
            subtitle: shellViewModel.providerName + " is " + shellViewModel.providerStatus + ". No API calls or network access."
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            model: shellViewModel.chatMessages

            delegate: Rectangle {
                width: ListView.view.width
                radius: 14
                color: modelData.indexOf("You:") === 0 ? "#173331" : "#102326"
                border.color: modelData.indexOf("You:") === 0 ? "#35f2c044" : "#7be8c733"
                implicitHeight: messageText.implicitHeight + 22

                Text {
                    id: messageText
                    anchors.fill: parent
                    anchors.margins: 11
                    text: modelData
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
                    shellViewModel.sendMessage(chatInput.text)
                    chatInput.clear()
                }
            }
        }
    }
}
