import QtQuick

Item {
    id: glow

    property color accent: SentinelTheme.accent
    property color secondaryAccent: SentinelTheme.accentTertiary
    property bool active: false
    property real glowScale: 1.0

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

    SequentialAnimation {
        loops: Animation.Infinite
        running: glow.visible

        NumberAnimation {
            target: outerGlow
            property: "scale"
            from: 0.985
            to: glow.active ? 1.030 : 1.012
            duration: SentinelTheme.durationAmbient
            easing.type: Easing.InOutSine
        }
        NumberAnimation {
            target: outerGlow
            property: "scale"
            to: 0.985
            duration: SentinelTheme.durationAmbient
            easing.type: Easing.InOutSine
        }
    }
}
