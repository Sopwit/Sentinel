pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: sidebar
    required property var viewModel

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
                text: sidebar.viewModel.configurationProfile
                color: "#35f2c0"
                font.pixelSize: 12
                font.letterSpacing: 1.2
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#35f2c044"
        }

        Repeater {
            model: sidebar.viewModel.availablePages

            Button {
                id: navButton
                required property string modelData

                Layout.fillWidth: true
                text: modelData
                flat: true
                highlighted: sidebar.viewModel.currentPage === navButton.modelData
                onClicked: sidebar.viewModel.setCurrentPage(navButton.modelData)

                contentItem: Text {
                    text: navButton.text
                    color: navButton.highlighted ? "#06110f" : "#d9fff4"
                    font.pixelSize: 14
                    font.bold: navButton.highlighted
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 14
                    color: navButton.highlighted ? "#35f2c0" : "#10232688"
                    border.color: navButton.highlighted ? "#35f2c0" : "#35f2c022"
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
                text: sidebar.viewModel.providerName
                color: "#d9fff4"
                font.pixelSize: 14
            }

            Label {
                text: "Status: " + sidebar.viewModel.providerStatus
                color: "#82aaa1"
                font.pixelSize: 12
            }

            Label {
                text: "Theme: " + sidebar.viewModel.themeName
                color: "#82aaa1"
                font.pixelSize: 12
            }
        }
    }
}
