import QtQuick

Item {
    id: pulse

    property color accent: SentinelTheme.accent
    property bool active: false
    property bool muted: false
    property int size: 8

    implicitWidth: size * 2
    implicitHeight: size * 2

    Rectangle {
        id: ring
        anchors.centerIn: parent
        width: pulse.size * 2
        height: width
        radius: width / 2
        color: "transparent"
        border.width: 1
        border.color: SentinelTheme.withAlpha(pulse.accent, pulse.muted ? 0.12 : 0.24)
        opacity: pulse.active ? 0.62 : 0.24
        scale: pulse.active ? 1.10 : 0.92

        Behavior on opacity {
            NumberAnimation {
                duration: MotionTokens.normal
                easing.type: MotionTokens.standard
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: MotionTokens.normal
                easing.type: MotionTokens.enter
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: pulse.size
        height: width
        radius: width / 2
        color: SentinelTheme.withAlpha(pulse.accent, pulse.muted ? 0.48 : 0.92)
        opacity: pulse.muted ? 0.45 : 0.9
    }
}
