import QtQuick
import QtQuick.Layouts
import Sentinel.Desktop

ShellPanel {
    id: agentsPage
    required property var viewModel
    readonly property bool compact: width < 760

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.pageMargin(agentsPage.width)
        spacing: SentinelTheme.contentSpacing(agentsPage.width)

        SectionTitle {
            title: "Agent Metadata"
            subtitle: "Read-only registry and activity metadata in a dedicated workspace."
        }

        GridLayout {
            Layout.fillWidth: true
            columns: agentsPage.compact ? 1 : 2
            columnSpacing: SentinelTheme.spaceSm
            rowSpacing: SentinelTheme.spaceSm

            InfoRow {
                compact: agentsPage.compact
                label: "Registered Agents"
                value: agentsPage.viewModel.registeredAgentCount.toString()
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Agent Activity"
                value: agentsPage.viewModel.agentActivityCount.toString()
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Current Agent"
                value: agentsPage.viewModel.currentAgentSummary
                Layout.columnSpan: agentsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Task Runtime"
                value: agentsPage.viewModel.agentTaskRuntimeStatus
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Queued Tasks"
                value: agentsPage.viewModel.agentTaskQueueCount.toString()
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Planned / Active"
                value: agentsPage.viewModel.agentTaskQueuePlannedCount.toString()
                       + " / "
                       + agentsPage.viewModel.agentTaskQueueActiveCount.toString()
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Blocked"
                value: agentsPage.viewModel.agentTaskQueueBlockedCount.toString()
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Completed / Refused"
                value: agentsPage.viewModel.agentTaskQueueCompletedCount.toString()
                       + " / "
                       + agentsPage.viewModel.agentTaskQueueRefusedCount.toString()
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Latest Task"
                value: agentsPage.viewModel.latestAgentTaskSummary
                Layout.columnSpan: agentsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Lifecycle"
                value: agentsPage.viewModel.latestAgentTaskLifecycleSummary
                Layout.columnSpan: agentsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Runtime Boundary"
                value: agentsPage.viewModel.agentTaskRuntimeSummary
                Layout.columnSpan: agentsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Planning Session"
                value: agentsPage.viewModel.agentPlanningSessionStatus
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Plans / Refused"
                value: agentsPage.viewModel.agentPlanningCandidateCount.toString()
                       + " / "
                       + agentsPage.viewModel.agentPlanningRefusedCount.toString()
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Planning Summary"
                value: agentsPage.viewModel.agentPlanningSessionSummary
                Layout.columnSpan: agentsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Planning Fallback"
                value: agentsPage.viewModel.agentPlanningFallbackSummary
                Layout.columnSpan: agentsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceXs

            Repeater {
                model: agentsPage.viewModel.agentTaskTraceSummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "Task Trace"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.agentPlanningArbitrationSummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "Arbitration"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.agentPlanningRefusalSummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "Refusal"
                    value: modelData
                    Layout.fillWidth: true
                }
            }
        }

        ActiveAgentsPanel {
            viewModel: agentsPage.viewModel
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
