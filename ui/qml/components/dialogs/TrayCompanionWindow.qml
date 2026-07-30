// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Window
import Sentinel.Desktop

Window {
    id: companionWin
    required property var viewModel
    property bool alwaysOnTop: true
    property bool menuOpen: false
    property bool hasUnread: false
    property bool scrolledUp: false
    property bool aiResponding: false
    property var lastMessageId: ""
    property real edgeMargin: 14

    title: qsTr("Sentinel Companion")
    width: 380
    height: 520
    minimumWidth: 300
    minimumHeight: 400
    maximumWidth: 600
    maximumHeight: 800
    flags: {
        var f = Qt.Window | Qt.FramelessWindowHint
        if (companionWin.alwaysOnTop) f |= Qt.WindowStaysOnTopHint
        return f
    }
    color: "transparent"

    function positionNearTray() {
        var g = viewModel ? viewModel.cursorScreenGeometry() : null
        if (!g || g.x === undefined) {
            g = { x: Screen.desktopAvailableX, y: Screen.desktopAvailableY,
                  width: Screen.desktopAvailableWidth, height: Screen.desktopAvailableHeight,
                  screenHeight: Screen.height }
        }
        companionWin.x = g.x + g.width - companionWin.width - edgeMargin
        if (g.y > 4) {
            companionWin.y = g.y + 8
        } else if (g.height < g.screenHeight) {
            companionWin.y = g.y + g.height - companionWin.height - 8
        } else {
            companionWin.y = g.y + g.height - companionWin.height - 8
        }
    }

    function toggleVisibility() {
        if (companionWin.visible) {
            companionWin.hide()
            if (viewModel) viewModel.companionChatVisible = false
        } else {
            positionNearTray()
            companionWin.show()
            companionWin.raise()
            companionWin.requestActivate()
            hasUnread = false
            if (viewModel) viewModel.companionChatVisible = true
            promptInput.forceActiveFocus()
        }
    }

    function sendPrompt(text) {
        var trimmed = text.trim()
        if (trimmed.length === 0) return
        if (viewModel) viewModel.sendMessage(trimmed)
        promptInput.text = ""
        aiResponding = true
        Qt.callLater(function() {
            chatListView.positionViewAtEnd()
        })
    }

    function copyMessage(text) {
        if (text.length > 0) {
            var field = textEditDummy
            field.text = text
            field.selectAll()
            field.copy()
        }
        copiedLabel.opacity = 1
        copiedToast.restart()
    }

    Timer {
        id: copiedToast
        interval: 1200
        onTriggered: copiedLabel.opacity = 0
    }

    TextEdit {
        id: textEditDummy
        visible: false
    }

    Label {
        id: copiedLabel
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 80
        text: "Copied!"
        font.pixelSize: SentinelTheme.fontSmall
        color: SentinelTheme.success
        padding: 8
        z: 100
        opacity: 0

        Behavior on opacity {
            NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
        }
    }

    Shortcut {
        sequence: "Esc"
        onActivated: {
            if (companionWin.menuOpen) {
                companionWin.menuOpen = false
            } else {
                companionWin.hide()
                if (viewModel) viewModel.companionChatVisible = false
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+Shift+C"
        onActivated: companionWin.toggleVisibility()
    }

    onActiveChanged: {
        if (!active && visible) {
            hasUnread = true
        }
    }

    // ── Main Card ────────────────────────────────────────────────
    Rectangle {
        id: mainCard
        anchors.fill: parent
        radius: 12
        color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.96)
        border.color: SentinelTheme.withAlpha(
            SentinelTheme.modeAccent(viewModel ? viewModel.currentModeName : "Sentinel"), 0.25)
        border.width: 1

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.30)
            shadowVerticalOffset: 4
            shadowBlur: 0.25
            shadowOpacity: 1.0
        }

        Accessible.role: Accessible.Grouping
        Accessible.name: "Companion chat window"

        // Top sheen
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 1
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            height: 1
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
            radius: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            // ── Header ────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                spacing: 6

                MouseArea {
                    id: headerDragArea
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    cursorShape: Qt.OpenHandCursor
                    property point startPos: Qt.point(0, 0)

                    Accessible.role: Accessible.TitleBar
                    Accessible.name: "Drag to move window"

                    onPressed: function(mouse) {
                        startPos = Qt.point(mouse.x, mouse.y)
                        cursorShape = Qt.ClosedHandCursor
                    }
                    onReleased: {
                        cursorShape = Qt.OpenHandCursor
                    }
                    onPositionChanged: function(mouse) {
                        if (pressed) {
                            var delta = Qt.point(mouse.x - startPos.x, mouse.y - startPos.y)
                            companionWin.x += delta.x
                            companionWin.y += delta.y
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 6

                        // Status dot
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: aiResponding
                                   ? SentinelTheme.warning
                                   : (viewModel && viewModel.companionPaused
                                      ? SentinelTheme.warning
                                      : SentinelTheme.success)
                            opacity: aiResponding ? 1.0 : 0.95

                            SequentialAnimation on opacity {
                                running: aiResponding || !(viewModel && viewModel.companionPaused)
                                loops: Animation.Infinite
                                NumberAnimation { to: 0.4; duration: aiResponding ? 600 : 1100; easing.type: Easing.InOutQuad }
                                NumberAnimation { to: 0.95; duration: aiResponding ? 600 : 1100; easing.type: Easing.InOutQuad }
                            }

                            Accessible.role: Accessible.Indicator
                            Accessible.name: {
                                if (aiResponding) return "AI is responding"
                                if (viewModel && viewModel.companionPaused) return "Companion is paused"
                                return "Companion is active"
                            }
                        }

                        Label {
                            text: qsTr("COMPANION")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 0.8

                            Accessible.role: Accessible.StaticText
                            Accessible.name: "Sentinel Companion"
                        }

                        // Mode badge
                        Rectangle {
                            height: 18
                            radius: 9
                            color: SentinelTheme.withAlpha(
                                SentinelTheme.modeAccent(viewModel ? viewModel.currentModeName : "Sentinel"), 0.15)
                            border.color: SentinelTheme.withAlpha(
                                SentinelTheme.modeAccent(viewModel ? viewModel.currentModeName : "Sentinel"), 0.3)
                            border.width: 1
                            implicitWidth: modeText.implicitWidth + 12

                            Label {
                                id: modeText
                                anchors.centerIn: parent
                                text: viewModel ? viewModel.currentModeName : "Fast"
                                color: SentinelTheme.modeAccent(viewModel ? viewModel.currentModeName : "Sentinel")
                                font.pixelSize: 9
                                font.bold: true
                            }
                        }
                    }
                }

                // Header actions
                RowLayout {
                    spacing: 1

                    SentinelButton {
                        implicitWidth: 28
                        implicitHeight: 28
                        flat: true
                        text: companionWin.menuOpen ? "\uD83D\uDCAC" : "\u2699\uFE0F"
                        font.pixelSize: 12
                        highlighted: companionWin.menuOpen
                        tooltipText: companionWin.menuOpen ? "Chat" : "Menu"
                        Accessible.name: companionWin.menuOpen ? "Show chat view" : "Show options menu"
                        onClicked: companionWin.menuOpen = !companionWin.menuOpen
                    }

                    SentinelButton {
                        implicitWidth: 28
                        implicitHeight: 28
                        flat: true
                        text: companionWin.alwaysOnTop ? "\uD83D\uDCCC" : "\uD83D\uDCCD"
                        font.pixelSize: 12
                        tooltipText: companionWin.alwaysOnTop ? "Unpin from top" : "Keep on top"
                        Accessible.name: companionWin.alwaysOnTop ? "Disable always on top" : "Enable always on top"
                        onClicked: companionWin.alwaysOnTop = !companionWin.alwaysOnTop
                    }

                    SentinelButton {
                        implicitWidth: 28
                        implicitHeight: 28
                        flat: true
                        text: "\u2197"
                        font.pixelSize: 13
                        font.bold: true
                        tooltipText: "Open full app"
                        Accessible.name: "Open full Sentinel application"
                        onClicked: {
                            companionWin.hide()
                            if (viewModel) {
                                viewModel.companionChatVisible = false
                                viewModel.requestWindowActive("Dashboard")
                            }
                        }
                    }

                    SentinelButton {
                        implicitWidth: 28
                        implicitHeight: 28
                        flat: true
                        text: "\u2715"
                        font.pixelSize: 12
                        tooltipText: "Close"
                        Accessible.name: "Close companion window"
                        onClicked: {
                            companionWin.hide()
                            if (viewModel) viewModel.companionChatVisible = false
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
            }

            // ── Body ──────────────────────────────────────────────
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                // ── VIEW 1: Chat ──────────────────────────────────
                ColumnLayout {
                    anchors.fill: parent
                    visible: !companionWin.menuOpen
                    spacing: 8

                    // Chat area
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ListView {
                            id: chatListView
                            anchors.fill: parent
                            anchors.bottomMargin: 4
                            clip: true
                            spacing: 6
                            model: viewModel ? viewModel.chatMessages : null
                            currentIndex: -1

                            Accessible.role: Accessible.List
                            Accessible.name: "Chat messages"

                            onCountChanged: {
                                if (!scrolledUp) {
                                    Qt.callLater(function() {
                                        chatListView.positionViewAtEnd()
                                    })
                                }
                            }

                            onMovementEnded: {
                                scrolledUp = chatListView.contentY < chatListView.contentHeight - chatListView.height - 60
                                if (!scrolledUp) hasUnread = false
                            }

                            delegate: Item {
                                id: msgDelegate
                                required property var model
                                property string msgRole: model.messageRole !== undefined ? model.messageRole : ""
                                property string msgContent: model.content !== undefined ? model.content : ""
                                property string msgTimestamp: model.timestamp !== undefined ? model.timestamp : ""
                                property string msgId: model.messageId !== undefined ? model.messageId : ""
                                property bool isUser: msgRole === "user"

                                width: chatListView.width
                                height: bubbleLoader.active ? bubbleLoader.height : 0

                                Loader {
                                    id: bubbleLoader
                                    width: parent.width
                                    active: msgContent.length > 0

                                    sourceComponent: Rectangle {
                                        width: parent.width
                                        height: bubbleLayout.implicitHeight + 16
                                        radius: 12
                                        color: isUser
                                               ? SentinelTheme.withAlpha(
                                                     SentinelTheme.modeAccent(viewModel ? viewModel.currentModeName : "Sentinel"), 0.12)
                                               : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.6)
                                        border.color: isUser
                                                      ? SentinelTheme.withAlpha(
                                                            SentinelTheme.modeAccent(viewModel ? viewModel.currentModeName : "Sentinel"), 0.25)
                                                      : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.07)
                                        border.width: 1

                                        layer.enabled: mouseInBubble.containsMouse
                                        layer.effect: MultiEffect {
                                            shadowEnabled: true
                                            shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                            shadowVerticalOffset: 1
                                            shadowBlur: 0.08
                                            shadowOpacity: 1.0
                                        }

                                        // Slide-in animation
                                        NumberAnimation on opacity {
                                            from: 0
                                            to: 1
                                            duration: 200
                                            easing.type: Easing.OutCubic
                                        }
                                        NumberAnimation on y {
                                            from: 20
                                            to: 0
                                            duration: 250
                                            easing.type: Easing.OutCubic
                                        }

                                        Accessible.role: Accessible.ListItem
                                        Accessible.name: (isUser ? "You" : "Sentinel AI") + ": " + msgContent

                                        ColumnLayout {
                                            id: bubbleLayout
                                            anchors.fill: parent
                                            anchors.margins: 10
                                            spacing: 4

                                            // Sender + timestamp row
                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 6

                                                Label {
                                                    text: isUser ? "\uD83D\uDC64" : "\uD83E\uDD16"
                                                    font.pixelSize: 12
                                                }

                                                Label {
                                                    text: isUser ? "You" : "Sentinel"
                                                    font.pixelSize: 10
                                                    font.bold: true
                                                    color: isUser
                                                           ? SentinelTheme.modeAccent(viewModel ? viewModel.currentModeName : "Sentinel")
                                                           : SentinelTheme.textPrimary
                                                }

                                                Item { Layout.fillWidth: true }

                                                Label {
                                                    text: msgTimestamp
                                                    font.pixelSize: 9
                                                    color: SentinelTheme.textMuted
                                                    visible: msgTimestamp.length > 0
                                                }
                                            }

                                            // Message content
                                            TextEdit {
                                                Layout.fillWidth: true
                                                text: msgContent
                                                readOnly: true
                                                selectByMouse: true
                                                wrapMode: Text.Wrap
                                                font.pixelSize: SentinelTheme.fontSmall
                                                color: SentinelTheme.textPrimary
                                                selectionColor: SentinelTheme.withAlpha(
                                                    SentinelTheme.modeAccent(viewModel ? viewModel.currentModeName : "Sentinel"), 0.3)
                                                textMargin: 0

                                                Accessible.role: Accessible.StaticText
                                                Accessible.name: msgContent
                                            }

                                            // Hover actions
                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 4
                                                visible: mouseInBubble.containsMouse

                                                Item { Layout.fillWidth: true }

                                                SentinelButton {
                                                    implicitWidth: 22
                                                    implicitHeight: 22
                                                    flat: true
                                                    text: "\uD83D\uDCCB"
                                                    font.pixelSize: 10
                                                    tooltipText: "Copy"
                                                    Accessible.name: "Copy message"
                                                    onClicked: companionWin.copyMessage(msgContent)
                                                }

                                                SentinelButton {
                                                    implicitWidth: 22
                                                    implicitHeight: 22
                                                    flat: true
                                                    text: "\uD83D\uDD04"
                                                    font.pixelSize: 10
                                                    tooltipText: "Retry"
                                                    Accessible.name: "Retry message"
                                                    visible: !isUser
                                                }
                                            }
                                        }

                                        MouseArea {
                                            id: mouseInBubble
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            acceptedButtons: Qt.NoButton
                                            cursorShape: Qt.ArrowCursor
                                        }
                                    }
                                }
                            }

                            // Empty state
                            ColumnLayout {
                                anchors.centerIn: parent
                                visible: chatListView.count === 0
                                spacing: 8
                                width: parent.width - 48

                                Rectangle {
                                    Layout.alignment: Qt.AlignHCenter
                                    implicitWidth: 48
                                    implicitHeight: 48
                                    radius: 24
                                    color: SentinelTheme.withAlpha(
                                        SentinelTheme.modeAccent(viewModel ? viewModel.currentModeName : "Sentinel"), 0.12)
                                    border.color: SentinelTheme.withAlpha(
                                        SentinelTheme.modeAccent(viewModel ? viewModel.currentModeName : "Sentinel"), 0.25)
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: "\u26A1"
                                        font.pixelSize: 24
                                    }

                                    Accessible.role: Accessible.Graphic
                                    Accessible.name: "Companion chat"
                                }

                                Label {
                                    text: qsTr("Sentinel Quick Companion")
                                    font.pixelSize: 15
                                    font.bold: true
                                    color: SentinelTheme.textPrimary
                                    horizontalAlignment: Text.AlignHCenter
                                    Layout.fillWidth: true
                                }

                                Label {
                                    text: qsTr("Ask anything or pick a quick action below.")
                                    font.pixelSize: SentinelTheme.fontSmall
                                    color: SentinelTheme.textMuted
                                    horizontalAlignment: Text.AlignHCenter
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        // Scroll-to-bottom button
                        Rectangle {
                            id: scrollBtn
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 8
                            width: 28
                            height: 28
                            radius: 14
                            color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.85)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.15)
                            border.width: 1
                            visible: scrolledUp

                            layer.enabled: scrollBtnMouse.containsMouse
                            layer.effect: MultiEffect {
                                shadowEnabled: true
                                shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                shadowVerticalOffset: 1
                                shadowBlur: 0.06
                                shadowOpacity: 1.0
                            }

                            Text {
                                anchors.centerIn: parent
                                text: "\u2193"
                                font.pixelSize: 14
                                font.bold: true
                                color: SentinelTheme.textPrimary
                            }

                            MouseArea {
                                id: scrollBtnMouse
                                hoverEnabled: true
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    chatListView.positionViewAtEnd()
                                    scrolledUp = false
                                    hasUnread = false
                                }
                            }

                            Accessible.role: Accessible.Button
                            Accessible.name: "Scroll to bottom"

                            NumberAnimation on opacity {
                                from: 0.6
                                to: 1.0
                                duration: 800
                                easing.type: Easing.InOutQuad
                                running: scrolledUp
                            }
                        }

                        // Typing indicator
                        Rectangle {
                            id: typingIndicator
                            anchors.left: parent.left
                            anchors.leftMargin: 4
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 4
                            height: 32
                            implicitWidth: 60
                            radius: 16
                            color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.6)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.07)
                            border.width: 1
                            visible: aiResponding

                            layer.enabled: true
                            layer.effect: MultiEffect {
                                shadowEnabled: true
                                shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                shadowVerticalOffset: 1
                                shadowBlur: 0.06
                                shadowOpacity: 1.0
                            }

                            Accessible.role: Accessible.Indicator
                            Accessible.name: "AI is typing"

                            RowLayout {
                                anchors.centerIn: parent
                                spacing: 4

                                Repeater {
                                    model: 3
                                    delegate: Rectangle {
                                        width: 6
                                        height: 6
                                        radius: 3
                                        color: SentinelTheme.textMuted

                                            SequentialAnimation on opacity {
                                                loops: Animation.Infinite
                                                running: aiResponding
                                                PauseAnimation { duration: index * 200 }
                                                NumberAnimation {
                                                    from: 0.3; to: 1.0
                                                    duration: 600
                                                    easing.type: Easing.InOutQuad
                                                }
                                                NumberAnimation {
                                                    from: 1.0; to: 0.3
                                                    duration: 600
                                                    easing.type: Easing.InOutQuad
                                                }
                                            }
                                    }
                                }
                            }
                        }
                    }

                    // Quick action chips
                    Flow {
                        Layout.fillWidth: true
                        Layout.preferredHeight: childrenRect.height
                        spacing: 4
                        visible: !companionWin.menuOpen

                        Repeater {
                            model: [
                                { label: "\u26A1 Summarize", prompt: "Please summarize this:" },
                                { label: "\uD83D\uDCA1 Explain", prompt: "Explain in simple terms:" },
                                { label: "\uD83D\uDEE0\uFE0F Fix Code", prompt: "Review and fix this code:" },
                                { label: "\uD83D\uDCDD Note", prompt: "Note down:" },
                                { label: "\uD83D\uDD0D Search", prompt: "Search for information on:" }
                            ]

                            delegate: Rectangle {
                                id: chipRect
                                required property var modelData
                                height: 26
                                width: chipText.implicitWidth + 16
                                radius: 13
                                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                border.width: 1

                                layer.enabled: chipMouse.containsMouse
                                layer.effect: MultiEffect {
                                    shadowEnabled: true
                                    shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                    shadowVerticalOffset: 1
                                    shadowBlur: 0.06
                                    shadowOpacity: 1.0
                                }

                                Accessible.role: Accessible.Button
                                Accessible.name: modelData.label

                                Text {
                                    id: chipText
                                    anchors.centerIn: parent
                                    text: modelData.label
                                    font.pixelSize: 11
                                    color: SentinelTheme.textPrimary
                                }

                                MouseArea {
                                    id: chipMouse
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        promptInput.text = modelData.prompt + " "
                                        promptInput.forceActiveFocus()
                                    }
                                }
                            }
                        }
                    }
                }

                // ── VIEW 2: Options Menu ──────────────────────────
                ScrollView {
                    id: optionsScroll
                    anchors.fill: parent
                    visible: companionWin.menuOpen
                    clip: true
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    ColumnLayout {
                        width: optionsScroll.width > 0 ? optionsScroll.width : parent.width
                        spacing: SentinelTheme.spaceSm

                        // ── Header Status Banner ──────────────────────────
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 52
                            radius: SentinelTheme.radiusMd
                            color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.08)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.25)
                            border.width: 1

                            layer.enabled: true
                            layer.effect: MultiEffect {
                                shadowEnabled: true
                                shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                shadowVerticalOffset: 1
                                shadowBlur: 0.06
                                shadowOpacity: 1.0
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: SentinelTheme.spaceSm
                                spacing: SentinelTheme.spaceSm

                                Rectangle {
                                    implicitWidth: 32
                                    implicitHeight: 32
                                    radius: 16
                                    color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.20)

                                    Label {
                                        anchors.centerIn: parent
                                        text: "🛡"
                                        font.pixelSize: 16
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Label {
                                        text: qsTr("Sentinel Companion AI")
                                        font.pixelSize: SentinelTheme.fontSmall
                                        font.bold: true
                                        color: SentinelTheme.textPrimary
                                    }

                                    Label {
                                        text: viewModel && viewModel.companionPaused ? qsTr("● Companion Paused") : qsTr("● Active • Local Execution")
                                        font.pixelSize: SentinelTheme.fontTiny
                                        color: viewModel && viewModel.companionPaused ? SentinelTheme.warning : SentinelTheme.success
                                    }
                                }

                                Rectangle {
                                    implicitHeight: 20
                                    implicitWidth: verLabel.implicitWidth + 12
                                    radius: 10
                                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                                    Label {
                                        id: verLabel
                                        anchors.centerIn: parent
                                        text: "v1.0.0"
                                        font.pixelSize: SentinelTheme.fontTiny
                                        font.bold: true
                                        color: SentinelTheme.textMuted
                                    }
                                }
                            }
                        }

                        // ── SECTION 1: Assistant Actions ──────────────────
                        Label {
                            text: qsTr("Assistant & Chat")
                            font.pixelSize: SentinelTheme.fontTiny
                            font.bold: true
                            color: SentinelTheme.textMuted
                            leftPadding: 4
                        }

                        Repeater {
                            model: [
                                { icon: "💬", text: qsTr("Quick Prompt"), shortcut: "Ctrl+Shift+C", action: "quickChat" },
                                { icon: "✨", text: qsTr("New Conversation"), shortcut: "Ctrl+N", action: "newChat" },
                                { icon: "🗑️", text: qsTr("Clear Current Chat"), shortcut: "", action: "clearChat" }
                            ]

                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: 36
                                radius: SentinelTheme.radiusSm
                                color: optMouseOver1.containsMouse
                                       ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.02)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)

                                layer.enabled: optMouseOver1.containsMouse
                                layer.effect: MultiEffect {
                                    shadowEnabled: true
                                    shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                    shadowVerticalOffset: 1
                                    shadowBlur: 0.06
                                    shadowOpacity: 1.0
                                }

                                Behavior on color { ColorAnimation { duration: 150 } }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: SentinelTheme.spaceSm
                                    anchors.rightMargin: SentinelTheme.spaceSm
                                    spacing: SentinelTheme.spaceSm

                                    Text {
                                        text: modelData.icon
                                        font.pixelSize: 14
                                    }

                                    Label {
                                        text: modelData.text
                                        font.pixelSize: SentinelTheme.fontSmall
                                        color: SentinelTheme.textPrimary
                                        Layout.fillWidth: true
                                    }

                                    Rectangle {
                                        visible: modelData.shortcut !== ""
                                        implicitHeight: 18
                                        implicitWidth: scLabel1.implicitWidth + 8
                                        radius: 4
                                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                                        Label {
                                            id: scLabel1
                                            anchors.centerIn: parent
                                            text: modelData.shortcut
                                            font.pixelSize: 9
                                            font.bold: true
                                            color: SentinelTheme.textMuted
                                        }
                                    }
                                }

                                MouseArea {
                                    id: optMouseOver1
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (modelData.action === "quickChat") {
                                            companionWin.menuOpen = false
                                        } else if (modelData.action === "newChat") {
                                            if (viewModel) viewModel.createConversation("New Conversation")
                                            companionWin.menuOpen = false
                                        } else if (modelData.action === "clearChat") {
                                            if (viewModel) viewModel.clearChat()
                                            companionWin.menuOpen = false
                                        }
                                    }
                                }
                            }
                        }

                        // ── SECTION 2: Controls & Pin ────────────────────
                        Label {
                            text: qsTr("Window & Behavior")
                            font.pixelSize: SentinelTheme.fontTiny
                            font.bold: true
                            color: SentinelTheme.textMuted
                            leftPadding: 4
                            Layout.topMargin: 4
                        }

                        Repeater {
                            model: [
                                { icon: companionWin.alwaysOnTop ? "📌" : "📍",
                                  text: qsTr("Always On Top"),
                                  shortcut: companionWin.alwaysOnTop ? "ON" : "OFF",
                                  action: "togglePin" },
                                { icon: viewModel && viewModel.companionPaused ? "▶" : "⏸",
                                  text: viewModel && viewModel.companionPaused ? qsTr("Resume Companion") : qsTr("Pause Companion"),
                                  shortcut: viewModel && viewModel.companionPaused ? "PAUSED" : "ACTIVE",
                                  action: "togglePause" }
                            ]

                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: 36
                                radius: SentinelTheme.radiusSm
                                color: optMouseOver2.containsMouse
                                       ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.02)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)

                                layer.enabled: optMouseOver2.containsMouse
                                layer.effect: MultiEffect {
                                    shadowEnabled: true
                                    shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                    shadowVerticalOffset: 1
                                    shadowBlur: 0.06
                                    shadowOpacity: 1.0
                                }

                                Behavior on color { ColorAnimation { duration: 150 } }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: SentinelTheme.spaceSm
                                    anchors.rightMargin: SentinelTheme.spaceSm
                                    spacing: SentinelTheme.spaceSm

                                    Text {
                                        text: modelData.icon
                                        font.pixelSize: 14
                                    }

                                    Label {
                                        text: modelData.text
                                        font.pixelSize: SentinelTheme.fontSmall
                                        color: SentinelTheme.textPrimary
                                        Layout.fillWidth: true
                                    }

                                    Rectangle {
                                        implicitHeight: 18
                                        implicitWidth: scLabel2.implicitWidth + 8
                                        radius: 4
                                        color: modelData.shortcut === "ON" || modelData.shortcut === "ACTIVE"
                                             ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.15)
                                             : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                                        Label {
                                            id: scLabel2
                                            anchors.centerIn: parent
                                            text: modelData.shortcut
                                            font.pixelSize: 9
                                            font.bold: true
                                            color: modelData.shortcut === "ON" || modelData.shortcut === "ACTIVE"
                                                 ? SentinelTheme.accent
                                                 : SentinelTheme.textMuted
                                        }
                                    }
                                }

                                MouseArea {
                                    id: optMouseOver2
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (modelData.action === "togglePin") {
                                            companionWin.alwaysOnTop = !companionWin.alwaysOnTop
                                        } else if (modelData.action === "togglePause") {
                                            if (viewModel) viewModel.toggleCompanionPause()
                                        }
                                    }
                                }
                            }
                        }

                        // ── SECTION 3: Application & Settings ────────────
                        Label {
                            text: qsTr("Application & System")
                            font.pixelSize: SentinelTheme.fontTiny
                            font.bold: true
                            color: SentinelTheme.textMuted
                            leftPadding: 4
                            Layout.topMargin: 4
                        }

                        Repeater {
                            model: [
                                { icon: "🖥️", text: qsTr("Open Full App"), shortcut: "Ctrl+1", action: "dashboard" },
                                { icon: "⚙️", text: qsTr("Settings"), shortcut: "Ctrl+,", action: "settings" },
                                { icon: "🔄", text: qsTr("Check Updates"), shortcut: "", action: "updates" },
                                { icon: "🚪", text: qsTr("Quit Sentinel"), shortcut: "Ctrl+Q", action: "quit" }
                            ]

                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: 36
                                radius: SentinelTheme.radiusSm
                                color: optMouseOver3.containsMouse
                                       ? (modelData.action === "quit" ? SentinelTheme.withAlpha(SentinelTheme.warning, 0.15) : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08))
                                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.02)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)

                                layer.enabled: optMouseOver3.containsMouse
                                layer.effect: MultiEffect {
                                    shadowEnabled: true
                                    shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                    shadowVerticalOffset: 1
                                    shadowBlur: 0.06
                                    shadowOpacity: 1.0
                                }

                                Behavior on color { ColorAnimation { duration: 150 } }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: SentinelTheme.spaceSm
                                    anchors.rightMargin: SentinelTheme.spaceSm
                                    spacing: SentinelTheme.spaceSm

                                    Text {
                                        text: modelData.icon
                                        font.pixelSize: 14
                                    }

                                    Label {
                                        text: modelData.text
                                        font.pixelSize: SentinelTheme.fontSmall
                                        color: modelData.action === "quit" && optMouseOver3.containsMouse
                                             ? SentinelTheme.warning
                                             : SentinelTheme.textPrimary
                                        Layout.fillWidth: true
                                    }

                                    Rectangle {
                                        visible: modelData.shortcut !== ""
                                        implicitHeight: 18
                                        implicitWidth: scLabel3.implicitWidth + 8
                                        radius: 4
                                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                                        Label {
                                            id: scLabel3
                                            anchors.centerIn: parent
                                            text: modelData.shortcut
                                            font.pixelSize: 9
                                            font.bold: true
                                            color: SentinelTheme.textMuted
                                        }
                                    }
                                }

                                MouseArea {
                                    id: optMouseOver3
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        switch (modelData.action) {
                                            case "dashboard":
                                                companionWin.hide()
                                                if (viewModel) {
                                                    viewModel.companionChatVisible = false
                                                    viewModel.requestWindowActive("Dashboard")
                                                }
                                                break
                                            case "settings":
                                                companionWin.hide()
                                                if (viewModel) {
                                                    viewModel.companionChatVisible = false
                                                    viewModel.requestWindowActive("Settings")
                                                }
                                                break
                                            case "updates":
                                                companionWin.hide()
                                                if (viewModel) {
                                                    viewModel.companionChatVisible = false
                                                    viewModel.checkForUpdates()
                                                }
                                                break
                                            case "quit":
                                                Qt.quit()
                                                break
                                        }
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        // Status Footer Card
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 32
                            radius: SentinelTheme.radiusSm
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                            Label {
                                anchors.fill: parent
                                anchors.margins: 6
                                text: viewModel ? viewModel.companionStatus : ""
                                font.pixelSize: 10
                                color: SentinelTheme.textMuted
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }

            // ── Prompt Input ──────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: Math.min(Math.max(promptInput.implicitHeight + 16, 44), 120)
                radius: 12
                color: SentinelTheme.backgroundBase
                border.color: promptInput.activeFocus
                              ? SentinelTheme.modeAccent(viewModel ? viewModel.currentModeName : "Sentinel")
                              : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.12)
                border.width: promptInput.activeFocus ? 1.5 : 1

                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                    shadowVerticalOffset: -1
                    shadowBlur: 0.06
                    shadowOpacity: 1.0
                }

                visible: !companionWin.menuOpen

                Accessible.role: Accessible.EditableText
                Accessible.name: "Chat input"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        TextArea {
                            id: promptInput
                            placeholderText: qsTr("Ask Sentinel...")
                            placeholderTextColor: SentinelTheme.textPlaceholder
                            color: SentinelTheme.textPrimary
                            font.pixelSize: 13
                            wrapMode: Text.Wrap
                            selectByMouse: true
                            background: null
                            topPadding: 4
                            bottomPadding: 4

                            Accessible.name: "Type your message"

                            Keys.onPressed: function(event) {
                                if (event.key === Qt.Key_Return && !(event.modifiers & Qt.ShiftModifier)) {
                                    event.accepted = true
                                    companionWin.sendPrompt(promptInput.text)
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        Layout.alignment: Qt.AlignBottom

                        SentinelButton {
                            text: "\uD83D\uDCE8"
                            implicitHeight: 32
                            implicitWidth: 40
                            font.pixelSize: 14
                            enabled: promptInput.text.trim().length > 0
                            Accessible.name: "Send message"
                            onClicked: companionWin.sendPrompt(promptInput.text)
                        }
                    }
                }
            }

            // ── Footer ────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                visible: !companionWin.menuOpen

                Label {
                    text: viewModel && viewModel.ollamaHealthStatus
                          ? viewModel.ollamaHealthStatus : qsTr("Tray Active")
                    font.pixelSize: 9
                    color: SentinelTheme.textMuted
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                // Unread indicator
                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: SentinelTheme.accent
                    visible: hasUnread && !companionWin.active

                    Accessible.role: Accessible.Indicator
                    Accessible.name: "Unread messages"
                }

                Label {
                    text: qsTr("Enter to send")
                    font.pixelSize: 9
                    color: SentinelTheme.textMuted
                }
            }
        }
    }

    // ── Resize handle ────────────────────────────────────────────
    Rectangle {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 16
        height: 16
    color: SentinelTheme.backgroundBase

        Accessible.role: Accessible.Graphic
        Accessible.name: "Resize window"

        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 4
            anchors.bottomMargin: 4
            width: 8
            height: 8
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.2)
            radius: 1
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeFDiagCursor
            property point startPos: Qt.point(0, 0)
            property size startSize: Qt.size(0, 0)

            onPressed: function(mouse) {
                startPos = Qt.point(mouse.x, mouse.y)
                startSize = Qt.size(companionWin.width, companionWin.height)
            }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    var dx = mouse.x - startPos.x
                    var dy = mouse.y - startPos.y
                    companionWin.width = Math.max(companionWin.minimumWidth,
                                                   Math.min(companionWin.maximumWidth,
                                                            startSize.width + dx))
                    companionWin.height = Math.max(companionWin.minimumHeight,
                                                    Math.min(companionWin.maximumHeight,
                                                             startSize.height + dy))
                }
            }
        }
    }
}
