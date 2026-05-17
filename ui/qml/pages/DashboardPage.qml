import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ScrollView {
    id: dashboardPage
    required property var viewModel
    readonly property bool wideLayout: width >= SentinelTheme.breakpointWide + 120
    readonly property bool compact: width < 860
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    GridLayout {
        width: dashboardPage.availableWidth
        columns: dashboardPage.wideLayout ? 12 : 1
        columnSpacing: SentinelTheme.spaceLg
        rowSpacing: SentinelTheme.spaceLg

        WorkspacePresence {
            viewModel: dashboardPage.viewModel
            compact: dashboardPage.compact
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.columnSpan: dashboardPage.wideLayout ? 8 : 1
            Layout.minimumHeight: dashboardPage.compact ? 300 : 420
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.columnSpan: dashboardPage.wideLayout ? 4 : 1
            spacing: SentinelTheme.spaceLg

            ShellPanel {
                Layout.fillWidth: true
                implicitHeight: stateColumn.implicitHeight + SentinelTheme.space2Xl
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.032)
                border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.08)
                bracketColor: SentinelTheme.withAlpha(SentinelTheme.accent, 0.20)
                bracketSize: 9

                ColumnLayout {
                    id: stateColumn
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.spaceLg
                    spacing: SentinelTheme.spaceSm

                    Label {
                        Layout.fillWidth: true
                        text: "LOCAL STATE"
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        font.letterSpacing: 2.2
                        elide: Text.ElideRight
                    }

                    InfoRow {
                        compact: true
                        label: "Runtime"
                        value: dashboardPage.viewModel.ollamaHealthStatus + " / " + dashboardPage.viewModel.localInferenceRuntimeState
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
                    }

                    InfoRow {
                        compact: true
                        label: "Voice / Piper"
                        value: dashboardPage.viewModel.voiceReadinessStatus + " / " + dashboardPage.viewModel.piperTtsStatus
                        Layout.fillWidth: true
                    }
                }
            }

            ShellPanel {
                Layout.fillWidth: true
                implicitHeight: summaryColumn.implicitHeight + SentinelTheme.space2Xl
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.032)
                border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.08)
                bracketColor: SentinelTheme.withAlpha(SentinelTheme.accent, 0.20)
                bracketSize: 9

                ColumnLayout {
                    id: summaryColumn
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.spaceLg
                    spacing: SentinelTheme.spaceSm

                    Label {
                        Layout.fillWidth: true
                        text: "ORCHESTRATION SNAPSHOT / " + dashboardPage.viewModel.orchestrationSnapshotStatus
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        font.letterSpacing: 2.2
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: dashboardPage.viewModel.orchestrationSnapshotSummary
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.86)
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                    }

                    InfoRow {
                        compact: true
                        label: "Readiness"
                        value: dashboardPage.viewModel.orchestrationReadinessStatus
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: true
                        label: "Agents"
                        value: dashboardPage.viewModel.registeredAgentCount + " registered / " + dashboardPage.viewModel.currentAgentSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: true
                        label: "Memory"
                        value: dashboardPage.viewModel.memoryCatalogCount + " taxonomy categories / " + dashboardPage.viewModel.memoryStatus
                        Layout.fillWidth: true
                    }
                }
            }

        }
    }
}
