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
}
