import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ShellPanel {
    id: agents
    required property var viewModel
    implicitHeight: 320
    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.032)
    border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.08)
    bracketColor: SentinelTheme.withAlpha(SentinelTheme.accent, 0.20)
    bracketSize: 9

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceMd

        Label {
            text: "AGENT METADATA"
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontTiny
            font.letterSpacing: 2.4
        }

        ListView {
            id: agentList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: SentinelTheme.spaceMd
            model: agents.viewModel.activeAgentSummaries

            delegate: RowLayout {
                id: agentRow
                required property int index
                required property string modelData
                readonly property string agentName: modelData.split(" (")[0]
                width: agentList.width
                height: 48
                spacing: SentinelTheme.spaceMd

                Rectangle {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    radius: 17
                    color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.07)
                    border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.18)

                    Label {
                        anchors.centerIn: parent
                        text: agentRow.agentName.charAt(0)
                        color: SentinelTheme.accent
                        font.pixelSize: SentinelTheme.fontTiny
                        font.letterSpacing: 1.2
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceXs

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: agentRow.agentName
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontSmall
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "metadata"
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontTiny
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: agentRow.modelData
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
