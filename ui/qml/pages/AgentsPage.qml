import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ScrollView {
    id: agentsPage
    required property var viewModel
    readonly property bool compact: width < 820
    readonly property bool developerMode: viewModel.developerModeEnabled
    readonly property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property int panelPadding: SentinelTheme.spaceLg
    property string selectedSection: "Overview"

    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    contentWidth: availableWidth

    function sectionHeight(content) {
        return content.implicitHeight + panelPadding * 2
    }

    Item {
        width: agentsPage.availableWidth
        implicitHeight: agentsColumn.implicitHeight

        Column {
            id: agentsColumn
            width: parent.width
            spacing: SentinelTheme.spaceLg

            ShellPanel {
                width: parent.width
                implicitHeight: agentTabs.implicitHeight + SentinelTheme.spaceMd * 2

                ColumnLayout {
                    id: agentTabs
                    x: SentinelTheme.spaceMd
                    y: SentinelTheme.spaceMd
                    width: parent.width - SentinelTheme.spaceMd * 2
                    spacing: SentinelTheme.spaceSm

                    Flow {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        Repeater {
                            model: ["Overview", "Tasks", "Capabilities", "Developer"]

                            Button {
                                id: agentTabButton
                                required property string modelData
                                enabled: modelData !== "Developer" || agentsPage.developerMode
                                text: modelData
                                onClicked: agentsPage.selectedSection = modelData

                                contentItem: Label {
                                    text: agentTabButton.text
                                    color: agentsPage.selectedSection === agentTabButton.modelData
                                           ? SentinelTheme.textPrimary
                                           : SentinelTheme.textMuted
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: SentinelTheme.fontSmall
                                }

                                background: Rectangle {
                                    implicitWidth: 112
                                    implicitHeight: 32
                                    radius: 16
                                    color: agentsPage.selectedSection === agentTabButton.modelData
                                           ? SentinelTheme.withAlpha(agentsPage.modeAccent, 0.14)
                                           : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.035)
                                    border.color: agentTabButton.activeFocus
                                                  ? SentinelTheme.focusBorder
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.075)
                                }
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Metadata-only; no execution active. Developer Mode reveals internals only."
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                    }
                }
            }

            ShellPanel {
                width: parent.width
                visible: agentsPage.selectedSection === "Overview"
                implicitHeight: agentsPage.sectionHeight(registryContent)

                ColumnLayout {
                    id: registryContent
                    x: agentsPage.panelPadding
                    y: agentsPage.panelPadding
                    width: parent.width - agentsPage.panelPadding * 2
                    spacing: SentinelTheme.spaceMd

                    SectionTitle {
                        title: "Registered Profiles"
                        subtitle: "Static profiles, not active workers."
                        Layout.fillWidth: true
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        StatusChip {
                            label: "Registered"
                            value: agentsPage.viewModel.registeredAgentCount.toString()
                            accent: agentsPage.modeAccent
                            selected: true
                        }

                        StatusChip {
                            label: "Activity"
                            value: agentsPage.viewModel.agentActivityCount.toString()
                            accent: SentinelTheme.accentSecondary
                            muted: agentsPage.viewModel.agentActivityCount === 0
                        }

                        StatusChip {
                            label: "Authority"
                            value: "metadata only"
                            accent: SentinelTheme.textMuted
                            muted: true
                        }
                    }

                    InfoRow {
                        compact: agentsPage.compact
                        label: "Current"
                        value: agentsPage.viewModel.currentAgentSummary
                        Layout.fillWidth: true
                    }
                }
            }

            GridLayout {
                width: parent.width
                visible: agentsPage.selectedSection === "Tasks"
                         || agentsPage.selectedSection === "Capabilities"
                         || agentsPage.selectedSection === "Overview"
                columns: agentsPage.compact ? 1 : 2
                columnSpacing: SentinelTheme.spaceLg
                rowSpacing: SentinelTheme.spaceLg

                ShellPanel {
                    Layout.fillWidth: true
                    visible: agentsPage.selectedSection === "Tasks"
                    implicitHeight: agentsPage.sectionHeight(taskRuntimeContent)

                    ColumnLayout {
                        id: taskRuntimeContent
                        x: agentsPage.panelPadding
                        y: agentsPage.panelPadding
                        width: parent.width - agentsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: "Task Runtime"
                            subtitle: "Static runtime state, no workers."
                            Layout.fillWidth: true
                        }

                        StatusChip {
                            label: "Runtime"
                            value: agentsPage.viewModel.agentTaskRuntimeStatus
                            accent: agentsPage.modeAccent
                            selected: true
                        }

                        InfoRow {
                            compact: true
                            label: "Boundary"
                            value: agentsPage.viewModel.agentTaskRuntimeSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Latest"
                            value: agentsPage.viewModel.latestAgentTaskSummary
                            Layout.fillWidth: true
                        }
                    }
                }

                ShellPanel {
                    Layout.fillWidth: true
                    visible: agentsPage.selectedSection === "Tasks"
                    implicitHeight: agentsPage.sectionHeight(taskQueueContent)

                    ColumnLayout {
                        id: taskQueueContent
                        x: agentsPage.panelPadding
                        y: agentsPage.panelPadding
                        width: parent.width - agentsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: "Task Queue"
                            subtitle: "Queued lifecycle metadata."
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: "Queued"
                                value: agentsPage.viewModel.agentTaskQueueCount.toString()
                                accent: agentsPage.modeAccent
                                selected: true
                            }

                            StatusChip {
                                label: "Planned"
                                value: agentsPage.viewModel.agentTaskQueuePlannedCount.toString()
                                accent: SentinelTheme.accentSecondary
                            }

                            StatusChip {
                                label: "Blocked"
                                value: agentsPage.viewModel.agentTaskQueueBlockedCount.toString()
                                accent: SentinelTheme.warning
                                muted: agentsPage.viewModel.agentTaskQueueBlockedCount === 0
                            }

                            StatusChip {
                                label: "Refused"
                                value: agentsPage.viewModel.agentTaskQueueRefusedCount.toString()
                                accent: SentinelTheme.warning
                                muted: agentsPage.viewModel.agentTaskQueueRefusedCount === 0
                            }
                        }

                        InfoRow {
                            compact: true
                            label: "Lifecycle"
                            value: agentsPage.viewModel.latestAgentTaskLifecycleSummary
                            Layout.fillWidth: true
                        }
                    }
                }

                ShellPanel {
                    Layout.fillWidth: true
                    visible: agentsPage.selectedSection === "Tasks"
                    implicitHeight: agentsPage.sectionHeight(planningContent)

                    ColumnLayout {
                        id: planningContent
                        x: agentsPage.panelPadding
                        y: agentsPage.panelPadding
                        width: parent.width - agentsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: "Planning Sessions"
                            subtitle: "Safety arbitration metadata only."
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: "Status"
                                value: agentsPage.viewModel.agentPlanningSessionStatus
                                accent: agentsPage.modeAccent
                                selected: true
                            }

                            StatusChip {
                                label: "Candidates"
                                value: agentsPage.viewModel.agentPlanningCandidateCount.toString()
                                accent: SentinelTheme.accentSecondary
                            }

                            StatusChip {
                                label: "Refused"
                                value: agentsPage.viewModel.agentPlanningRefusedCount.toString()
                                accent: SentinelTheme.warning
                                muted: agentsPage.viewModel.agentPlanningRefusedCount === 0
                            }
                        }

                        InfoRow {
                            compact: true
                            label: "Summary"
                            value: agentsPage.viewModel.agentPlanningSessionSummary
                            Layout.fillWidth: true
                        }
                    }
                }

                ShellPanel {
                    Layout.fillWidth: true
                    visible: agentsPage.selectedSection === "Capabilities"
                    implicitHeight: agentsPage.sectionHeight(capabilityContent)

                    ColumnLayout {
                        id: capabilityContent
                        x: agentsPage.panelPadding
                        y: agentsPage.panelPadding
                        width: parent.width - agentsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: "Capability Registry"
                            subtitle: "Capabilities are labels, not grants."
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: "Enabled"
                                value: agentsPage.viewModel.agentCapabilityEnabledCount.toString()
                                accent: SentinelTheme.success
                            }

                            StatusChip {
                                label: "Disabled"
                                value: agentsPage.viewModel.agentCapabilityDisabledCount.toString()
                                accent: SentinelTheme.textMuted
                                muted: true
                            }

                            StatusChip {
                                label: "Restricted"
                                value: agentsPage.viewModel.agentCapabilityRestrictedCount.toString()
                                accent: SentinelTheme.warning
                                muted: agentsPage.viewModel.agentCapabilityRestrictedCount === 0
                            }
                        }

                        Repeater {
                            model: Math.min(4, agentsPage.viewModel.agentCapabilitySummaries.length)

                            InfoRow {
                                required property int index
                                compact: true
                                label: "Capability"
                                value: agentsPage.viewModel.agentCapabilitySummaries[index]
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                ShellPanel {
                    Layout.fillWidth: true
                    visible: agentsPage.selectedSection === "Capabilities"
                    implicitHeight: agentsPage.sectionHeight(toolContent)

                    ColumnLayout {
                        id: toolContent
                        x: agentsPage.panelPadding
                        y: agentsPage.panelPadding
                        width: parent.width - agentsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: "Tool Contracts"
                            subtitle: "Contracts describe future policy; they do not execute."
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: "Enabled"
                                value: agentsPage.viewModel.toolContractEnabledCount.toString()
                                accent: SentinelTheme.success
                            }

                            StatusChip {
                                label: "Disabled"
                                value: agentsPage.viewModel.toolContractDisabledCount.toString()
                                accent: SentinelTheme.textMuted
                                muted: true
                            }

                            StatusChip {
                                label: "Restricted"
                                value: agentsPage.viewModel.toolContractRestrictedCount.toString()
                                accent: SentinelTheme.warning
                                muted: agentsPage.viewModel.toolContractRestrictedCount === 0
                            }
                        }

                        Repeater {
                            model: Math.min(4, agentsPage.viewModel.toolContractSummaries.length)

                            InfoRow {
                                required property int index
                                compact: true
                                label: "Contract"
                                value: agentsPage.viewModel.toolContractSummaries[index]
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                ShellPanel {
                    Layout.fillWidth: true
                    visible: agentsPage.selectedSection === "Overview"
                    implicitHeight: agentsPage.sectionHeight(voiceContent)

                    ColumnLayout {
                        id: voiceContent
                        x: agentsPage.panelPadding
                        y: agentsPage.panelPadding
                        width: parent.width - agentsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: "Voice Readiness"
                            subtitle: "Prepared or missing only; no microphone or playback controls."
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Voice"
                            value: agentsPage.viewModel.voiceReadinessStatus
                                   + " / "
                                   + agentsPage.viewModel.voiceRuntimeHealth
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Whisper"
                            value: agentsPage.viewModel.whisperTranscriptionStatus
                                   + " / "
                                   + agentsPage.viewModel.whisperTranscriptionReadinessSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Piper"
                            value: agentsPage.viewModel.piperSynthesisStatus
                                   + " / "
                                   + agentsPage.viewModel.piperSynthesisReadinessSummary
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            ShellPanel {
                width: parent.width
                visible: agentsPage.selectedSection === "Developer" && agentsPage.developerMode
                implicitHeight: agentsPage.sectionHeight(developerContent)

                ColumnLayout {
                    id: developerContent
                    x: agentsPage.panelPadding
                    y: agentsPage.panelPadding
                    width: parent.width - agentsPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                    SectionTitle {
                        title: "Developer Metadata"
                        subtitle: "Trace, arbitration, permission, sandbox, and voice pipeline summaries."
                        Layout.fillWidth: true
                    }

                    Repeater {
                        model: agentsPage.viewModel.agentTaskTraceSummaries
                        InfoRow {
                            required property string modelData
                            compact: true
                            label: "Task Trace"
                            value: modelData
                            Layout.fillWidth: true
                        }
                    }

                    Repeater {
                        model: agentsPage.viewModel.agentPlanningArbitrationSummaries
                        InfoRow {
                            required property string modelData
                            compact: true
                            label: "Arbitration"
                            value: modelData
                            Layout.fillWidth: true
                        }
                    }

                    Repeater {
                        model: agentsPage.viewModel.toolContractPermissionSummaries
                        InfoRow {
                            required property string modelData
                            compact: true
                            label: "Permission"
                            value: modelData
                            Layout.fillWidth: true
                        }
                    }

                    Repeater {
                        model: agentsPage.viewModel.toolContractSandboxSummaries
                        InfoRow {
                            required property string modelData
                            compact: true
                            label: "Sandbox"
                            value: modelData
                            Layout.fillWidth: true
                        }
                    }

                    Repeater {
                        model: agentsPage.viewModel.voicePipelineSessionStageReadinessSummaries
                        InfoRow {
                            required property string modelData
                            compact: true
                            label: "Voice Stage"
                            value: modelData
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }
}
