import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ShellPanel {
    id: settingsPage
    required property var viewModel
    readonly property bool compact: width < 720

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.pageMargin(settingsPage.width)
        spacing: SentinelTheme.contentSpacing(settingsPage.width)

        SectionTitle {
            title: "Settings Foundation"
            subtitle: "JSON-backed local settings for desktop shell preferences. Runtime tests still use InMemorySettingsStore."
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

            SentinelTextField {
                id: themeField
                Layout.fillWidth: true
                text: settingsPage.viewModel.themeName
                onEditingFinished: settingsPage.viewModel.setThemeName(text)
            }

            Label {
                text: "Config Profile"
                color: SentinelTheme.textMuted
            }

            SentinelTextField {
                id: profileField
                Layout.fillWidth: true
                text: settingsPage.viewModel.configurationProfile
                onEditingFinished: settingsPage.viewModel.setConfigurationProfile(text)
            }
        }

        SectionTitle {
            title: "Routing Preference"
            subtitle: "Metadata-only model routing preference. No provider calls, API keys, downloads, or model execution are enabled."
        }

        GridLayout {
            Layout.fillWidth: true
            columns: settingsPage.compact ? 1 : 2
            columnSpacing: SentinelTheme.spaceSm
            rowSpacing: SentinelTheme.spaceSm

            Label {
                text: "Routing Mode"
                color: SentinelTheme.textMuted
            }

            ComboBox {
                id: routingModeCombo
                Layout.fillWidth: true
                model: settingsPage.viewModel.availableRoutingModes
                currentIndex: settingsPage.viewModel.availableRoutingModes.indexOf(settingsPage.viewModel.currentRoutingMode)
                onActivated: settingsPage.viewModel.setRoutingModeByName(currentText)
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Route Status"
                value: settingsPage.viewModel.modelRoutingStatus
                Layout.columnSpan: settingsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Selected Route"
                value: settingsPage.viewModel.selectedModelProviderSummary
                Layout.columnSpan: settingsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Task Plan"
                value: settingsPage.viewModel.latestTaskPlanStatus + " (" + settingsPage.viewModel.plannedTaskStepCount + " step)"
                Layout.columnSpan: settingsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Plan Summary"
                value: settingsPage.viewModel.latestTaskPlanSummary
                Layout.columnSpan: settingsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Preferred Agent"
                value: settingsPage.viewModel.currentAgentSummary
                Layout.columnSpan: settingsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Memory Affinity"
                value: settingsPage.viewModel.currentMemoryAffinitySummary
                Layout.columnSpan: settingsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }
        }

        SectionTitle {
            title: "Agent Registry"
            subtitle: "Read-only metadata for future orchestration. No autonomous execution, tools, provider calls, or background workers are enabled."
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            InfoRow {
                compact: settingsPage.compact
                label: "Registered Agents"
                value: settingsPage.viewModel.registeredAgentCount.toString()
            }

            Repeater {
                model: settingsPage.viewModel.activeAgentSummaries

                InfoRow {
                    compact: settingsPage.compact
                    label: "Agent"
                    value: modelData
                    Layout.fillWidth: true
                }
            }
        }

        SectionTitle {
            title: "Orchestration Readiness"
            subtitle: "Deterministic metadata diagnostics only. No provider probing, networking, filesystem scans, model calls, or execution are performed."
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            InfoRow {
                compact: settingsPage.compact
                label: "Readiness"
                value: settingsPage.viewModel.orchestrationReadinessStatus
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Summary"
                value: settingsPage.viewModel.orchestrationReadinessSummary
                Layout.fillWidth: true
            }

            Repeater {
                model: settingsPage.viewModel.orchestrationDiagnostics

                InfoRow {
                    compact: settingsPage.compact
                    label: "Diagnostic"
                    value: modelData
                    Layout.fillWidth: true
                }
            }
        }

        SectionTitle {
            title: "Local Runtime Boundary"
            subtitle: "Read-only local runtime metadata. No process launch, model execution, downloads, streaming, or provider calls are enabled."
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            InfoRow {
                compact: settingsPage.compact
                label: "Runtime"
                value: settingsPage.viewModel.localRuntimeStatus + " / " + settingsPage.viewModel.localRuntimeHealth
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Summary"
                value: settingsPage.viewModel.localRuntimeSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Request Boundary"
                value: settingsPage.viewModel.localRuntimeResponseStatus + " / " + settingsPage.viewModel.localRuntimeResponseSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Session"
                value: settingsPage.viewModel.localRuntimeSessionStatus + " / " + settingsPage.viewModel.localRuntimeSessionHealth
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Session Summary"
                value: settingsPage.viewModel.localRuntimeSessionSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Allocation"
                value: settingsPage.viewModel.localRuntimeAllocationSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Reservation"
                value: settingsPage.viewModel.localRuntimeReservationSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Negotiation"
                value: settingsPage.viewModel.runtimeNegotiationSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Negotiation Profile"
                value: settingsPage.viewModel.runtimeNegotiationProfileSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Local-Only Enforcement"
                value: settingsPage.viewModel.localOnlyRuntimeEnforcementSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Permission Policy"
                value: settingsPage.viewModel.runtimePermissionDecision + " / " + settingsPage.viewModel.runtimePermissionSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Safety Policy"
                value: settingsPage.viewModel.runtimeSafetyDecision + " / " + settingsPage.viewModel.runtimeSafetySummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Request Pipeline"
                value: settingsPage.viewModel.runtimePipelineStatus + " / " + settingsPage.viewModel.runtimePipelineSummary
                Layout.fillWidth: true
            }

            Repeater {
                model: settingsPage.viewModel.localRuntimeCapabilities

                InfoRow {
                    compact: settingsPage.compact
                    label: "Capability"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: settingsPage.viewModel.runtimePipelineTraceSummaries

                InfoRow {
                    compact: settingsPage.compact
                    label: "Pipeline Trace"
                    value: modelData
                    Layout.fillWidth: true
                }
            }
        }

        SectionTitle {
            title: "Execution Lifecycle"
            subtitle: "Read-only lifecycle and session coordination metadata. Execution remains intentionally disabled."
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            InfoRow {
                compact: settingsPage.compact
                label: "Lifecycle"
                value: settingsPage.viewModel.executionLifecycleState + " / " + settingsPage.viewModel.executionLifecycleStatus
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Lifecycle Summary"
                value: settingsPage.viewModel.executionLifecycleSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Session"
                value: settingsPage.viewModel.executionSessionId + " / " + settingsPage.viewModel.executionSessionStatus
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Ownership"
                value: settingsPage.viewModel.executionSessionOwnership + " / " + settingsPage.viewModel.executionCoordinationMode
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Session Summary"
                value: settingsPage.viewModel.executionSessionSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Snapshot"
                value: settingsPage.viewModel.executionCoordinationSnapshotSummary
                Layout.fillWidth: true
            }

            Repeater {
                model: settingsPage.viewModel.executionLifecycleTraceSummaries

                InfoRow {
                    compact: settingsPage.compact
                    label: "Lifecycle Trace"
                    value: modelData
                    Layout.fillWidth: true
                }
            }
        }

        SectionTitle {
            title: "Runtime Context Session"
            subtitle: "Read-only conversation/session metadata. Chat history and agent runtime context remain separate."
        }

        GridLayout {
            Layout.fillWidth: true
            columns: settingsPage.compact ? 1 : 2
            columnSpacing: SentinelTheme.spaceSm
            rowSpacing: SentinelTheme.spaceSm

            InfoRow {
                compact: settingsPage.compact
                label: "Session"
                value: settingsPage.viewModel.conversationSessionId + " / " + settingsPage.viewModel.conversationSessionStatus
                Layout.columnSpan: settingsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Interaction"
                value: settingsPage.viewModel.interactionMode + " / " + settingsPage.viewModel.attentionState
                Layout.columnSpan: settingsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Context Window"
                value: settingsPage.viewModel.contextWindowSummary
                Layout.columnSpan: settingsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "State"
                value: settingsPage.viewModel.conversationState + " / " + settingsPage.viewModel.conversationTransitionStatus
                Layout.columnSpan: settingsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Transition"
                value: settingsPage.viewModel.conversationTransitionSummary
                Layout.columnSpan: settingsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }
        }

        SectionTitle {
            title: "Runtime Integration Readiness"
            subtitle: "Adapter, provider bridge, and Ollama local runtime metadata. Model selection and explicit local toggles are the only actions here."
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            InfoRow {
                compact: settingsPage.compact
                label: "Adapter"
                value: settingsPage.viewModel.localRuntimeAdapterStatus + " / " + settingsPage.viewModel.localRuntimeAdapterHealth
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Adapter Summary"
                value: settingsPage.viewModel.localRuntimeAdapterSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Provider Bridge"
                value: settingsPage.viewModel.providerRuntimeBridgeStatus + " / " + settingsPage.viewModel.providerRuntimeBridgeSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Bridge Response"
                value: settingsPage.viewModel.providerRuntimeBridgeResponseSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Readiness"
                value: settingsPage.viewModel.runtimeIntegrationReadinessStatus + " / " + settingsPage.viewModel.runtimeIntegrationReadinessSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Ollama Endpoint"
                value: settingsPage.viewModel.ollamaEndpoint
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Ollama Health"
                value: settingsPage.viewModel.ollamaConnectionStatus + " / " + settingsPage.viewModel.ollamaHealthStatus
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Ollama Summary"
                value: settingsPage.viewModel.ollamaHealthSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Installed Models"
                value: settingsPage.viewModel.ollamaModelCount.toString()
                Layout.fillWidth: true
            }

            ComboBox {
                id: localModelCombo
                Layout.fillWidth: true
                enabled: settingsPage.viewModel.ollamaModelCount > 0
                model: settingsPage.viewModel.ollamaModelNames
                currentIndex: settingsPage.viewModel.ollamaModelNames.indexOf(settingsPage.viewModel.selectedLocalModel)
                displayText: currentIndex >= 0 ? currentText : settingsPage.viewModel.selectedLocalModelStatus + " / No model selected"
                onActivated: settingsPage.viewModel.selectedLocalModel = currentText
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Model Status"
                value: settingsPage.viewModel.selectedLocalModelStatus
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
                label: "Model Metadata"
                value: settingsPage.viewModel.selectedLocalModelMetadataSummary
                Layout.fillWidth: true
            }

            Repeater {
                model: settingsPage.viewModel.modelRecommendationSummaries

                InfoRow {
                    compact: settingsPage.compact
                    label: "Recommendation"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            SectionTitle {
                title: "Model Management"
                subtitle: "Read-only readiness metadata. Pull, delete, and install actions remain future scoped."
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Management Status"
                value: settingsPage.viewModel.modelManagementStatus
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Management Summary"
                value: settingsPage.viewModel.modelManagementSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Action Availability"
                value: settingsPage.viewModel.modelManagementActionAvailability
                Layout.fillWidth: true
            }

            Repeater {
                model: settingsPage.viewModel.modelRequirementSummaries

                InfoRow {
                    compact: settingsPage.compact
                    label: "Requirement"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            SectionTitle {
                title: "Voice Readiness"
                subtitle: "Read-only voice architecture metadata. Recording, playback, Whisper, Piper, subprocesses, downloads, cloud calls, and API keys remain out of scope."
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Voice Mode"
                value: settingsPage.viewModel.voiceRuntimeMode + " / " + settingsPage.viewModel.voiceReadinessStatus
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Voice Summary"
                value: settingsPage.viewModel.voiceReadinessSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "TTS"
                value: settingsPage.viewModel.textToSpeechStatus + " / " + settingsPage.viewModel.textToSpeechSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "STT"
                value: settingsPage.viewModel.speechToTextStatus + " / " + settingsPage.viewModel.speechToTextSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Voice Session"
                value: settingsPage.viewModel.voiceSessionStatus + " / " + settingsPage.viewModel.voiceSessionSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Voice Pipeline"
                value: settingsPage.viewModel.voicePipelineStatus + " / " + settingsPage.viewModel.voicePipelineSummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Voice Runtime"
                value: settingsPage.viewModel.voiceRuntimeStatus + " / " + settingsPage.viewModel.voiceRuntimeSummary
                Layout.fillWidth: true
            }

            Repeater {
                model: settingsPage.viewModel.voiceCapabilitySummaries

                InfoRow {
                    compact: settingsPage.compact
                    label: "Voice Capability"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: settingsPage.viewModel.voiceReadinessChecks

                InfoRow {
                    compact: settingsPage.compact
                    label: "Voice Check"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: settingsPage.viewModel.voiceRuntimeCheckSummaries

                InfoRow {
                    compact: settingsPage.compact
                    label: "Runtime Check"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: settingsPage.viewModel.voicePipelineTraceSummaries

                InfoRow {
                    compact: settingsPage.compact
                    label: "Voice Trace"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            CheckBox {
                id: localChatInferenceToggle
                Layout.fillWidth: true
                text: "Local chat inference"
                checked: settingsPage.viewModel.localChatInferenceEnabled
                onToggled: settingsPage.viewModel.localChatInferenceEnabled = checked
            }

            CheckBox {
                id: localInferenceStreamingToggle
                Layout.fillWidth: true
                text: "Local response streaming"
                checked: settingsPage.viewModel.localInferenceStreamingEnabled
                onToggled: settingsPage.viewModel.localInferenceStreamingEnabled = checked
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Chat Routing"
                value: settingsPage.viewModel.localChatInferenceStatus + " / " + settingsPage.viewModel.localChatInferenceSummary
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
                label: "Inference State"
                value: settingsPage.viewModel.localInferenceRuntimeState + " / " + settingsPage.viewModel.localInferenceStatus
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Inference Latency"
                value: settingsPage.viewModel.localInferenceLatencySummary
                Layout.fillWidth: true
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Streaming"
                value: settingsPage.viewModel.localInferenceStreamStatus + " / " + settingsPage.viewModel.localInferenceStreamSummary
                Layout.fillWidth: true
            }

            Repeater {
                model: settingsPage.viewModel.localRuntimeAdapterCapabilitySummaries

                InfoRow {
                    compact: settingsPage.compact
                    label: "Adapter Capability"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: settingsPage.viewModel.ollamaModelSummaries

                InfoRow {
                    compact: settingsPage.compact
                    label: "Ollama Model"
                    value: modelData
                    Layout.fillWidth: true
                }
            }

            Repeater {
                model: settingsPage.viewModel.runtimeIntegrationReadinessChecks

                InfoRow {
                    compact: settingsPage.compact
                    label: "Integration Check"
                    value: modelData
                    Layout.fillWidth: true
                }
            }
        }

        SectionTitle {
            title: "Provider Catalog"
            subtitle: "Read-only metadata placeholders for future model management. No setup, credentials, downloads, networking, or execution are enabled."
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            InfoRow {
                compact: settingsPage.compact
                label: "Catalog Entries"
                value: settingsPage.viewModel.providerCatalogCount.toString()
            }

            Repeater {
                model: settingsPage.viewModel.providerCatalogSummaries

                InfoRow {
                    compact: settingsPage.compact
                    label: "Provider"
                    value: modelData
                    Layout.fillWidth: true
                }
            }
        }

        SectionTitle {
            title: "Memory Taxonomy"
            subtitle: "Read-only category metadata for future recall planning. No embeddings, semantic search, or autonomous memory writes are enabled."
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            InfoRow {
                compact: settingsPage.compact
                label: "Memory Categories"
                value: settingsPage.viewModel.memoryCatalogCount.toString()
            }

            Repeater {
                model: settingsPage.viewModel.memoryCatalogSummaries

                InfoRow {
                    compact: settingsPage.compact
                    label: "Memory"
                    value: modelData
                    Layout.fillWidth: true
                }
            }
        }

        SectionTitle {
            title: "Local Data Maintenance"
            subtitle: "Settings are stored separately and are not deleted by memory/chat clear actions."
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            InfoRow {
                compact: settingsPage.compact
                label: "Memory Store"
                value: settingsPage.viewModel.memoryStatus + " (" + settingsPage.viewModel.memoryMaintenanceStatus + ")"
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Chat History"
                value: settingsPage.viewModel.chatHistoryStatus + " (" + settingsPage.viewModel.chatMaintenanceStatus + ")"
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: settingsPage.compact ? 1 : 2
            columnSpacing: SentinelTheme.spaceSm
            rowSpacing: SentinelTheme.spaceSm

            SentinelButton {
                text: "Clear Local Memory"
                enabled: settingsPage.viewModel.memoryStatus === "Available"
                Layout.fillWidth: settingsPage.compact
                onClicked: clearMemoryDialog.open()
            }

            SentinelButton {
                text: "Clear Chat History"
                Layout.fillWidth: settingsPage.compact
                onClicked: clearChatDialog.open()
            }
        }

        Label {
            Layout.fillWidth: true
            text: "Future settings should remain local-first and pass through AppSettings rather than being implemented in QML."
            color: SentinelTheme.textMuted
            wrapMode: Text.WordWrap
        }

        Item {
            Layout.fillHeight: true
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
            text: "This clears local chat history. Settings are kept."
            color: SentinelTheme.textPrimary
            wrapMode: Text.WordWrap
            width: 320
        }

        onAccepted: settingsPage.viewModel.clearChat()
    }
}
