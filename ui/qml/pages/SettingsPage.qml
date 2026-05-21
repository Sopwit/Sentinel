import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ScrollView {
    id: settingsPage
    required property var viewModel
    readonly property bool compact: width < 760
    readonly property int panelPadding: SentinelTheme.spaceLg
    property bool showAdvancedContextDetails: false
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    function sectionHeight(content) {
        return content.implicitHeight + panelPadding * 2
    }

    contentWidth: availableWidth

    Item {
        width: settingsPage.availableWidth
        implicitHeight: settingsColumn.implicitHeight

        Column {
            id: settingsColumn
            width: parent.width
            spacing: SentinelTheme.spaceLg

            SectionTitle {
                width: parent.width
                title: "Settings"
                subtitle: "Local preferences and read-only runtime state."
            }

            ShellPanel {
                width: parent.width
                implicitHeight: settingsPage.sectionHeight(generalContent)

                ColumnLayout {
                    id: generalContent
                    x: settingsPage.panelPadding
                    y: settingsPage.panelPadding
                    width: parent.width - settingsPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                    SectionTitle {
                        title: "General"
                        subtitle: "Desktop shell preferences."
                        Layout.fillWidth: true
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: settingsPage.compact ? 1 : 2
                        columnSpacing: SentinelTheme.spaceSm
                        rowSpacing: SentinelTheme.spaceSm

                        Label {
                            text: "Theme"
                            color: SentinelTheme.textMuted
                        }

                        InfoRow {
                            Layout.fillWidth: true
                            compact: true
                            label: "Active"
                            value: settingsPage.viewModel.themeName
                        }

                        Label {
                            text: "Config Profile"
                            color: SentinelTheme.textMuted
                        }

                        InfoRow {
                            Layout.fillWidth: true
                            compact: true
                            label: "Active"
                            value: settingsPage.viewModel.configurationProfile
                        }
                    }

                    InfoRow {
                        compact: settingsPage.compact
                        label: "Storage"
                        value: "Memory " + settingsPage.viewModel.memoryStatus + " / Chat " + settingsPage.viewModel.chatHistoryStatus
                        Layout.fillWidth: true
                    }
                }
            }

            ShellPanel {
                width: parent.width
                implicitHeight: settingsPage.sectionHeight(localAiContent)

                ColumnLayout {
                    id: localAiContent
                    x: settingsPage.panelPadding
                    y: settingsPage.panelPadding
                    width: parent.width - settingsPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: "Local AI / Ollama"
                    subtitle: "Loopback-only health and discovery metadata."
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Endpoint"
                    value: settingsPage.viewModel.ollamaEndpoint
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Health"
                    value: settingsPage.viewModel.ollamaConnectionStatus + " / " + settingsPage.viewModel.ollamaHealthStatus
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Discovered Models"
                    value: settingsPage.viewModel.ollamaModelCount.toString()
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Selected Model"
                    value: settingsPage.viewModel.selectedLocalModelSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Why unavailable"
                    value: settingsPage.viewModel.ollamaModelCount > 0 ? settingsPage.viewModel.ollamaHealthSummary : "Start Ollama and install/select a local model."
                    Layout.fillWidth: true
                }
            }
        }

            ShellPanel {
                width: parent.width
                implicitHeight: settingsPage.sectionHeight(modelContent)

                ColumnLayout {
                    id: modelContent
                    x: settingsPage.panelPadding
                    y: settingsPage.panelPadding
                    width: parent.width - settingsPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: "Model Selection"
                    subtitle: "Selection is persisted configuration only."
                    Layout.fillWidth: true
                }

                ComboBox {
                    Layout.fillWidth: true
                    enabled: settingsPage.viewModel.ollamaModelCount > 0
                    model: settingsPage.viewModel.ollamaModelNames
                    currentIndex: settingsPage.viewModel.ollamaModelNames.indexOf(settingsPage.viewModel.selectedLocalModel)
                    displayText: currentIndex >= 0 ? currentText : settingsPage.viewModel.selectedLocalModelStatus + " / No model selected"
                    onActivated: settingsPage.viewModel.selectedLocalModel = currentText
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Status"
                    value: settingsPage.viewModel.selectedLocalModelStatus
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Selected Metadata"
                    value: settingsPage.viewModel.selectedLocalModelMetadataSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Runtime Badge"
                    value: settingsPage.viewModel.activeLocalRuntimeBadge
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Management"
                    value: settingsPage.viewModel.modelManagementStatus + " / " + settingsPage.viewModel.modelManagementActionAvailability
                    Layout.fillWidth: true
                }

                Repeater {
                    model: Math.min(2, settingsPage.viewModel.modelRecommendationSummaries.length)

                    InfoRow {
                        required property int index
                        compact: settingsPage.compact
                        label: "Recommendation"
                        value: settingsPage.viewModel.modelRecommendationSummaries[index]
                        Layout.fillWidth: true
                    }
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Setup Hint"
                    value: settingsPage.viewModel.ollamaModelCount > 0 ? "Select one discovered local model. Downloads are not available here." : "Start Ollama and install/select a local model."
                    Layout.fillWidth: true
                }
            }
        }

            ShellPanel {
                width: parent.width
                implicitHeight: settingsPage.sectionHeight(chatContent)

                ColumnLayout {
                    id: chatContent
                    x: settingsPage.panelPadding
                    y: settingsPage.panelPadding
                    width: parent.width - settingsPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: "Streaming / Chat"
                    subtitle: "Explicit local chat routing controls."
                    Layout.fillWidth: true
                }

                CheckBox {
                    Layout.fillWidth: true
                    text: "Local chat inference"
                    checked: settingsPage.viewModel.localChatInferenceEnabled
                    onToggled: settingsPage.viewModel.localChatInferenceEnabled = checked
                }

                CheckBox {
                    Layout.fillWidth: true
                    text: "Local response streaming"
                    checked: settingsPage.viewModel.localInferenceStreamingEnabled
                    onToggled: settingsPage.viewModel.localInferenceStreamingEnabled = checked
                }

                CheckBox {
                    Layout.fillWidth: true
                    text: "Use local memory/context in chat"
                    checked: settingsPage.viewModel.promptContextInjectionEnabled
                    onToggled: settingsPage.viewModel.promptContextInjectionEnabled = checked
                }

                CheckBox {
                    Layout.fillWidth: true
                    text: "Semantic supplemental prompt inclusion"
                    checked: settingsPage.viewModel.semanticPromptInclusionEnabled
                    onToggled: settingsPage.viewModel.semanticPromptInclusionEnabled = checked
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceSm

                    StatusChip {
                        label: "Prompt context"
                        value: settingsPage.viewModel.promptContextInjectionEnabled ? "opt-in on"
                                                                                     : "opt-in off"
                        accent: settingsPage.viewModel.promptContextInjectionEnabled ? SentinelTheme.success
                                                                                     : SentinelTheme.textMuted
                        muted: !settingsPage.viewModel.promptContextInjectionEnabled
                    }

                    StatusChip {
                        label: "Retrieval"
                        value: "deterministic"
                        accent: SentinelTheme.accentTertiary
                    }

                    StatusChip {
                        label: "Semantic"
                        value: "disabled"
                        accent: SentinelTheme.warning
                    }

                    StatusChip {
                        label: "Summary"
                        value: settingsPage.viewModel.conversationSummaryStatus
                        accent: SentinelTheme.accent
                    }
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Chat Routing"
                    value: settingsPage.viewModel.localChatInferenceStatus + " / " + settingsPage.viewModel.localChatInferenceSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Inference"
                    value: settingsPage.viewModel.localInferenceRuntimeState + " / " + settingsPage.viewModel.localInferenceStatus
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Streaming"
                    value: settingsPage.viewModel.localInferenceStreamStatus + " / " + settingsPage.viewModel.localInferenceStreamSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Context"
                    value: (settingsPage.viewModel.promptContextInjectionEnabled ? "On / " : "Off / ")
                           + settingsPage.viewModel.promptContextInjectionStatus
                           + " / "
                           + settingsPage.viewModel.promptContextInjectedBlockCount
                           + " blocks"
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Semantic Inclusion"
                    value: (settingsPage.viewModel.semanticPromptInclusionEnabled ? "On / " : "Off / ")
                           + settingsPage.viewModel.semanticPromptInclusionStatus
                           + " / "
                           + settingsPage.viewModel.semanticPromptInclusionIncludedCount
                           + " supplements"
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Window"
                    value: settingsPage.viewModel.conversationWindowStatus
                           + " / "
                           + settingsPage.viewModel.conversationWindowBudgetSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Truncation"
                    value: settingsPage.viewModel.conversationWindowIncludedMessageCount
                           + " included / "
                           + settingsPage.viewModel.conversationWindowOmittedMessageCount
                           + " omitted / "
                           + settingsPage.viewModel.conversationWindowTruncatedMessageCount
                           + " truncated"
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Summary"
                    value: settingsPage.viewModel.conversationSummaryStatus
                           + " / "
                           + settingsPage.viewModel.conversationSummaryBlockCount
                           + " blocks / "
                           + settingsPage.viewModel.conversationSummaryMessageCount
                           + " older messages"
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Summary Budget"
                    value: settingsPage.viewModel.conversationSummaryBudgetSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Retrieval"
                    value: settingsPage.viewModel.retrievalPlanningStatus
                           + " / "
                           + settingsPage.viewModel.retrievalPlanningBudgetSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Retrieval Sources"
                    value: settingsPage.viewModel.retrievalPlanningSelectedSourceCount
                           + " selected / "
                           + settingsPage.viewModel.retrievalPlanningExcludedSourceCount
                           + " excluded / "
                           + settingsPage.viewModel.retrievalPlanningTruncatedCandidateCount
                           + " truncated"
                    Layout.fillWidth: true
                }
            }
        }

            ShellPanel {
                width: parent.width
                implicitHeight: settingsPage.sectionHeight(semanticContent)

                ColumnLayout {
                    id: semanticContent
                    x: settingsPage.panelPadding
                    y: settingsPage.panelPadding
                    width: parent.width - settingsPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: "Semantic / Vector Readiness"
                    subtitle: "Abstraction metadata only. Semantic retrieval is not active."
                    Layout.fillWidth: true
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceSm

                    StatusChip {
                        label: "Semantic retrieval"
                        value: "disabled"
                        accent: SentinelTheme.warning
                    }

                    StatusChip {
                        label: "Provider"
                        value: settingsPage.viewModel.semanticProviderMode
                        accent: SentinelTheme.textMuted
                        muted: true
                    }

                    StatusChip {
                        label: "Vector index"
                        value: settingsPage.viewModel.vectorIndexedItemCount + " indexed"
                        accent: SentinelTheme.textMuted
                        muted: true
                    }

                    StatusChip {
                        label: "Authority"
                        value: "deterministic"
                        accent: SentinelTheme.success
                    }

                    StatusChip {
                        label: "Arbitration"
                        value: settingsPage.viewModel.semanticArbitrationStatus
                        accent: SentinelTheme.textMuted
                        muted: true
                    }
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Semantic Retrieval"
                    value: settingsPage.viewModel.semanticRetrievalStatus
                           + " / "
                           + settingsPage.viewModel.semanticRetrievalSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Selected Provider"
                    value: settingsPage.viewModel.selectedSemanticProviderName
                           + " / "
                           + settingsPage.viewModel.semanticProviderReadiness
                           + " / "
                           + settingsPage.viewModel.semanticProviderHealth
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Provider Capabilities"
                    value: settingsPage.viewModel.semanticProviderCapabilitySummaries.join(", ")
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Activation Readiness"
                    value: settingsPage.viewModel.semanticActivationReadiness
                           + " / "
                           + settingsPage.viewModel.semanticActivationSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Activation Steps"
                    value: settingsPage.viewModel.semanticActivationRequiredSteps.join(" / ")
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Candidate Orchestration"
                    value: settingsPage.viewModel.semanticCandidateStatus
                           + " / "
                           + settingsPage.viewModel.semanticCandidateBudgetSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Candidate Counts"
                    value: settingsPage.viewModel.semanticCandidateSelectedCount
                           + " selected / "
                           + settingsPage.viewModel.semanticCandidateExcludedCount
                           + " excluded / "
                           + settingsPage.viewModel.semanticCandidateTruncatedCount
                           + " truncated"
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Hybrid Retrieval"
                    value: settingsPage.viewModel.hybridRetrievalStatus
                           + " / "
                           + settingsPage.viewModel.hybridRetrievalSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Arbitration Simulation"
                    value: settingsPage.viewModel.semanticArbitrationReadiness
                           + " / "
                           + settingsPage.viewModel.semanticArbitrationBudgetSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Embedding Runtime"
                    value: settingsPage.viewModel.embeddingRuntimeReadiness
                           + " / "
                           + settingsPage.viewModel.embeddingRuntimeSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Isolated Embedding Test"
                    value: settingsPage.viewModel.isolatedEmbeddingRuntimeStatus
                           + " / "
                           + settingsPage.viewModel.isolatedEmbeddingRuntimeHealth
                           + " / "
                           + settingsPage.viewModel.isolatedEmbeddingRuntimeSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Embedding Runtime Bounds"
                    value: settingsPage.viewModel.isolatedEmbeddingRuntimeBoundedState
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Vector Persistence"
                    value: settingsPage.viewModel.vectorPersistenceStatus
                           + " / "
                           + settingsPage.viewModel.vectorPersistenceHealth
                           + " / "
                           + settingsPage.viewModel.vectorPersistenceSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Vector Index Lifecycle"
                    value: settingsPage.viewModel.vectorPersistenceBoundedState
                           + " / "
                           + settingsPage.viewModel.vectorPersistenceIndexedItemCount
                           + " indexed"
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Semantic Search"
                    value: settingsPage.viewModel.semanticSearchStatus
                           + " / "
                           + settingsPage.viewModel.semanticSearchReadiness
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Semantic Search Bounds"
                    value: settingsPage.viewModel.semanticSearchBudgetSummary
                           + " / "
                           + settingsPage.viewModel.semanticSearchArbitrationSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Hybrid Bridge"
                    value: settingsPage.viewModel.hybridBridgeStatus
                           + " / "
                           + settingsPage.viewModel.hybridBridgeReadiness
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Bridge Arbitration"
                    value: settingsPage.viewModel.hybridBridgeBudgetSummary
                           + " / "
                           + settingsPage.viewModel.hybridBridgeFallbackSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Semantic Acceptance"
                    value: settingsPage.viewModel.semanticAcceptanceStatus
                           + " / "
                           + settingsPage.viewModel.semanticAcceptanceReadiness
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Acceptance Bounds"
                    value: settingsPage.viewModel.semanticAcceptanceBudgetSummary
                           + " / "
                           + settingsPage.viewModel.semanticAcceptanceFallbackSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Supplement Assembly"
                    value: settingsPage.viewModel.semanticSupplementAssemblyStatus
                           + " / "
                           + settingsPage.viewModel.semanticSupplementAssemblyReadiness
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Supplement Bounds"
                    value: settingsPage.viewModel.semanticSupplementAssemblyBudgetSummary
                           + " / "
                           + settingsPage.viewModel.semanticSupplementAssemblySafetySummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Prompt Authority"
                    value: settingsPage.viewModel.semanticPromptAuthorityStatus
                           + " / "
                           + settingsPage.viewModel.semanticPromptAuthorityReadinessSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Authority Audit"
                    value: settingsPage.viewModel.semanticPromptAuthorityDecisionSummary
                           + " / "
                           + settingsPage.viewModel.semanticPromptAuthorityFallbackSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Prompt Inclusion"
                    value: settingsPage.viewModel.semanticPromptInclusionStatus
                           + " / "
                           + settingsPage.viewModel.semanticPromptInclusionBudgetSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Inclusion Audit"
                    value: settingsPage.viewModel.semanticPromptInclusionFallbackSummary
                           + " / deterministic authority "
                           + (settingsPage.viewModel.semanticPromptInclusionDeterministicAuthorityPreserved ? "preserved" : "blocked")
                    Layout.fillWidth: true
                }

                SentinelButton {
                    text: settingsPage.showAdvancedContextDetails ? "Hide Advanced Details"
                                                                  : "Show Advanced Details"
                    Layout.preferredWidth: 190
                    onClicked: settingsPage.showAdvancedContextDetails = !settingsPage.showAdvancedContextDetails
                }

                Repeater {
                    model: settingsPage.viewModel.semanticRetrievalReadinessChecks

                    Label {
                        required property string modelData
                        visible: settingsPage.showAdvancedContextDetails
                        text: modelData
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.hybridRetrievalReadinessChecks

                    Label {
                        required property string modelData
                        visible: settingsPage.showAdvancedContextDetails
                        text: modelData
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.semanticArbitrationChecks

                    Label {
                        required property string modelData
                        visible: settingsPage.showAdvancedContextDetails
                        text: modelData
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.embeddingRuntimeConstraintSummaries

                    Label {
                        required property string modelData
                        visible: settingsPage.showAdvancedContextDetails
                        text: modelData
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.semanticCandidateParticipationSummaries

                    Label {
                        required property string modelData
                        visible: settingsPage.showAdvancedContextDetails
                        text: modelData
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.isolatedEmbeddingRuntimeChecks

                    Label {
                        required property string modelData
                        visible: settingsPage.showAdvancedContextDetails
                        text: modelData
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.vectorPersistenceChecks

                    Label {
                        required property string modelData
                        visible: settingsPage.showAdvancedContextDetails
                        text: modelData
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.semanticSearchChecks

                    Label {
                        required property string modelData
                        visible: settingsPage.showAdvancedContextDetails
                        text: modelData
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.hybridBridgeChecks

                    Label {
                        required property string modelData
                        visible: settingsPage.showAdvancedContextDetails
                        text: modelData
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.semanticAcceptanceChecks

                    Label {
                        required property string modelData
                        visible: settingsPage.showAdvancedContextDetails
                        text: modelData
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.semanticSupplementAssemblyChecks

                    Label {
                        required property string modelData
                        visible: settingsPage.showAdvancedContextDetails
                        text: modelData
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.semanticPromptAuthorityChecks

                    Label {
                        required property string modelData
                        visible: settingsPage.showAdvancedContextDetails
                        text: modelData
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.semanticPromptInclusionChecks

                    Label {
                        required property string modelData
                        visible: settingsPage.showAdvancedContextDetails
                        text: modelData
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }

            ShellPanel {
                width: parent.width
                implicitHeight: settingsPage.sectionHeight(voiceContent)

                ColumnLayout {
                    id: voiceContent
                    x: settingsPage.panelPadding
                    y: settingsPage.panelPadding
                    width: parent.width - settingsPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: "Voice Configuration"
                    subtitle: "Local paths with immediate readiness validation."
                    Layout.fillWidth: true
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: settingsPage.compact ? 1 : 2
                    columnSpacing: SentinelTheme.spaceSm
                    rowSpacing: SentinelTheme.spaceSm

                    Label {
                        text: "Piper bin"
                        color: SentinelTheme.textMuted
                    }

                    SentinelTextField {
                        id: piperBinaryField
                        Layout.fillWidth: true
                        text: settingsPage.viewModel.piperBinaryPath
                        placeholderText: "Piper binary path"
                        onEditingFinished: settingsPage.viewModel.piperBinaryPath = text
                    }

                    Label {
                        text: ""
                        visible: !settingsPage.compact
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Piper binary path. Must exist and be executable."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        font.pixelSize: SentinelTheme.fontSmall
                    }

                    Label {
                        text: "Piper model"
                        color: SentinelTheme.textMuted
                    }

                    SentinelTextField {
                        id: piperModelField
                        Layout.fillWidth: true
                        text: settingsPage.viewModel.piperModelPath
                        placeholderText: "Piper .onnx model path"
                        onEditingFinished: settingsPage.viewModel.piperModelPath = text
                    }

                    Label {
                        text: ""
                        visible: !settingsPage.compact
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Piper .onnx model path. Must exist and be readable."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        font.pixelSize: SentinelTheme.fontSmall
                    }

                    Label {
                        text: "Whisper bin"
                        color: SentinelTheme.textMuted
                    }

                    SentinelTextField {
                        id: whisperBinaryField
                        Layout.fillWidth: true
                        text: settingsPage.viewModel.whisperBinaryPath
                        placeholderText: "Whisper binary path"
                        onEditingFinished: settingsPage.viewModel.whisperBinaryPath = text
                    }

                    Label {
                        text: ""
                        visible: !settingsPage.compact
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Whisper binary path. Must exist and be executable."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        font.pixelSize: SentinelTheme.fontSmall
                    }

                    Label {
                        text: "Whisper model"
                        color: SentinelTheme.textMuted
                    }

                    SentinelTextField {
                        id: whisperModelField
                        Layout.fillWidth: true
                        text: settingsPage.viewModel.whisperModelPath
                        placeholderText: "Whisper model folder or model file"
                        onEditingFinished: settingsPage.viewModel.whisperModelPath = text
                    }

                    Label {
                        text: ""
                        visible: !settingsPage.compact
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Whisper model folder or model file. Must exist and be readable."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        font.pixelSize: SentinelTheme.fontSmall
                    }
                }

                SentinelButton {
                    text: "Apply Paths"
                    Layout.fillWidth: true
                    onClicked: settingsPage.viewModel.applyVoiceConfigurationPaths(
                        piperBinaryField.text,
                        piperModelField.text,
                        whisperBinaryField.text,
                        whisperModelField.text)
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Readiness"
                    value: settingsPage.viewModel.voiceConfigurationReadinessSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Runtime"
                    value: settingsPage.viewModel.voiceRuntimeReadinessSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Voice Pipeline"
                    value: settingsPage.viewModel.voicePipelineSessionStatus + " / " + settingsPage.viewModel.voicePipelineSessionSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Pipeline Safety"
                    value: settingsPage.viewModel.voicePipelineSessionSafetySummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Audio File"
                    value: settingsPage.viewModel.audioFileSessionStatus + " / " + settingsPage.viewModel.audioFileSessionReadinessSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Audio Safety"
                    value: settingsPage.viewModel.audioFileSessionSafetySummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Configured / Missing"
                    value: settingsPage.viewModel.voiceRuntimeConfiguredCount.toString()
                           + " / "
                           + settingsPage.viewModel.voiceRuntimeMissingCount.toString()
                           + " refused "
                           + settingsPage.viewModel.voiceRuntimeRefusedCount.toString()
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Piper TTS"
                    value: settingsPage.viewModel.piperFileOutputReadinessStatus + " / " + settingsPage.viewModel.piperFileOutputReadinessSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Piper Synthesis"
                    value: settingsPage.viewModel.piperSynthesisStatus + " / " + settingsPage.viewModel.piperSynthesisReadinessSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "TTS Result"
                    value: settingsPage.viewModel.piperSynthesisLastSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "TTS Fallback"
                    value: settingsPage.viewModel.piperSynthesisFallbackSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Whisper STT"
                    value: settingsPage.viewModel.whisperPreparationReadinessStatus + " / " + settingsPage.viewModel.whisperPreparationReadinessSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Piper Runtime"
                    value: settingsPage.viewModel.piperRuntimeReadinessSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Whisper Runtime"
                    value: settingsPage.viewModel.whisperRuntimeReadinessSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Whisper STT Runtime"
                    value: settingsPage.viewModel.whisperTranscriptionStatus + " / " + settingsPage.viewModel.whisperTranscriptionReadinessSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "STT Result"
                    value: settingsPage.viewModel.whisperTranscriptionLastSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "STT Fallback"
                    value: settingsPage.viewModel.whisperTranscriptionFallbackSummary
                    Layout.fillWidth: true
                }

                Repeater {
                    model: settingsPage.viewModel.audioFileSupportedExtensionSummaries

                    InfoRow {
                        compact: settingsPage.compact
                        label: "Audio Ext"
                        value: modelData
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.voiceConfigurationStatusBadges

                    InfoRow {
                        compact: settingsPage.compact
                        label: "Status"
                        value: modelData
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.voiceConfigurationValidationSummaries

                    InfoRow {
                        compact: settingsPage.compact
                        label: "Check"
                        value: modelData
                        Layout.fillWidth: true
                    }
                }

                Repeater {
                    model: settingsPage.viewModel.voiceConfigurationHintSummaries

                    InfoRow {
                        compact: settingsPage.compact
                        label: "Hint"
                        value: modelData
                        Layout.fillWidth: true
                    }
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Execution"
                    value: settingsPage.viewModel.voiceRuntimeExecutionAllowed ? "Enabled by policy" : "Disabled. No microphone, playback, Piper, or Whisper execution is available here."
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Safety"
                    value: settingsPage.viewModel.voiceRuntimeSafetyStatus + " / " + settingsPage.viewModel.voiceRuntimeSafetySummary + " / " + settingsPage.viewModel.voiceRuntimeSafetyReportSummary + " / " + settingsPage.viewModel.piperSynthesisSafetySummary + " / " + settingsPage.viewModel.whisperTranscriptionSafetySummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Sandbox"
                    value: settingsPage.viewModel.voiceRuntimeSandboxSummary
                    Layout.fillWidth: true
                }
            }
        }

            ShellPanel {
                width: parent.width
                implicitHeight: settingsPage.sectionHeight(safetyContent)

                ColumnLayout {
                    id: safetyContent
                    x: settingsPage.panelPadding
                    y: settingsPage.panelPadding
                    width: parent.width - settingsPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: "Safety / Diagnostics"
                    subtitle: "Read-only metadata for runtime boundaries."
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Routing Mode"
                    value: settingsPage.viewModel.currentRoutingMode + " / " + settingsPage.viewModel.modelRoutingStatus
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Selected Route"
                    value: settingsPage.viewModel.selectedModelProviderSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Orchestration"
                    value: settingsPage.viewModel.orchestrationReadinessStatus + " / " + settingsPage.viewModel.orchestrationReadinessSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Runtime Safety"
                    value: settingsPage.viewModel.runtimeSafetyDecision + " / " + settingsPage.viewModel.runtimeSafetySummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Permissions"
                    value: settingsPage.viewModel.runtimePermissionDecision + " / " + settingsPage.viewModel.runtimePermissionSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Integration"
                    value: settingsPage.viewModel.runtimeIntegrationReadinessStatus + " / " + settingsPage.viewModel.runtimeIntegrationReadinessSummary
                    Layout.fillWidth: true
                }
            }
        }

            ShellPanel {
                width: parent.width
                implicitHeight: settingsPage.sectionHeight(maintenanceContent)

                ColumnLayout {
                    id: maintenanceContent
                    x: settingsPage.panelPadding
                    y: settingsPage.panelPadding
                    width: parent.width - settingsPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: "Local Data"
                    subtitle: "Settings, memory, and chat history remain separate."
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Memory Store"
                    value: settingsPage.viewModel.memoryStatus + " (" + settingsPage.viewModel.memoryMaintenanceStatus + ")"
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Chat History"
                    value: settingsPage.viewModel.conversationHistorySummaryText
                           + " / "
                           + settingsPage.viewModel.chatMaintenanceStatus
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Chat Save"
                    value: settingsPage.viewModel.conversationLastSavedStatus
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Chat Restore"
                    value: settingsPage.viewModel.conversationLastRestoredStatus
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Transcript Browser"
                    value: settingsPage.viewModel.conversationBrowserStatus
                           + " / "
                           + settingsPage.viewModel.conversationBrowserSummaryText
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Current Transcript"
                    value: settingsPage.viewModel.conversationListCurrentTitle
                           + " / "
                           + settingsPage.viewModel.conversationListCurrentMessageCount
                           + (settingsPage.viewModel.conversationListCurrentMessageCount === 1 ? " message" : " messages")
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Transcript Status"
                    value: settingsPage.viewModel.conversationListCurrentPersistenceStatus
                           + " / "
                           + settingsPage.viewModel.conversationListCurrentLastUpdatedSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Multi-conversation readiness"
                    value: settingsPage.viewModel.conversationCurrentStorageMode
                           + " -> "
                           + settingsPage.viewModel.conversationFutureStorageMode
                           + " / "
                           + settingsPage.viewModel.conversationMigrationReadiness
                           + " / "
                           + settingsPage.viewModel.conversationMigrationStatusSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Conversation Store"
                    value: settingsPage.viewModel.conversationStoreStatus
                           + " / "
                           + settingsPage.viewModel.conversationStoreConversationCount
                           + (settingsPage.viewModel.conversationStoreConversationCount === 1 ? " conversation" : " conversations")
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Active Conversation"
                    value: settingsPage.viewModel.activeConversationSummary
                           + " / "
                           + settingsPage.viewModel.activeConversationStateSummary
                           + " / "
                           + settingsPage.viewModel.activeConversationCount
                           + " active, "
                           + settingsPage.viewModel.archivedConversationCount
                           + " archived"
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Delete Policy"
                    value: settingsPage.viewModel.conversationDeletePolicyStatus
                           + " / "
                           + settingsPage.viewModel.conversationDeletePolicySummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Delete Readiness"
                    value: settingsPage.viewModel.conversationDeleteReadinessStatus
                           + " / "
                           + settingsPage.viewModel.conversationDeleteReadinessSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Schema plan"
                    value: settingsPage.viewModel.conversationSchemaStatusSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Search"
                    value: settingsPage.viewModel.conversationListCurrentSearchAvailabilitySummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Export"
                    value: settingsPage.viewModel.conversationListCurrentExportAvailabilitySummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Export File"
                    value: settingsPage.viewModel.conversationExportLastFileName
                    Layout.fillWidth: true
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: settingsPage.compact ? 1 : 2
                    columnSpacing: SentinelTheme.spaceSm
                    rowSpacing: SentinelTheme.spaceSm

                    SentinelButton {
                        text: "Clear Local Memory"
                        enabled: settingsPage.viewModel.memoryStatus === "Available"
                        Layout.fillWidth: true
                        onClicked: clearMemoryDialog.open()
                    }

                    SentinelButton {
                        text: "Clear Chat History"
                        Layout.fillWidth: true
                        onClicked: clearChatDialog.open()
                    }

                    SentinelButton {
                        text: "Export Markdown"
                        enabled: settingsPage.viewModel.conversationExportAvailable
                        Layout.fillWidth: true
                        onClicked: settingsPage.viewModel.exportTranscript("markdown")
                    }

                    SentinelButton {
                        text: "Export JSON"
                        enabled: settingsPage.viewModel.conversationExportAvailable
                        Layout.fillWidth: true
                        onClicked: settingsPage.viewModel.exportTranscript("json")
                    }
                }
            }
        }
    }
    }

    Dialog {
        id: clearMemoryDialog
        title: "Clear local memory?"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent

        Label {
            text: "This clears local memory entries only. Settings are kept."
            color: SentinelTheme.textPrimary
            wrapMode: Text.WordWrap
            width: 320
        }

        onAccepted: settingsPage.viewModel.clearMemory()
    }

    Dialog {
        id: clearChatDialog
        title: "Clear chat history?"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent

        Label {
            text: "This clears the runtime transcript and persisted local chat history when available, then restores the single initial system message. Settings and memory are kept."
            color: SentinelTheme.textPrimary
            wrapMode: Text.WordWrap
            width: 360
        }

        onAccepted: settingsPage.viewModel.clearChat()
    }
}
