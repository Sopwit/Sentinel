import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: homeChat
    required property var viewModel
    property bool compact: width < 760
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property bool chatReady: viewModel.localChatSendAvailable
    readonly property bool canSend: viewModel.localChatSendAvailable
    readonly property string sendState: viewModel.chatSendLifecycleState
    readonly property bool sendBusy: sendState === "validating" || sendState === "sending"
                                     || sendState === "streaming"
    readonly property bool streamingActive: sendState === "streaming"
                                            || viewModel.localInferenceRuntimeState === "Streaming"
    readonly property string uiSelfCheck: "chat-scroll-safe-area composer-visible no-bridge-duplication"
    readonly property string disabledReason: viewModel.activeConversationArchived
                                             ? viewModel.activeConversationStateSummary
                                             : viewModel.localChatSendAvailabilitySummary

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

    function sendComposerText() {
        var prompt = promptInput.text.trim()
        if (prompt.length === 0 || !homeChat.canSend || homeChat.sendBusy)
            return
        var accepted = homeChat.viewModel.sendMessage(promptInput.text)
        var lifecycle = homeChat.viewModel.chatSendLifecycleState
        if (accepted || lifecycle === "sending" || lifecycle === "streaming"
                || lifecycle === "completed" || lifecycle === "failed") {
            promptInput.clear()
            recentMessages.followNewMessages = true
            homeChat.scrollToLatest(true)
        }
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
                    text: homeChat.viewModel.conversationListCurrentTitle
                    color: SentinelTheme.textPrimary
                    font.pixelSize: homeChat.compact ? SentinelTheme.fontTitle : SentinelTheme.fontTitle + 2
                    maximumLineCount: 1
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: homeChat.viewModel.conversationLastRestoredStatus
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    maximumLineCount: 1
                    elide: Text.ElideRight
                }
            }

            StatusChip {
                label: qsTr("Provider")
                value: homeChat.viewModel.activeRuntimeProviderLabel
                accent: homeChat.chatReady ? SentinelTheme.success : SentinelTheme.textMuted
                muted: !homeChat.chatReady
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            StatusChip {
                label: qsTr("Model")
                value: homeChat.viewModel.selectedLocalModel.length > 0
                       ? homeChat.viewModel.selectedLocalModel
                       : qsTr("No model")
                accent: SentinelTheme.accentTertiary
                muted: homeChat.viewModel.selectedLocalModelStatus !== "Available"
            }

            StatusChip {
                label: qsTr("Scope")
                value: homeChat.viewModel.activeRuntimeLocalOnlySummary
                accent: SentinelTheme.calmAccent
                muted: homeChat.viewModel.activeRuntimeLocalOnlySummary !== "Local Only"
            }

            StatusChip {
                label: qsTr("Workspace")
                value: homeChat.viewModel.workspacePermissionPosture
                accent: SentinelTheme.textMuted
                muted: true
            }

            StatusChip {
                label: qsTr("Profile")
                value: homeChat.viewModel.selectedSkillProfileName
                accent: SentinelTheme.calmAccent
                muted: false
            }

            StatusChip {
                label: qsTr("Runtime")
                value: homeChat.viewModel.activeRuntimeReadinessState
                accent: homeChat.chatReady ? SentinelTheme.success : SentinelTheme.textMuted
                muted: !homeChat.chatReady
            }

            StatusChip {
                label: qsTr("API Keys")
                value: qsTr("Not stored")
                accent: SentinelTheme.textMuted
                muted: true
            }
        }

        ListView {
            id: recentMessages
            property bool followNewMessages: true
            function nearBottom() {
                return contentHeight <= height || (contentY + height >= contentHeight - 96)
            }
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 210
            visible: true
            clip: true
            spacing: homeChat.compact ? SentinelTheme.spaceSm : SentinelTheme.spaceMd
            model: homeChat.viewModel.chatMessages
            boundsBehavior: Flickable.StopAtBounds
            boundsMovement: Flickable.StopAtBounds
            maximumFlickVelocity: 2200
            flickDeceleration: 5200
            bottomMargin: SentinelTheme.spaceXl + SentinelTheme.spaceMd
            ScrollBar.vertical: ScrollBar {
                id: recentMessagesScrollBar
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 4
                    radius: 2
                    color: SentinelTheme.withAlpha(homeChat.modeAccent, recentMessagesScrollBar.active ? 0.34 : 0.18)
                }
                background: Rectangle {
                    color: "transparent"
                }
            }
            onCountChanged: homeChat.scrollToLatest(false)
            Component.onCompleted: Qt.callLater(positionViewAtEnd)
            onMovementStarted: followNewMessages = nearBottom()
            onMovementEnded: followNewMessages = nearBottom()
            onFlickEnded: followNewMessages = nearBottom()

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
                    spacing: SentinelTheme.spaceXs

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            Layout.fillWidth: true
                            text: recentMessage.messageRole === "user" ? qsTr("You") : qsTr("Sentinel")
                            color: recentMessage.messageRole === "user"
                                   ? homeChat.modeAccent
                                   : SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontTiny
                            elide: Text.ElideRight
                        }

                        Button {
                            id: copyButton
                            Layout.preferredWidth: 46
                            Layout.preferredHeight: 24
                            text: qsTr("Copy")
                            hoverEnabled: true
                            focusPolicy: Qt.StrongFocus
                            onClicked: {
                                messageBody.forceActiveFocus()
                                messageBody.selectAll()
                                messageBody.copy()
                                messageBody.deselect()
                            }

                            contentItem: Text {
                                text: copyButton.text
                                color: copyButton.enabled ? SentinelTheme.textMuted : SentinelTheme.textPlaceholder
                                font.pixelSize: SentinelTheme.fontTiny
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusSm
                                color: InteractionTokens.surfaceColor(copyButton.hovered, copyButton.down,
                                                                       copyButton.activeFocus,
                                                                       homeChat.modeAccent)
                                border.color: InteractionTokens.borderColor(copyButton.activeFocus,
                                                                             copyButton.hovered,
                                                                             false,
                                                                             homeChat.modeAccent)
                            }
                        }
                    }

                    TextEdit {
                        id: messageBody
                        Layout.fillWidth: true
                        text: recentMessage.content
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                        readOnly: true
                        selectByMouse: true
                        selectByKeyboard: true
                        textFormat: TextEdit.PlainText
                        selectionColor: SentinelTheme.withAlpha(homeChat.modeAccent, 0.34)
                        selectedTextColor: SentinelTheme.textPrimary
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: homeChat.viewModel.conversationHistoryMessageCount <= 1
            text: qsTr("Start with a focused question, a draft to revise, or notes to organize. Local Ollama only; no cloud provider is active.")
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

        Label {
            Layout.fillWidth: true
            visible: homeChat.sendState !== "idle"
                     && (homeChat.sendBusy || homeChat.viewModel.developerModeEnabled
                         || homeChat.sendState === "refused" || homeChat.sendState === "failed"
                         || homeChat.sendState === "cancelled")
            text: homeChat.viewModel.developerModeEnabled
                  ? homeChat.sendState + " / " + homeChat.viewModel.chatSendLifecycleSummary
                  : homeChat.viewModel.chatSendLifecycleSummary
            color: homeChat.sendState === "refused" || homeChat.sendState === "failed"
                   || homeChat.sendState === "cancelled"
                   ? SentinelTheme.warning
                   : SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true
            visible: homeChat.viewModel.promptContextInjectionEnabled
            text: homeChat.viewModel.promptContextUsedSummary
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            maximumLineCount: 1
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            visible: homeChat.viewModel.conversationSummaryGenerationStatus === "Planned"
                     || homeChat.viewModel.conversationSummaryAvailable
                     || homeChat.viewModel.conversationSummaryGenerationStatus === "Blocked"
            text: homeChat.viewModel.conversationSummaryGenerationStatus === "Planned"
                  ? homeChat.viewModel.conversationSummaryReadinessSummary
                  : (homeChat.viewModel.promptContextInjectionEnabled
                     ? homeChat.viewModel.summaryContinuityContributionSummary
                     : homeChat.viewModel.conversationSummaryReadinessSummary)
            color: homeChat.viewModel.conversationSummaryGenerationStatus === "Blocked"
                   || homeChat.viewModel.summaryContinuityStatus === "Stale"
                   ? SentinelTheme.warning
                   : SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            maximumLineCount: 2
            wrapMode: Text.WordWrap
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: homeChat.viewModel.contextExplainabilityVisible
                     && homeChat.viewModel.promptContextInjectionEnabled
            spacing: SentinelTheme.spaceXs

            Button {
                id: contextReasoningToggle
                property bool expanded: false
                Layout.fillWidth: true
                text: (expanded ? qsTr("Context reasoning -") : qsTr("Context reasoning +"))
                      + "  " + homeChat.viewModel.contextReasoningSummary
                hoverEnabled: true
                activeFocusOnTab: true
                onClicked: expanded = !expanded

                contentItem: Text {
                    text: contextReasoningToggle.text
                    color: SentinelTheme.textPrimary
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                }

                background: Rectangle {
                    radius: SentinelTheme.radiusMd
                    color: contextReasoningToggle.activeFocus
                           ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.14)
                           : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.030)
                    border.color: contextReasoningToggle.activeFocus
                                  ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.62)
                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.080)
                }
            }

            TextArea {
                Layout.fillWidth: true
                visible: contextReasoningToggle.expanded
                readOnly: true
                activeFocusOnTab: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                text: homeChat.viewModel.contextReasoningOrderingSummary + "\n"
                      + homeChat.viewModel.contextReasoningFallbackSummary + "\n"
                      + homeChat.viewModel.contextReasoningInclusionHints.slice(0, 3).join("\n")
                color: SentinelTheme.textMuted
                selectedTextColor: SentinelTheme.backgroundBase
                selectionColor: SentinelTheme.modeAccent(homeChat.viewModel.currentModeName)
                font.pixelSize: SentinelTheme.fontSmall
                background: Rectangle {
                    radius: SentinelTheme.radiusMd
                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.38)
                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: SentinelTheme.radiusMd
            color: promptInput.activeFocus
                   ? SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.82)
                   : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.68)
            border.color: InteractionTokens.borderColor(promptInput.activeFocus, composerMouse.containsMouse,
                                                         false, homeChat.modeAccent)
            implicitHeight: Math.max(76, composerLayout.implicitHeight + SentinelTheme.spaceMd)

            MouseArea {
                id: composerMouse
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }

            RowLayout {
                id: composerLayout
                x: SentinelTheme.spaceSm
                y: SentinelTheme.spaceXs
                width: parent.width - SentinelTheme.spaceSm * 2
                spacing: SentinelTheme.spaceSm

                Button {
                    id: attachPlaceholder
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    text: "+"
                    enabled: false
                    hoverEnabled: true

                    contentItem: Text {
                        text: attachPlaceholder.text
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontControl
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 17
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.026)
                        border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.070)
                    }
                }

                Button {
                    id: summaryAction
                    Layout.preferredWidth: 116
                    Layout.preferredHeight: 34
                    text: homeChat.viewModel.conversationSummaryAvailable ? qsTr("Summary Ready") : qsTr("Generate Summary")
                    enabled: homeChat.viewModel.localChatSendAvailable && !homeChat.sendBusy
                             && homeChat.viewModel.conversationSummaryGenerationStatus !== "Planned"
                    hoverEnabled: true
                    onClicked: homeChat.viewModel.requestConversationSummaryGeneration()

                    contentItem: Text {
                        text: summaryAction.text
                        color: summaryAction.enabled ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    background: Rectangle {
                        radius: 17
                        color: InteractionTokens.surfaceColor(summaryAction.hovered, summaryAction.down,
                                                               summaryAction.activeFocus,
                                                               homeChat.modeAccent)
                        border.color: homeChat.viewModel.conversationSummaryAvailable
                                      ? SentinelTheme.withAlpha(SentinelTheme.success, 0.32)
                                      : InteractionTokens.borderColor(summaryAction.activeFocus,
                                                                       summaryAction.hovered,
                                                                       false,
                                                                       homeChat.modeAccent)
                    }
                }

                TextArea {
                    id: promptInput
                    Layout.fillWidth: true
                    Layout.minimumHeight: 48
                    Layout.maximumHeight: 126
                    placeholderText: homeChat.chatReady ? (homeChat.sendBusy ? qsTr("Sentinel is responding") : qsTr("Ask Sentinel"))
                                                        : homeChat.viewModel.localChatSendAvailabilitySummary
                    enabled: !homeChat.viewModel.activeConversationArchived && !homeChat.sendBusy
                    color: SentinelTheme.textPrimary
                    placeholderTextColor: SentinelTheme.textPlaceholder
                    wrapMode: TextEdit.WordWrap
                    selectByMouse: true
                    selectionColor: SentinelTheme.withAlpha(homeChat.modeAccent, 0.34)
                    selectedTextColor: SentinelTheme.textPrimary
                    background: Rectangle {
                        color: "transparent"
                    }
                    Keys.onPressed: function(event) {
                        if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                                && !(event.modifiers & Qt.ShiftModifier)) {
                            homeChat.sendComposerText()
                            event.accepted = true
                        } else if (event.key === Qt.Key_Escape) {
                            focus = false
                            event.accepted = true
                        }
                    }
                }

                SentinelButton {
                    id: sendButton
                    visible: true
                    text: qsTr("Send")
                    Layout.preferredWidth: 82
                    Layout.alignment: Qt.AlignBottom
                    enabled: promptInput.text.trim().length > 0 && homeChat.canSend
                             && !homeChat.sendBusy
                    opacity: enabled ? 1.0 : 0.58
                    onClicked: homeChat.sendComposerText()
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: !homeChat.chatReady
            text: homeChat.disabledReason + (homeChat.chatReady ? "" : qsTr(" Local Ollama only. No cloud provider active."))
            color: !homeChat.chatReady ? SentinelTheme.textMuted : SentinelTheme.warning
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }
    }
}
