import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: headerBar
    required property var viewModel
    property bool compact: false

    function modeIndex() {
        return headerBar.viewModel.availableModes.indexOf(headerBar.viewModel.currentModeName)
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: headerBar.compact ? SentinelTheme.spaceMd : SentinelTheme.spaceLg
        columns: headerBar.compact ? 1 : 2
        columnSpacing: SentinelTheme.spaceLg
        rowSpacing: SentinelTheme.spaceSm

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceXs

            Label {
                text: headerBar.viewModel.currentModeName
                color: SentinelTheme.textPrimary
                font.pixelSize: headerBar.compact ? SentinelTheme.fontTitle : SentinelTheme.fontHeader
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: "Desktop shell bridge active. Local-first foundation, no network provider configured."
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontBody
                wrapMode: Text.WordWrap
            }
        }

        ComboBox {
            Layout.preferredWidth: headerBar.compact ? 180 : 230
            model: headerBar.viewModel.availableModes
            currentIndex: headerBar.modeIndex()
            onActivated: headerBar.viewModel.setModeByName(currentText)
        }
    }
}
