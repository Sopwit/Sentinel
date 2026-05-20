import QtQuick
import QtQuick.Controls.Basic

Item {
    id: orb
    required property var viewModel
    property color accent: SentinelTheme.modeAccent(viewModel.currentModeName)
    property color secondaryAccent: SentinelTheme.modeSecondaryAccent(viewModel.currentModeName)
    property real glowScale: SentinelTheme.modeGlowScale(viewModel.currentModeName)
    property bool compact: width < 360
    property bool active: false
    readonly property real safeSize: Math.max(1, Math.min(width, height))
    readonly property int particleCount: orb.compact ? 72 : 108

    implicitWidth: compact ? 320 : 520
    implicitHeight: implicitWidth

    Rectangle {
        anchors.centerIn: parent
        width: orb.safeSize * 0.98
        height: width
        radius: width / 2
        color: SentinelTheme.withAlpha(orb.accent, (orb.active ? 0.050 : 0.030) * orb.glowScale)
        border.color: SentinelTheme.withAlpha(orb.accent, orb.active ? 0.055 : 0.028)

        Behavior on color {
            ColorAnimation {
                duration: SentinelTheme.durationSlow
                easing.type: SentinelTheme.easingStandard
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: orb.safeSize * 0.72
        height: width
        radius: width / 2
        color: SentinelTheme.withAlpha(orb.secondaryAccent, (orb.active ? 0.060 : 0.032) * orb.glowScale)
        border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.045)
    }

    Item {
        id: outerOrbit
        anchors.centerIn: parent
        width: orb.safeSize * 0.90
        height: width

        RotationAnimation on rotation {
            loops: Animation.Infinite
            from: 0
            to: 360
            duration: orb.viewModel.currentModeName === "Tactical Mode" ? 14000 : SentinelTheme.durationOrbit
            running: orb.visible
        }

        Repeater {
            model: 5

            Rectangle {
                required property int index
                anchors.centerIn: parent
                width: Math.max(1, outerOrbit.width - index * (orb.compact ? 34 : 52))
                height: width
                radius: width / 2
                color: "transparent"
                border.color: SentinelTheme.withAlpha(orb.accent, (orb.active ? 0.055 : 0.030) + index * 0.010)
                border.width: 1
                rotation: index * 17
            }
        }

        Rectangle {
            width: 7
            height: 7
            radius: 4
            color: orb.active ? orb.accent : SentinelTheme.withAlpha(orb.accent, 0.58)
            x: outerOrbit.width / 2 - width / 2
            y: -height / 2
            opacity: 0.92
        }
    }

    Item {
        id: pointCloud
        anchors.centerIn: parent
        width: orb.safeSize * 0.64
        height: width

        RotationAnimation on rotation {
            loops: Animation.Infinite
            from: 0
            to: 360
            duration: SentinelTheme.durationOrbit * (orb.active ? 0.86 : 1.28)
            running: orb.visible
        }

        Repeater {
            model: orb.particleCount

            Rectangle {
                required property int index
                readonly property real theta: index * 2.399963
                readonly property real cloudLayer: (index % 17) / 17
                readonly property real radiusFactor: 0.18 + cloudLayer * 0.72
                readonly property real ySquash: 0.42 + (index % 5) * 0.055
                readonly property real dotSize: 1.4 + (index % 4) * 0.45

                width: dotSize
                height: dotSize
                radius: dotSize / 2
                x: pointCloud.width / 2 + Math.cos(theta) * pointCloud.width * 0.48 * radiusFactor - width / 2
                y: pointCloud.height / 2 + Math.sin(theta * 1.37) * pointCloud.height * 0.48 * radiusFactor * ySquash - height / 2
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.42 + (index % 6) * 0.055)
                opacity: orb.active ? 0.72 + (index % 5) * 0.05 : 0.44 + (index % 5) * 0.025
            }
        }
    }

    Item {
        id: reverseOrbit
        anchors.centerIn: parent
        width: orb.safeSize * 0.62
        height: width

        RotationAnimation on rotation {
            loops: Animation.Infinite
            from: 360
            to: 0
            duration: SentinelTheme.durationOrbit * 0.72
            running: orb.visible
        }

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: "transparent"
            border.color: SentinelTheme.withAlpha(orb.accent, 0.055)
        }

        Rectangle {
            width: 6
            height: 6
            radius: 3
            color: orb.secondaryAccent
            x: -width / 2
            y: reverseOrbit.height / 2 - height / 2
            opacity: 0.85
        }
    }

    Rectangle {
        id: core
        anchors.centerIn: parent
        width: orb.safeSize * 0.34
        height: width
        radius: width / 2
        color: SentinelTheme.withAlpha(orb.accent, (orb.active ? 0.13 : 0.075) * orb.glowScale)
        border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)

        SequentialAnimation on scale {
            id: coreBreath
            loops: Animation.Infinite
            running: orb.visible
            NumberAnimation {
                from: orb.active ? 0.96 : 0.985
                to: orb.active ? 1.045 : 1.015
                duration: SentinelTheme.durationAmbient
                easing.type: Easing.InOutSine
            }
            NumberAnimation {
                to: orb.active ? 0.96 : 0.985
                duration: SentinelTheme.durationAmbient
                easing.type: Easing.InOutSine
            }
        }

        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.54
            height: width
            radius: width / 2
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
            border.color: SentinelTheme.withAlpha(orb.accent, 0.18)
        }

        Label {
            anchors.centerIn: parent
            text: ""
            color: SentinelTheme.textPrimary
            font.pixelSize: parent.width * 0.30
            font.weight: Font.Light
        }
    }
}
