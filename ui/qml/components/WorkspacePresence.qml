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
    readonly property bool streamingActive: viewModel.localInferenceStreamingText.length > 0
                                            || viewModel.localInferenceRuntimeState === "Streaming"
    readonly property bool runtimeReady: viewModel.ollamaHealthStatus === "Available"
                                  || viewModel.ollamaHealthStatus === "Ready"
                                  || viewModel.selectedLocalModelStatus === "Available"
                                  || viewModel.selectedLocalModelStatus === "Fallback"
    readonly property bool contextActive: viewModel.contextAssemblyAvailableSourceCount > 0
    readonly property bool retrievalActive: viewModel.retrievalPlanningSelectedSourceCount > 0
    readonly property bool companionMode: viewModel.currentModeName === "Companion Mode"
    readonly property bool focusMode: viewModel.currentModeName === "Focus Mode"
    readonly property bool telemetryMode: viewModel.currentModeName === "Mission Mode"
                                          || viewModel.currentModeName === "System Mode"
                                          || viewModel.currentModeName === "Tactical Mode"
    readonly property string topLeftStatusText: "OLLAMA / "
                                                + presence.viewModel.ollamaHealthStatus
                                                + "\nMODE / "
                                                + presence.viewModel.currentRoutingMode
    readonly property string topRightStatusText: "CHAT / "
                                                 + presence.viewModel.localChatInferenceStatus
                                                 + "\nSTREAM / "
                                                 + presence.viewModel.localInferenceStreamStatus

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

            GlowSurface {
                anchors.centerIn: parent
                width: Math.min(scene.safeWidth * 0.88, presence.compact ? 390 : 620)
                height: width
                accent: presence.modeAccent
                secondaryAccent: presence.secondaryAccent
                active: presence.activityActive || presence.contextActive || presence.retrievalActive
                glowScale: SentinelTheme.modeGlowScale(presence.viewModel.currentModeName)
                reducedMotion: presence.focusMode
            }

            SentinelOrb {
                viewModel: presence.viewModel
                compact: presence.compact
                active: presence.activityActive || presence.contextActive || presence.retrievalActive
                reducedMotion: presence.focusMode
                width: Math.min(scene.safeWidth * 0.70, scene.safeHeight * 0.86, presence.compact ? 320 : 500)
                height: width
                anchors.centerIn: parent
            }

            Flow {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: presence.compact ? SentinelTheme.spaceLg : SentinelTheme.space2Xl
                width: Math.min(parent.width * 0.88, presence.compact ? 360 : 560)
                spacing: SentinelTheme.spaceSm

                RuntimeBadge {
                    label: "RETRIEVAL"
                    value: presence.retrievalActive ? "deterministic authority" : "standing by"
                    accent: SentinelTheme.success
                    active: presence.retrievalActive
                    visible: presence.telemetryMode
                }

                RuntimeBadge {
                    label: "CONTEXT"
                    value: presence.contextActive ? presence.viewModel.contextAssemblyAvailableSourceCount + " sources" : "not assembled"
                    accent: SentinelTheme.accentTertiary
                    active: presence.contextActive
                    muted: !presence.contextActive
                    visible: !presence.companionMode || presence.contextActive
                }

                RuntimeBadge {
                    label: "SEMANTIC"
                    value: "disabled by policy"
                    accent: SentinelTheme.warning
                    muted: true
                    visible: presence.telemetryMode
                }

                RuntimeBadge {
                    label: "RUNTIME"
                    value: presence.runtimeReady ? "local ready" : "local unavailable"
                    accent: presence.runtimeReady ? SentinelTheme.accent : SentinelTheme.textMuted
                    active: presence.runtimeReady
                    muted: !presence.runtimeReady
                }

                RuntimeBadge {
                    label: "STREAM"
                    value: presence.streamingActive ? "active" : "inactive"
                    accent: SentinelTheme.accentSecondary
                    active: presence.streamingActive
                    muted: !presence.streamingActive
                }
            }

            Label {
                anchors.left: parent.left
                anchors.top: parent.top
                visible: !presence.companionMode && !presence.focusMode
                text: presence.topLeftStatusText
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
                lineHeight: 1.35
            }

            Label {
                anchors.right: parent.right
                anchors.top: parent.top
                visible: !presence.companionMode && !presence.focusMode
                text: presence.topRightStatusText
                color: SentinelTheme.textMuted
                horizontalAlignment: Text.AlignRight
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
                lineHeight: 1.35
            }

            Label {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.bottomMargin: presence.compact ? 0 : SentinelTheme.spaceXs
                text: "MODEL / " + presence.viewModel.selectedLocalModelStatus
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
            }

            Label {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: presence.compact ? 0 : SentinelTheme.spaceXs
                visible: !presence.focusMode
                text: "VOICE / " + presence.viewModel.voiceReadinessStatus
                color: SentinelTheme.textMuted
                horizontalAlignment: Text.AlignRight
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
            }
        }

        Label {
            Layout.fillWidth: true
            visible: !presence.focusMode
            text: presence.viewModel.localRuntimeSummary
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.80)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: SentinelTheme.fontBody
            font.weight: Font.Light
        }
    }
}
