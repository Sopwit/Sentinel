import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ScrollView {
    id: settingsPage
    required property var viewModel
    readonly property bool compact: width < 760
    readonly property int panelPadding: SentinelTheme.spaceLg
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
                    subtitle: "Local path configuration with read-only validation hints."
                    Layout.fillWidth: true
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: settingsPage.compact ? 1 : 2
                    columnSpacing: SentinelTheme.spaceSm
                    rowSpacing: SentinelTheme.spaceSm

                    Label {
                        text: "Piper Binary"
                        color: SentinelTheme.textMuted
                    }

                    SentinelTextField {
                        Layout.fillWidth: true
                        text: settingsPage.viewModel.piperBinaryPath
                        placeholderText: "/path/to/piper"
                        onEditingFinished: settingsPage.viewModel.piperBinaryPath = text
                    }

                    Label {
                        text: ""
                        visible: !settingsPage.compact
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Piper command used by a future gated TTS path. It is not run here."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        font.pixelSize: SentinelTheme.fontSizeSmall
                    }

                    Label {
                        text: "Piper Model"
                        color: SentinelTheme.textMuted
                    }

                    SentinelTextField {
                        Layout.fillWidth: true
                        text: settingsPage.viewModel.piperModelPath
                        placeholderText: "/path/to/model.onnx"
                        onEditingFinished: settingsPage.viewModel.piperModelPath = text
                    }

                    Label {
                        text: ""
                        visible: !settingsPage.compact
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Local Piper voice model path. Sentinel only checks whether this path is readable."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        font.pixelSize: SentinelTheme.fontSizeSmall
                    }

                    Label {
                        text: "Whisper Binary"
                        color: SentinelTheme.textMuted
                    }

                    SentinelTextField {
                        Layout.fillWidth: true
                        text: settingsPage.viewModel.whisperBinaryPath
                        placeholderText: "/path/to/whisper"
                        onEditingFinished: settingsPage.viewModel.whisperBinaryPath = text
                    }

                    Label {
                        text: ""
                        visible: !settingsPage.compact
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Whisper command for future STT work. It is not launched or probed here."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        font.pixelSize: SentinelTheme.fontSizeSmall
                    }

                    Label {
                        text: "Whisper Model"
                        color: SentinelTheme.textMuted
                    }

                    SentinelTextField {
                        Layout.fillWidth: true
                        text: settingsPage.viewModel.whisperModelPath
                        placeholderText: "/path/to/whisper-model-or-directory"
                        onEditingFinished: settingsPage.viewModel.whisperModelPath = text
                    }

                    Label {
                        text: ""
                        visible: !settingsPage.compact
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Whisper model file or directory. Only the configured path is checked."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        font.pixelSize: SentinelTheme.fontSizeSmall
                    }
                }

                InfoRow {
                    compact: settingsPage.compact
                    label: "Readiness"
                    value: settingsPage.viewModel.voiceConfigurationReadinessSummary
                    Layout.fillWidth: true
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
                    label: "Piper TTS"
                    value: settingsPage.viewModel.piperTtsStatus + " / " + settingsPage.viewModel.piperTtsFileOutputStatus
                    Layout.fillWidth: true
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
                    value: settingsPage.viewModel.voiceRuntimeSafetyStatus + " / " + settingsPage.viewModel.voiceRuntimeSafetySummary
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
                    value: settingsPage.viewModel.chatHistoryStatus + " (" + settingsPage.viewModel.chatMaintenanceStatus + ")"
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
            text: "This clears local chat history. Settings are kept."
            color: SentinelTheme.textPrimary
            wrapMode: Text.WordWrap
            width: 320
        }

        onAccepted: settingsPage.viewModel.clearChat()
    }
}
