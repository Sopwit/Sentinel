import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: badge

    property string label: ""
    property string value: ""
    property color accent: SentinelTheme.accent
    property bool active: false
    property bool muted: false

    radius: SentinelTheme.radiusMd
    color: SentinelTheme.withAlpha(badge.accent, badge.active ? 0.105 : badge.muted ? 0.030 : 0.055)
    border.color: SentinelTheme.withAlpha(badge.accent, badge.active ? 0.34 : badge.muted ? 0.10 : 0.18)
    implicitWidth: Math.max(132, content.implicitWidth + SentinelTheme.spaceMd * 2)
    implicitHeight: 44
    opacity: badge.muted ? 0.74 : 1.0

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

    RowLayout {
        id: content
        anchors.fill: parent
        anchors.leftMargin: SentinelTheme.spaceMd
        anchors.rightMargin: SentinelTheme.spaceMd
        spacing: SentinelTheme.spaceSm

        StatusPulse {
            Layout.preferredWidth: 14
            Layout.preferredHeight: 14
            size: 7
            accent: badge.accent
            active: badge.active
            muted: badge.muted
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
                Layout.fillWidth: true
                text: badge.label
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.4
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                text: badge.value
                color: badge.muted ? SentinelTheme.textMuted : SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
            }
        }
    }
}
