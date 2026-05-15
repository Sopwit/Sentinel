import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: headerBar
    required property var viewModel

    function modeIndex() {
        return headerBar.viewModel.availableModes.indexOf(headerBar.viewModel.currentModeName)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 18

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: headerBar.viewModel.currentModeName
                color: "#d9fff4"
                font.pixelSize: 27
                font.bold: true
            }

            Label {
                text: "Desktop shell bridge active. Local-first foundation, no network provider configured."
                color: "#82aaa1"
                font.pixelSize: 13
            }
        }

        ComboBox {
            Layout.preferredWidth: 230
            model: headerBar.viewModel.availableModes
            currentIndex: headerBar.modeIndex()
            onActivated: headerBar.viewModel.setModeByName(currentText)
        }
    }
}
