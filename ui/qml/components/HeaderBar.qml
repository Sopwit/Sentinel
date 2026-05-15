import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: headerBar
    required property var viewModel
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)

    color: "transparent"
    border.color: "transparent"
    showBrackets: false

    function modeIndex() {
        return headerBar.viewModel.availableModes.indexOf(headerBar.viewModel.currentModeName)
    }

    GridLayout {
        anchors.fill: parent
        anchors.leftMargin: headerBar.compact ? SentinelTheme.spaceMd : 0
        anchors.rightMargin: headerBar.compact ? SentinelTheme.spaceMd : SentinelTheme.spaceLg
        anchors.topMargin: SentinelTheme.spaceSm
        anchors.bottomMargin: SentinelTheme.spaceSm
        columns: headerBar.compact ? 1 : 2
        columnSpacing: SentinelTheme.spaceLg
        rowSpacing: SentinelTheme.spaceSm

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceXs

            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceSm

                Rectangle {
                    width: 6
                    height: 6
                    radius: 3
                    color: headerBar.modeAccent
                    opacity: 0.9
                }

                Label {
                    text: "SENTINEL / OPERATING LAYER"
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontTiny
                    font.letterSpacing: 2.6
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            Label {
                text: headerBar.viewModel.currentPage === "Dashboard" ? "Good evening, Operator" : headerBar.viewModel.currentPage
                color: SentinelTheme.textPrimary
                font.pixelSize: headerBar.compact ? SentinelTheme.fontTitle : SentinelTheme.fontDisplay
                font.weight: Font.Light
            }

            Label {
                Layout.fillWidth: true
                text: headerBar.viewModel.currentPage === "Dashboard"
                      ? "The system is calm. Local runtime layers are breathing in coherence."
                      : headerBar.viewModel.currentModeName + " - " + SentinelTheme.modeStatusText(headerBar.viewModel.currentModeName)
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontBody
                wrapMode: Text.WordWrap
            }
        }

        RowLayout {
            Layout.alignment: headerBar.compact ? Qt.AlignLeft : Qt.AlignRight | Qt.AlignVCenter
            spacing: SentinelTheme.spaceSm

            Rectangle {
                visible: !headerBar.compact
                Layout.preferredWidth: 236
                Layout.preferredHeight: SentinelTheme.controlHeight
                radius: SentinelTheme.controlHeight / 2
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.045)
                border.color: SentinelTheme.withAlpha(headerBar.modeAccent, 0.08)

                Label {
                    anchors.fill: parent
                    anchors.leftMargin: SentinelTheme.spaceMd
                    anchors.rightMargin: SentinelTheme.spaceMd
                    text: "Local alpha / no network provider"
                    color: SentinelTheme.textPlaceholder
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: SentinelTheme.fontSmall
                    elide: Text.ElideRight
                }
            }

            ComboBox {
                Layout.preferredWidth: headerBar.compact ? 190 : 230
                model: headerBar.viewModel.availableModes
                currentIndex: headerBar.modeIndex()
                onActivated: headerBar.viewModel.setModeByName(currentText)
            }
        }
    }
}
