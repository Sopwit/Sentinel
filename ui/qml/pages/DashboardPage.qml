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

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: SentinelTheme.spaceMd
                rowSpacing: SentinelTheme.spaceMd

                SentinelMetricCard {
                    Layout.fillWidth: true
                    label: "Cognition"
                    value: "94.2"
                    unit: "%"
                    trend: "+0.8"
                }

                SentinelMetricCard {
                    Layout.fillWidth: true
                    label: "Throughput"
                    value: "12.4"
                    unit: "k/s"
                    trend: "+312"
                }

                SentinelMetricCard {
                    Layout.fillWidth: true
                    label: "Memory Index"
                    value: "2.18"
                    unit: "M"
                    trend: "+1,204"
                }

                SentinelMetricCard {
                    Layout.fillWidth: true
                    label: "Signal"
                    value: "-42"
                    unit: "dBm"
                    trend: "stable"
                }
            }

            ShellPanel {
                Layout.fillWidth: true
                implicitHeight: 430
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.032)
                border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.08)
                bracketColor: SentinelTheme.withAlpha(SentinelTheme.accent, 0.20)
                bracketSize: 9

                ColumnLayout {
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

                    Repeater {
                        model: Math.min(4, dashboardPage.viewModel.orchestrationSignals.length)

                        Label {
                            required property int index
                            Layout.fillWidth: true
                            text: dashboardPage.viewModel.orchestrationSignals[index]
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontTiny
                            elide: Text.ElideRight
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "READINESS / " + dashboardPage.viewModel.orchestrationReadinessStatus
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        font.letterSpacing: 2.2
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: dashboardPage.viewModel.orchestrationReadinessSummary
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.78)
                        font.pixelSize: SentinelTheme.fontTiny
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "SESSION / " + dashboardPage.viewModel.conversationSessionStatus
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        font.letterSpacing: 2.2
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: dashboardPage.viewModel.contextWindowSummary
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.78)
                        font.pixelSize: SentinelTheme.fontTiny
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "STATE / " + dashboardPage.viewModel.conversationState
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        font.letterSpacing: 2.2
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: dashboardPage.viewModel.conversationTransitionSummary
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.72)
                        font.pixelSize: SentinelTheme.fontTiny
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                }
            }

        }

        CognitionStreamPanel {
            viewModel: dashboardPage.viewModel
            Layout.fillWidth: true
            Layout.columnSpan: dashboardPage.wideLayout ? 12 : 1
        }
    }
}
