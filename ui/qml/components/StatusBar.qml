import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import Sentinel.Desktop

ShellPanel {
    id: statusBar
    required property var viewModel
    property bool compact: width < 820
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)

    radius: SentinelTheme.radiusLg
    color: "transparent"
    border.color: "transparent"
    showBrackets: false

    layer.enabled: true
    layer.smooth: false
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: "#000000"
        shadowOpacity: 0.06
        shadowBlur: 0.06
        shadowHorizontalOffset: 1
        shadowVerticalOffset: 1
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: SentinelTheme.spaceLg
        anchors.rightMargin: SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceMd

        Label {
            text: statusBar.viewModel.ollamaHealthStatus
            color: SentinelTheme.textPrimary
            font.pixelSize: SentinelTheme.fontSmall
            maximumLineCount: 1
            elide: Text.ElideRight
        }

        Label {
            text: qsTr("Model: %1").arg(statusBar.viewModel.selectedLocalModelStatus)
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            maximumLineCount: 1
            elide: Text.ElideRight
        }

        Label {
            text: qsTr("Chat: %1").arg(statusBar.viewModel.localChatInferenceStatus)
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            visible: !statusBar.compact
            maximumLineCount: 1
            elide: Text.ElideRight
        }

        Label {
            text: qsTr("Stream: %1").arg(statusBar.viewModel.localInferenceStreamStatus)
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            visible: !statusBar.compact
            maximumLineCount: 1
            elide: Text.ElideRight
        }

        Label {
            text: qsTr("Voice: %1").arg(statusBar.viewModel.voiceReadinessStatus)
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            visible: !statusBar.compact
            maximumLineCount: 1
            elide: Text.ElideRight
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
