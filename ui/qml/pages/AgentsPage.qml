import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ShellPanel {
    id: agentsPage
    required property var viewModel
    readonly property bool compact: width < 760

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.pageMargin(agentsPage.width)
        spacing: SentinelTheme.contentSpacing(agentsPage.width)

        SectionTitle {
            title: "Agent Metadata"
            subtitle: "Read-only registry and activity metadata in a dedicated workspace."
        }

        GridLayout {
            Layout.fillWidth: true
            columns: agentsPage.compact ? 1 : 2
            columnSpacing: SentinelTheme.spaceSm
            rowSpacing: SentinelTheme.spaceSm

            InfoRow {
                compact: agentsPage.compact
                label: "Registered Agents"
                value: agentsPage.viewModel.registeredAgentCount.toString()
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Agent Activity"
                value: agentsPage.viewModel.agentActivityCount.toString()
            }

            InfoRow {
                compact: agentsPage.compact
                label: "Current Agent"
                value: agentsPage.viewModel.currentAgentSummary
                Layout.columnSpan: agentsPage.compact ? 1 : 2
                Layout.fillWidth: true
            }
        }

        ActiveAgentsPanel {
            viewModel: agentsPage.viewModel
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
