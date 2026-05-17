import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: statusBar
    required property var viewModel
    property bool compact: width < 820
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)

    radius: SentinelTheme.radiusLg
    color: "transparent"
    border.color: "transparent"
    showBrackets: false

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: SentinelTheme.spaceLg
        anchors.rightMargin: SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceMd

        Label {
            text: statusBar.viewModel.ollamaHealthStatus
            color: SentinelTheme.textPrimary
            font.pixelSize: SentinelTheme.fontSmall
        }

        Label {
            text: "Mode: " + statusBar.viewModel.currentModeName
            color: statusBar.modeAccent
            font.pixelSize: SentinelTheme.fontSmall
        }

        Label {
            text: "Model: " + statusBar.viewModel.selectedLocalModelStatus
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
        }

        Label {
            text: "Chat: " + statusBar.viewModel.localChatInferenceStatus
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            visible: !statusBar.compact
        }

        Label {
            text: "Stream: " + statusBar.viewModel.localInferenceStreamStatus
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            visible: !statusBar.compact
        }

        Label {
            text: "Voice: " + statusBar.viewModel.voiceReadinessStatus
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            visible: !statusBar.compact
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            text: statusBar.viewModel.orchestrationReadinessStatus + " / " + statusBar.viewModel.runtimeSafetyDecision
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            elide: Text.ElideRight
            Layout.maximumWidth: statusBar.compact ? 180 : 320
        }
    }
}
