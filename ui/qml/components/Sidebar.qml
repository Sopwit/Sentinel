import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 18

        ColumnLayout {
            spacing: 3

            Label {
                text: "SENTINEL"
                color: "#d9fff4"
                font.pixelSize: 24
                font.bold: true
                font.letterSpacing: 4
            }

            Label {
                text: shellViewModel.configurationProfile
                color: "#35f2c0"
                font.pixelSize: 12
                font.letterSpacing: 1.2
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#35f2c044"
        }

        Repeater {
            model: shellViewModel.availablePages

            Button {
                Layout.fillWidth: true
                text: modelData
                flat: true
                highlighted: shellViewModel.currentPage === modelData
                onClicked: shellViewModel.setCurrentPage(modelData)

                contentItem: Text {
                    text: parent.text
                    color: parent.highlighted ? "#06110f" : "#d9fff4"
                    font.pixelSize: 14
                    font.bold: parent.highlighted
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 14
                    color: parent.highlighted ? "#35f2c0" : "#10232688"
                    border.color: parent.highlighted ? "#35f2c0" : "#35f2c022"
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "Provider"
                color: "#82aaa1"
                font.pixelSize: 11
                font.letterSpacing: 1.1
            }

            Label {
                text: shellViewModel.providerName
                color: "#d9fff4"
                font.pixelSize: 14
            }

            Label {
                text: "Theme: " + shellViewModel.themeName
                color: "#82aaa1"
                font.pixelSize: 12
            }
        }
    }
}
