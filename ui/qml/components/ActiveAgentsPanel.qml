import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: agents
    implicitHeight: 220
    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.032)
    border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.08)
    bracketColor: SentinelTheme.withAlpha(SentinelTheme.accent, 0.20)
    bracketSize: 9

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceMd

        Label {
            text: "ACTIVE AGENTS"
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontTiny
            font.letterSpacing: 2.4
        }

        Repeater {
            model: 4

            RowLayout {
                required property int index
                readonly property string agentName: index === 0 ? "Atlas"
                                                : index === 1 ? "Orin"
                                                : index === 2 ? "Vela" : "Kaze"
                readonly property string agentTask: index === 0 ? "Perimeter cognition scan"
                                                : index === 1 ? "Synthesizing brief"
                                                : index === 2 ? "Listening / ambient signals"
                                                : "Indexing memory shards"
                readonly property int agentLoad: index === 0 ? 62
                                             : index === 1 ? 88
                                             : index === 2 ? 24 : 47
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceMd

                Rectangle {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    radius: 17
                    color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.07)
                    border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.18)

                    Label {
                        anchors.centerIn: parent
                        text: agentName.charAt(0)
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
                            text: agentName
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontSmall
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            text: agentLoad + "%"
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontTiny
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: agentTask
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 2
                        radius: 1
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)

                        Rectangle {
                            width: parent.width * agentLoad / 100
                            height: parent.height
                            radius: 1
                            color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.72)
                        }
                    }
                }
            }
        }
    }
}
