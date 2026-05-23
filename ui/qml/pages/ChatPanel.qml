import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ShellPanel {
    id: chatPanel
    required property var viewModel
    property bool compact: width < 520
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property bool chatReady: viewModel.localChatSendAvailable
    readonly property int contentPadding: compact ? SentinelTheme.spaceMd : SentinelTheme.spaceLg
    readonly property string uiSelfCheck: "conversation-workflow-search-sections metadata-menu-local-only"
    property string renameStatusText: ""
    property string actionStatusText: ""
    property string conversationSearchText: ""
    property string conversationFilter: "All"

    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.046)
    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)

    function isPinned(conversationId) {
        var index = viewModel.conversationIds.indexOf(conversationId)
        return index >= 0 && viewModel.conversationPinnedSummaries[index] === "Pinned"
    }

    function rowSection(index) {
        var id = viewModel.conversationIds[index]
        if (viewModel.conversationArchivedSummaries[index] === "Archived")
            return "Archived"
        if (isPinned(id))
            return "Pinned"
        return "Recent"
    }

    function filterMatches(index) {
        var id = viewModel.conversationIds[index]
        var archived = viewModel.conversationArchivedSummaries[index] === "Archived"
        if (conversationFilter === "Pinned" && !isPinned(id))
            return false
        if (conversationFilter === "Recent" && (archived || isPinned(id)))
            return false
        if (conversationFilter === "Archived" && !archived)
            return false

        var query = conversationSearchText.trim().toLowerCase()
        if (query.length === 0)
            return true

        var haystack = [
            viewModel.conversationTitles[index],
            viewModel.conversationMessageCountSummaries[index],
            viewModel.conversationLastUpdatedSummaries[index],
            viewModel.conversationArchivedSummaries[index],
            viewModel.conversationPinnedSummaries[index],
            viewModel.conversationActiveSummaries[index]
        ].join(" ").toLowerCase()
        return haystack.indexOf(query) >= 0
    }

    function sectionRank(section) {
        if (section === "Pinned")
            return 0
        if (section === "Recent")
            return 1
        return 2
    }

    function rebuildConversationRows() {
        var rows = []
        for (var i = 0; i < viewModel.conversationIds.length; ++i) {
            if (filterMatches(i))
                rows.push({ sourceIndex: i, section: rowSection(i) })
        }
        rows.sort(function(a, b) {
            var rankDelta = sectionRank(a.section) - sectionRank(b.section)
            return rankDelta !== 0 ? rankDelta : a.sourceIndex - b.sourceIndex
        })

        conversationRows.clear()
        var lastSection = ""
        for (var r = 0; r < rows.length; ++r) {
            conversationRows.append({
                sourceIndex: rows[r].sourceIndex,
                section: rows[r].section,
                showHeader: rows[r].section !== lastSection
            })
            lastSection = rows[r].section
        }
    }

    Component.onCompleted: rebuildConversationRows()
    onConversationSearchTextChanged: rebuildConversationRows()
    onConversationFilterChanged: rebuildConversationRows()

    Connections {
        target: chatPanel.viewModel
        function onChatMessagesChanged() {
            chatPanel.rebuildConversationRows()
        }
    }

    ListModel {
        id: conversationRows
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: chatPanel.contentPadding
        spacing: SentinelTheme.spaceMd

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            Rectangle {
                Layout.preferredWidth: 6
                Layout.preferredHeight: 6
                radius: 3
                color: chatPanel.chatReady ? SentinelTheme.success : SentinelTheme.textMuted
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceXs

                Label {
                    Layout.fillWidth: true
                    text: "AI BRIDGE"
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontTiny
                    font.letterSpacing: 2.4
                }

                Label {
                    Layout.fillWidth: true
                    text: chatPanel.viewModel.localChatSendAvailabilitySummary
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                }
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            StatusChip {
                label: "Provider"
                value: "Local Ollama"
                accent: chatPanel.chatReady ? SentinelTheme.success : SentinelTheme.textMuted
                muted: !chatPanel.chatReady
            }

            StatusChip {
                label: "Restored"
                value: chatPanel.viewModel.conversationLastRestoredStatus
                accent: SentinelTheme.calmAccent
                muted: false
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            ColumnLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceXs

                Label {
                    Layout.fillWidth: true
                    text: "Conversations"
                    color: SentinelTheme.textPrimary
                    font.pixelSize: SentinelTheme.fontCard
                }

                Label {
                    Layout.fillWidth: true
                    text: chatPanel.viewModel.activeConversationCount + " active / "
                          + chatPanel.viewModel.archivedConversationCount + " archived"
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                }
            }

            SentinelButton {
                text: "New"
                Layout.preferredWidth: 78
                onClicked: {
                    var nextNumber = chatPanel.viewModel.conversationStoreConversationCount + 1
                    var createdId = chatPanel.viewModel.createConversation("Conversation " + nextNumber)
                    actionStatusText = createdId.length > 0 ? "New conversation created." : "New conversation unavailable."
                    renameInput.clear()
                }
            }
        }

        SentinelTextField {
            id: conversationSearch
            Layout.fillWidth: true
            placeholderText: "Filter conversations locally"
            text: chatPanel.conversationSearchText
            onTextChanged: chatPanel.conversationSearchText = text
            Keys.onEscapePressed: {
                clear()
                focus = false
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceXs

            Repeater {
                model: ["All", "Pinned", "Recent", "Archived"]
                delegate: Button {
                    id: filterButton
                    required property string modelData
                    text: modelData
                    height: 28
                    leftPadding: SentinelTheme.spaceSm
                    rightPadding: SentinelTheme.spaceSm
                    focusPolicy: Qt.StrongFocus
                    hoverEnabled: true
                    onClicked: chatPanel.conversationFilter = modelData

                    contentItem: Text {
                        text: filterButton.text
                        color: chatPanel.conversationFilter === filterButton.text
                               ? SentinelTheme.textPrimary
                               : SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: SentinelTheme.radiusSm
                        color: InteractionTokens.surfaceColor(filterButton.hovered, filterButton.down,
                                                               chatPanel.conversationFilter === filterButton.text,
                                                               chatPanel.modeAccent)
                        border.color: InteractionTokens.borderColor(filterButton.activeFocus, filterButton.hovered,
                                                                     chatPanel.conversationFilter === filterButton.text,
                                                                     chatPanel.modeAccent)
                    }
                }
            }
        }

        ListView {
            id: conversationList
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 220
            clip: true
            spacing: SentinelTheme.spaceXs
            model: conversationRows
            boundsBehavior: Flickable.StopAtBounds
            boundsMovement: Flickable.StopAtBounds
            maximumFlickVelocity: 2200
            flickDeceleration: 5200
            bottomMargin: SentinelTheme.spaceMd
            activeFocusOnTab: true
            ScrollBar.vertical: ScrollBar {
                id: conversationListScrollBar
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 4
                    radius: 2
                    color: SentinelTheme.withAlpha(chatPanel.modeAccent, conversationListScrollBar.active ? 0.34 : 0.18)
                }
                background: Rectangle {
                    color: "transparent"
                }
            }

            delegate: Column {
                id: conversationDelegate
                required property int sourceIndex
                required property string section
                required property bool showHeader
                readonly property string conversationId: chatPanel.viewModel.conversationIds[sourceIndex]
                readonly property bool active: conversationId === chatPanel.viewModel.activeConversationId
                readonly property bool archived: chatPanel.viewModel.conversationArchivedSummaries[sourceIndex] === "Archived"
                readonly property bool pinned: chatPanel.isPinned(conversationId)

                width: ListView.view.width
                spacing: SentinelTheme.spaceXs

                Label {
                    width: parent.width
                    visible: conversationDelegate.showHeader
                    height: visible ? implicitHeight + SentinelTheme.spaceXs : 0
                    text: conversationDelegate.section
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontTiny
                    font.letterSpacing: 1.4
                    topPadding: SentinelTheme.spaceXs
                }

                Rectangle {
                    id: conversationItem
                    width: parent.width
                    radius: SentinelTheme.radiusMd
                    color: conversationDelegate.active ? SentinelTheme.withAlpha(chatPanel.modeAccent, 0.11)
                                                       : conversationClick.containsMouse ? SentinelTheme.withAlpha(chatPanel.modeAccent, 0.060)
                                                       : conversationDelegate.archived ? SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.030)
                                                                                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.024)
                    border.color: conversationDelegate.active ? SentinelTheme.withAlpha(chatPanel.modeAccent, 0.22)
                                                              : conversationClick.containsMouse ? SentinelTheme.withAlpha(chatPanel.modeAccent, 0.14)
                                                                                                : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.055)
                    opacity: conversationDelegate.archived && !conversationDelegate.active ? 0.72 : 1.0
                    implicitHeight: conversationRow.implicitHeight + SentinelTheme.spaceMd

                    Behavior on color {
                        ColorAnimation {
                            duration: MotionTokens.fast
                            easing.type: MotionTokens.standard
                        }
                    }

                    Behavior on border.color {
                        ColorAnimation {
                            duration: MotionTokens.fast
                            easing.type: MotionTokens.standard
                        }
                    }

                    MouseArea {
                        id: conversationClick
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (!conversationDelegate.active)
                                chatPanel.viewModel.switchConversation(conversationDelegate.conversationId)
                        }
                    }

                    Rectangle {
                        width: 3
                        height: conversationDelegate.active ? parent.height - SentinelTheme.spaceMd : 0
                        radius: 2
                        anchors.left: parent.left
                        anchors.leftMargin: SentinelTheme.spaceXs
                        anchors.verticalCenter: parent.verticalCenter
                        color: chatPanel.modeAccent
                        opacity: conversationDelegate.active ? 0.86 : 0.0
                    }

                    RowLayout {
                        id: conversationRow
                        x: SentinelTheme.spaceSm
                        y: SentinelTheme.spaceXs
                        width: parent.width - SentinelTheme.spaceSm * 2
                        spacing: SentinelTheme.spaceSm

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceXs

                                Text {
                                    Layout.fillWidth: true
                                    text: chatPanel.viewModel.conversationTitles[conversationDelegate.sourceIndex]
                                    color: conversationDelegate.archived && !conversationDelegate.active
                                           ? SentinelTheme.textMuted
                                           : SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontSmall
                                    font.bold: conversationDelegate.active
                                    maximumLineCount: 2
                                    wrapMode: Text.WordWrap
                                    elide: Text.ElideRight
                                }

                                Text {
                                    visible: conversationDelegate.pinned
                                    text: "PIN"
                                    color: chatPanel.modeAccent
                                    font.pixelSize: SentinelTheme.fontTiny
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: (conversationDelegate.active ? "Current / " : "")
                                      + chatPanel.viewModel.conversationMessageCountSummaries[conversationDelegate.sourceIndex]
                                      + " / "
                                      + chatPanel.viewModel.conversationLastUpdatedSummaries[conversationDelegate.sourceIndex]
                                      + (conversationDelegate.archived ? " / Archived" : "")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontTiny
                                maximumLineCount: 2
                                wrapMode: Text.WordWrap
                            }
                        }

                        Button {
                            id: overflowButton
                            Layout.preferredWidth: 30
                            Layout.preferredHeight: 30
                            text: "\u22ef"
                            hoverEnabled: true
                            focusPolicy: Qt.StrongFocus
                            onClicked: overflowMenu.open()

                            contentItem: Text {
                                text: overflowButton.text
                                color: SentinelTheme.textPrimary
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: SentinelTheme.fontControl
                            }

                            background: Rectangle {
                                radius: 15
                                color: InteractionTokens.surfaceColor(overflowButton.hovered, overflowButton.down,
                                                                       overflowMenu.visible,
                                                                       chatPanel.modeAccent)
                                border.color: InteractionTokens.borderColor(overflowButton.activeFocus,
                                                                             overflowButton.hovered,
                                                                             overflowMenu.visible,
                                                                             chatPanel.modeAccent)
                            }

                            Menu {
                                id: overflowMenu
                                width: 244
                                x: overflowButton.width - width
                                y: overflowButton.height + SentinelTheme.spaceXs
                                padding: SentinelTheme.spaceXs
                                modal: true
                                dim: false

                                background: Rectangle {
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                    border.color: InteractionTokens.borderColor(false, true, false,
                                                                                 chatPanel.modeAccent)
                                }

                                MenuItem {
                                    text: "Rename"
                                    onTriggered: {
                                        if (!conversationDelegate.active)
                                            chatPanel.viewModel.switchConversation(conversationDelegate.conversationId)
                                        renameInput.text = chatPanel.viewModel.conversationTitles[conversationDelegate.sourceIndex]
                                        renameInput.forceActiveFocus()
                                        renameInput.selectAll()
                                    }
                                }

                                MenuItem {
                                    text: conversationDelegate.pinned ? "Unpin" : "Pin"
                                    onTriggered: {
                                        var ok = conversationDelegate.pinned
                                                 ? chatPanel.viewModel.unpinConversation(conversationDelegate.conversationId)
                                                 : chatPanel.viewModel.pinConversation(conversationDelegate.conversationId)
                                        actionStatusText = ok
                                                           ? (conversationDelegate.pinned ? "Conversation unpinned." : "Conversation pinned.")
                                                           : "Conversation pin update refused."
                                    }
                                }

                                MenuItem {
                                    text: conversationDelegate.archived ? "Unarchive" : "Archive"
                                    onTriggered: {
                                        var ok = conversationDelegate.archived
                                                 ? chatPanel.viewModel.unarchiveConversation(conversationDelegate.conversationId)
                                                 : chatPanel.viewModel.archiveConversation(conversationDelegate.conversationId)
                                        actionStatusText = ok
                                                           ? (conversationDelegate.archived ? "Conversation unarchived." : "Conversation archived.")
                                                           : "Conversation update refused."
                                    }
                                }

                                MenuSeparator {}

                                MenuItem {
                                    text: "Duplicate"
                                    onTriggered: {
                                        var duplicateId = chatPanel.viewModel.duplicateConversation(
                                            conversationDelegate.conversationId)
                                        actionStatusText = duplicateId.length > 0
                                                           ? chatPanel.viewModel.conversationDuplicateLastResultSummary
                                                           : "Conversation duplicate refused."
                                    }
                                }

                                MenuItem {
                                    id: deleteDisabledItem
                                    text: "Permanent delete is not enabled yet. Archive is available."
                                    enabled: false

                                    contentItem: Text {
                                        text: deleteDisabledItem.text
                                        color: SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontTiny
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            visible: conversationRows.count === 0
            radius: SentinelTheme.radiusMd
            color: SentinelTheme.withAlpha(chatPanel.modeAccent, 0.045)
            border.color: SentinelTheme.withAlpha(chatPanel.modeAccent, 0.12)
            implicitHeight: emptyLabel.implicitHeight + SentinelTheme.spaceMd

            Label {
                id: emptyLabel
                x: SentinelTheme.spaceSm
                y: SentinelTheme.spaceXs
                width: parent.width - SentinelTheme.spaceSm * 2
                text: chatPanel.conversationFilter === "Pinned"
                      ? "No pinned conversations yet."
                      : chatPanel.conversationSearchText.trim().length > 0
                        ? "No local conversation metadata matches this filter."
                        : chatPanel.viewModel.conversationBrowserEmptyStateSummary
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontSmall
                wrapMode: Text.WordWrap
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: SentinelTheme.radiusMd
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.028)
            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
            implicitHeight: selectedStatusColumn.implicitHeight + SentinelTheme.spaceMd

            ColumnLayout {
                id: selectedStatusColumn
                x: SentinelTheme.spaceSm
                y: SentinelTheme.spaceXs
                width: parent.width - SentinelTheme.spaceSm * 2
                spacing: SentinelTheme.spaceXs

                InfoRow {
                    compact: true
                    label: "Current"
                    value: chatPanel.viewModel.activeConversationSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: true
                    label: "Continuity"
                    value: chatPanel.viewModel.conversationLastRestoredStatus
                    Layout.fillWidth: true
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            SentinelTextField {
                id: renameInput
                Layout.fillWidth: true
                placeholderText: "Rename current conversation"
                enabled: chatPanel.viewModel.activeConversationId.length > 0
                onAccepted: {
                    if (renameButton.enabled)
                        renameButton.clicked()
                }
                Keys.onEscapePressed: {
                    clear()
                    focus = false
                }
            }

            SentinelButton {
                id: renameButton
                text: "Rename"
                enabled: renameInput.text.trim().length > 0
                Layout.preferredWidth: 92
                onClicked: {
                    var renamed = chatPanel.viewModel.renameConversation(
                        chatPanel.viewModel.activeConversationId,
                        renameInput.text)
                    chatPanel.renameStatusText = renamed ? "Rename saved." : "Rename refused."
                    if (renamed)
                        renameInput.clear()
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: chatPanel.renameStatusText.length > 0 || chatPanel.actionStatusText.length > 0
            text: chatPanel.renameStatusText.length > 0 ? chatPanel.renameStatusText : chatPanel.actionStatusText
            color: text.indexOf("refused") >= 0 ? SentinelTheme.warning : SentinelTheme.success
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }
    }
}
