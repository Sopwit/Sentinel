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
    readonly property string uiSelfCheck: "developer-gated balanced-cards wrapped-contracts bottom-safe-scroll"
    property string selectedSection: "Overview"
    onDeveloperModeChanged: {
        if (!developerMode && selectedSection === "Developer")
            selectedSection = "Overview"
    }

    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AsNeeded
        contentItem: Rectangle {
            implicitWidth: 4
            radius: 2
            color: SentinelTheme.withAlpha(agentsPage.modeAccent, parent.active ? 0.34 : 0.18)
        }
        background: Rectangle {
            color: "transparent"
        }
    }
    contentWidth: availableWidth

    function sectionHeight(content) {
        return content.implicitHeight + panelPadding * 2
    }

    function metadataAccent(summary) {
        var lower = summary.toLowerCase()
        if (lower.indexOf("refused") >= 0 || lower.indexOf("blocked") >= 0
                || lower.indexOf("restricted") >= 0 || lower.indexOf("disabled") >= 0)
            return SentinelTheme.warning
        return agentsPage.modeAccent
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
                            model: agentsPage.developerMode
                                   ? ["Overview", "Tasks", "Capabilities", "Developer"]
                                   : ["Overview", "Tasks", "Capabilities"]

                            Button {
                                id: agentTabButton
                                required property string modelData
                                readonly property bool active: agentsPage.selectedSection === agentTabButton.modelData
                                text: modelData
                                hoverEnabled: true
                                onClicked: agentsPage.selectedSection = modelData

                                contentItem: Label {
                                    text: agentTabButton.text
                                    color: agentTabButton.active
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
                                    color: InteractionTokens.surfaceColor(agentTabButton.hovered, agentTabButton.down,
                                                                           agentTabButton.active,
                                                                           agentsPage.modeAccent)
                                    border.color: InteractionTokens.borderColor(agentTabButton.activeFocus, agentTabButton.hovered,
                                                                                 agentTabButton.active,
                                                                                 agentsPage.modeAccent)

                                    Behavior on color {
                                        ColorAnimation {
                                            duration: MotionTokens.fast
                                            easing.type: MotionTokens.standard
                                        }
                                    }

                                    Behavior on border.color {
                                        ColorAnimation {
                                            duration: MotionTokens.fast
                                            easing.type: MotionTokens.standard
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: agentsPage.developerMode
                              ? "Metadata-only; no execution active. Developer internals are visible."
                              : "Metadata-only; no execution active."
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

                    Label {
                        Layout.fillWidth: true
                        text: "Profiles are registered for visibility only. Nothing runs in the background."
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
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
                    Layout.preferredHeight: 220

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
                    Layout.preferredHeight: 220

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
                                accent: agentsPage.viewModel.agentTaskQueueBlockedCount > 0
                                        ? SentinelTheme.warning
                                        : SentinelTheme.textMuted
                                muted: agentsPage.viewModel.agentTaskQueueBlockedCount === 0
                            }

                            StatusChip {
                                label: "Refused"
                                value: agentsPage.viewModel.agentTaskQueueRefusedCount.toString()
                                accent: agentsPage.viewModel.agentTaskQueueRefusedCount > 0
                                        ? SentinelTheme.warning
                                        : SentinelTheme.textMuted
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
                    Layout.preferredHeight: 220

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
                                accent: agentsPage.viewModel.agentPlanningRefusedCount > 0
                                        ? SentinelTheme.warning
                                        : SentinelTheme.textMuted
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
                    Layout.preferredHeight: 260

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
                                accent: agentsPage.viewModel.agentCapabilityRestrictedCount > 0
                                        ? SentinelTheme.warning
                                        : SentinelTheme.textMuted
                                muted: agentsPage.viewModel.agentCapabilityRestrictedCount === 0
                            }
                        }

                        Repeater {
                            model: Math.min(4, agentsPage.viewModel.agentCapabilitySummaries.length)

                            Rectangle {
                                id: capabilityRow
                                required property int index
                                readonly property color rowAccent: agentsPage.metadataAccent(
                                    agentsPage.viewModel.agentCapabilitySummaries[index])
                                Layout.fillWidth: true
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(capabilityRow.rowAccent,
                                                               capabilityHover.containsMouse ? 0.060 : 0.034)
                                border.color: SentinelTheme.withAlpha(capabilityRow.rowAccent,
                                                                      capabilityHover.containsMouse ? 0.16 : 0.080)
                                implicitHeight: capabilityText.implicitHeight + SentinelTheme.spaceSm
                                scale: capabilityHover.containsMouse ? InteractionTokens.cardHoverLift : 1.0

                                Behavior on color {
                                    ColorAnimation {
                                        duration: MotionTokens.duration(MotionTokens.fast,
                                                                        agentsPage.viewModel.currentModeName)
                                        easing.type: MotionTokens.standard
                                    }
                                }

                                Behavior on border.color {
                                    ColorAnimation {
                                        duration: MotionTokens.duration(MotionTokens.fast,
                                                                        agentsPage.viewModel.currentModeName)
                                        easing.type: MotionTokens.standard
                                    }
                                }

                                Behavior on scale {
                                    NumberAnimation {
                                        duration: MotionTokens.duration(MotionTokens.fast,
                                                                        agentsPage.viewModel.currentModeName)
                                        easing.type: MotionTokens.enter
                                    }
                                }

                                MouseArea {
                                    id: capabilityHover
                                    anchors.fill: parent
                                    acceptedButtons: Qt.NoButton
                                    hoverEnabled: true
                                }

                                Text {
                                    id: capabilityText
                                    x: SentinelTheme.spaceSm
                                    y: SentinelTheme.spaceXs
                                    width: parent.width - SentinelTheme.spaceSm * 2
                                    text: agentsPage.viewModel.agentCapabilitySummaries[capabilityRow.index]
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontSmall
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 5
                                }
                            }
                        }
                    }
                }

                ShellPanel {
                    Layout.fillWidth: true
                    visible: agentsPage.selectedSection === "Capabilities"
                    implicitHeight: agentsPage.sectionHeight(toolContent)
                    Layout.preferredHeight: 260

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
                                accent: agentsPage.viewModel.toolContractRestrictedCount > 0
                                        ? SentinelTheme.warning
                                        : SentinelTheme.textMuted
                                muted: agentsPage.viewModel.toolContractRestrictedCount === 0
                            }
                        }

                        Repeater {
                            model: Math.min(4, agentsPage.viewModel.toolContractSummaries.length)

                            Rectangle {
                                id: contractRow
                                required property int index
                                readonly property color rowAccent: agentsPage.metadataAccent(
                                    agentsPage.viewModel.toolContractSummaries[index])
                                Layout.fillWidth: true
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(contractRow.rowAccent,
                                                               contractHover.containsMouse ? 0.060 : 0.034)
                                border.color: SentinelTheme.withAlpha(contractRow.rowAccent,
                                                                      contractHover.containsMouse ? 0.16 : 0.080)
                                implicitHeight: contractText.implicitHeight + SentinelTheme.spaceSm
                                scale: contractHover.containsMouse ? InteractionTokens.cardHoverLift : 1.0

                                Behavior on color {
                                    ColorAnimation {
                                        duration: MotionTokens.duration(MotionTokens.fast,
                                                                        agentsPage.viewModel.currentModeName)
                                        easing.type: MotionTokens.standard
                                    }
                                }

                                Behavior on border.color {
                                    ColorAnimation {
                                        duration: MotionTokens.duration(MotionTokens.fast,
                                                                        agentsPage.viewModel.currentModeName)
                                        easing.type: MotionTokens.standard
                                    }
                                }

                                Behavior on scale {
                                    NumberAnimation {
                                        duration: MotionTokens.duration(MotionTokens.fast,
                                                                        agentsPage.viewModel.currentModeName)
                                        easing.type: MotionTokens.enter
                                    }
                                }

                                MouseArea {
                                    id: contractHover
                                    anchors.fill: parent
                                    acceptedButtons: Qt.NoButton
                                    hoverEnabled: true
                                }

                                Text {
                                    id: contractText
                                    x: SentinelTheme.spaceSm
                                    y: SentinelTheme.spaceXs
                                    width: parent.width - SentinelTheme.spaceSm * 2
                                    text: agentsPage.viewModel.toolContractSummaries[contractRow.index]
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontSmall
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 5
                                }
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
                        title: "Task Traces"
                        subtitle: "Read-only traces and latest task metadata."
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

                    SectionTitle {
                        title: "Queue / Lifecycle"
                        subtitle: "Queue counts and latest lifecycle transition."
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: true
                        label: "Queue"
                        value: agentsPage.viewModel.agentTaskQueueCount
                               + " queued / "
                               + agentsPage.viewModel.agentTaskQueueBlockedCount
                               + " blocked / "
                               + agentsPage.viewModel.agentTaskQueueRefusedCount
                               + " refused"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: true
                        label: "Lifecycle"
                        value: agentsPage.viewModel.latestAgentTaskLifecycleSummary
                        Layout.fillWidth: true
                    }

                    SectionTitle {
                        title: "Planning / Arbitration"
                        subtitle: "Planner selection and refusal summaries."
                        Layout.fillWidth: true
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

                    SectionTitle {
                        title: "Capability Readiness"
                        subtitle: "Capabilities are descriptive metadata, not grants."
                        Layout.fillWidth: true
                    }

                    Repeater {
                        model: agentsPage.viewModel.agentCapabilityReadinessSummaries
                        InfoRow {
                            required property string modelData
                            compact: true
                            label: "Readiness"
                            value: modelData
                            Layout.fillWidth: true
                        }
                    }

                    SectionTitle {
                        title: "Tool Contracts"
                        subtitle: "Permission and sandbox contracts do not execute tools."
                        Layout.fillWidth: true
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

                    SectionTitle {
                        title: "Voice Runtime"
                        subtitle: "Voice pipeline readiness metadata only."
                        Layout.fillWidth: true
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

            Item {
                width: parent.width
                height: SentinelTheme.space2Xl
            }
        }
    }
}
