import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts

Window {
    id: root

    property string modeName: "Sentinel"
    property string statusText: qsTr("Initializing...")

    signal dismissed()

    opacity: 1.0
    Behavior on opacity { NumberAnimation { duration: 200 } }

    width: 420
    height: 320
    flags: Qt.SplashScreen | Qt.FramelessWindowHint
    color: "transparent"
    modality: Qt.ApplicationModal
    transientParent: null

    x: Screen.width / 2 - width / 2
    y: Screen.height / 2 - height / 2

    Rectangle {
        anchors.fill: parent
        radius: SentinelTheme.radiusPanel
        color: SentinelTheme.backgroundBase
        border.color: SentinelTheme.withAlpha(SentinelTheme.modeAccent(root.modeName), 0.25)
        border.width: 1

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.35)
            shadowVerticalOffset: 6
            shadowBlur: 0.3
            shadowOpacity: 1.0
        }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: SentinelTheme.spaceLg

            Image {
                Layout.alignment: Qt.AlignHCenter
                source: ":/icons/dev.sentinel.Sentinel.png"
                sourceSize.width: 64
                sourceSize.height: 64
                visible: status === Image.Ready
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Sentinel")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontHeader
                font.weight: Font.Bold
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Personal AI Assistant")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontBody
            }

            Item { height: 8 }

            Rectangle {
                id: progressTrack
                Layout.alignment: Qt.AlignHCenter
                width: 160
                height: 3
                radius: 1.5
                color: SentinelTheme.withAlpha(SentinelTheme.modeAccent(root.modeName), 0.15)

                Rectangle {
                    id: progressBar
                    width: progressTrack.width * 0.6
                    height: progressTrack.height
                    radius: progressTrack.radius
                    color: SentinelTheme.modeAccent(root.modeName)

                    SequentialAnimation on x {
                        loops: Animation.Infinite
                        running: true
                        NumberAnimation { from: 0; to: progressTrack.width - progressBar.width; duration: 1200; easing.type: Easing.InOutQuad }
                        NumberAnimation { from: progressTrack.width - progressBar.width; to: 0; duration: 1200; easing.type: Easing.InOutQuad }
                    }
                }
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: root.statusText
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                opacity: 0.7
            }
        }
    }
}
