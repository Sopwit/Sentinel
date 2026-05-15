import QtQuick

Item {
    id: atmosphere
    property string modeName: "Companion Mode"
    property color accentColor: SentinelTheme.modeAccent(modeName)

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: SentinelTheme.backgroundRaised
            }
            GradientStop {
                position: 0.58
                color: SentinelTheme.backgroundBase
            }
            GradientStop {
                position: 1.0
                color: SentinelTheme.backgroundDeep
            }
        }
    }

    Repeater {
        model: 24

        Rectangle {
            required property int index
            width: 1 + index % 3
            height: width
            radius: width / 2
            x: (index * 137) % Math.max(1, atmosphere.width)
            y: (index * 61) % Math.max(1, atmosphere.height)
            color: SentinelTheme.withAlpha(atmosphere.accentColor, 0.18)
            opacity: modeName === "Minimal Mode" ? 0.18 : 0.58

            SequentialAnimation on y {
                loops: Animation.Infinite
                running: atmosphere.visible
                PauseAnimation {
                    duration: index * 35
                }
                NumberAnimation {
                    from: (index * 61) % Math.max(1, atmosphere.height)
                    to: ((index * 61) % Math.max(1, atmosphere.height)) - 14
                    duration: 2600 + (index % 6) * 420
                    easing.type: Easing.InOutSine
                }
                NumberAnimation {
                    to: (index * 61) % Math.max(1, atmosphere.height)
                    duration: 2600 + (index % 6) * 420
                    easing.type: Easing.InOutSine
                }
            }
        }
    }

    Rectangle {
        width: Math.min(parent.width * 0.72, 620)
        height: width
        radius: width / 2
        x: -width * 0.32
        y: -height * 0.28
        color: SentinelTheme.withAlpha(accentColor, 0.08 * SentinelTheme.modeGlowScale(modeName))
        border.color: SentinelTheme.withAlpha(accentColor, 0.06)
        border.width: 1
    }

    Rectangle {
        width: Math.min(parent.width * 0.92, 900)
        height: width
        radius: width / 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        color: SentinelTheme.withAlpha(accentColor, 0.045 * SentinelTheme.modeGlowScale(modeName))
        border.color: SentinelTheme.withAlpha(accentColor, 0.035)
        border.width: 1
    }

    Rectangle {
        width: Math.min(parent.width * 0.58, 560)
        height: width
        radius: width / 2
        anchors.right: parent.right
        anchors.rightMargin: -width * 0.35
        anchors.verticalCenter: parent.verticalCenter
        color: SentinelTheme.withAlpha(SentinelTheme.accentSecondary, 0.075 * SentinelTheme.modeGlowScale(modeName))
        border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.03)
        border.width: 1
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        opacity: modeName === "Minimal Mode" ? 0.07 : 0.16

        Grid {
            id: meshGrid
            anchors.centerIn: parent
            columns: Math.max(1, Math.ceil(atmosphere.width / 56))
            rows: Math.max(1, Math.ceil(atmosphere.height / 56))
            spacing: 55

            Repeater {
                model: meshGrid.columns * meshGrid.rows

                Rectangle {
                    width: 1
                    height: 1
                    radius: 0.5
                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.32)
                }
            }
        }
    }
}
