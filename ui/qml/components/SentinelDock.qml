pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: dock
    required property var viewModel
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)

    implicitWidth: compact ? 320 : 390
    implicitHeight: compact ? 58 : 62
    radius: SentinelTheme.radiusPill
    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.07)
    border.color: SentinelTheme.withAlpha(modeAccent, 0.14)
    showBrackets: false

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: SentinelTheme.spaceMd
        anchors.rightMargin: SentinelTheme.spaceMd
        anchors.topMargin: 7
        anchors.bottomMargin: 7
        spacing: SentinelTheme.spaceXs

        Rectangle {
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
            Layout.alignment: Qt.AlignVCenter
            radius: 15
            color: SentinelTheme.withAlpha(dock.modeAccent, 0.12)
            border.color: SentinelTheme.withAlpha(dock.modeAccent, 0.24)

            Label {
                anchors.centerIn: parent
                text: "S"
                color: dock.modeAccent
                font.pixelSize: SentinelTheme.fontControl
                font.weight: Font.Light
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            Layout.topMargin: SentinelTheme.spaceSm
            Layout.bottomMargin: SentinelTheme.spaceSm
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
        }

        Repeater {
            model: dock.viewModel.availablePages

            Button {
                id: dockButton
                required property string modelData
                readonly property bool active: dock.viewModel.currentPage === modelData
                readonly property string glyph: modelData === "Dashboard" ? "CORE" : modelData === "Memory" ? "MEM" : "SYS"

                Layout.fillWidth: true
                Layout.preferredHeight: 38
                flat: true
                hoverEnabled: true
                focusPolicy: Qt.StrongFocus
                onClicked: dock.viewModel.currentPage = modelData

                contentItem: Column {
                    spacing: 2

                    Text {
                        width: parent.width
                        text: dockButton.glyph
                        color: dockButton.active ? dock.modeAccent : SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        font.letterSpacing: 1.8
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        width: parent.width
                        text: dockButton.modelData === "Dashboard" ? "Presence" : dockButton.modelData
                        color: dockButton.active ? SentinelTheme.textPrimary : SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.72)
                        font.pixelSize: 9
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }
                }

                background: Rectangle {
                    radius: SentinelTheme.radiusPill
                    color: dockButton.active ? SentinelTheme.withAlpha(dock.modeAccent, 0.10) : dockButton.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.055) : "transparent"
                    border.color: dockButton.activeFocus ? SentinelTheme.focusBorder : dockButton.active || dockButton.hovered ? SentinelTheme.withAlpha(dock.modeAccent, 0.18) : "transparent"

                    Behavior on color {
                        ColorAnimation {
                            duration: SentinelTheme.durationFast
                            easing.type: SentinelTheme.easingStandard
                        }
                    }
                }
            }
        }
    }
}
