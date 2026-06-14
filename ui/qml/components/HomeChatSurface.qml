import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
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
    readonly property bool sidebarEffectiveOpen: conversationSidebarOpen && !compact
    property bool conversationSidebarOpen: true
    property string conversationFilter: ""

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

    function restoreDraft(text) {
        promptInput.text = text
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

    function filteredConversationIndexes() {
        var normalized = conversationFilter.trim().toLowerCase()
        var result = []
        for (var i = 0; i < viewModel.conversationTitles.length; ++i) {
            var title = viewModel.conversationTitles[i]
            var id = viewModel.conversationIds[i]
            if (normalized.length === 0
                    || title.toLowerCase().indexOf(normalized) >= 0
                    || id.toLowerCase().indexOf(normalized) >= 0) {
                result.push(i)
            }
        }
        return result
    }

    onStreamingActiveChanged: {
        if (!streamingActive)
            scrollToLatest(true)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: homeChat.compact ? SentinelTheme.spaceMd : SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceMd

        ShellPanel {
            id: conversationRail
            Layout.preferredWidth: homeChat.sidebarEffectiveOpen ? Math.min(300, Math.max(238, homeChat.width * 0.22)) : 44
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.34)
            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.055)
            showBrackets: false
            clip: true

            Behavior on Layout.preferredWidth {
                NumberAnimation {
                    duration: MotionTokens.duration(MotionTokens.normal, homeChat.viewModel.currentModeName)
                    easing.type: MotionTokens.enter
                }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceSm

                Button {
                    id: sidebarToggle
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    text: homeChat.sidebarEffectiveOpen ? qsTr("Hide") : "\u2630"
                    hoverEnabled: true
                    onClicked: homeChat.conversationSidebarOpen = !homeChat.conversationSidebarOpen

                    contentItem: Text {
                        text: sidebarToggle.text
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontSmall
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    background: Rectangle {
                        radius: SentinelTheme.radiusMd
                        color: InteractionTokens.surfaceColor(sidebarToggle.hovered, sidebarToggle.down,
                                                               homeChat.sidebarEffectiveOpen,
                                                               homeChat.modeAccent)
                        border.color: InteractionTokens.borderColor(sidebarToggle.activeFocus,
                                                                     sidebarToggle.hovered,
                                                                     homeChat.sidebarEffectiveOpen,
                                                                     homeChat.modeAccent)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: homeChat.sidebarEffectiveOpen
                    spacing: SentinelTheme.spaceSm

                    SentinelTextField {
                        id: conversationSearch
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search conversations")
                        text: homeChat.conversationFilter
                        onTextChanged: homeChat.conversationFilter = text
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceXs

                        SentinelButton {
                            text: qsTr("New")
                            Layout.fillWidth: true
                            onClicked: {
                                homeChat.viewModel.createConversation(qsTr("New Chat"))
                                homeChat.conversationSidebarOpen = !homeChat.compact
                            }
                        }

                        SentinelButton {
                            text: qsTr("Search")
                            enabled: conversationSearch.text.trim().length > 0
                            Layout.fillWidth: true
                            onClicked: homeChat.viewModel.searchConversation(conversationSearch.text)
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Pinned")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        font.bold: true
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(96, contentHeight)
                        clip: true
                        spacing: SentinelTheme.spaceXs
                        model: homeChat.viewModel.conversationPinnedSummaries

                        delegate: Label {
                            required property string modelData
                            width: ListView.view.width
                            text: modelData
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontSmall
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Recent")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        font.bold: true
                    }

                    ListView {
                        id: conversationList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 120
                        clip: true
                        spacing: SentinelTheme.spaceXs
                        model: homeChat.filteredConversationIndexes()

                        delegate: Button {
                            id: conversationButton
                            required property int modelData
                            readonly property string conversationId: homeChat.viewModel.conversationIds[modelData]
                            readonly property string conversationTitle: homeChat.viewModel.conversationTitles[modelData]
                            readonly property bool active: conversationId === homeChat.viewModel.activeConversationId
                            width: ListView.view.width
                            height: 38
                            hoverEnabled: true
                            onClicked: homeChat.viewModel.switchConversation(conversationId)

                            contentItem: Text {
                                text: conversationButton.conversationTitle
                                color: conversationButton.active ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                verticalAlignment: Text.AlignVCenter
                                maximumLineCount: 1
                                elide: Text.ElideRight
                                leftPadding: SentinelTheme.spaceSm
                                rightPadding: SentinelTheme.spaceSm
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusMd
                                color: InteractionTokens.surfaceColor(conversationButton.hovered,
                                                                       conversationButton.down,
                                                                       conversationButton.active,
                                                                       homeChat.modeAccent)
                                border.color: InteractionTokens.borderColor(conversationButton.activeFocus,
                                                                             conversationButton.hovered,
                                                                             conversationButton.active,
                                                                             homeChat.modeAccent)
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Archived") + "  " + homeChat.viewModel.archivedConversationCount
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        font.bold: true
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(84, contentHeight)
                        clip: true
                        spacing: SentinelTheme.spaceXs
                        model: homeChat.viewModel.conversationArchivedSummaries

                        delegate: Label {
                            required property string modelData
                            width: ListView.view.width
                            text: modelData
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontTiny
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0
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
                label: qsTr("Role")
                value: homeChat.viewModel.modelRoleAssignmentSummaries.length > 0
                       ? homeChat.viewModel.modelRoleAssignmentSummaries[0].split(" - ")[1]
                       : qsTr("No role")
                accent: SentinelTheme.calmAccent
                muted: true
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
                label: qsTr("Permission")
                value: homeChat.viewModel.defaultPermissionPolicyState
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
                label: qsTr("Agent")
                value: homeChat.viewModel.agentRuntimeStatus
                accent: SentinelTheme.warning
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

                        Button {
                            id: messageMenuButton
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 24
                            text: "\u22ef"
                            hoverEnabled: true
                            focusPolicy: Qt.StrongFocus
                            onClicked: messageMenu.open()

                            contentItem: Text {
                                text: messageMenuButton.text
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusSm
                                color: InteractionTokens.surfaceColor(messageMenuButton.hovered,
                                                                       messageMenuButton.down,
                                                                       messageMenuButton.activeFocus,
                                                                       homeChat.modeAccent)
                                border.color: InteractionTokens.borderColor(messageMenuButton.activeFocus,
                                                                             messageMenuButton.hovered,
                                                                             false,
                                                                             homeChat.modeAccent)
                            }

                            Menu {
                                id: messageMenu
                                MenuItem {
                                    text: qsTr("Edit")
                                    enabled: recentMessage.messageRole === "user"
                                    onTriggered: {
                                        promptInput.text = recentMessage.content
                                        promptInput.forceActiveFocus()
                                    }
                                }
                                MenuItem {
                                    text: qsTr("Regenerate")
                                    enabled: recentMessage.messageRole !== "user" && homeChat.canSend && !homeChat.sendBusy
                                    onTriggered: homeChat.viewModel.sendMessage(qsTr("Regenerate the previous response."))
                                }
                                MenuItem {
                                    text: qsTr("Pin")
                                    onTriggered: homeChat.viewModel.pinConversation(homeChat.viewModel.activeConversationId)
                                }
                                MenuItem {
                                    text: qsTr("Delete")
                                    onTriggered: homeChat.viewModel.requestPermanentDeleteConversation(homeChat.viewModel.activeConversationId)
                                }
                                MenuItem {
                                    text: qsTr("Export Markdown")
                                    onTriggered: homeChat.viewModel.exportTranscript("markdown")
                                }
                                MenuItem {
                                    text: qsTr("Export TXT")
                                    onTriggered: homeChat.viewModel.exportTranscript("txt")
                                }
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

        Flow {
            Layout.fillWidth: true
            visible: homeChat.viewModel.contextExplainabilityVisible
                     && homeChat.viewModel.conversationRuntimeActiveModel !== "None"
            spacing: SentinelTheme.spaceSm

            StatusChip {
                label: qsTr("Answered By")
                value: homeChat.viewModel.conversationRuntimeActiveModel
                accent: homeChat.modeAccent
                muted: false
            }

            StatusChip {
                label: qsTr("Provider")
                value: homeChat.viewModel.conversationRuntimeActiveRoute
                accent: homeChat.modeAccent
                muted: false
            }

            StatusChip {
                label: qsTr("Reason")
                value: qsTr("Primary role assignment")
                accent: homeChat.modeAccent
                muted: true
            }
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

            DropArea {
                anchors.fill: parent
                onDropped: function(drop) {
                    if (drop.hasUrls && drop.urls.length > 0)
                        homeChat.viewModel.attachFileToChat(drop.urls[0].toString().replace("file://", ""))
                }
            }

            RowLayout {
                id: composerLayout
                x: SentinelTheme.spaceSm
                y: SentinelTheme.spaceXs
                width: parent.width - SentinelTheme.spaceSm * 2
                spacing: SentinelTheme.spaceSm

                Button {
                    id: attachButton
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    text: "+"
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Attach file")
                    onClicked: attachmentDialog.open()

                    contentItem: Text {
                        text: attachButton.text
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontControl
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 17
                        color: InteractionTokens.surfaceColor(attachButton.hovered, attachButton.down,
                                                               attachButton.activeFocus,
                                                               homeChat.modeAccent)
                        border.color: InteractionTokens.borderColor(attachButton.activeFocus,
                                                                     attachButton.hovered,
                                                                     false,
                                                                     homeChat.modeAccent)
                    }
                }

                Button {
                    id: pasteAttachButton
                    Layout.preferredWidth: 54
                    Layout.preferredHeight: 34
                    text: qsTr("Paste")
                    enabled: promptInput.text.trim().length > 0
                    hoverEnabled: true
                    onClicked: {
                        if (homeChat.viewModel.pasteAttachment("pasted-attachment.txt", promptInput.text))
                            promptInput.clear()
                    }

                    contentItem: Text {
                        text: pasteAttachButton.text
                        color: pasteAttachButton.enabled ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    background: Rectangle {
                        radius: 17
                        color: InteractionTokens.surfaceColor(pasteAttachButton.hovered,
                                                               pasteAttachButton.down,
                                                               pasteAttachButton.activeFocus,
                                                               homeChat.modeAccent)
                        border.color: InteractionTokens.borderColor(pasteAttachButton.activeFocus,
                                                                     pasteAttachButton.hovered,
                                                                     false,
                                                                     homeChat.modeAccent)
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
                    onTextChanged: homeChat.viewModel.recoveryDraftText = text
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
                    text: homeChat.sendBusy ? qsTr("Stop") : qsTr("Send")
                    Layout.preferredWidth: 82
                    Layout.alignment: Qt.AlignBottom
                    enabled: homeChat.sendBusy || (promptInput.text.trim().length > 0
                                                   && homeChat.canSend)
                    opacity: enabled ? 1.0 : 0.58
                    onClicked: {
                        if (homeChat.sendBusy)
                            homeChat.viewModel.cancelLocalInference()
                        else
                            homeChat.sendComposerText()
                    }
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

        Flow {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            StatusChip {
                label: qsTr("Workspace")
                value: homeChat.viewModel.selectedWorkspaceName
                accent: homeChat.modeAccent
                selected: true
            }

            StatusChip {
                label: qsTr("Attachments")
                value: homeChat.viewModel.attachmentStatus
                accent: homeChat.viewModel.attachmentSummaries.length > 0 ? SentinelTheme.success
                                                                           : SentinelTheme.textMuted
                muted: homeChat.viewModel.attachmentSummaries.length === 0
            }

            StatusChip {
                label: qsTr("Knowledge")
                value: homeChat.viewModel.localKnowledgeBaseStatus
                accent: homeChat.viewModel.localKnowledgeBaseEnabled ? SentinelTheme.success
                                                                     : SentinelTheme.textMuted
                muted: !homeChat.viewModel.localKnowledgeBaseEnabled
            }
        }

        Repeater {
            model: homeChat.viewModel.attachmentSummaries

            InfoRow {
                required property string modelData
                compact: homeChat.compact
                label: qsTr("Attachment")
                value: modelData
                Layout.fillWidth: true
                valueMaximumLineCount: 2
            }
        }
    }

    FileDialog {
        id: attachmentDialog
        title: qsTr("Attach file")
        fileMode: FileDialog.OpenFile
        nameFilters: [
            qsTr("Documents (*.pdf *.txt *.md *.markdown *.docx *.csv *.json)"),
            qsTr("Source files (*.cpp *.h *.hpp *.qml *.js *.ts *.py *.java *.cs *.go *.rs *.swift)"),
            qsTr("All files (*)")
        ]
        onAccepted: homeChat.viewModel.attachFileToChat(selectedFile.toString().replace("file://", ""))
    }
}
