import QtQuick

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
                                   statusChip.selected ? InteractionTokens.selectedOpacity
                                                       : statusChip.muted ? 0.026 : 0.052)
    border.color: SentinelTheme.withAlpha(statusChip.selected ? statusChip.accent : SentinelTheme.textPrimary,
                                          statusChip.selected ? 0.20 : statusChip.muted ? 0.040 : 0.070)
    implicitWidth: Math.min(Math.max(chipText.implicitWidth + SentinelTheme.spaceMd * 2 + 10, 88), 560)
    implicitHeight: 28
    scale: hoverArea.containsMouse && !statusChip.muted ? InteractionTokens.cardHoverLift : 1.0

    Behavior on color {
        ColorAnimation {
            duration: MotionTokens.normal
            easing.type: MotionTokens.standard
        }
    }

    Behavior on border.color {
        ColorAnimation {
            duration: MotionTokens.normal
            easing.type: MotionTokens.standard
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: MotionTokens.fast
            easing.type: MotionTokens.enter
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        radius: parent.radius
        color: "transparent"
        border.width: 1
        border.color: "transparent"
        opacity: statusChip.muted ? 0.45 : 1.0

        Behavior on border.color {
            ColorAnimation {
                duration: MotionTokens.normal
                easing.type: MotionTokens.standard
            }
        }
    }

    Text {
        id: chipText
        anchors.centerIn: parent
        width: Math.min(implicitWidth, Math.max(0, statusChip.width - SentinelTheme.spaceMd * 2 - 12))
        text: statusChip.label.length > 0 ? statusChip.label + ": " + statusChip.value
                                          : statusChip.value
        color: statusChip.muted ? SentinelTheme.textMuted : SentinelTheme.textPrimary
        font.pixelSize: SentinelTheme.fontSmall
        wrapMode: Text.NoWrap
        maximumLineCount: 1
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
