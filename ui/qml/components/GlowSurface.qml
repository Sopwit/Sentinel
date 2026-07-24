import QtQuick
import Sentinel.Desktop

Item {
    id: glow

    property color accent: SentinelTheme.accent
    property color secondaryAccent: SentinelTheme.accentTertiary
    property bool active: false
    property real glowScale: 1.0
    property bool reducedMotion: false

    implicitWidth: 420
    implicitHeight: 420

    Rectangle {
        id: outerGlow
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height) * 0.98
        height: width
        radius: width / 2
        color: SentinelTheme.withAlpha(glow.accent, (glow.active ? 0.060 : 0.028) * glow.glowScale)
        border.color: SentinelTheme.withAlpha(glow.accent, glow.active ? 0.050 : 0.026)
        scale: glow.active ? 1.018 : 1.0

        Behavior on color {
            ColorAnimation {
                duration: MotionTokens.normal
                easing.type: MotionTokens.standard
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: reducedMotion ? 0 : MotionTokens.duration(MotionTokens.slow, "")
                easing.type: MotionTokens.enter
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height) * 0.74
        height: width
        radius: width / 2
        color: SentinelTheme.withAlpha(glow.secondaryAccent, (glow.active ? 0.052 : 0.024) * glow.glowScale)
        border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.038)
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height) * 0.46
        height: width
        radius: width / 2
        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, glow.active ? 0.030 : 0.018)
        border.color: SentinelTheme.withAlpha(glow.accent, 0.10)
    }
}
