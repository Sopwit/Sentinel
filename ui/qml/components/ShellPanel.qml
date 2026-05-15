import QtQuick

Rectangle {
    id: panel

    radius: SentinelTheme.radiusPanel
    color: SentinelTheme.panel
    border.color: SentinelTheme.accentBorderSubtle
    border.width: 1

    property color bracketColor: SentinelTheme.accentBorder
    property bool showBrackets: true
    property int bracketSize: 12

    Rectangle {
        visible: panel.showBrackets
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: SentinelTheme.spaceSm
        anchors.topMargin: SentinelTheme.spaceSm
        width: panel.bracketSize
        height: 1
        color: panel.bracketColor
        z: 20
    }

    Rectangle {
        visible: panel.showBrackets
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: SentinelTheme.spaceSm
        anchors.topMargin: SentinelTheme.spaceSm
        width: 1
        height: panel.bracketSize
        color: panel.bracketColor
        z: 20
    }

    Rectangle {
        visible: panel.showBrackets
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: SentinelTheme.spaceSm
        anchors.topMargin: SentinelTheme.spaceSm
        width: panel.bracketSize
        height: 1
        color: panel.bracketColor
        z: 20
    }

    Rectangle {
        visible: panel.showBrackets
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: SentinelTheme.spaceSm
        anchors.topMargin: SentinelTheme.spaceSm
        width: 1
        height: panel.bracketSize
        color: panel.bracketColor
        z: 20
    }

    Rectangle {
        visible: panel.showBrackets
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: SentinelTheme.spaceSm
        anchors.bottomMargin: SentinelTheme.spaceSm
        width: panel.bracketSize
        height: 1
        color: panel.bracketColor
        z: 20
    }

    Rectangle {
        visible: panel.showBrackets
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: SentinelTheme.spaceSm
        anchors.bottomMargin: SentinelTheme.spaceSm
        width: 1
        height: panel.bracketSize
        color: panel.bracketColor
        z: 20
    }

    Rectangle {
        visible: panel.showBrackets
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: SentinelTheme.spaceSm
        anchors.bottomMargin: SentinelTheme.spaceSm
        width: panel.bracketSize
        height: 1
        color: panel.bracketColor
        z: 20
    }

    Rectangle {
        visible: panel.showBrackets
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: SentinelTheme.spaceSm
        anchors.bottomMargin: SentinelTheme.spaceSm
        width: 1
        height: panel.bracketSize
        color: panel.bracketColor
        z: 20
    }
}
