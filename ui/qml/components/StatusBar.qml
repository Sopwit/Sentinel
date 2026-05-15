import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: statusBar
    required property var viewModel

    radius: 16

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 14

        Label {
            text: "Status: Local Alpha"
            color: "#d9fff4"
            font.pixelSize: 12
        }

        Label {
            text: "Mode: " + statusBar.viewModel.currentModeName
            color: "#82aaa1"
            font.pixelSize: 12
        }

        Label {
            text: "Memory: " + statusBar.viewModel.memoryStatus
            color: "#82aaa1"
            font.pixelSize: 12
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            text: "Provider: " + statusBar.viewModel.providerName + " / " + statusBar.viewModel.providerStatus
            color: "#82aaa1"
            font.pixelSize: 12
        }
    }
}
