import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

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
            model: [
                stream.viewModel.latestAgentPipelineSummary,
                stream.viewModel.latestAgentActivitySummary,
                stream.viewModel.runtimeContextSummary,
                stream.viewModel.conversationTransitionSummary,
                stream.viewModel.localInferenceSummary
            ]

            RowLayout {
                required property int index
                required property string modelData
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceMd

                Label {
                    Layout.preferredWidth: 58
                    text: "0" + (index + 1)
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
                    text: modelData
                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.88)
                    font.pixelSize: SentinelTheme.fontSmall
                    elide: Text.ElideRight
                }
            }
        }
    }
}
