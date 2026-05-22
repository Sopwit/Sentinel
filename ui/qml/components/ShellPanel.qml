import QtQuick

Rectangle {
    id: panel

    radius: SentinelTheme.radiusPanel
    color: SentinelTheme.panel
    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.070)
    border.width: 1

    property color bracketColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.0)
    property bool showBrackets: false
    property int bracketSize: 12
    property color edgeLightColor: bracketColor
    property real edgeLightOpacity: 0.0

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: SentinelTheme.spaceLg
        anchors.rightMargin: SentinelTheme.spaceLg
        height: 1
        color: panel.edgeLightColor
        opacity: panel.edgeLightOpacity
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: SentinelTheme.space2Xl
        anchors.rightMargin: SentinelTheme.space2Xl
        height: 1
        color: panel.edgeLightColor
        opacity: panel.edgeLightOpacity * 0.46
    }

    Rectangle {
        visible: false
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
        visible: false
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
        visible: false
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
        visible: false
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
        visible: false
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
        visible: false
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
        visible: false
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
        visible: false
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
