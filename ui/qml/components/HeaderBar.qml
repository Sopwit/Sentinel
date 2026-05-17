import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: headerBar
    required property var viewModel
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property int modeButtonWidth: compact ? 190 : 230

    color: "transparent"
    border.color: "transparent"
    showBrackets: false

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
                    Layout.preferredWidth: 6
                    Layout.preferredHeight: 6
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
                    text: headerBar.viewModel.ollamaHealthStatus + " / " + headerBar.viewModel.selectedLocalModelStatus
                    color: SentinelTheme.textPlaceholder
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: SentinelTheme.fontSmall
                    elide: Text.ElideRight
                }
            }

            Button {
                id: modeButton
                Layout.preferredWidth: headerBar.modeButtonWidth
                Layout.preferredHeight: SentinelTheme.controlHeight
                hoverEnabled: true
                focusPolicy: Qt.StrongFocus
                text: headerBar.viewModel.currentModeName
                onClicked: modePopup.open()

                contentItem: RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: SentinelTheme.spaceMd
                    anchors.rightMargin: SentinelTheme.spaceMd
                    spacing: SentinelTheme.spaceSm

                    Label {
                        Layout.fillWidth: true
                        text: modeButton.text
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontSmall
                        elide: Text.ElideRight
                    }

                    Label {
                        text: "\u25be"
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                    }
                }

                background: Rectangle {
                    radius: SentinelTheme.controlHeight / 2
                    color: modeButton.down || modePopup.opened
                           ? SentinelTheme.withAlpha(headerBar.modeAccent, 0.18)
                           : modeButton.hovered
                             ? SentinelTheme.withAlpha(headerBar.modeAccent, 0.12)
                             : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.045)
                    border.color: modeButton.activeFocus ? SentinelTheme.focusBorder
                                                         : SentinelTheme.withAlpha(headerBar.modeAccent, 0.18)
                    border.width: 1
                }

                Popup {
                    id: modePopup
                    y: modeButton.height + SentinelTheme.spaceXs
                    width: headerBar.modeButtonWidth
                    modal: false
                    focus: true
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                    padding: SentinelTheme.spaceXs

                    background: Rectangle {
                        radius: SentinelTheme.radiusLg
                        color: SentinelTheme.backgroundRaised
                        border.color: SentinelTheme.withAlpha(headerBar.modeAccent, 0.22)
                    }

                    contentItem: ColumnLayout {
                        spacing: SentinelTheme.spaceXs

                        Repeater {
                            model: headerBar.viewModel.availableModes

                            Button {
                                id: modeOption
                                required property string modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: SentinelTheme.controlHeight
                                flat: true
                                hoverEnabled: true
                                text: modelData
                                onClicked: {
                                    headerBar.viewModel.setModeByName(modelData)
                                    modePopup.close()
                                }

                                contentItem: Label {
                                    text: modeOption.text
                                    color: modeOption.modelData === headerBar.viewModel.currentModeName
                                           ? SentinelTheme.textPrimary
                                           : SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: modeOption.hovered
                                           ? SentinelTheme.withAlpha(headerBar.modeAccent, 0.10)
                                           : "transparent"
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
