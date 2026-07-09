import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

ShellPanel {
    id: homeChat
    required property var viewModel
    property bool compact: width < 760
    readonly property bool inChatMode: (viewModel.chatMessages && viewModel.chatMessages.count > 0) || (promptInput.text.trim().length > 0)
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
    property string sidebarView: "recent"   // "recent" | "pinned" | "archived"
    readonly property real resolutionScale: Math.max(0.7, Math.min(1.4, homeChat.height / 860.0))

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

    // Returns true if the conversation at the given conversationIds index is pinned
    function isPinned(index) {
        return viewModel.conversationPinnedSummaries[index] === "Pinned"
    }

    // Returns true if the conversation at the given conversationIds index is archived
    function isArchived(index) {
        return viewModel.conversationArchivedSummaries[index] === "Archived"
    }

    // Returns indexes of conversations matching the current view/filter
    // view: "recent" | "pinned" | "archived"
    function filteredConversationIndexes(view) {
        var normalized = conversationFilter.trim().toLowerCase()
        var result = []
        for (var i = 0; i < viewModel.conversationIds.length; ++i) {
            var archived = isArchived(i)
            var pinned = isPinned(i)
            if (view === "pinned" && !pinned) continue
            if (view === "archived" && !archived) continue
            if (view === "recent" && (pinned || archived)) continue
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

    onSidebarViewChanged: {
        conversationList.model = filteredConversationIndexes(sidebarView)
    }

    onConversationFilterChanged: {
        conversationList.model = filteredConversationIndexes(sidebarView)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: homeChat.compact ? SentinelTheme.spaceMd : SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceMd

        ShellPanel {
            id: conversationRail
            Layout.preferredWidth: homeChat.inChatMode ? (homeChat.sidebarEffectiveOpen ? Math.min(300, Math.max(238, homeChat.width * 0.22)) : 44) : 0
            visible: Layout.preferredWidth > 0
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

            // Collapsed strip: New Chat + expand toggle
            ColumnLayout {
                visible: !homeChat.sidebarEffectiveOpen
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceXs
                width: 32

                Button {
                    id: collapsedNewChatBtn
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("New Chat")
                    onClicked: homeChat.viewModel.createConversation(qsTr("New Chat"))
                    contentItem: Text {
                        text: "+"
                        color: SentinelTheme.textPrimary
                        font.pixelSize: 20
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: SentinelTheme.radiusMd
                        color: InteractionTokens.surfaceColor(collapsedNewChatBtn.hovered, collapsedNewChatBtn.down, false, homeChat.modeAccent)
                        border.color: InteractionTokens.borderColor(collapsedNewChatBtn.activeFocus, collapsedNewChatBtn.hovered, false, homeChat.modeAccent)
                    }
                }

                Button {
                    id: sidebarToggleCollapsed
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Show sidebar")
                    onClicked: homeChat.conversationSidebarOpen = true
                    contentItem: Text {
                        text: "\u2630"
                        color: SentinelTheme.textPrimary
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: SentinelTheme.radiusMd
                        color: InteractionTokens.surfaceColor(sidebarToggleCollapsed.hovered, sidebarToggleCollapsed.down, false, homeChat.modeAccent)
                        border.color: InteractionTokens.borderColor(sidebarToggleCollapsed.activeFocus, sidebarToggleCollapsed.hovered, false, homeChat.modeAccent)
                    }
                }
            }

            // Expanded panel
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceSm
                visible: homeChat.sidebarEffectiveOpen

                // Top bar: New Chat icon + Search field (with search icon inside) + Hide icon
                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceXs

                    // New Chat icon button
                    Button {
                        id: newChatIconBtn
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 30
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("New Chat")
                        onClicked: {
                            homeChat.viewModel.createConversation(qsTr("New Chat"))
                            homeChat.conversationSidebarOpen = !homeChat.compact
                        }
                        contentItem: Text {
                            text: "\u2B1C"
                            color: SentinelTheme.textPrimary
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: SentinelTheme.radiusMd
                            color: InteractionTokens.surfaceColor(newChatIconBtn.hovered, newChatIconBtn.down, false, homeChat.modeAccent)
                            border.color: InteractionTokens.borderColor(newChatIconBtn.activeFocus, newChatIconBtn.hovered, false, homeChat.modeAccent)
                        }
                    }

                    // Search field with embedded search icon on the right
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        radius: SentinelTheme.radiusMd
                        color: conversationSearch.activeFocus
                               ? SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                               : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.48)
                        border.color: conversationSearch.activeFocus
                                      ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.55)
                                      : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)

                        TextInput {
                            id: conversationSearch
                            anchors.left: parent.left
                            anchors.right: searchIconBtn.left
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: SentinelTheme.spaceSm
                            anchors.rightMargin: SentinelTheme.spaceXs
                            text: homeChat.conversationFilter
                            onTextChanged: homeChat.conversationFilter = text
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontSmall
                            selectionColor: SentinelTheme.withAlpha(homeChat.modeAccent, 0.34)
                            selectedTextColor: SentinelTheme.textPrimary
                            clip: true

                            Text {
                                anchors.fill: parent
                                text: qsTr("Search")
                                color: SentinelTheme.textPlaceholder
                                font.pixelSize: SentinelTheme.fontSmall
                                verticalAlignment: Text.AlignVCenter
                                visible: conversationSearch.text.length === 0 && !conversationSearch.activeFocus
                            }
                        }

                        // Search icon button (right side of field)
                        Button {
                            id: searchIconBtn
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.rightMargin: 3
                            width: 24
                            height: 24
                            hoverEnabled: true
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Search")
                            enabled: conversationSearch.text.trim().length > 0
                            onClicked: homeChat.viewModel.searchConversation(conversationSearch.text)
                            contentItem: Text {
                                text: "\uD83D\uDD0D"
                                font.pixelSize: 11
                                color: searchIconBtn.enabled ? SentinelTheme.textMuted : SentinelTheme.textPlaceholder
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                radius: SentinelTheme.radiusSm
                                color: searchIconBtn.hovered ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.12) : "transparent"
                            }
                        }
                    }

                    // Hide icon button (compact, right side of sidebar)
                    Button {
                        id: sidebarToggleOpen
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 30
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Hide sidebar")
                        onClicked: homeChat.conversationSidebarOpen = false
                        contentItem: Text {
                            text: "\u2715"
                            color: SentinelTheme.textMuted
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: SentinelTheme.radiusMd
                            color: InteractionTokens.surfaceColor(sidebarToggleOpen.hovered, sidebarToggleOpen.down, false, homeChat.modeAccent)
                            border.color: InteractionTokens.borderColor(sidebarToggleOpen.activeFocus, sidebarToggleOpen.hovered, false, homeChat.modeAccent)
                        }
                    }
                }

                // Section tabs: Recent | Pinned | Archived
                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceXs

                    Button {
                        id: recentTabBtn
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Recent chats")
                        onClicked: homeChat.sidebarView = "recent"
                        contentItem: Text {
                            text: qsTr("Recent")
                            color: homeChat.sidebarView === "recent" ? homeChat.modeAccent : SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontTiny
                            font.bold: homeChat.sidebarView === "recent"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: SentinelTheme.radiusSm
                            color: homeChat.sidebarView === "recent"
                                   ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.12)
                                   : (recentTabBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06) : "transparent")
                            border.color: homeChat.sidebarView === "recent"
                                          ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.28)
                                          : "transparent"
                        }
                    }

                    Button {
                        id: pinnedTabBtn
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 26
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Pinned chats")
                        onClicked: homeChat.sidebarView = (homeChat.sidebarView === "pinned" ? "recent" : "pinned")
                        contentItem: Text {
                            text: "\uD83D\uDCCC"
                            font.pixelSize: 12
                            color: homeChat.sidebarView === "pinned" ? homeChat.modeAccent : SentinelTheme.textMuted
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: SentinelTheme.radiusSm
                            color: homeChat.sidebarView === "pinned"
                                   ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.12)
                                   : (pinnedTabBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06) : "transparent")
                            border.color: homeChat.sidebarView === "pinned"
                                          ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.28)
                                          : "transparent"
                        }
                    }

                    Button {
                        id: archivedTabBtn
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 26
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Archived chats")
                        onClicked: homeChat.sidebarView = (homeChat.sidebarView === "archived" ? "recent" : "archived")
                        contentItem: Text {
                            text: "\uD83D\uDDC4"
                            font.pixelSize: 12
                            color: homeChat.sidebarView === "archived" ? homeChat.modeAccent : SentinelTheme.textMuted
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: SentinelTheme.radiusSm
                            color: homeChat.sidebarView === "archived"
                                   ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.12)
                                   : (archivedTabBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06) : "transparent")
                            border.color: homeChat.sidebarView === "archived"
                                          ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.28)
                                          : "transparent"
                        }
                    }
                }

                // Unified conversation list (Recent / Pinned / Archived views)
                ListView {
                    id: conversationList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 100
                    clip: true
                    spacing: SentinelTheme.spaceXs
                    // Re-evaluate model when view changes or conversation data changes
                    model: homeChat.filteredConversationIndexes(homeChat.sidebarView)

                    // Force refresh when sidebarView tab changes
                    onVisibleChanged: {
                        if (visible)
                            model = homeChat.filteredConversationIndexes(homeChat.sidebarView)
                    }

                    // Force refresh when underlying data changes
                    Connections {
                        target: homeChat.viewModel
                        function onConversationIdsChanged() { conversationList.model = homeChat.filteredConversationIndexes(homeChat.sidebarView) }
                        function onConversationTitlesChanged() { conversationList.model = homeChat.filteredConversationIndexes(homeChat.sidebarView) }
                        function onConversationPinnedSummariesChanged() { conversationList.model = homeChat.filteredConversationIndexes(homeChat.sidebarView) }
                        function onConversationArchivedSummariesChanged() { conversationList.model = homeChat.filteredConversationIndexes(homeChat.sidebarView) }
                    }

                    delegate: Item {
                        id: convItem
                        required property int modelData   // this is the sourceIndex into conversationIds
                        readonly property string convId: homeChat.viewModel.conversationIds[modelData]
                        readonly property string convTitle: homeChat.viewModel.conversationTitles[modelData]
                        readonly property bool active: convId === homeChat.viewModel.activeConversationId
                        readonly property bool pinned: homeChat.isPinned(modelData)
                        readonly property bool archived: homeChat.isArchived(modelData)
                        width: ListView.view.width
                        height: 38

                        Button {
                            id: convItemBtn
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: homeChat.viewModel.switchConversation(convItem.convId)

                            contentItem: RowLayout {
                                spacing: 3
                                anchors.left: parent.left
                                anchors.right: convItemMenuBtn.left
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.leftMargin: SentinelTheme.spaceSm
                                anchors.rightMargin: SentinelTheme.spaceXs

                                Text {
                                    Layout.fillWidth: true
                                    text: convItem.convTitle
                                    color: convItem.active ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    font.bold: convItem.active
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                }

                                Text {
                                    visible: convItem.pinned
                                    text: "\uD83D\uDCCC"
                                    font.pixelSize: 9
                                    color: homeChat.modeAccent
                                }
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusMd
                                color: InteractionTokens.surfaceColor(convItemBtn.hovered, convItemBtn.down, convItem.active, homeChat.modeAccent)
                                border.color: InteractionTokens.borderColor(convItemBtn.activeFocus, convItemBtn.hovered, convItem.active, homeChat.modeAccent)
                            }
                        }

                        // Overflow menu button — appears on hover
                        Button {
                            id: convItemMenuBtn
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.rightMargin: 2
                            width: 22
                            height: 22
                            hoverEnabled: true
                            visible: convItemBtn.hovered || convItemMenuBtn.hovered
                            onClicked: convItemMenu.open()
                            contentItem: Text {
                                text: "\u22EF"
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                radius: SentinelTheme.radiusSm
                                color: convItemMenuBtn.hovered ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.14) : "transparent"
                            }

                            Menu {
                                id: convItemMenu

                                // Pin / Unpin
                                MenuItem {
                                    text: convItem.pinned ? qsTr("Unpin") : qsTr("Pin")
                                    onTriggered: {
                                        if (convItem.pinned)
                                            homeChat.viewModel.unpinConversation(convItem.convId)
                                        else
                                            homeChat.viewModel.pinConversation(convItem.convId)
                                    }
                                }

                                // Archive / Unarchive
                                MenuItem {
                                    text: convItem.archived ? qsTr("Unarchive") : qsTr("Archive")
                                    onTriggered: {
                                        if (convItem.archived)
                                            homeChat.viewModel.unarchiveConversation(convItem.convId)
                                        else
                                            homeChat.viewModel.archiveConversation(convItem.convId)
                                    }
                                }

                                // Delete — intentionally disabled at this phase
                                // Backend always refuses; archive is the available action
                                MenuSeparator {}

                                MenuItem {
                                    id: convDeleteDisabledItem
                                    text: qsTr("Permanent delete not yet available — use Archive")
                                    enabled: false

                                    contentItem: Text {
                                        text: convDeleteDisabledItem.text
                                        color: SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontTiny
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }
                    }

                    // Empty state
                    Label {
                        anchors.centerIn: parent
                        visible: conversationList.count === 0
                        text: homeChat.sidebarView === "pinned" ? qsTr("No pinned chats")
                              : homeChat.sidebarView === "archived" ? qsTr("No archived chats")
                              : qsTr("No chats yet")
                        color: SentinelTheme.textPlaceholder
                        font.pixelSize: SentinelTheme.fontSmall
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            spacing: homeChat.inChatMode ? SentinelTheme.spaceMd : SentinelTheme.spaceMd * homeChat.resolutionScale

            Item {
                id: homeCenterWrapper
                Layout.fillWidth: true
                Layout.fillHeight: !homeChat.inChatMode
                visible: !homeChat.inChatMode

                ColumnLayout {
                    anchors.centerIn: parent
                    width: Math.min(parent.width, 720 * homeChat.resolutionScale)
                    spacing: SentinelTheme.spaceMd * 1.5 * homeChat.resolutionScale

                    ColumnLayout {
                        id: greetingArea
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceMd * homeChat.resolutionScale
                        Layout.alignment: Qt.AlignHCenter

                        Label {
                            id: greetingLabel
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                            horizontalAlignment: Text.AlignHCenter
                            text: {
                                var hour = new Date().getHours()
                                if (hour >= 6 && hour < 12) {
                                    return qsTr("Günaydın")
                                } else if (hour >= 12 && hour < 18) {
                                    return qsTr("İyi günler")
                                } else if (hour >= 18 && hour < 22) {
                                    return qsTr("İyi akşamlar")
                                } else {
                                    return qsTr("İyi geceler")
                                }
                            }
                            color: SentinelTheme.textPrimary
                            font.pixelSize: (homeChat.compact ? SentinelTheme.fontDisplay : SentinelTheme.fontHero) * homeChat.resolutionScale
                            font.bold: true
                            font.family: "Outfit"
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Bugün size nasıl yardımcı olabilirim?")
                            color: SentinelTheme.textMuted
                            font.pixelSize: (homeChat.compact ? SentinelTheme.fontBody : SentinelTheme.fontCard) * homeChat.resolutionScale
                            wrapMode: Text.WordWrap
                        }
                    }

                    Rectangle {
                        id: homeComposer
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter
                        radius: SentinelTheme.radiusMd * homeChat.resolutionScale
                        color: homePromptInput.activeFocus
                               ? SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.82)
                               : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.68)
                        border.color: InteractionTokens.borderColor(homePromptInput.activeFocus, homeComposerMouse.containsMouse,
                                                                     false, homeChat.modeAccent)
                        implicitHeight: Math.max(76 * homeChat.resolutionScale, homeComposerLayout.implicitHeight + SentinelTheme.spaceMd * homeChat.resolutionScale)

                        MouseArea {
                            id: homeComposerMouse
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
                            id: homeComposerLayout
                            x: SentinelTheme.spaceSm * homeChat.resolutionScale
                            y: SentinelTheme.spaceXs * homeChat.resolutionScale
                            width: parent.width - (SentinelTheme.spaceSm * 2) * homeChat.resolutionScale
                            spacing: SentinelTheme.spaceSm * homeChat.resolutionScale

                            Button {
                                id: homeAttachButton
                                Layout.preferredWidth: 34 * homeChat.resolutionScale
                                Layout.preferredHeight: 34 * homeChat.resolutionScale
                                text: "+"
                                hoverEnabled: true
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Attach file")
                                onClicked: attachmentDialog.open()

                                contentItem: Text {
                                    text: homeAttachButton.text
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontControl * homeChat.resolutionScale
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                background: Rectangle {
                                    radius: 17 * homeChat.resolutionScale
                                    color: InteractionTokens.surfaceColor(homeAttachButton.hovered, homeAttachButton.down,
                                                                           homeAttachButton.activeFocus,
                                                                           homeChat.modeAccent)
                                    border.color: InteractionTokens.borderColor(homeAttachButton.activeFocus,
                                                                                 homeAttachButton.hovered,
                                                                                 false,
                                                                                 homeChat.modeAccent)
                                }
                            }

                            TextArea {
                                id: homePromptInput
                                Layout.fillWidth: true
                                Layout.minimumHeight: 48 * homeChat.resolutionScale
                                Layout.maximumHeight: 126 * homeChat.resolutionScale
                                placeholderText: homeChat.chatReady ? (homeChat.sendBusy ? qsTr("Sentinel is responding") : qsTr("Ask Sentinel"))
                                                                    : homeChat.viewModel.localChatSendAvailabilitySummary
                                enabled: !homeChat.viewModel.activeConversationArchived && !homeChat.sendBusy
                                color: SentinelTheme.textPrimary
                                placeholderTextColor: SentinelTheme.textPlaceholder
                                wrapMode: TextEdit.WordWrap
                                selectByMouse: true
                                selectionColor: SentinelTheme.withAlpha(homeChat.modeAccent, 0.34)
                                selectedTextColor: SentinelTheme.textPrimary
                                font.pixelSize: (homeChat.compact ? SentinelTheme.fontBody : SentinelTheme.fontControl) * homeChat.resolutionScale
                                onTextChanged: {
                                    homeChat.viewModel.recoveryDraftText = text
                                    if (text.trim().length > 0) {
                                        promptInput.text = text
                                        promptInput.cursorPosition = promptInput.text.length
                                        homePromptInput.clear()
                                        promptInput.forceActiveFocus()
                                    }
                                }
                                background: Rectangle {
                                    color: "transparent"
                                }
                                Keys.onPressed: function(event) {
                                    if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                                            && !(event.modifiers & Qt.ShiftModifier)) {
                                        promptInput.text = homePromptInput.text
                                        homePromptInput.clear()
                                        homeChat.sendComposerText()
                                        event.accepted = true
                                    } else if (event.key === Qt.Key_Escape) {
                                        focus = false
                                        event.accepted = true
                                    }
                                }
                            }

                            SentinelButton {
                                id: homeSendButton
                                text: homeChat.sendBusy ? qsTr("Stop") : qsTr("Send")
                                Layout.preferredWidth: 82 * homeChat.resolutionScale
                                Layout.alignment: Qt.AlignBottom
                                font.pixelSize: SentinelTheme.fontControl * homeChat.resolutionScale
                                enabled: homeChat.sendBusy || (homePromptInput.text.trim().length > 0
                                                               && homeChat.canSend)
                                opacity: enabled ? 1.0 : 0.58
                                onClicked: {
                                    if (homeChat.sendBusy) {
                                        homeChat.viewModel.cancelLocalInference()
                                    } else {
                                        promptInput.text = homePromptInput.text
                                        homePromptInput.clear()
                                        homeChat.sendComposerText()
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter
                        spacing: SentinelTheme.spaceSm * homeChat.resolutionScale

                        Label {
                            text: qsTr("Provider")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                        }

                        ComboBox {
                            id: homeProviderSelector
                            Layout.preferredWidth: Math.min(200, 160 * homeChat.resolutionScale)
                            Layout.preferredHeight: 32 * homeChat.resolutionScale
                            model: homeChat.viewModel.selectableRuntimeProviderLabels
                            currentIndex: homeChat.viewModel.selectableRuntimeProviderIds.indexOf(homeChat.viewModel.selectedRuntimeProvider)
                            onActivated: function(index) {
                                if (index >= 0 && index < homeChat.viewModel.selectableRuntimeProviderIds.length)
                                    homeChat.viewModel.selectedRuntimeProvider = homeChat.viewModel.selectableRuntimeProviderIds[index]
                            }
                            displayText: currentIndex >= 0 ? homeChat.viewModel.selectableRuntimeProviderLabels[currentIndex] : homeChat.viewModel.activeRuntimeProviderLabel

                            contentItem: Text {
                                text: homeProviderSelector.displayText
                                color: homeProviderSelector.currentIndex >= 0 ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: SentinelTheme.spaceSm
                                rightPadding: SentinelTheme.spaceSm
                                elide: Text.ElideRight
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusMd * homeChat.resolutionScale
                                color: homeProviderSelector.hovered
                                       ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.10)
                                       : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.48)
                                border.color: homeProviderSelector.activeFocus
                                              ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.62)
                                              : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                            }

                            popup: Popup {
                                y: homeProviderSelector.height + 4
                                width: homeProviderSelector.width
                                implicitHeight: Math.min(contentItem.implicitHeight + 2 * padding, 280)
                                padding: SentinelTheme.spaceXs

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.96)
                                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                }

                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: contentHeight
                                    model: homeProviderSelector.delegateModel
                                    currentIndex: homeProviderSelector.highlightedIndex
                                    ScrollBar.vertical: ScrollBar {
                                        policy: ScrollBar.AsNeeded
                                    }
                                }
                            }

                            delegate: ItemDelegate {
                                width: homeProviderSelector.width - SentinelTheme.spaceXs * 2
                                height: 32 * homeChat.resolutionScale
                                highlighted: homeProviderSelector.highlightedIndex === index
                                hoverEnabled: true

                                contentItem: Text {
                                    text: modelData
                                    color: highlighted ? homeChat.modeAccent : SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                    font.bold: homeChat.viewModel.selectableRuntimeProviderIds[index] === homeChat.viewModel.selectedRuntimeProvider
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                    leftPadding: SentinelTheme.spaceSm
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusSm
                                    color: highlighted || hovered
                                           ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.10)
                                           : "transparent"
                                }
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 1
                            Layout.preferredHeight: 20 * homeChat.resolutionScale
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                        }

                        Label {
                            text: qsTr("Model")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                        }

                        ComboBox {
                            id: homeModelSelector
                            Layout.preferredWidth: Math.min(280, 220 * homeChat.resolutionScale)
                            Layout.preferredHeight: 32 * homeChat.resolutionScale
                            model: homeChat.viewModel.ollamaModelNames
                            currentIndex: {
                                var names = homeChat.viewModel.ollamaModelNames
                                var selected = homeChat.viewModel.selectedLocalModel
                                for (var i = 0; i < names.length; ++i) {
                                    if (names[i] === selected) return i
                                }
                                return -1
                            }
                            onActivated: function(index) {
                                if (index >= 0 && index < homeChat.viewModel.ollamaModelNames.length)
                                    homeChat.viewModel.selectedLocalModel = homeChat.viewModel.ollamaModelNames[index]
                            }
                            displayText: currentIndex >= 0 ? homeChat.viewModel.ollamaModelNames[currentIndex] : qsTr("No model")

                            contentItem: Text {
                                text: homeModelSelector.displayText
                                color: homeModelSelector.currentIndex >= 0 ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: SentinelTheme.spaceSm
                                rightPadding: SentinelTheme.spaceSm
                                elide: Text.ElideRight
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusMd * homeChat.resolutionScale
                                color: homeModelSelector.hovered
                                       ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.10)
                                       : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.48)
                                border.color: homeModelSelector.activeFocus
                                              ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.62)
                                              : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                            }

                            popup: Popup {
                                y: homeModelSelector.height + 4
                                width: homeModelSelector.width
                                implicitHeight: Math.min(contentItem.implicitHeight + 2 * padding, 280)
                                padding: SentinelTheme.spaceXs

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.96)
                                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                }

                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: contentHeight
                                    model: homeModelSelector.delegateModel
                                    currentIndex: homeModelSelector.highlightedIndex
                                    ScrollBar.vertical: ScrollBar {
                                        policy: ScrollBar.AsNeeded
                                    }
                                }
                            }

                            delegate: ItemDelegate {
                                width: homeModelSelector.width - SentinelTheme.spaceXs * 2
                                height: 32 * homeChat.resolutionScale
                                highlighted: homeModelSelector.highlightedIndex === index
                                hoverEnabled: true

                                contentItem: Text {
                                    text: modelData
                                    color: highlighted ? homeChat.modeAccent : SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                    font.bold: modelData === homeChat.viewModel.selectedLocalModel
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                    leftPadding: SentinelTheme.spaceSm
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusSm
                                    color: highlighted || hovered
                                           ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.10)
                                           : "transparent"
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: homeChat.inChatMode
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

        ListView {
            id: recentMessages
            property bool followNewMessages: true
            function nearBottom() {
                return contentHeight <= height || (contentY + height >= contentHeight - 96)
            }
            Layout.fillWidth: true
            Layout.fillHeight: homeChat.inChatMode
            Layout.minimumHeight: homeChat.inChatMode ? 210 : 0
            visible: homeChat.inChatMode
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

                                MenuSeparator {}

                                MenuItem {
                                    readonly property bool convPinned: {
                                        var idx = homeChat.viewModel.conversationIds.indexOf(homeChat.viewModel.activeConversationId)
                                        return idx >= 0 && homeChat.isPinned(idx)
                                    }
                                    text: convPinned ? qsTr("Unpin conversation") : qsTr("Pin conversation")
                                    onTriggered: {
                                        var idx = homeChat.viewModel.conversationIds.indexOf(homeChat.viewModel.activeConversationId)
                                        if (idx >= 0) {
                                            if (convPinned)
                                                homeChat.viewModel.unpinConversation(homeChat.viewModel.activeConversationId)
                                            else
                                                homeChat.viewModel.pinConversation(homeChat.viewModel.activeConversationId)
                                        }
                                    }
                                }
                                MenuItem {
                                    readonly property bool convArchived: {
                                        var idx = homeChat.viewModel.conversationIds.indexOf(homeChat.viewModel.activeConversationId)
                                        return idx >= 0 && homeChat.isArchived(idx)
                                    }
                                    text: convArchived ? qsTr("Unarchive conversation") : qsTr("Archive conversation")
                                    onTriggered: {
                                        if (convArchived)
                                            homeChat.viewModel.unarchiveConversation(homeChat.viewModel.activeConversationId)
                                        else
                                            homeChat.viewModel.archiveConversation(homeChat.viewModel.activeConversationId)
                                    }
                                }

                                MenuSeparator {}

                                MenuItem {
                                    text: qsTr("Export Markdown")
                                    onTriggered: homeChat.viewModel.exportTranscript("markdown")
                                }
                                MenuItem {
                                    text: qsTr("Export TXT")
                                    onTriggered: homeChat.viewModel.exportTranscript("txt")
                                }

                                MenuSeparator {}

                                MenuItem {
                                    id: msgDeleteDisabledItem
                                    text: qsTr("Permanent delete not yet available — use Archive")
                                    enabled: false
                                    contentItem: Text {
                                        text: msgDeleteDisabledItem.text
                                        color: SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontTiny
                                        wrapMode: Text.WordWrap
                                    }
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
            visible: homeChat.inChatMode && homeChat.viewModel.conversationHistoryMessageCount <= 1
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
            visible: homeChat.inChatMode && homeChat.streamingActive
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
            visible: homeChat.inChatMode
                     && homeChat.sendState !== "idle"
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
            visible: homeChat.inChatMode && homeChat.viewModel.promptContextInjectionEnabled
            text: homeChat.viewModel.promptContextUsedSummary
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            maximumLineCount: 1
            elide: Text.ElideRight
        }

        Flow {
            Layout.fillWidth: true
            visible: homeChat.inChatMode
                     && homeChat.viewModel.contextExplainabilityVisible
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
            visible: homeChat.inChatMode
                     && (homeChat.viewModel.conversationSummaryGenerationStatus === "Planned"
                         || homeChat.viewModel.conversationSummaryAvailable
                         || homeChat.viewModel.conversationSummaryGenerationStatus === "Blocked")
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
            visible: homeChat.inChatMode
                     && homeChat.viewModel.contextExplainabilityVisible
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
            visible: homeChat.inChatMode
            Layout.maximumWidth: 9999
            Layout.alignment: Qt.AlignHCenter
            radius: SentinelTheme.radiusMd * homeChat.resolutionScale
            color: promptInput.activeFocus
                   ? SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.82)
                   : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.68)
            border.color: InteractionTokens.borderColor(promptInput.activeFocus, composerMouse.containsMouse,
                                                         false, homeChat.modeAccent)
            implicitHeight: Math.max(76 * homeChat.resolutionScale, composerLayout.implicitHeight + SentinelTheme.spaceMd * homeChat.resolutionScale)

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
                x: SentinelTheme.spaceSm * homeChat.resolutionScale
                y: SentinelTheme.spaceXs * homeChat.resolutionScale
                width: parent.width - (SentinelTheme.spaceSm * 2) * homeChat.resolutionScale
                spacing: SentinelTheme.spaceSm * homeChat.resolutionScale

                Button {
                    id: attachButton
                    Layout.preferredWidth: 34 * homeChat.resolutionScale
                    Layout.preferredHeight: 34 * homeChat.resolutionScale
                    text: "+"
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Attach file")
                    onClicked: attachmentDialog.open()

                    contentItem: Text {
                        text: attachButton.text
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontControl * homeChat.resolutionScale
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 17 * homeChat.resolutionScale
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
                    Layout.preferredWidth: 54 * homeChat.resolutionScale
                    Layout.preferredHeight: 34 * homeChat.resolutionScale
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
                        font.pixelSize: SentinelTheme.fontTiny * homeChat.resolutionScale
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    background: Rectangle {
                        radius: 17 * homeChat.resolutionScale
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
                    visible: homeChat.inChatMode
                    Layout.preferredWidth: 116 * homeChat.resolutionScale
                    Layout.preferredHeight: 34 * homeChat.resolutionScale
                    text: homeChat.viewModel.conversationSummaryAvailable ? qsTr("Summary Ready") : qsTr("Generate Summary")
                    enabled: homeChat.viewModel.localChatSendAvailable && !homeChat.sendBusy
                             && homeChat.viewModel.conversationSummaryGenerationStatus !== "Planned"
                    hoverEnabled: true
                    onClicked: homeChat.viewModel.requestConversationSummaryGeneration()

                    contentItem: Text {
                        text: summaryAction.text
                        color: summaryAction.enabled ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny * homeChat.resolutionScale
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    background: Rectangle {
                        radius: 17 * homeChat.resolutionScale
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
                    Layout.minimumHeight: 48 * homeChat.resolutionScale
                    Layout.maximumHeight: 126 * homeChat.resolutionScale
                    placeholderText: homeChat.chatReady ? (homeChat.sendBusy ? qsTr("Sentinel is responding") : qsTr("Ask Sentinel"))
                                                        : homeChat.viewModel.localChatSendAvailabilitySummary
                    enabled: !homeChat.viewModel.activeConversationArchived && !homeChat.sendBusy
                    color: SentinelTheme.textPrimary
                    placeholderTextColor: SentinelTheme.textPlaceholder
                    wrapMode: TextEdit.WordWrap
                    selectByMouse: true
                    selectionColor: SentinelTheme.withAlpha(homeChat.modeAccent, 0.34)
                    selectedTextColor: SentinelTheme.textPrimary
                    font.pixelSize: (homeChat.compact ? SentinelTheme.fontBody : SentinelTheme.fontControl) * homeChat.resolutionScale
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
                    Layout.preferredWidth: 82 * homeChat.resolutionScale
                    Layout.alignment: Qt.AlignBottom
                    font.pixelSize: SentinelTheme.fontControl * homeChat.resolutionScale
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
            visible: homeChat.inChatMode && !homeChat.chatReady
            text: homeChat.disabledReason + (homeChat.chatReady ? "" : qsTr(" Local Ollama only. No cloud provider active."))
            color: !homeChat.chatReady ? SentinelTheme.textMuted : SentinelTheme.warning
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
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
