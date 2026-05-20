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
        opacity: pulse.active ? 0.58 : 0.24
    }

    Rectangle {
        anchors.centerIn: parent
        width: pulse.size
        height: width
        radius: width / 2
        color: SentinelTheme.withAlpha(pulse.accent, pulse.muted ? 0.48 : 0.92)
        opacity: pulse.muted ? 0.45 : 0.9
    }

    SequentialAnimation {
        loops: Animation.Infinite
        running: pulse.visible && pulse.active

        ParallelAnimation {
            NumberAnimation {
                target: ring
                property: "scale"
                from: 0.88
                to: 1.16
                duration: SentinelTheme.durationAmbient
                easing.type: Easing.InOutSine
            }
            NumberAnimation {
                target: ring
                property: "opacity"
                from: 0.34
                to: 0.72
                duration: SentinelTheme.durationAmbient
                easing.type: Easing.InOutSine
            }
        }
        ParallelAnimation {
            NumberAnimation {
                target: ring
                property: "scale"
                to: 0.88
                duration: SentinelTheme.durationAmbient
                easing.type: Easing.InOutSine
            }
            NumberAnimation {
                target: ring
                property: "opacity"
                to: 0.34
                duration: SentinelTheme.durationAmbient
                easing.type: Easing.InOutSine
            }
        }
    }
}
