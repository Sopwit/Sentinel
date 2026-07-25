import QtQuick
import Sentinel.Desktop

Item {
    id: atmosphere
    property string modeName: "Sentinel"
    property color accentColor: SentinelTheme.modeAccent(modeName)

    Rectangle {
        anchors.fill: parent
        color: SentinelTheme.backgroundBase

        Behavior on color {
            ColorAnimation {
                duration: MotionTokens.normal
                easing.type: MotionTokens.standard
            }
        }
    }

    // Subtle gradient overlay for depth — accent at top, secondary at bottom
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: SentinelTheme.withAlpha(atmosphere.accentColor, 0.030) }
            GradientStop { position: 0.5; color: "transparent" }
            GradientStop { position: 1.0; color: SentinelTheme.withAlpha(SentinelTheme.accentSecondary, 0.020) }
        }
    }

    // Ambient drifting glow orb
    Rectangle {
        id: ambientGlow
        width: parent.width * 0.45
        height: parent.height * 0.45
        radius: width / 2
        color: SentinelTheme.withAlpha(atmosphere.accentColor, 0.030)

        SequentialAnimation on x {
            loops: Animation.Infinite
            NumberAnimation {
                from: -parent.width * 0.1
                to: parent.width * 0.6
                duration: 16000
                easing.type: Easing.InOutSine
            }
            NumberAnimation {
                from: parent.width * 0.6
                to: -parent.width * 0.1
                duration: 16000
                easing.type: Easing.InOutSine
            }
        }

        SequentialAnimation on y {
            loops: Animation.Infinite
            NumberAnimation {
                from: -parent.height * 0.1
                to: parent.height * 0.5
                duration: 22000
                easing.type: Easing.InOutSine
            }
            NumberAnimation {
                from: parent.height * 0.5
                to: -parent.height * 0.1
                duration: 22000
                easing.type: Easing.InOutSine
            }
        }

        SequentialAnimation on opacity {
            loops: Animation.Infinite
            NumberAnimation { from: 0.0; to: 0.6; duration: 6000; easing.type: Easing.InOutSine }
            PauseAnimation { duration: 4000 }
            NumberAnimation { from: 0.6; to: 0.0; duration: 6000; easing.type: Easing.InOutSine }
            PauseAnimation { duration: 4000 }
        }
    }
}
