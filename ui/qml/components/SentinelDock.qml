pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic

ShellPanel {
    id: dock
    required property var viewModel
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property var dockPages: ["Memory", "Dashboard", "Agents"]
    readonly property int horizontalPadding: compact ? SentinelTheme.spaceSm : SentinelTheme.spaceMd
    readonly property int verticalPadding: compact ? SentinelTheme.spaceXs : SentinelTheme.spaceSm
    readonly property int itemSpacing: compact ? SentinelTheme.spaceXs : SentinelTheme.spaceSm
    readonly property int itemWidth: Math.floor((width - horizontalPadding * 2 - itemSpacing * 2) / 3)
    readonly property int itemHeight: height - verticalPadding * 2

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
    implicitHeight: compact ? 58 : 62
    radius: SentinelTheme.radiusPill
    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.82)
    border.color: SentinelTheme.withAlpha(modeAccent, 0.20)
    showBrackets: false
    clip: true

    Row {
        id: dockItems
        anchors.centerIn: parent
        width: dock.itemWidth * dock.dockPages.length
               + dock.itemSpacing * (dock.dockPages.length - 1)
        height: dock.itemHeight
        spacing: dock.itemSpacing

        Repeater {
            model: dock.dockPages

            Button {
                id: dockButton
                required property string modelData
                readonly property bool active: dock.viewModel.currentPage === modelData

                width: dock.itemWidth
                height: dock.itemHeight
                flat: true
                hoverEnabled: true
                focusPolicy: Qt.StrongFocus
                onClicked: dock.viewModel.currentPage = modelData

                contentItem: Item {
                    Row {
                        anchors.centerIn: parent
                        spacing: dock.compact ? 0 : 6
                        height: parent.height

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: dock.pageIcon(dockButton.modelData)
                            color: dockButton.active ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                            font.pixelSize: dock.compact ? 16 : SentinelTheme.fontSmall
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            visible: !dock.compact
                            text: dock.pageLabel(dockButton.modelData)
                            color: dockButton.active ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }
                    }
                }

                background: Rectangle {
                    radius: SentinelTheme.radiusPill
                    color: dockButton.active
                           ? SentinelTheme.withAlpha(dock.modeAccent, 0.14)
                           : dockButton.hovered
                             ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
                             : "transparent"
                    border.color: dockButton.activeFocus ? SentinelTheme.focusBorder
                                                         : dockButton.active
                                                           ? SentinelTheme.withAlpha(dock.modeAccent, 0.30)
                                                           : "transparent"
                    border.width: 1
                }
            }
        }
    }
}
