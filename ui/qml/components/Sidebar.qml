pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: sidebar
    required property var viewModel

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceLg

        ColumnLayout {
            spacing: SentinelTheme.spaceXs

            Label {
                text: "SENTINEL"
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBrand
                font.bold: true
                font.letterSpacing: 4
            }

            Label {
                text: sidebar.viewModel.configurationProfile
                color: SentinelTheme.accent
                font.pixelSize: SentinelTheme.fontSmall
                font.letterSpacing: 1.2
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
                onClicked: sidebar.viewModel.setCurrentPage(navButton.modelData)

                contentItem: Text {
                    text: navButton.text
                    color: navButton.highlighted ? SentinelTheme.textOnAccent : SentinelTheme.textPrimary
                    font.pixelSize: SentinelTheme.fontControl
                    font.bold: navButton.highlighted
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: SentinelTheme.radiusMd
                    color: navButton.highlighted ? SentinelTheme.accent : SentinelTheme.surfaceSoft
                    border.color: navButton.highlighted ? SentinelTheme.accent : SentinelTheme.accentBorderSubtle
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
            }

            Label {
                text: "Status: " + sidebar.viewModel.providerStatus
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
            }

            Label {
                text: "Theme: " + sidebar.viewModel.themeName
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
            }
        }
    }
}
