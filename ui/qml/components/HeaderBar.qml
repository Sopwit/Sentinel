import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    function modeIndex() {
        return shellViewModel.availableModes.indexOf(shellViewModel.currentModeName)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 18

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: shellViewModel.currentModeName
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
            model: shellViewModel.availableModes
            currentIndex: modeIndex()
            onActivated: shellViewModel.setModeByName(currentText)
        }
    }
}
