import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

GridLayout {
    id: dashboardPage
    required property var viewModel
    readonly property bool wideLayout: width >= SentinelTheme.breakpointWide

    columns: dashboardPage.wideLayout ? 2 : 1
    columnSpacing: SentinelTheme.spaceMd
    rowSpacing: SentinelTheme.spaceMd

    ShellPanel {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: 0

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: SentinelTheme.pageMargin(dashboardPage.width)
            spacing: SentinelTheme.contentSpacing(dashboardPage.width)

            SectionTitle {
                title: "Operations Dashboard"
                subtitle: "Minimal state overview for the Sentinel Desktop Alpha shell."
            }

            GridLayout {
                Layout.fillWidth: true
                columns: dashboardPage.width < 620 ? 1 : dashboardPage.width < 940 ? 2 : 3
                columnSpacing: SentinelTheme.spaceSm
                rowSpacing: SentinelTheme.spaceSm

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
                    anchors.margins: SentinelTheme.cardPadding
                    spacing: SentinelTheme.spaceSm

                    Label {
                        text: "Agent Runtime"
                        color: SentinelTheme.accent
                        font.pixelSize: SentinelTheme.fontBody
                        font.letterSpacing: 1.2
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        InfoRow {
                            compact: dashboardPage.width < 620
                            label: "Pipeline"
                            value: dashboardPage.viewModel.latestAgentPipelineStatus
                                   + " - "
                                   + dashboardPage.viewModel.latestAgentPipelineSummary
                        }

                        InfoRow {
                            compact: dashboardPage.width < 620
                            label: "Context"
                            value: dashboardPage.viewModel.runtimeContextStatus
                                   + " - "
                                   + dashboardPage.viewModel.runtimeContextSummary
                        }

                        InfoRow {
                            compact: dashboardPage.width < 620
                            label: "Planned Tools"
                            value: dashboardPage.viewModel.runtimeContextActiveToolIds.length > 0
                                   ? dashboardPage.viewModel.runtimeContextActiveToolIds.join(", ")
                                   : "None"
                        }

                        InfoRow {
                            compact: dashboardPage.width < 620
                            label: "Activity"
                            value: dashboardPage.viewModel.agentActivityCount
                                   + " - "
                                   + dashboardPage.viewModel.latestAgentActivitySummary
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
                    anchors.margins: SentinelTheme.cardPadding
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
                        font.pixelSize: SentinelTheme.fontControl
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
        Layout.fillWidth: !dashboardPage.wideLayout
        Layout.preferredWidth: dashboardPage.wideLayout ? 410 : -1
        Layout.preferredHeight: dashboardPage.wideLayout ? -1 : 340
        Layout.fillHeight: true
        compact: !dashboardPage.wideLayout
    }
}
