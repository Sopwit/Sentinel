import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: statusChip

    property string label: ""
    property string value: ""
    property color accent: SentinelTheme.accent
    property bool muted: false
    property bool active: false
    property bool selected: false

    radius: SentinelTheme.radiusPill
    color: SentinelTheme.withAlpha(statusChip.accent,
                                   statusChip.selected ? 0.145 : statusChip.muted ? 0.035 : 0.075)
    border.color: SentinelTheme.withAlpha(statusChip.accent,
                                          statusChip.selected ? 0.48 : statusChip.muted ? 0.12 : 0.28)
    implicitWidth: chipText.implicitWidth + SentinelTheme.spaceMd * 2 + 14
    implicitHeight: 30
    scale: hoverArea.containsMouse ? 1.018 : 1.0

    Behavior on color {
        ColorAnimation {
            duration: SentinelTheme.durationNormal
            easing.type: SentinelTheme.easingStandard
        }
    }

    Behavior on border.color {
        ColorAnimation {
            duration: SentinelTheme.durationNormal
            easing.type: SentinelTheme.easingStandard
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: SentinelTheme.durationFast
            easing.type: SentinelTheme.easingEmphasized
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        radius: parent.radius
        color: "transparent"
        border.width: 1
        border.color: SentinelTheme.withAlpha(statusChip.accent,
                                              hoverArea.containsMouse || statusChip.selected ? 0.20 : 0.0)
        opacity: statusChip.muted ? 0.45 : 1.0

        Behavior on border.color {
            ColorAnimation {
                duration: SentinelTheme.durationNormal
                easing.type: SentinelTheme.easingStandard
            }
        }
    }

    Text {
        id: chipText
        anchors.centerIn: parent
        width: Math.min(implicitWidth, statusChip.width - SentinelTheme.spaceMd * 2 - 12)
        text: statusChip.label.length > 0 ? statusChip.label + ": " + statusChip.value
                                          : statusChip.value
        color: statusChip.muted ? SentinelTheme.textMuted : SentinelTheme.textPrimary
        font.pixelSize: SentinelTheme.fontSmall
        elide: Text.ElideRight
    }

    StatusPulse {
        visible: statusChip.active
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: SentinelTheme.spaceSm
        size: 6
        accent: statusChip.accent
        active: statusChip.active
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
    }
}
