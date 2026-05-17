import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: presence
    required property var viewModel
    property bool compact: width < 620
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    property color secondaryAccent: SentinelTheme.modeSecondaryAccent(viewModel.currentModeName)
    readonly property bool activityActive: viewModel.localInferenceBusy
                                           || viewModel.localInferenceStreamingText.length > 0
                                           || viewModel.voicePipelineStatus !== "Idle"

    color: SentinelTheme.modePanelColor(viewModel.currentModeName)
    border.color: SentinelTheme.withAlpha(modeAccent, 0.16)
    bracketColor: SentinelTheme.withAlpha(modeAccent, 0.24)
    bracketSize: 12
    implicitHeight: compact ? 300 : 420
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: presence.compact ? SentinelTheme.spaceLg : SentinelTheme.space2Xl
        spacing: SentinelTheme.spaceSm

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            Label {
                text: "CORE / SENTINEL"
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 2.6
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: presence.viewModel.orchestrationReadinessStatus
                color: presence.modeAccent
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 2.4
            }
        }

        Item {
            id: scene
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: presence.compact ? 220 : 300
            readonly property real safeWidth: Math.max(1, width)
            readonly property real safeHeight: Math.max(1, height)
            readonly property real safeSize: Math.max(1, Math.min(safeWidth, safeHeight))

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(scene.safeWidth * 0.88, presence.compact ? 390 : 620)
                height: width
                radius: width / 2
                color: SentinelTheme.withAlpha(presence.secondaryAccent, presence.activityActive ? 0.060 : 0.026)
                border.color: SentinelTheme.withAlpha(presence.modeAccent, 0.050)
            }

            SentinelOrb {
                viewModel: presence.viewModel
                compact: presence.compact
                active: presence.activityActive
                width: Math.min(scene.safeWidth * 0.70, scene.safeHeight * 0.86, presence.compact ? 320 : 500)
                height: width
                anchors.centerIn: parent
            }

            Label {
                anchors.left: parent.left
                anchors.top: parent.top
                text: "OLLAMA / " + presence.viewModel.ollamaHealthStatus + "\nMODE / " + presence.viewModel.currentRoutingMode
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
                lineHeight: 1.35
            }

            Label {
                anchors.right: parent.right
                anchors.top: parent.top
                text: "CHAT / " + presence.viewModel.localChatInferenceStatus + "\nSTREAM / " + presence.viewModel.localInferenceStreamStatus
                color: SentinelTheme.textMuted
                horizontalAlignment: Text.AlignRight
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
                lineHeight: 1.35
            }

            Label {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                text: "MODEL / " + presence.viewModel.selectedLocalModelStatus
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
            }

            Label {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                text: "VOICE / " + presence.viewModel.voiceReadinessStatus
                color: SentinelTheme.textMuted
                horizontalAlignment: Text.AlignRight
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
            }
        }

        Label {
            Layout.fillWidth: true
            text: presence.viewModel.localRuntimeSummary
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.80)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: SentinelTheme.fontBody
            font.weight: Font.Light
        }
    }
}
