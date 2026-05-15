import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: memoryPage
    required property var viewModel

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        spacing: 14

        SectionTitle {
            title: "Runtime Memory"
            subtitle: "Local key-value memory with dedicated persistence. Settings and chat history remain separate."
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            TextField {
                id: memoryKey
                Layout.preferredWidth: 220
                placeholderText: "key"
                color: "#d9fff4"
                placeholderTextColor: "#648c83"
            }

            TextField {
                id: memoryValue
                Layout.fillWidth: true
                placeholderText: "value"
                color: "#d9fff4"
                placeholderTextColor: "#648c83"
            }

            Button {
                text: "Store"
                enabled: memoryKey.text.trim().length > 0
                onClicked: {
                    memoryPage.viewModel.remember(memoryKey.text, memoryValue.text)
                    memoryKey.clear()
                    memoryValue.clear()
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: memoryPage.viewModel.memoryEntries

            delegate: Rectangle {
                id: memoryDelegate
                required property string modelData

                width: ListView.view.width
                radius: 12
                color: "#102326aa"
                border.color: "#35f2c022"
                implicitHeight: memoryText.implicitHeight + 18

                Text {
                    id: memoryText
                    anchors.fill: parent
                    anchors.margins: 9
                    text: memoryDelegate.modelData
                    color: "#d9fff4"
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
