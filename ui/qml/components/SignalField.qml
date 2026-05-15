import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: signal
    implicitHeight: 170
    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.032)
    border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.08)
    bracketColor: SentinelTheme.withAlpha(SentinelTheme.accent, 0.20)
    bracketSize: 9

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceMd

        Label {
            text: "SIGNAL FIELD"
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontTiny
            font.letterSpacing: 2.4
        }

        Row {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 3

            Repeater {
                model: 48

                Rectangle {
                    required property int index
                    width: Math.max(2, (signal.width - SentinelTheme.spaceLg * 2 - 150) / 48)
                    height: 14 + (Math.sin(index * 0.55) * 0.5 + 0.5) * 64 + (index % 6) * 3
                    anchors.verticalCenter: parent.verticalCenter
                    radius: width / 2
                    color: SentinelTheme.withAlpha(index % 3 === 0 ? SentinelTheme.accentTertiary : SentinelTheme.accent, 0.52)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: "AMBIENT / HARMONIC"
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.6
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: "432 HZ"
                color: SentinelTheme.accent
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.6
            }
        }
    }
}
