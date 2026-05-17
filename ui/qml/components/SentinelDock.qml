pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: dock
    required property var viewModel
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property var dockPages: ["Memory", "Dashboard", "Agents"]
    readonly property int currentPageIndex: dock.dockPages.indexOf(
                                                dock.viewModel.currentPage)

    function pageIcon(pageName) {
        if (pageName === "Dashboard")
            return "\u25ce"
        if (pageName === "Memory")
            return "\u25a6"
        if (pageName === "Agents")
            return "\u25c7"
        return "\u2699"
    }

    function pageLabel(pageName) {
        if (pageName === "Dashboard")
            return "Home"
        if (pageName === "Memory")
            return "Runtime/Memory"
        return pageName
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

        Item {
            id: tabsHost
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                id: activeTrack
                visible: dock.currentPageIndex >= 0
                readonly property int pageCount: Math.max(1, dock.dockPages.length)
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
                    model: dock.dockPages

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

                        contentItem: Text {
                            anchors.centerIn: parent
                            text: dock.compact ? dock.pageIcon(dockButton.modelData) : dock.pageIcon(dockButton.modelData) + "  " + dock.pageLabel(dockButton.modelData)
                            color: dockButton.active ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                            font.pixelSize: dock.compact ? 16 : SentinelTheme.fontSmall
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
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

    }
}
