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

        }

        CognitionStreamPanel {
            viewModel: dashboardPage.viewModel
            Layout.fillWidth: true
            Layout.columnSpan: dashboardPage.wideLayout ? 12 : 1
        }
    }
}
