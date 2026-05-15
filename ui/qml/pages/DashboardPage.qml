import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

RowLayout {
    id: dashboardPage
    required property var viewModel

    spacing: SentinelTheme.spaceMd

    ShellPanel {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: SentinelTheme.spaceLg
            spacing: SentinelTheme.spaceLg

            SectionTitle {
                title: "Operations Dashboard"
                subtitle: "Minimal state overview for the Sentinel Desktop Alpha shell."
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: 12
                rowSpacing: 12

                MetricCard {
                    label: "Core"
                    value: "Online"
                }
                MetricCard {
                    label: "Memory"
                    value: "Runtime"
                }
                MetricCard {
                    label: "Network"
                    value: "Disabled"
                }
            }

            ShellPanel {
                Layout.fillWidth: true
                color: SentinelTheme.panelStrong

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.spaceLg
                    spacing: SentinelTheme.spaceSm

                    Label {
                        text: "Agent Runtime"
                        color: SentinelTheme.accent
                        font.pixelSize: SentinelTheme.fontBody
                        font.letterSpacing: 1.2
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 14
                        rowSpacing: 8

                        Label {
                            text: "Pipeline"
                            color: SentinelTheme.textMuted
                        }

                        Label {
                            Layout.fillWidth: true
                            text: dashboardPage.viewModel.latestAgentPipelineStatus
                                  + " - "
                                  + dashboardPage.viewModel.latestAgentPipelineSummary
                            color: SentinelTheme.textPrimary
                            wrapMode: Text.WordWrap
                        }

                        Label {
                            text: "Context"
                            color: SentinelTheme.textMuted
                        }

                        Label {
                            Layout.fillWidth: true
                            text: dashboardPage.viewModel.runtimeContextStatus
                                  + " - "
                                  + dashboardPage.viewModel.runtimeContextSummary
                            color: SentinelTheme.textPrimary
                            wrapMode: Text.WordWrap
                        }

                        Label {
                            text: "Planned Tools"
                            color: SentinelTheme.textMuted
                        }

                        Label {
                            Layout.fillWidth: true
                            text: dashboardPage.viewModel.runtimeContextActiveToolIds.length > 0
                                  ? dashboardPage.viewModel.runtimeContextActiveToolIds.join(", ")
                                  : "None"
                            color: SentinelTheme.textPrimary
                            wrapMode: Text.WordWrap
                        }

                        Label {
                            text: "Activity"
                            color: SentinelTheme.textMuted
                        }

                        Label {
                            Layout.fillWidth: true
                            text: dashboardPage.viewModel.agentActivityCount
                                  + " - "
                                  + dashboardPage.viewModel.latestAgentActivitySummary
                            color: SentinelTheme.textPrimary
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            ShellPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: SentinelTheme.panelMuted

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.spaceLg
                    spacing: SentinelTheme.spaceSm

                    Label {
                        text: "Current Posture"
                        color: SentinelTheme.accent
                        font.pixelSize: SentinelTheme.fontBody
                        font.letterSpacing: 1.2
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Mode: " + dashboardPage.viewModel.currentModeName
                              + ". The app exposes only local echo-provider chat, runtime memory, settings, and lightweight plugin/integration contracts."
                        color: SentinelTheme.textPrimary
                        font.pixelSize: 16
                        lineHeight: 1.25
                        wrapMode: Text.WordWrap
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }

    ChatPanel {
        viewModel: dashboardPage.viewModel
        Layout.preferredWidth: 410
        Layout.fillHeight: true
    }
}
