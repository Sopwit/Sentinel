import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: stream
    required property var viewModel

    implicitHeight: 220
    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.032)
    border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.08)
    bracketColor: SentinelTheme.withAlpha(SentinelTheme.accent, 0.20)
    bracketSize: 9

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceSm

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: "COGNITION STREAM"
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 2.4
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: "LIVE / " + stream.viewModel.agentActivityCount
                color: SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.72)
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
            }
        }

        Repeater {
            model: 5

            RowLayout {
                required property int index
                readonly property string rowTime: index === 0 ? "00:14:02"
                                              : index === 1 ? "00:13:48"
                                              : index === 2 ? "00:13:31"
                                              : index === 3 ? "00:13:12" : "00:12:55"
                readonly property string rowMessage: index === 0 ? stream.viewModel.latestAgentPipelineSummary
                                                 : index === 1 ? "Synthesized local context bridge across runtime memory placeholders"
                                                 : index === 2 ? "Agent metadata perimeter scan complete / no execution requested"
                                                 : index === 3 ? "Re-balanced visual posture for " + stream.viewModel.currentModeName
                                                 : "Drafted local echo response / awaiting operator input"
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceMd

                Label {
                    Layout.preferredWidth: 58
                    text: rowTime
                    color: SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.72)
                    font.pixelSize: SentinelTheme.fontTiny
                    font.letterSpacing: 1.1
                }

                Rectangle {
                    Layout.preferredWidth: 4
                    Layout.preferredHeight: 4
                    radius: 2
                    color: SentinelTheme.accent
                    opacity: 0.68
                }

                Label {
                    Layout.fillWidth: true
                    text: rowMessage
                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.88)
                    font.pixelSize: SentinelTheme.fontSmall
                    elide: Text.ElideRight
                }
            }
        }
    }
}
