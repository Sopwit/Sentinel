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

            InfoRow {
                compact: agentsPage.compact
                label: "Capability Registry"
                value: agentsPage.viewModel.agentCapabilityRegistryStatus
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Capabilities"
                value: agentsPage.viewModel.agentCapabilityEnabledCount.toString()
                       + " enabled / "
                       + agentsPage.viewModel.agentCapabilityDisabledCount.toString()
                       + " disabled / "
                       + agentsPage.viewModel.agentCapabilityRestrictedCount.toString()
                       + " restricted"
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Registry Summary"
                value: agentsPage.viewModel.agentCapabilityRegistrySummary
                Layout.columnSpan: agentsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Tool Contracts"
                value: agentsPage.viewModel.toolContractRegistryStatus
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Contracts"
                value: agentsPage.viewModel.toolContractEnabledCount.toString()
                       + " enabled / "
                       + agentsPage.viewModel.toolContractDisabledCount.toString()
                       + " disabled / "
                       + agentsPage.viewModel.toolContractRestrictedCount.toString()
                       + " restricted"
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Contract Summary"
                value: agentsPage.viewModel.toolContractRegistrySummary
                Layout.columnSpan: agentsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Voice Runtime"
                value: agentsPage.viewModel.voiceRuntimeHealth
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Voice Config"
                value: agentsPage.viewModel.voiceRuntimeConfiguredCount.toString()
                       + " configured / "
                       + agentsPage.viewModel.voiceRuntimeMissingCount.toString()
                       + " missing / "
                       + agentsPage.viewModel.voiceRuntimeRefusedCount.toString()
                       + " refused"
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Voice Readiness"
                value: agentsPage.viewModel.voiceRuntimeReadinessSummary
                Layout.columnSpan: agentsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Voice Pipeline"
                value: agentsPage.viewModel.voicePipelineSessionStatus
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Pipeline Stages"
                value: agentsPage.viewModel.voicePipelineSessionReadyStageCount.toString()
                       + " ready / "
                       + agentsPage.viewModel.voicePipelineSessionBlockedStageCount.toString()
                       + " blocked"
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Whisper STT"
                value: agentsPage.viewModel.whisperTranscriptionStatus
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Piper TTS"
                value: agentsPage.viewModel.piperSynthesisStatus
            }

            InfoRow {
                compact: agentsPage.compact
                label: "STT Boundary"
                value: agentsPage.viewModel.whisperTranscriptionReadinessSummary
                Layout.columnSpan: agentsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: agentsPage.compact
                label: "TTS Boundary"
                value: agentsPage.viewModel.piperSynthesisReadinessSummary
                Layout.columnSpan: agentsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Pipeline Summary"
                value: agentsPage.viewModel.voicePipelineSessionSummary
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

            Repeater {
                model: agentsPage.viewModel.agentCapabilitySummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: modelData.indexOf("[Enabled Metadata") >= 0 ? "Capability" : "Restricted Capability"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.agentCapabilityReadinessSummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "Capability Readiness"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.agentCapabilitySafetySummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "Capability Safety"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.toolContractSummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: modelData.indexOf("[Enabled Metadata") >= 0 ? "Tool Contract" : "Restricted Tool"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.toolContractPermissionSummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "Contract Permission"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.toolContractSandboxSummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "Contract Sandbox"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.toolContractReadinessSummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "Contract Readiness"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.voicePipelineSessionStageReadinessSummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "Voice Stage"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.voicePipelineSessionTraceSummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "Voice Trace"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.toolContractSafetySummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "Contract Safety"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.voiceRuntimeReadinessChecks

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "Voice Runtime"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.whisperTranscriptionTraceSummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "STT Trace"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: agentsPage.viewModel.piperSynthesisTraceSummaries

                InfoRow {
                    required property string modelData
                    compact: agentsPage.compact
                    label: "TTS Trace"
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
