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
    readonly property string uiSelfCheck: "chat-scroll-safe-area composer-visible no-bridge-duplication"
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
    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)

    function scrollToLatest(force) {
        if (force || recentMessages.followNewMessages)
            Qt.callLater(recentMessages.positionViewAtEnd)
    }

    function focusComposer() {
        promptInput.forceActiveFocus()
    }

    onStreamingActiveChanged: {
        if (!streamingActive)
            scrollToLatest(true)
    }

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
                    text: "Sentinel"
                    color: SentinelTheme.textPrimary
                    font.pixelSize: homeChat.compact ? SentinelTheme.fontTitle : SentinelTheme.fontTitle + 2
                    maximumLineCount: 1
                    elide: Text.ElideRight
                }
            }

            StatusChip {
                label: "Provider"
                value: "local"
                accent: homeChat.chatReady ? SentinelTheme.success : SentinelTheme.textMuted
                muted: !homeChat.chatReady
            }
        }

        ListView {
            id: recentMessages
            property bool followNewMessages: true
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 210
            visible: true
            clip: true
            spacing: SentinelTheme.spaceSm
            model: homeChat.viewModel.chatMessages
            boundsBehavior: Flickable.StopAtBounds
            boundsMovement: Flickable.StopAtBounds
            maximumFlickVelocity: 2200
            flickDeceleration: 5200
            bottomMargin: SentinelTheme.spaceMd
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 4
                    radius: 2
                    color: SentinelTheme.withAlpha(homeChat.modeAccent, parent.active ? 0.34 : 0.18)
                }
                background: Rectangle {
                    color: "transparent"
                }
            }
            onCountChanged: homeChat.scrollToLatest(false)
            Component.onCompleted: Qt.callLater(positionViewAtEnd)
            onMovementEnded: followNewMessages = atYEnd || contentHeight <= height
            onFlickEnded: followNewMessages = atYEnd || contentHeight <= height

            add: Transition {
                NumberAnimation {
                    property: "opacity"
                    from: 0.0
                    to: 1.0
                    duration: MotionTokens.duration(MotionTokens.message, homeChat.viewModel.currentModeName)
                    easing.type: MotionTokens.enter
                }
                NumberAnimation {
                    property: "scale"
                    from: 0.985
                    to: 1.0
                    duration: MotionTokens.duration(MotionTokens.message, homeChat.viewModel.currentModeName)
                    easing.type: MotionTokens.enter
                }
            }

            delegate: Rectangle {
                id: recentMessage
                required property int index
                required property string messageRole
                required property string content
                readonly property bool displayable: messageRole !== "system"

                width: ListView.view.width
                height: displayable ? recentMessageColumn.implicitHeight + SentinelTheme.spaceMd : 0
                visible: displayable
                radius: SentinelTheme.radiusMd
                color: messageRole === "user"
                       ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.105)
                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.026)
                border.color: SentinelTheme.withAlpha(messageRole === "user"
                                                      ? homeChat.modeAccent
                                                      : SentinelTheme.textPrimary,
                                                      messageRole === "user" ? 0.13 : 0.07)
                opacity: 1.0
                scale: 1.0

                Behavior on color {
                    ColorAnimation {
                        duration: MotionTokens.normal
                        easing.type: MotionTokens.standard
                    }
                }

                Rectangle {
                    width: 3
                    height: parent.height - SentinelTheme.spaceSm
                    radius: 2
                    anchors.left: parent.left
                    anchors.leftMargin: SentinelTheme.spaceXs
                    anchors.verticalCenter: parent.verticalCenter
                    visible: recentMessage.messageRole !== "user"
                    color: SentinelTheme.withAlpha(homeChat.modeAccent, 0.42)
                }

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
                        maximumLineCount: 18
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: homeChat.viewModel.conversationHistoryMessageCount <= 1
            text: "Start a local conversation. Ollama only; no cloud provider is active."
            color: SentinelTheme.textPrimary
            font.pixelSize: SentinelTheme.fontBody
            wrapMode: Text.WordWrap
            leftPadding: SentinelTheme.spaceMd
            rightPadding: SentinelTheme.spaceMd
            topPadding: SentinelTheme.spaceSm
            bottomPadding: SentinelTheme.spaceSm
            background: Rectangle {
                radius: SentinelTheme.radiusMd
                color: SentinelTheme.withAlpha(homeChat.modeAccent, 0.045)
                border.color: SentinelTheme.withAlpha(homeChat.modeAccent, 0.12)
            }
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
                                                    : "Local provider chat is not ready"
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
                opacity: enabled ? 1.0 : 0.58
                onClicked: {
                    homeChat.viewModel.sendMessage(promptInput.text)
                    promptInput.clear()
                    recentMessages.followNewMessages = true
                    homeChat.scrollToLatest(true)
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
