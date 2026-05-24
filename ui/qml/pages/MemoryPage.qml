import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ScrollView {
    id: memoryPage
    required property var viewModel
    readonly property bool compact: width < 720
    readonly property int panelPadding: SentinelTheme.spaceLg
    readonly property bool developerMode: viewModel.developerModeEnabled
    readonly property bool retrievalActive: viewModel.retrievalPlanningSelectedSourceCount > 0
    readonly property bool contextActive: viewModel.contextAssemblyAvailableSourceCount > 0
    readonly property string uiSelfCheck: "developer-gated grouped-diagnostics bottom-safe-scroll"
    property string selectedSection: "Overview"
    onDeveloperModeChanged: {
        if (!developerMode && selectedSection === "Developer")
            selectedSection = "Overview"
    }
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical: ScrollBar {
        id: memoryPageScrollBar
        policy: ScrollBar.AsNeeded
        contentItem: Rectangle {
            implicitWidth: 4
            radius: 2
            color: SentinelTheme.withAlpha(SentinelTheme.modeAccent(memoryPage.viewModel.currentModeName),
                                           memoryPageScrollBar.active ? 0.34 : 0.18)
        }
        background: Rectangle {
            color: "transparent"
        }
    }

    contentWidth: availableWidth

    Item {
        width: memoryPage.availableWidth
        implicitHeight: memoryColumn.implicitHeight

        Column {
            id: memoryColumn
            width: parent.width
            spacing: SentinelTheme.contentSpacing(memoryPage.width)

            ShellPanel {
                width: parent.width
                implicitHeight: memoryTabs.implicitHeight + SentinelTheme.spaceMd * 2

                ColumnLayout {
                    id: memoryTabs
                    x: SentinelTheme.spaceMd
                    y: SentinelTheme.spaceMd
                    width: parent.width - SentinelTheme.spaceMd * 2
                    spacing: SentinelTheme.spaceSm

                    Flow {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        Repeater {
                            model: memoryPage.developerMode
                                   ? ["Overview", "Recall", "Local Data", "Developer"]
                                   : ["Overview", "Recall", "Local Data"]

                            Button {
                                id: memoryTabButton
                                required property string modelData
                                readonly property bool active: memoryPage.selectedSection === memoryTabButton.modelData
                                text: modelData
                                hoverEnabled: true
                                onClicked: memoryPage.selectedSection = modelData

                                contentItem: Label {
                                    text: memoryTabButton.text
                                    color: memoryTabButton.active
                                           ? SentinelTheme.textPrimary
                                           : SentinelTheme.textMuted
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: SentinelTheme.fontSmall
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    implicitWidth: 112
                                    implicitHeight: 32
                                    radius: 16
                                    color: InteractionTokens.surfaceColor(memoryTabButton.hovered, memoryTabButton.down,
                                                                           memoryTabButton.active,
                                                                           SentinelTheme.modeAccent(memoryPage.viewModel.currentModeName))
                                    border.color: InteractionTokens.borderColor(memoryTabButton.activeFocus, memoryTabButton.hovered,
                                                                                 memoryTabButton.active,
                                                                                 SentinelTheme.modeAccent(memoryPage.viewModel.currentModeName))

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
                        visible: false
                        text: "Developer metadata is hidden. Enable Developer Mode in Settings to view read-only internals."
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                    }
                }
            }

            ShellPanel {
                width: parent.width
                visible: memoryPage.selectedSection === "Overview"
                implicitHeight: memoryStatusColumn.implicitHeight + memoryPage.panelPadding * 2
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.040)

                ColumnLayout {
                    id: memoryStatusColumn
                    x: memoryPage.panelPadding
                    y: memoryPage.panelPadding
                    width: parent.width - memoryPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Key-value Store"
                        value: memoryPage.viewModel.memoryStatus + " (" + memoryPage.viewModel.memoryMaintenanceStatus + ")"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Chat History"
                        value: memoryPage.viewModel.chatHistoryStatus + " (" + memoryPage.viewModel.chatMaintenanceStatus + ")"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Semantic Memory"
                        value: "Planned; disabled in normal use"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Context Quality"
                        value: memoryPage.viewModel.conversationSalienceIncludedCount
                               + " included / "
                               + memoryPage.viewModel.conversationSalienceExcludedCount
                               + " excluded / "
                               + memoryPage.viewModel.conversationSalienceAllocationSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        visible: memoryPage.viewModel.conversationCompressionStatus !== "Not Needed"
                                 || memoryPage.viewModel.conversationCompressionCandidateCount > 0
                        label: "Conversation Compression"
                        value: memoryPage.viewModel.conversationCompressionStatus + " - "
                               + memoryPage.viewModel.conversationCompressionPressureSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Summary Readiness"
                        value: memoryPage.viewModel.conversationSummaryGenerationStatus + " - "
                               + memoryPage.viewModel.conversationSummaryReadinessSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Continuity"
                        value: memoryPage.viewModel.summaryContinuityStatus + " - "
                               + memoryPage.viewModel.summaryContinuityContributionSummary
                        Layout.fillWidth: true
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        StatusChip {
                            label: "Deterministic retrieval"
                            value: "active"
                            accent: SentinelTheme.success
                            active: memoryPage.retrievalActive
                            selected: true
                        }

                        StatusChip {
                            label: "Prompt injection"
                            value: memoryPage.viewModel.promptContextInjectionEnabled ? "opt-in on"
                                                                                      : "opt-in off"
                            accent: memoryPage.viewModel.promptContextInjectionEnabled ? SentinelTheme.success
                                                                                       : SentinelTheme.textMuted
                            muted: !memoryPage.viewModel.promptContextInjectionEnabled
                            active: memoryPage.viewModel.promptContextInjectionEnabled && memoryPage.contextActive
                        }

                    }

                    Label {
                        Layout.fillWidth: true
                        visible: memoryPage.viewModel.memoryEntryCount === 0
                        text: "Memory is empty until you save a local key-value note."
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                    }
                }
            }

            ShellPanel {
                width: parent.width
                visible: memoryPage.selectedSection === "Developer" && memoryPage.developerMode
                implicitHeight: contextPipelineColumn.implicitHeight + memoryPage.panelPadding * 2
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.034)

                ColumnLayout {
                    id: contextPipelineColumn
                    x: memoryPage.panelPadding
                    y: memoryPage.panelPadding
                    width: parent.width - memoryPage.panelPadding * 2
                    spacing: SentinelTheme.spaceMd

                    SectionTitle {
                        title: "Developer Context"
                        subtitle: "Committed memory feeds deterministic recall, assembly, and retrieval planning. Semantic retrieval remains disabled."
                        Layout.fillWidth: true
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: memoryPage.compact ? 1 : 2
                        columnSpacing: SentinelTheme.spaceSm
                        rowSpacing: SentinelTheme.spaceSm

                        InfoRow {
                            compact: true
                            label: "Committed Memory"
                            value: memoryPage.viewModel.memoryEntryCount + " key-value entries"
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Recall"
                            value: memoryPage.viewModel.memoryRecallStatus + " / "
                                   + memoryPage.viewModel.memoryRecallResultCount + " matches"
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Context Assembly"
                            value: memoryPage.viewModel.contextAssemblyStatus + " / "
                                   + memoryPage.viewModel.contextAssemblyAvailableSourceCount
                                   + " sources / "
                                   + memoryPage.viewModel.contextAssemblyCandidateBlockCount
                                   + " blocks"
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Retrieval Planning"
                            value: memoryPage.viewModel.retrievalPlanningStatus + " / "
                                   + memoryPage.viewModel.retrievalPlanningSelectedSourceCount
                                   + " selected sources / "
                                   + memoryPage.viewModel.retrievalPlanningExcludedCandidateCount
                                   + " excluded"
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Salience Budget"
                            value: memoryPage.viewModel.conversationSalienceAllocationSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Salience Counts"
                            value: memoryPage.viewModel.conversationSalienceIncludedCount
                                   + " included / "
                                   + memoryPage.viewModel.conversationSalienceExcludedCount
                                   + " excluded / "
                                   + memoryPage.viewModel.conversationSalienceTruncatedCount
                                   + " truncated"
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Semantic Readiness"
                            value: memoryPage.viewModel.semanticRetrievalStatus + " / "
                                   + memoryPage.viewModel.selectedSemanticProviderName + " / "
                                   + memoryPage.viewModel.semanticActivationReadiness
                            Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: true
                        valueMaximumLineCount: 8
                        label: "Salience Trace"
                        value: memoryPage.viewModel.conversationSalienceTraceSummaries.length === 0
                               ? "No prompt context assembled yet."
                               : memoryPage.viewModel.conversationSalienceTraceSummaries.join(" / ")
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: true
                        valueMaximumLineCount: 8
                        label: "Memory Trace"
                        value: memoryPage.viewModel.memoryRelevanceTraceSummaries.length === 0
                               ? "No memory relevance evaluated yet."
                               : memoryPage.viewModel.memoryRelevanceTraceSummaries.join(" / ")
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: true
                        valueMaximumLineCount: 8
                        label: "Memory Exclusions"
                        value: memoryPage.viewModel.memoryRelevanceExclusionSummaries.length === 0
                               ? "No memory exclusions."
                               : memoryPage.viewModel.memoryRelevanceExclusionSummaries.join(" / ")
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: true
                        valueMaximumLineCount: 8
                        label: "Provider Capability"
                        value: memoryPage.viewModel.semanticProviderReadiness + " / "
                               + memoryPage.viewModel.semanticProviderCapabilitySummaries.join(", ")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Candidate Boundary"
                            value: memoryPage.viewModel.memoryCandidateCount
                                   + " candidates / "
                                   + memoryPage.viewModel.committedMemoryCandidateCount
                                   + " committed candidates / approval is not storage"
                            Layout.fillWidth: true
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        StatusChip {
                            label: "Candidates"
                            value: "review metadata"
                            accent: SentinelTheme.accentSecondary
                            selected: memoryPage.viewModel.memoryCandidateCount > 0
                        }

                        StatusChip {
                            label: "Committed"
                            value: "local key-value memory"
                            accent: SentinelTheme.success
                            selected: memoryPage.viewModel.memoryEntryCount > 0
                        }

                        StatusChip {
                            label: "Semantic"
                            value: memoryPage.viewModel.semanticProviderMode
                            accent: SentinelTheme.textMuted
                            muted: true
                            selected: true
                        }
                    }
                }
            }

            ShellPanel {
                width: parent.width
                visible: memoryPage.selectedSection === "Recall"
                implicitHeight: recallColumn.implicitHeight + memoryPage.panelPadding * 2

                ColumnLayout {
                    id: recallColumn
                    x: memoryPage.panelPadding
                    y: memoryPage.panelPadding
                    width: parent.width - memoryPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                    SectionTitle {
                        title: "Local Memory Recall"
                        subtitle: "Searches saved key-value memory only; it does not search chats."
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Recall Policy"
                        value: memoryPage.viewModel.memoryRecallPolicyStatus + " - "
                               + memoryPage.viewModel.memoryRecallPolicySummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Recall Status"
                        value: memoryPage.viewModel.memoryRecallStatus + " - "
                               + memoryPage.viewModel.memoryRecallSummaryText
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Memory Entries"
                        value: memoryPage.viewModel.memoryEntryCount + " committed / "
                               + memoryPage.viewModel.memoryRecallResultCount + " recall matches"
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        SentinelTextField {
                            id: memoryRecallQuery
                            Layout.fillWidth: true
                            placeholderText: "literal key or value"
                            onAccepted: memoryPage.viewModel.recallLocalMemory(text)
                        }

                        SentinelButton {
                            text: "Recall"
                            enabled: memoryRecallQuery.text.trim().length > 0
                            Layout.preferredWidth: 96
                            onClicked: memoryPage.viewModel.recallLocalMemory(memoryRecallQuery.text)
                        }

                        SentinelButton {
                            text: "Clear"
                            Layout.preferredWidth: 96
                            onClicked: {
                                memoryRecallQuery.clear()
                                memoryPage.viewModel.clearLocalMemoryRecall()
                            }
                        }
                    }

                    Repeater {
                        model: memoryPage.viewModel.memoryRecallResultSummaries

                        Label {
                            required property string modelData
                            text: modelData
                            color: SentinelTheme.textPrimary
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }

                    Label {
                        visible: memoryPage.viewModel.memoryRecallResultCount === 0
                                 && memoryPage.viewModel.memoryRecallStatus === "Completed"
                        text: "No saved local memory matched that query."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            ShellPanel {
                width: parent.width
                visible: memoryPage.selectedSection === "Developer" && memoryPage.developerMode
                implicitHeight: contextAssemblyColumn.implicitHeight + memoryPage.panelPadding * 2

                ColumnLayout {
                    id: contextAssemblyColumn
                    x: memoryPage.panelPadding
                    y: memoryPage.panelPadding
                    width: parent.width - memoryPage.panelPadding * 2
                    spacing: SentinelTheme.spaceMd

                    SectionTitle {
                        title: "Context"
                        subtitle: "Planning metadata only. Prompt assembly and automatic attachment remain disabled."
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Policy"
                        value: memoryPage.viewModel.contextAssemblyPolicyStatus + " - "
                               + memoryPage.viewModel.contextAssemblyPolicySummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Readiness"
                        value: memoryPage.viewModel.contextAssemblyStatus + " - "
                               + memoryPage.viewModel.contextAssemblySummaryText
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Sources"
                        value: memoryPage.viewModel.contextAssemblyAvailableSourceCount + " available / "
                               + memoryPage.viewModel.contextAssemblySourceCount + " requested / "
                               + memoryPage.viewModel.contextAssemblyCandidateBlockCount + " blocks / ~"
                               + memoryPage.viewModel.contextAssemblyEstimatedSize + " chars"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Availability"
                        value: "Conversation " + memoryPage.viewModel.conversationContextAvailability
                               + " / Memory " + memoryPage.viewModel.committedMemoryContextAvailability
                               + " / Runtime " + memoryPage.viewModel.runtimeMetadataContextAvailability
                               + " / Orchestration " + memoryPage.viewModel.orchestrationContextAvailability
                        Layout.fillWidth: true
                    }

                    Repeater {
                        model: memoryPage.viewModel.contextAssemblySourceSummaries

                        Label {
                            required property string modelData
                            text: modelData
                            color: SentinelTheme.textMuted
                            wrapMode: Text.WordWrap
                            maximumLineCount: 5
                            Layout.fillWidth: true
                        }
                    }

                    SectionTitle {
                        title: "Retrieval"
                        subtitle: "Deterministic candidate selection and budget metadata."
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Retrieval Planning"
                        value: memoryPage.viewModel.retrievalPlanningStatus + " - "
                               + memoryPage.viewModel.retrievalPlanningSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Salience Budget"
                        value: memoryPage.viewModel.conversationSalienceBudgetSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Salience Allocation"
                        value: memoryPage.viewModel.conversationSalienceAllocationSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Retrieval Sources"
                        value: memoryPage.viewModel.retrievalPlanningSelectedSourceCount
                               + " selected / "
                               + memoryPage.viewModel.retrievalPlanningExcludedSourceCount
                               + " excluded / "
                               + memoryPage.viewModel.retrievalPlanningSelectedCandidateCount
                               + " blocks"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        valueMaximumLineCount: 8
                        label: "Salience Trace"
                        value: memoryPage.viewModel.conversationSalienceTraceSummaries.length === 0
                               ? "No salience evaluated yet."
                               : memoryPage.viewModel.conversationSalienceTraceSummaries.join(" / ")
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        valueMaximumLineCount: 8
                        label: "Salience Exclusions"
                        value: memoryPage.viewModel.conversationSalienceExclusionSummaries.length === 0
                               ? "No salience exclusions."
                               : memoryPage.viewModel.conversationSalienceExclusionSummaries.join(" / ")
                        Layout.fillWidth: true
                    }

                    SectionTitle {
                        title: "Conversation Summary Pipeline"
                        subtitle: "Manual-only summary preparation metadata; generation execution is unavailable."
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Summary Availability"
                        value: (memoryPage.viewModel.conversationSummaryAvailable ? "Available" : "Unavailable")
                               + " / " + memoryPage.viewModel.conversationSummaryBlockedReason
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Summary Gain"
                        value: memoryPage.viewModel.conversationSummaryEstimatedCompressionGain
                               + " / " + memoryPage.viewModel.conversationSummaryPreviewSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Summary Persistence"
                        value: memoryPage.viewModel.conversationSummaryPersistenceSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Summary Inclusion"
                        value: memoryPage.viewModel.conversationSummaryInjectionSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Freshness"
                        value: memoryPage.viewModel.summaryContinuityFreshnessSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Coverage"
                        value: memoryPage.viewModel.summaryContinuityCoverageSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Ordering"
                        value: memoryPage.viewModel.summaryContinuityOrderingSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Context Reasoning"
                        value: memoryPage.viewModel.contextReasoningSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Contribution"
                        valueMaximumLineCount: 8
                        value: memoryPage.viewModel.contextReasoningContributionSummaries.length === 0
                               ? "No context contributions recorded."
                               : memoryPage.viewModel.contextReasoningContributionSummaries.join(" / ")
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Fallback"
                        value: memoryPage.viewModel.contextReasoningFallbackSummary
                        Layout.fillWidth: true
                    }

                    TextArea {
                        id: contextReasoningDiagnostics
                        Layout.fillWidth: true
                        Layout.preferredHeight: memoryPage.viewModel.developerModeEnabled ? 132 : 86
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.Wrap
                        text: memoryPage.viewModel.developerModeEnabled
                              ? memoryPage.viewModel.contextReasoningDeveloperTraces.join("\n")
                              : memoryPage.viewModel.contextReasoningInclusionHints.concat(
                                    memoryPage.viewModel.contextReasoningExclusionHints).join("\n")
                        color: SentinelTheme.textPrimary
                        selectedTextColor: SentinelTheme.backgroundBase
                        selectionColor: SentinelTheme.modeAccent(memoryPage.viewModel.currentModeName)
                        font.pixelSize: SentinelTheme.fontSmall
                        background: Rectangle {
                            radius: SentinelTheme.radiusMd
                            color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.34)
                            border.color: contextReasoningDiagnostics.activeFocus
                                          ? SentinelTheme.withAlpha(SentinelTheme.modeAccent(memoryPage.viewModel.currentModeName), 0.62)
                                          : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.070)
                        }
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        visible: memoryPage.viewModel.developerModeEnabled
                        label: "Continuity Budget"
                        value: memoryPage.viewModel.summaryContinuityBudgetTrace
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        valueMaximumLineCount: 8
                        label: "Summary Segments"
                        value: memoryPage.viewModel.conversationSummaryCandidateSegments.length === 0
                               ? "No summary segments planned."
                               : memoryPage.viewModel.conversationSummaryCandidateSegments.join(" / ")
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        valueMaximumLineCount: 8
                        label: "Summary Trace"
                        value: memoryPage.viewModel.conversationSummaryGenerationTraceSummaries.length === 0
                               ? "No summary trace prepared."
                               : memoryPage.viewModel.conversationSummaryGenerationTraceSummaries.join(" / ")
                        Layout.fillWidth: true
                    }

                    SectionTitle {
                        title: "Conversation Compression"
                        subtitle: "Readiness-only compression metadata."
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Compression Readiness"
                        value: memoryPage.viewModel.conversationCompressionReadinessSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Compression Budget"
                        value: memoryPage.viewModel.conversationCompressionBudgetSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Compression Selection"
                        value: memoryPage.viewModel.conversationCompressionSelectedCandidateCount
                               + " selected / "
                               + memoryPage.viewModel.conversationCompressionCandidateCount
                               + " candidates / "
                               + memoryPage.viewModel.conversationCompressionFallbackReason
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        valueMaximumLineCount: 8
                        label: "Compression Candidates"
                        value: memoryPage.viewModel.conversationCompressionCandidateSummaries.length === 0
                               ? "No compression candidates planned."
                               : memoryPage.viewModel.conversationCompressionCandidateSummaries.join(" / ")
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        valueMaximumLineCount: 8
                        label: "Compression Trace"
                        value: memoryPage.viewModel.conversationCompressionTraceSummaries.length === 0
                               ? memoryPage.viewModel.conversationCompressionTraceSummary
                               : memoryPage.viewModel.conversationCompressionTraceSummaries.join(" / ")
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Semantic Arbitration"
                        value: memoryPage.viewModel.semanticArbitrationStatus + " - "
                               + memoryPage.viewModel.semanticArbitrationSummary
                        Layout.fillWidth: true
                    }

                    SectionTitle {
                        title: "Semantic / Vector"
                        subtitle: "Readiness-only vector and semantic search metadata."
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Embedding Runtime"
                        value: memoryPage.viewModel.embeddingRuntimeReadiness + " - "
                               + memoryPage.viewModel.embeddingRuntimeBudgetSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Isolated Embedding Test"
                        value: memoryPage.viewModel.isolatedEmbeddingRuntimeStatus + " - "
                               + memoryPage.viewModel.isolatedEmbeddingRuntimeBoundedState
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Vector Persistence"
                        value: memoryPage.viewModel.vectorPersistenceReadiness + " - "
                               + memoryPage.viewModel.vectorPersistenceBoundedState + " / "
                               + memoryPage.viewModel.vectorPersistenceIndexedItemCount
                               + " indexed"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Semantic Search"
                        value: memoryPage.viewModel.semanticSearchStatus + " - "
                               + memoryPage.viewModel.semanticSearchRuntimeState + " / "
                               + memoryPage.viewModel.semanticSearchCandidateCount
                               + " candidates"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Hybrid Bridge"
                        value: memoryPage.viewModel.hybridBridgeStatus + " - "
                               + memoryPage.viewModel.hybridBridgeFallbackSummary + " / "
                               + memoryPage.viewModel.hybridBridgeCandidateCount
                               + " metadata candidates"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Semantic Acceptance"
                        value: memoryPage.viewModel.semanticAcceptanceStatus + " - "
                               + memoryPage.viewModel.semanticAcceptanceFallbackSummary + " / "
                               + memoryPage.viewModel.semanticAcceptanceAcceptedCount
                               + " approved supplements"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Supplement Assembly"
                        value: memoryPage.viewModel.semanticSupplementAssemblyStatus + " - "
                               + memoryPage.viewModel.semanticSupplementAssemblyBudgetSummary
                               + " / "
                               + memoryPage.viewModel.semanticSupplementAssemblyBlockCount
                               + " metadata blocks"
                        Layout.fillWidth: true
                    }

                    SectionTitle {
                        title: "Prompt Authority"
                        subtitle: "Semantic supplements remain non-authoritative."
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Prompt Authority"
                        value: memoryPage.viewModel.semanticPromptAuthorityStatus + " - "
                               + memoryPage.viewModel.semanticPromptAuthorityAuditSummary
                               + " / non-authoritative"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Prompt Inclusion"
                        value: memoryPage.viewModel.semanticPromptInclusionStatus + " - "
                               + memoryPage.viewModel.semanticPromptInclusionFallbackSummary
                               + " / "
                               + memoryPage.viewModel.semanticPromptInclusionIncludedCount
                               + " included supplements"
                        Layout.fillWidth: true
                    }

                    SectionTitle {
                        title: "Diagnostics"
                        subtitle: "Counts and summaries only; no prompts or vectors are exposed."
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Candidate Boundary"
                        value: memoryPage.viewModel.memoryCandidateCount
                               + " candidates / "
                               + memoryPage.viewModel.committedMemoryCandidateCount
                               + " committed candidates"
                        Layout.fillWidth: true
                    }
                }
            }

            ShellPanel {
                width: parent.width
                visible: memoryPage.selectedSection === "Local Data"
                implicitHeight: candidateColumn.implicitHeight + memoryPage.panelPadding * 2

                ColumnLayout {
                    id: candidateColumn
                    x: memoryPage.panelPadding
                    y: memoryPage.panelPadding
                    width: parent.width - memoryPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Memory Candidates"
                        value: memoryPage.viewModel.memoryCandidateCount + " total / "
                               + memoryPage.viewModel.pendingMemoryCandidateCount + " pending / "
                               + memoryPage.viewModel.approvedMemoryCandidateCount + " approved / "
                               + memoryPage.viewModel.rejectedMemoryCandidateCount + " rejected / "
                               + memoryPage.viewModel.archivedMemoryCandidateCount + " archived / "
                               + memoryPage.viewModel.committedMemoryCandidateCount + " committed"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Last Review"
                        value: memoryPage.viewModel.lastMemoryCandidateReviewStatus + " - "
                               + memoryPage.viewModel.lastMemoryCandidateReviewSummary
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "Approved means reviewed metadata, not committed long-term memory."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Commit Readiness"
                        value: memoryPage.viewModel.memoryCommitReadinessStatus + " - "
                               + memoryPage.viewModel.memoryCommitReadinessSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Commit Target"
                        value: memoryPage.viewModel.memoryCommitPlanCount + " planned / "
                               + memoryPage.viewModel.memoryCommitTargetSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Last Commit"
                        value: memoryPage.viewModel.lastMemoryCommitStatus + " - "
                               + memoryPage.viewModel.lastMemoryCommitResultSummary
                        Layout.fillWidth: true
                    }

                    Repeater {
                        model: memoryPage.viewModel.memoryCommitReadinessChecks

                        Label {
                            required property string modelData
                            text: modelData
                            color: SentinelTheme.textMuted
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }

                    Repeater {
                        model: memoryPage.viewModel.memoryCandidateIds

                        ColumnLayout {
                            required property int index
                            required property string modelData
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceXs

                            readonly property string candidateId: modelData
                            readonly property string candidateState: memoryPage.viewModel.memoryCandidateReviewStates[index]
                            readonly property string candidateCommitStatus: memoryPage.viewModel.memoryCandidateCommitStatuses[index]
                            readonly property string candidateSummary: memoryPage.viewModel.memoryCandidateSummaries[index]
                            readonly property string commitSummary: memoryPage.viewModel.memoryCommitCandidateSummaries[index]

                            InfoRow {
                                compact: memoryPage.compact
                                label: "Candidate"
                                value: candidateSummary
                                Layout.fillWidth: true
                            }

                            InfoRow {
                                compact: memoryPage.compact
                                label: "Commit Plan"
                                value: commitSummary
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                visible: candidateState !== "Archived"
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceXs

                                SentinelButton {
                                    text: "Approve"
                                    enabled: candidateState === "Pending Review"
                                    Layout.preferredWidth: 96
                                    onClicked: memoryPage.viewModel.approveMemoryCandidate(candidateId)
                                }

                                SentinelButton {
                                    text: "Reject"
                                    enabled: candidateState === "Pending Review"
                                    Layout.preferredWidth: 96
                                    onClicked: memoryPage.viewModel.rejectMemoryCandidate(candidateId)
                                }

                                SentinelButton {
                                    text: "Reset"
                                    enabled: (candidateState === "Approved" || candidateState === "Rejected")
                                             && candidateCommitStatus !== "Committed"
                                    Layout.preferredWidth: 96
                                    onClicked: memoryPage.viewModel.resetMemoryCandidate(candidateId)
                                }

                                SentinelButton {
                                    visible: candidateState === "Approved"
                                    text: "Commit"
                                    enabled: candidateCommitStatus !== "Committed"
                                    Layout.preferredWidth: 96
                                    onClicked: memoryPage.viewModel.requestMemoryCandidateCommit(candidateId)
                                }

                                Item {
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }

                    Label {
                        visible: memoryPage.viewModel.memoryCandidateCount === 0
                        text: "No memory candidates pending review."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            GridLayout {
                width: parent.width
                visible: memoryPage.selectedSection === "Local Data"
                columns: memoryPage.compact ? 1 : 3
                columnSpacing: SentinelTheme.spaceSm
                rowSpacing: SentinelTheme.spaceSm

                Label {
                    Layout.fillWidth: true
                    Layout.columnSpan: memoryPage.compact ? 1 : 3
                    text: "Key = memory name/topic. Value = saved local note. Store saves local memory only; no cloud or model call."
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                }

                SentinelTextField {
                    id: memoryKey
                    Layout.fillWidth: true
                    Layout.preferredWidth: memoryPage.compact ? -1 : 220
                    placeholderText: "Key: memory name or topic"
                }

                SentinelTextField {
                    id: memoryValue
                    Layout.fillWidth: true
                    placeholderText: "Value: saved local note"
                }

                SentinelButton {
                    text: "Store"
                    enabled: memoryKey.text.trim().length > 0
                    Layout.fillWidth: memoryPage.compact
                    onClicked: {
                        memoryPage.viewModel.remember(memoryKey.text, memoryValue.text)
                        memoryKey.clear()
                        memoryValue.clear()
                    }
                }
            }

            ListView {
                id: memoryList
                width: parent.width
                visible: memoryPage.selectedSection === "Local Data"
                height: Math.min(420, Math.max(220, memoryPage.height - 360))
                clip: true
                spacing: SentinelTheme.spaceSm
                model: memoryPage.viewModel.memoryEntries
                boundsBehavior: Flickable.StopAtBounds
                boundsMovement: Flickable.StopAtBounds
                maximumFlickVelocity: 2200
                flickDeceleration: 5200

                delegate: Rectangle {
                    id: memoryDelegate
                    required property string modelData

                    width: ListView.view.width
                    radius: SentinelTheme.radiusSm
                    color: SentinelTheme.surfaceMuted
                    border.color: SentinelTheme.accentBorderSubtle
                    implicitHeight: memoryText.implicitHeight + 18

                    Text {
                        id: memoryText
                        x: SentinelTheme.spaceSm
                        y: SentinelTheme.spaceSm
                        width: parent.width - SentinelTheme.spaceSm * 2
                        text: memoryDelegate.modelData
                        color: SentinelTheme.textPrimary
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Label {
                width: parent.width
                visible: memoryPage.selectedSection === "Local Data" && memoryList.count === 0
                text: "No saved key-value memory yet. Store saves local memory only; it does not call a model, use cloud, or extract automatically."
                color: SentinelTheme.textMuted
                horizontalAlignment: Text.AlignHCenter
            }

            Item {
                width: parent.width
                height: SentinelTheme.space2Xl
            }
        }
    }
}
