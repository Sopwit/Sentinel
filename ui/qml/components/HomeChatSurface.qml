import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: homeChat
    required property var viewModel
    property bool compact: width < 760
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property bool modelReady: viewModel.selectedLocalModelStatus === "Available"
                                      || viewModel.selectedLocalModelStatus === "Fallback"
    readonly property bool chatReady: viewModel.localChatInferenceEnabled && modelReady
    readonly property bool canSend: chatReady
                                    && !viewModel.localInferenceBusy
                                    && !viewModel.activeConversationArchived
    readonly property bool streamingActive: viewModel.localInferenceStreamingText.length > 0
                                            || viewModel.localInferenceRuntimeState === "Streaming"
    readonly property string disabledReason: viewModel.activeConversationArchived
                                             ? viewModel.activeConversationStateSummary
                                             : !viewModel.localChatInferenceEnabled
                                               ? "Enable Local chat inference in Settings to send with Ollama."
                                               : !modelReady
                                                 ? "Select an available local Ollama model before sending."
                                                 : viewModel.localInferenceBusy
                                                   ? "Sentinel is responding."
                                                   : ""

    radius: SentinelTheme.radiusPanel
    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.58)
    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.070)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: homeChat.compact ? SentinelTheme.spaceMd : SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceMd

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            ColumnLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceXs

                Label {
                    Layout.fillWidth: true
                    text: "LOCAL ASSISTANT"
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontTiny
                    font.letterSpacing: 1.8
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: "Ask Sentinel"
                    color: SentinelTheme.textPrimary
                    font.pixelSize: homeChat.compact ? SentinelTheme.fontTitle : SentinelTheme.fontTitle + 2
                    wrapMode: Text.WordWrap
                }
            }

            StatusChip {
                label: "Provider"
                value: "Local Ollama only"
                accent: homeChat.chatReady ? SentinelTheme.success : SentinelTheme.textMuted
                muted: !homeChat.chatReady
            }
        }

        ListView {
            id: recentMessages
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 210
            visible: true
            clip: true
            spacing: SentinelTheme.spaceSm
            model: homeChat.viewModel.chatMessages
            boundsBehavior: Flickable.StopAtBounds
            onCountChanged: Qt.callLater(positionViewAtEnd)
            Component.onCompleted: Qt.callLater(positionViewAtEnd)

            delegate: Rectangle {
                id: recentMessage
                required property int index
                required property string messageRole
                required property string content
                readonly property bool displayable: messageRole !== "system"

                width: ListView.view.width
                height: displayable ? recentMessageColumn.implicitHeight + SentinelTheme.spaceMd : 0
                visible: displayable
                radius: SentinelTheme.radiusSm
                color: messageRole === "user"
                       ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.075)
                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.026)
                border.color: SentinelTheme.withAlpha(messageRole === "user"
                                                      ? homeChat.modeAccent
                                                      : SentinelTheme.textPrimary,
                                                      0.08)

                ColumnLayout {
                    id: recentMessageColumn
                    x: SentinelTheme.spaceSm
                    y: SentinelTheme.spaceSm
                    width: parent.width - SentinelTheme.spaceSm * 2
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: recentMessage.messageRole === "user" ? "You" : "Sentinel"
                        color: recentMessage.messageRole === "user"
                               ? homeChat.modeAccent
                               : SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: recentMessage.content
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                        maximumLineCount: 12
                        elide: Text.ElideRight
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: homeChat.viewModel.conversationHistoryMessageCount <= 1
            text: "No conversation yet. Local Ollama only; no cloud provider is active."
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontBody
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            visible: homeChat.streamingActive
            radius: SentinelTheme.radiusSm
            color: SentinelTheme.withAlpha(homeChat.modeAccent, 0.055)
            border.color: SentinelTheme.withAlpha(homeChat.modeAccent, 0.12)
            implicitHeight: streamLabel.implicitHeight + SentinelTheme.spaceSm

            Label {
                id: streamLabel
                x: SentinelTheme.spaceSm
                y: SentinelTheme.spaceXs
                width: parent.width - SentinelTheme.spaceSm * 2
                text: homeChat.viewModel.localInferenceRuntimeState
                      + (homeChat.viewModel.localInferenceStreamingText.length > 0
                         ? " / " + homeChat.viewModel.localInferenceStreamingText
                         : "")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontSmall
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            SentinelTextField {
                id: promptInput
                Layout.fillWidth: true
                placeholderText: homeChat.chatReady ? "Type a prompt for Sentinel"
                                                    : "Local Ollama chat is not ready"
                enabled: homeChat.canSend
                onAccepted: {
                    if (sendButton.visible && sendButton.enabled)
                        sendButton.clicked()
                }
            }

            SentinelButton {
                id: sendButton
                visible: homeChat.chatReady
                text: "Send"
                Layout.preferredWidth: 92
                enabled: promptInput.text.trim().length > 0 && homeChat.canSend
                onClicked: {
                    homeChat.viewModel.sendMessage(promptInput.text)
                    promptInput.clear()
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: !homeChat.chatReady || homeChat.disabledReason.length > 0
            text: !homeChat.chatReady
                  ? homeChat.disabledReason + " Local Ollama only. No cloud provider active."
                  : homeChat.disabledReason
            color: !homeChat.chatReady ? SentinelTheme.textMuted : SentinelTheme.warning
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }
    }
}
