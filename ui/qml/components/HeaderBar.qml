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
        anchors.margins: SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceLg

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceXs

            Label {
                text: headerBar.viewModel.currentModeName
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontHeader
                font.bold: true
            }

            Label {
                text: "Desktop shell bridge active. Local-first foundation, no network provider configured."
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontBody
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
