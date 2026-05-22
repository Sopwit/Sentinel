import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ScrollView {
    id: dashboardPage
    required property var viewModel
    readonly property bool wideLayout: width >= SentinelTheme.breakpointWide + 120
    readonly property bool compact: width < 860
    readonly property int panelPadding: SentinelTheme.spaceLg
    readonly property string runtimeStatusText: dashboardPage.viewModel.ollamaHealthStatus
                                                + " / "
                                                + dashboardPage.viewModel.localInferenceRuntimeState
    readonly property bool runtimeReady: dashboardPage.viewModel.ollamaHealthStatus === "Available"
                                  || dashboardPage.viewModel.ollamaHealthStatus === "Ready"
                                  || dashboardPage.viewModel.selectedLocalModelStatus === "Available"
                                  || dashboardPage.viewModel.selectedLocalModelStatus === "Fallback"
    readonly property bool streamingActive: dashboardPage.viewModel.localInferenceStreamingText.length > 0
                                            || dashboardPage.viewModel.localInferenceRuntimeState === "Streaming"
    readonly property bool companionMode: dashboardPage.viewModel.currentModeName === "Companion Mode"
    readonly property bool focusMode: dashboardPage.viewModel.currentModeName === "Focus Mode"
    readonly property bool telemetryMode: dashboardPage.viewModel.currentModeName === "Mission Mode"
                                          || dashboardPage.viewModel.currentModeName === "System Mode"
                                          || dashboardPage.viewModel.currentModeName === "Tactical Mode"
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    contentWidth: availableWidth

    Item {
        width: dashboardPage.availableWidth
        implicitHeight: dashboardGrid.implicitHeight

        GridLayout {
            id: dashboardGrid
            width: parent.width
            columns: dashboardPage.wideLayout ? 12 : 1
            columnSpacing: SentinelTheme.spaceLg
            rowSpacing: SentinelTheme.spaceLg

            ColumnLayout {
                Layout.fillWidth: true
                Layout.columnSpan: dashboardPage.wideLayout && !dashboardPage.focusMode ? 8 : 1
                spacing: dashboardPage.focusMode ? SentinelTheme.spaceMd : SentinelTheme.spaceLg

                HomeChatSurface {
                    viewModel: dashboardPage.viewModel
                    compact: dashboardPage.compact || dashboardPage.focusMode
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 430
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: !dashboardPage.focusMode
                Layout.columnSpan: dashboardPage.wideLayout ? 4 : 1
                spacing: SentinelTheme.spaceLg

                ShellPanel {
                    Layout.fillWidth: true
                    implicitHeight: stateColumn.implicitHeight + dashboardPage.panelPadding * 2
                    color: SentinelTheme.modePanelColor(dashboardPage.viewModel.currentModeName)

                    ColumnLayout {
                        id: stateColumn
                        x: dashboardPage.panelPadding
                        y: dashboardPage.panelPadding
                        width: parent.width - dashboardPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        Label {
                            Layout.fillWidth: true
                            text: "LOCAL STATUS"
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontTiny
                            font.letterSpacing: 2.2
                            elide: Text.ElideRight
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            RuntimeBadge {
                                label: "LOCAL"
                                value: dashboardPage.runtimeReady ? "ready" : "unavailable"
                                accent: dashboardPage.runtimeReady ? SentinelTheme.accent : SentinelTheme.textMuted
                                active: dashboardPage.runtimeReady
                                muted: !dashboardPage.runtimeReady
                            }

                            RuntimeBadge {
                                label: "STREAM"
                                value: dashboardPage.streamingActive ? "active" : "inactive"
                                accent: SentinelTheme.accentSecondary
                                active: dashboardPage.streamingActive
                                muted: !dashboardPage.streamingActive
                            }
                        }

                        InfoRow {
                            compact: true
                            label: "Runtime"
                            value: dashboardPage.runtimeStatusText
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Selected Model"
                            value: dashboardPage.viewModel.selectedLocalModelSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Chat Inference"
                            value: dashboardPage.viewModel.localChatInferenceStatus
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Streaming"
                            value: dashboardPage.viewModel.localInferenceStreamStatus
                            Layout.fillWidth: true
                            visible: !dashboardPage.companionMode || dashboardPage.streamingActive
                        }

                        Label {
                            Layout.fillWidth: true
                            text: dashboardPage.viewModel.ollamaModelCount > 0 ? dashboardPage.viewModel.selectedLocalModelSummary : "Start Ollama and install/select a local model."
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.86)
                            font.pixelSize: SentinelTheme.fontSmall
                            wrapMode: Text.WordWrap
                            maximumLineCount: 4
                            elide: Text.ElideRight
                        }

                        InfoRow {
                            compact: true
                            label: "Voice"
                            value: dashboardPage.viewModel.voiceReadinessStatus
                                   + " / Whisper "
                                   + dashboardPage.viewModel.whisperTranscriptionStatus
                                   + " / Piper "
                                   + dashboardPage.viewModel.piperSynthesisStatus
                            Layout.fillWidth: true
                            visible: dashboardPage.telemetryMode
                        }

                        InfoRow {
                            compact: true
                            label: "Agents"
                            value: dashboardPage.telemetryMode
                                   ? dashboardPage.viewModel.agentTaskRuntimeStatus
                                     + " / "
                                     + dashboardPage.viewModel.agentTaskQueueCount
                                     + " queued"
                                   : dashboardPage.viewModel.registeredAgentCount + " registered metadata"
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: "Provider"
                            value: "Local Ollama only / No cloud provider active"
                            Layout.fillWidth: true
                            visible: dashboardPage.telemetryMode
                        }
                    }
                }
            }
        }
    }
}
