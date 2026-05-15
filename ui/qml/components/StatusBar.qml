import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: statusBar
    required property var viewModel

    radius: SentinelTheme.radiusLg

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: SentinelTheme.spaceLg
        anchors.rightMargin: SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceMd

        Label {
            text: "Status: Local Alpha"
            color: SentinelTheme.textPrimary
            font.pixelSize: SentinelTheme.fontSmall
        }

        Label {
            text: "Mode: " + statusBar.viewModel.currentModeName
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
        }

        Label {
            text: "Memory: " + statusBar.viewModel.memoryStatus
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
        }

        Label {
            text: "Chat: " + statusBar.viewModel.chatHistoryStatus
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
        }

        Label {
            text: "Agent: " + statusBar.viewModel.agentStatus
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
        }

        Label {
            text: "Tools: " + statusBar.viewModel.availableToolCount
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            text: "Provider: " + statusBar.viewModel.providerName + " / " + statusBar.viewModel.providerStatus
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
        }
    }
}
