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
