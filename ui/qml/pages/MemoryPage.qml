import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ScrollView {
    id: memoryPage
    required property var viewModel
    readonly property bool compact: width < 720
    readonly property int panelPadding: SentinelTheme.spaceLg
    readonly property bool retrievalActive: viewModel.retrievalPlanningSelectedSourceCount > 0
    readonly property bool contextActive: viewModel.contextAssemblyAvailableSourceCount > 0
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    contentWidth: availableWidth

    Item {
        width: memoryPage.availableWidth
        implicitHeight: memoryColumn.implicitHeight

        Column {
            id: memoryColumn
            width: parent.width
            spacing: SentinelTheme.contentSpacing(memoryPage.width)

            SectionTitle {
                width: parent.width
                title: "Runtime Memory"
                subtitle: "Key-value memory, chat history status, and future semantic memory stay separate."
            }

            ShellPanel {
                width: parent.width
                implicitHeight: memoryStatusColumn.implicitHeight + memoryPage.panelPadding * 2
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.040)
                border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.12)
                bracketColor: SentinelTheme.withAlpha(SentinelTheme.accent, 0.24)
                edgeLightColor: SentinelTheme.withAlpha(SentinelTheme.accent, 0.36)

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
                        value: memoryPage.viewModel.semanticRetrievalStatus + " - "
                               + memoryPage.viewModel.semanticReadiness
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
                            label: "Semantic retrieval"
                            value: "disabled"
                            accent: SentinelTheme.warning
                            muted: true
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

                        StatusChip {
                            label: "Vector index"
                            value: memoryPage.viewModel.vectorIndexedItemCount + " items"
                            accent: SentinelTheme.accentSecondary
                            muted: true
                        }

                        StatusChip {
                            label: "Arbitration"
                            value: memoryPage.viewModel.semanticArbitrationStatus
                            accent: SentinelTheme.textMuted
                            muted: true
                        }
                    }
                }
            }

            ShellPanel {
                width: parent.width
                implicitHeight: contextPipelineColumn.implicitHeight + memoryPage.panelPadding * 2
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.034)
                border.color: SentinelTheme.withAlpha(SentinelTheme.accentTertiary, 0.13)
                bracketColor: SentinelTheme.withAlpha(SentinelTheme.accentTertiary, 0.24)
                edgeLightColor: SentinelTheme.withAlpha(SentinelTheme.accentTertiary, 0.34)

                ColumnLayout {
                    id: contextPipelineColumn
                    x: memoryPage.panelPadding
                    y: memoryPage.panelPadding
                    width: parent.width - memoryPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                    SectionTitle {
                        title: "Context Pipeline"
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
                                   + " selected sources / deterministic"
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
                            label: "Semantic path"
                            value: memoryPage.viewModel.semanticProviderMode
                            accent: SentinelTheme.warning
                            muted: true
                            selected: true
                        }
                    }
                }
            }

            ShellPanel {
                width: parent.width
                implicitHeight: recallColumn.implicitHeight + memoryPage.panelPadding * 2

                ColumnLayout {
                    id: recallColumn
                    x: memoryPage.panelPadding
                    y: memoryPage.panelPadding
                    width: parent.width - memoryPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                    SectionTitle {
                        title: "Local Memory Recall"
                        subtitle: "Literal read-only search over committed key-value memory."
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
                        text: "No committed memory entries matched the recall query."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            ShellPanel {
                width: parent.width
                implicitHeight: contextAssemblyColumn.implicitHeight + memoryPage.panelPadding * 2

                ColumnLayout {
                    id: contextAssemblyColumn
                    x: memoryPage.panelPadding
                    y: memoryPage.panelPadding
                    width: parent.width - memoryPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                    SectionTitle {
                        title: "Context Assembly"
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
                            Layout.fillWidth: true
                        }
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
                        label: "Retrieval Budget"
                        value: memoryPage.viewModel.retrievalPlanningBudgetSummary
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
                        label: "Semantic Arbitration"
                        value: memoryPage.viewModel.semanticArbitrationStatus + " - "
                               + memoryPage.viewModel.semanticArbitrationSummary
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
                }
            }

            ShellPanel {
                width: parent.width
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
                columns: memoryPage.compact ? 1 : 3
                columnSpacing: SentinelTheme.spaceSm
                rowSpacing: SentinelTheme.spaceSm

                SentinelTextField {
                    id: memoryKey
                    Layout.fillWidth: true
                    Layout.preferredWidth: memoryPage.compact ? -1 : 220
                    placeholderText: "key"
                }

                SentinelTextField {
                    id: memoryValue
                    Layout.fillWidth: true
                    placeholderText: "value"
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
                height: Math.min(420, Math.max(220, memoryPage.height - 360))
                clip: true
                spacing: SentinelTheme.spaceSm
                model: memoryPage.viewModel.memoryEntries

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
                visible: memoryList.count === 0
                text: "No key-value memory entries stored."
                color: SentinelTheme.textMuted
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
