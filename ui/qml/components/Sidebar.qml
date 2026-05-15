pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: sidebar
    required property var viewModel
    property bool compact: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: sidebar.compact ? SentinelTheme.spaceMd : SentinelTheme.spaceLg
        spacing: sidebar.compact ? SentinelTheme.spaceMd : SentinelTheme.spaceLg

        ColumnLayout {
            spacing: SentinelTheme.spaceXs

            Label {
                text: "SENTINEL"
                color: SentinelTheme.textPrimary
                font.pixelSize: sidebar.compact ? SentinelTheme.fontTitle : SentinelTheme.fontBrand
                font.bold: true
                font.letterSpacing: sidebar.compact ? 2 : 4
            }

            Label {
                text: sidebar.viewModel.configurationProfile
                color: SentinelTheme.accent
                font.pixelSize: SentinelTheme.fontSmall
                font.letterSpacing: 1.2
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: SentinelTheme.accentBorder
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
                hoverEnabled: true
                focusPolicy: Qt.StrongFocus
                onClicked: sidebar.viewModel.setCurrentPage(navButton.modelData)

                contentItem: Text {
                    text: navButton.text
                    color: navButton.highlighted ? SentinelTheme.textOnAccent : SentinelTheme.textPrimary
                    font.pixelSize: SentinelTheme.fontControl
                    font.bold: navButton.highlighted || navButton.hovered || navButton.activeFocus
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight

                    Behavior on color {
                        ColorAnimation {
                            duration: SentinelTheme.durationFast
                            easing.type: SentinelTheme.easingStandard
                        }
                    }
                }

                background: Rectangle {
                    radius: SentinelTheme.radiusMd
                    color: navButton.highlighted ? SentinelTheme.accent : navButton.hovered ? SentinelTheme.surfaceHover : SentinelTheme.surfaceSoft
                    border.color: navButton.activeFocus ? SentinelTheme.focusBorder : navButton.highlighted || navButton.hovered ? SentinelTheme.accentBorder : SentinelTheme.accentBorderSubtle

                    Behavior on color {
                        ColorAnimation {
                            duration: SentinelTheme.durationFast
                            easing.type: SentinelTheme.easingStandard
                        }
                    }

                    Behavior on border.color {
                        ColorAnimation {
                            duration: SentinelTheme.durationFast
                            easing.type: SentinelTheme.easingStandard
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            Label {
                text: "Provider"
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.1
            }

            Label {
                text: sidebar.viewModel.providerName
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontControl
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: "Status: " + sidebar.viewModel.providerStatus
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: "Theme: " + sidebar.viewModel.themeName
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }
}
