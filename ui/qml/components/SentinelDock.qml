pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: dock
    required property var viewModel
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property int currentPageIndex: dock.viewModel.availablePages.indexOf(
                                                dock.viewModel.currentPage)

    function shortCode(pageName) {
        if (pageName === "Dashboard")
            return "CORE"
        if (pageName === "Memory")
            return "MEM"
        return "SYS"
    }

    implicitWidth: compact ? 320 : 440
    implicitHeight: compact ? 60 : 66
    radius: SentinelTheme.radiusPill
    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.82)
    border.color: SentinelTheme.withAlpha(modeAccent, 0.20)
    showBrackets: false
    clip: true

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: SentinelTheme.spaceMd
        anchors.rightMargin: SentinelTheme.spaceMd
        anchors.topMargin: 8
        anchors.bottomMargin: 8
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

        Item {
            id: tabsHost
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                id: activeTrack
                visible: dock.currentPageIndex >= 0
                readonly property int pageCount: Math.max(1, dock.viewModel.availablePages.length)
                readonly property real tabWidth: tabsHost.width / pageCount
                readonly property real tabPadding: compact ? 4 : 5
                x: tabWidth * Math.max(0, dock.currentPageIndex) + tabPadding
                y: 0
                width: tabWidth - tabPadding * 2
                height: tabsHost.height
                radius: SentinelTheme.radiusPill
                color: SentinelTheme.withAlpha(dock.modeAccent, 0.14)
                border.color: SentinelTheme.withAlpha(dock.modeAccent, 0.30)

                Behavior on x {
                    NumberAnimation {
                        duration: SentinelTheme.durationNormal
                        easing.type: SentinelTheme.easingEmphasized
                    }
                }
            }

            RowLayout {
                anchors.fill: parent
                spacing: compact ? 4 : 6

                Repeater {
                    model: dock.viewModel.availablePages

                    Button {
                        id: dockButton
                        required property string modelData
                        readonly property bool active: dock.viewModel.currentPage === modelData

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        flat: true
                        hoverEnabled: true
                        focusPolicy: Qt.StrongFocus
                        onClicked: dock.viewModel.currentPage = modelData

                        contentItem: Column {
                            anchors.centerIn: parent
                            spacing: 1

                            Text {
                                width: parent.width
                                text: dock.shortCode(dockButton.modelData)
                                color: dockButton.active ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontTiny
                                font.letterSpacing: 1.7
                                horizontalAlignment: Text.AlignHCenter
                            }

                            Text {
                                width: parent.width
                                text: dockButton.modelData === "Dashboard" ? "Presence"
                                                                          : dockButton.modelData
                                color: dockButton.active ? SentinelTheme.withAlpha(
                                                               SentinelTheme.textPrimary, 0.92)
                                                         : SentinelTheme.withAlpha(
                                                               SentinelTheme.textMuted, 0.72)
                                font.pixelSize: 9
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                            }
                        }

                        background: Rectangle {
                            radius: SentinelTheme.radiusPill
                            color: dockButton.hovered && !dockButton.active
                                   ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
                                   : "transparent"
                            border.color: dockButton.activeFocus ? SentinelTheme.focusBorder
                                                                 : "transparent"
                        }
                    }
                }
            }
        }

        Label {
            visible: !dock.compact
            text: dock.viewModel.currentModeName
            color: SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.84)
            font.pixelSize: SentinelTheme.fontTiny
            font.letterSpacing: 1.2
            elide: Text.ElideRight
            Layout.maximumWidth: 110
        }
    }
}
