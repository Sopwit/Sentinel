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
    property string controlledTaskGoal: ""
    property string selectedControlledTaskId: ""

    function sectionLabel(section) {
        if (section === "Overview")
            return qsTr("Overview")
        if (section === "Tasks")
            return qsTr("Tasks")
        if (section === "Capabilities")
            return qsTr("Capabilities")
        if (section === "Developer")
            return qsTr("Developer")
        return section
    }
    onDeveloperModeChanged: {
        if (!developerMode && selectedSection === "Developer")
            selectedSection = "Overview"
    }

    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical: ScrollBar {
        id: agentsPageScrollBar
        policy: ScrollBar.AsNeeded
        contentItem: Rectangle {
            implicitWidth: 4
            radius: 2
            color: SentinelTheme.withAlpha(agentsPage.modeAccent, agentsPageScrollBar.active ? 0.34 : 0.18)
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
                                text: agentsPage.sectionLabel(modelData)
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
                              ? qsTr("Metadata-only; no execution active. Developer internals are visible.")
                              : qsTr("Metadata-only; no execution active.")
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
                        title: qsTr("Registered Profiles")
                        subtitle: qsTr("Static profiles, not active workers.")
                        Layout.fillWidth: true
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        StatusChip {
                            label: qsTr("Registered")
                            value: agentsPage.viewModel.agentRuntimeAgentCount.toString()
                            accent: agentsPage.modeAccent
                            selected: true
                        }

                        StatusChip {
                            label: qsTr("Activity")
                            value: agentsPage.viewModel.agentActivityCount.toString()
                            accent: SentinelTheme.accentSecondary
                            muted: agentsPage.viewModel.agentActivityCount === 0
                        }

                        StatusChip {
                            label: qsTr("Approval")
                            value: agentsPage.viewModel.agentRuntimeApprovalPosture
                            accent: SentinelTheme.warning
                            muted: false
                        }
                    }

                    InfoRow {
                        compact: agentsPage.compact
                        label: qsTr("Runtime")
                        value: agentsPage.viewModel.agentRuntimeSummary
                        Layout.fillWidth: true
                        valueMaximumLineCount: 4
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Agent execution is disabled. Planning surfaces are dry-run metadata only.")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                    }

                    Repeater {
                        model: Math.min(3, agentsPage.viewModel.modelRoleAssignmentSummaries.length)

                        InfoRow {
                            required property int index
                            compact: agentsPage.compact
                            label: qsTr("Model Role")
                            value: agentsPage.viewModel.modelRoleAssignmentSummaries[index]
                            Layout.fillWidth: true
                        }
                    }

                    Repeater {
                        model: agentsPage.viewModel.agentRuntimeAgentSummaries

                        Rectangle {
                            required property string modelData
                            Layout.fillWidth: true
                            radius: SentinelTheme.radiusMd
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.024)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.050)
                            implicitHeight: agentCatalogLabel.implicitHeight + SentinelTheme.spaceMd

                            Label {
                                id: agentCatalogLabel
                                x: SentinelTheme.spaceSm
                                y: SentinelTheme.spaceXs
                                width: parent.width - SentinelTheme.spaceSm * 2
                                text: modelData
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }

            ShellPanel {
                width: parent.width
                visible: agentsPage.selectedSection === "Overview"
                implicitHeight: agentsPage.sectionHeight(agentPlanPreviewContent)

                ColumnLayout {
                    id: agentPlanPreviewContent
                    x: agentsPage.panelPadding
                    y: agentsPage.panelPadding
                    width: parent.width - agentsPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                    SectionTitle {
                        title: qsTr("Plan Preview")
                        subtitle: qsTr("Inspectable dry-run plan. Approval cannot enable execution.")
                        Layout.fillWidth: true
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        StatusChip {
                            label: qsTr("Plan")
                            value: agentsPage.viewModel.agentPlanId
                            accent: agentsPage.modeAccent
                            selected: true
                        }

                        StatusChip {
                            label: qsTr("Risk")
                            value: agentsPage.viewModel.agentPlanEstimatedRisk
                            accent: agentsPage.viewModel.agentPlanEstimatedRisk === "Critical"
                                    || agentsPage.viewModel.agentPlanEstimatedRisk === "High"
                                    ? SentinelTheme.warning
                                    : SentinelTheme.calmAccent
                            muted: false
                        }

                        StatusChip {
                            label: qsTr("Approval")
                            value: agentsPage.viewModel.agentPlanApprovalState
                            accent: SentinelTheme.warning
                            muted: false
                        }
                    }

                    InfoRow {
                        compact: agentsPage.compact
                        label: qsTr("Goal")
                        value: agentsPage.viewModel.agentPlanGoalSummary
                        Layout.fillWidth: true
                        valueMaximumLineCount: 4
                    }

                    InfoRow {
                        compact: agentsPage.compact
                        label: qsTr("Steps")
                        value: agentsPage.viewModel.agentPlanSteps.join("\n")
                        Layout.fillWidth: true
                        valueMaximumLineCount: 8
                    }

                    InfoRow {
                        compact: agentsPage.compact
                        label: qsTr("Tools")
                        value: agentsPage.viewModel.agentPlanRequiredTools.join("\n")
                        Layout.fillWidth: true
                        valueMaximumLineCount: 8
                    }

                    InfoRow {
                        compact: agentsPage.compact
                        label: qsTr("Permissions")
                        value: agentsPage.viewModel.agentPlanRequiredPermissions.join("\n")
                        Layout.fillWidth: true
                        valueMaximumLineCount: 6
                    }

                    InfoRow {
                        compact: agentsPage.compact
                        label: qsTr("Refusal")
                        value: agentsPage.viewModel.agentPlanRefusalReason
                        Layout.fillWidth: true
                        valueMaximumLineCount: 4
                    }
                }
            }

            ShellPanel {
                width: parent.width
                visible: agentsPage.selectedSection === "Overview"
                implicitHeight: agentsPage.sectionHeight(toolGatewayOverviewContent)

                ColumnLayout {
                    id: toolGatewayOverviewContent
                    x: agentsPage.panelPadding
                    y: agentsPage.panelPadding
                    width: parent.width - agentsPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                    SectionTitle {
                        title: qsTr("Tool Gateway")
                        subtitle: qsTr("Tool readiness and permission posture only.")
                        Layout.fillWidth: true
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        StatusChip {
                            label: qsTr("Tools")
                            value: agentsPage.viewModel.toolGatewayToolCount.toString()
                            accent: agentsPage.modeAccent
                            selected: true
                        }

                        StatusChip {
                            label: qsTr("Metadata")
                            value: agentsPage.viewModel.toolGatewayMetadataSafeCount.toString()
                            accent: SentinelTheme.calmAccent
                        }

                        StatusChip {
                            label: qsTr("Refused")
                            value: agentsPage.viewModel.toolGatewayRefusedCount.toString()
                            accent: agentsPage.viewModel.toolGatewayRefusedCount > 0
                                    ? SentinelTheme.warning
                                    : SentinelTheme.textMuted
                            muted: agentsPage.viewModel.toolGatewayRefusedCount === 0
                        }

                        StatusChip {
                            label: qsTr("Posture")
                            value: agentsPage.viewModel.toolGatewayPermissionPosture
                            accent: SentinelTheme.textMuted
                            muted: true
                        }
                    }

                    InfoRow {
                        compact: agentsPage.compact
                        label: qsTr("Boundary")
                        value: agentsPage.viewModel.toolGatewaySummary
                        Layout.fillWidth: true
                        valueMaximumLineCount: 4
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
                    implicitHeight: agentsPage.sectionHeight(controlledTasksContent)
                    Layout.preferredHeight: 520

                    ColumnLayout {
                        id: controlledTasksContent
                        x: agentsPage.panelPadding
                        y: agentsPage.panelPadding
                        width: parent.width - agentsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Controlled Agent Tasks")
                            subtitle: qsTr("User-approved, one visible step at a time.")
                            Layout.fillWidth: true
                        }

                        SentinelTextField {
                            Layout.fillWidth: true
                            placeholderText: qsTr("Describe a multi-step task")
                            text: agentsPage.controlledTaskGoal
                            onTextChanged: agentsPage.controlledTaskGoal = text
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            SentinelButton {
                                text: qsTr("Plan")
                                onClicked: {
                                    var id = agentsPage.viewModel.planControlledAgentTask(
                                                agentsPage.controlledTaskGoal)
                                    if (id.length > 0)
                                        agentsPage.selectedControlledTaskId = id
                                }
                            }

                            SentinelButton {
                                text: qsTr("Approve")
                                enabled: agentsPage.selectedControlledTaskId.length > 0
                                onClicked: agentsPage.viewModel.approveControlledAgentTask(
                                               agentsPage.selectedControlledTaskId, "Approve Once")
                            }

                            SentinelButton {
                                text: qsTr("Start")
                                enabled: agentsPage.selectedControlledTaskId.length > 0
                                onClicked: agentsPage.viewModel.startControlledAgentTask(
                                               agentsPage.selectedControlledTaskId)
                            }

                            SentinelButton {
                                text: qsTr("Run Step")
                                enabled: agentsPage.selectedControlledTaskId.length > 0
                                onClicked: agentsPage.viewModel.executeControlledAgentStep(
                                               agentsPage.selectedControlledTaskId)
                            }

                            SentinelButton {
                                text: qsTr("Skip")
                                enabled: agentsPage.selectedControlledTaskId.length > 0
                                onClicked: agentsPage.viewModel.skipControlledAgentStep(
                                               agentsPage.selectedControlledTaskId)
                            }

                            SentinelButton {
                                text: qsTr("Retry")
                                enabled: agentsPage.selectedControlledTaskId.length > 0
                                onClicked: agentsPage.viewModel.retryControlledAgentStep(
                                               agentsPage.selectedControlledTaskId)
                            }

                            SentinelButton {
                                text: qsTr("Cancel")
                                enabled: agentsPage.selectedControlledTaskId.length > 0
                                onClicked: agentsPage.viewModel.cancelControlledAgentTask(
                                               agentsPage.selectedControlledTaskId)
                            }
                        }

                        InfoRow {
                            compact: true
                            label: qsTr("Active")
                            value: agentsPage.viewModel.controlledTaskActiveSummary
                            Layout.fillWidth: true
                            valueMaximumLineCount: 4
                        }

                        InfoRow {
                            compact: true
                            label: qsTr("Current Step")
                            value: agentsPage.viewModel.controlledTaskCurrentStep
                            Layout.fillWidth: true
                            valueMaximumLineCount: 4
                        }

                        InfoRow {
                            compact: true
                            label: qsTr("Progress")
                            value: agentsPage.viewModel.controlledTaskProgressSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: qsTr("Plan")
                            value: agentsPage.viewModel.controlledTaskPlanSteps.join("\n")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 8
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
                            title: qsTr("Task Queue")
                            subtitle: qsTr("Queued lifecycle metadata.")
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: qsTr("Queued")
                                value: agentsPage.viewModel.agentTaskQueueCount.toString()
                                accent: agentsPage.modeAccent
                                selected: true
                            }

                            StatusChip {
                                label: qsTr("Planned")
                                value: agentsPage.viewModel.agentTaskQueuePlannedCount.toString()
                                accent: SentinelTheme.accentSecondary
                            }

                            StatusChip {
                                label: qsTr("Blocked")
                                value: agentsPage.viewModel.agentTaskQueueBlockedCount.toString()
                                accent: agentsPage.viewModel.agentTaskQueueBlockedCount > 0
                                        ? SentinelTheme.warning
                                        : SentinelTheme.textMuted
                                muted: agentsPage.viewModel.agentTaskQueueBlockedCount === 0
                            }

                            StatusChip {
                                label: qsTr("Refused")
                                value: agentsPage.viewModel.agentTaskQueueRefusedCount.toString()
                                accent: agentsPage.viewModel.agentTaskQueueRefusedCount > 0
                                        ? SentinelTheme.warning
                                        : SentinelTheme.textMuted
                                muted: agentsPage.viewModel.agentTaskQueueRefusedCount === 0
                            }
                        }

                        InfoRow {
                            compact: true
                            label: qsTr("Lifecycle")
                            value: agentsPage.viewModel.controlledTaskQueueSummaries.join("\n")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 8
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            SentinelButton {
                                text: qsTr("Move Up")
                                enabled: agentsPage.selectedControlledTaskId.length > 0
                                onClicked: agentsPage.viewModel.reorderControlledAgentTask(
                                               agentsPage.selectedControlledTaskId, 0)
                            }

                            SentinelButton {
                                text: qsTr("Export MD")
                                enabled: agentsPage.selectedControlledTaskId.length > 0
                                onClicked: agentsPage.viewModel.exportControlledAgentTask(
                                               agentsPage.selectedControlledTaskId, "Markdown")
                            }

                            SentinelButton {
                                text: qsTr("Export JSON")
                                enabled: agentsPage.selectedControlledTaskId.length > 0
                                onClicked: agentsPage.viewModel.exportControlledAgentTask(
                                               agentsPage.selectedControlledTaskId, "JSON")
                            }
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
                            title: qsTr("Planning Sessions")
                            subtitle: qsTr("Safety arbitration metadata only.")
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: qsTr("Status")
                                value: agentsPage.viewModel.agentPlanningSessionStatus
                                accent: agentsPage.modeAccent
                                selected: true
                            }

                            StatusChip {
                                label: qsTr("Candidates")
                                value: agentsPage.viewModel.agentPlanningCandidateCount.toString()
                                accent: SentinelTheme.accentSecondary
                            }

                            StatusChip {
                                label: qsTr("Refused")
                                value: agentsPage.viewModel.agentPlanningRefusedCount.toString()
                                accent: agentsPage.viewModel.agentPlanningRefusedCount > 0
                                        ? SentinelTheme.warning
                                        : SentinelTheme.textMuted
                                muted: agentsPage.viewModel.agentPlanningRefusedCount === 0
                            }
                        }

                        InfoRow {
                            compact: true
                            label: qsTr("Summary")
                            value: agentsPage.viewModel.controlledTaskExplainabilitySummaries.join("\n")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 10
                        }

                        InfoRow {
                            compact: true
                            label: qsTr("Permissions")
                            value: agentsPage.viewModel.controlledTaskPermissionSummaries.join("\n")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 8
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            SentinelButton {
                                text: qsTr("Allow Notes")
                                onClicked: agentsPage.viewModel.setControlledToolPermission(
                                               "Notes", "Allow For Workspace")
                            }

                            SentinelButton {
                                text: qsTr("Deny Terminal")
                                onClicked: agentsPage.viewModel.setControlledToolPermission(
                                               "Terminal", "Deny")
                            }
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
                            title: qsTr("Capability Registry")
                            subtitle: qsTr("Capabilities are labels, not grants.")
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: qsTr("Enabled")
                                value: agentsPage.viewModel.agentCapabilityEnabledCount.toString()
                                accent: SentinelTheme.success
                            }

                            StatusChip {
                                label: qsTr("Disabled")
                                value: agentsPage.viewModel.agentCapabilityDisabledCount.toString()
                                accent: SentinelTheme.textMuted
                                muted: true
                            }

                            StatusChip {
                                label: qsTr("Restricted")
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

                    InfoRow {
                        compact: true
                        label: "Runtime Model"
                        value: agentsPage.viewModel.activeRuntimeProviderLabel + " / "
                               + agentsPage.viewModel.modelRegistrySummary
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
                        title: "Tool Gateway"
                        subtitle: "Registry diagnostics are read-only and non-executing."
                        Layout.fillWidth: true
                    }

                    Repeater {
                        model: agentsPage.viewModel.toolGatewayDeveloperDiagnostics
                        InfoRow {
                            required property string modelData
                            compact: true
                            label: "Gateway"
                            value: modelData
                            Layout.fillWidth: true
                        }
                    }

                    SectionTitle {
                        title: "Agent Runtime"
                        subtitle: "Dry-run plan diagnostics; no approval path executes."
                        Layout.fillWidth: true
                    }

                    Repeater {
                        model: agentsPage.viewModel.agentRuntimeDeveloperDiagnostics
                        InfoRow {
                            required property string modelData
                            compact: true
                            label: "Runtime"
                            value: modelData
                            Layout.fillWidth: true
                        }
                    }

                    Repeater {
                        model: agentsPage.viewModel.agentPlanDiagnostics
                        InfoRow {
                            required property string modelData
                            compact: true
                            label: "Plan"
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
