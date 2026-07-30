// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import Sentinel.Desktop

Rectangle {
    id: root

    property var notifData: null
    property var viewModel: null

    signal markRead(string id)
    signal togglePin(string id)
    signal archive(string id)
    signal remove(string id)

    height: notifData ? contentColumn.implicitHeight + 24 : 0
    radius: SentinelTheme.radiusMd
    color: {
        if (mouseArea.containsMouse) return SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.15)
        if (notifData && !notifData.read && !notifData.archived) return SentinelTheme.withAlpha(SentinelTheme.accent, 0.06)
        return "transparent"
    }

    layer.enabled: mouseArea.containsMouse
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
        shadowVerticalOffset: 1
        shadowBlur: 0.06
        shadowOpacity: 1.0
    }

    Behavior on color { ColorAnimation { duration: MotionTokens.fast } }

    visible: notifData !== null
    clip: true

    Accessible.role: Accessible.ListItem
    Accessible.name: notifData ? notifData.category + ": " + notifData.title + ". " + notifData.body : ""

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onClicked: {
            if (notifData && !notifData.read && viewModel) {
                viewModel.markNotificationRead(notifData.id)
            }
        }
    }

    function priorityColor(p) {
        switch (p) {
            case "Critical": return "#e74c3c"
            case "High": return "#e67e22"
            case "Low": return "#7f8c8d"
            default: return SentinelTheme.accent
        }
    }

    function priorityIcon(p) {
        switch (p) {
            case "Critical": return "\u26A0"
            case "High": return "\u2191"
            case "Low": return "\u2193"
            default: return "\u25CF"
        }
    }

    function categoryIcon(cat) {
        switch (cat) {
            case "Tasks": return "\u26A1"
            case "Models": return "\uD83E\uDDE0"
            case "Updates": return "\uD83D\uDD04"
            case "Brain": return "\uD83D\uDCA1"
            case "Workspace": return "\uD83D\uDCC1"
            case "Security": return "\uD83D\uDEE1\uFE0F"
            default: return "\uD83D\uDD14"
        }
    }

    function timeAgo(ts) {
        var now = new Date().getTime()
        var diff = now - ts
        if (diff < 60000) return "just now"
        if (diff < 3600000) return Math.floor(diff / 60000) + "m ago"
        if (diff < 86400000) return Math.floor(diff / 3600000) + "h ago"
        return Math.floor(diff / 86400000) + "d ago"
    }

    RowLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Rectangle {
            id: priorityBar
            width: 3
            height: parent.height
            radius: 1.5
            color: notifData ? priorityColor(notifData.priority) : "transparent"
            Layout.fillHeight: true

            Accessible.role: Accessible.Graphic
            Accessible.name: "Priority: " + (notifData ? notifData.priority : "Normal")
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            RowLayout {
                spacing: 8
                Layout.fillWidth: true

                Text {
                    text: notifData ? categoryIcon(notifData.category) + " " + notifData.category : ""
                    font.pixelSize: SentinelTheme.fontSmall
                    font.bold: true
                    color: notifData ? priorityColor(notifData.priority) : SentinelTheme.textMuted

                    Accessible.role: Accessible.StaticText
                    Accessible.name: notifData ? "Category: " + notifData.category : ""
                }

                Rectangle {
                    visible: notifData && !notifData.read && !notifData.archived
                    width: 8
                    height: 8
                    radius: 4
                    color: SentinelTheme.accent

                    Accessible.role: Accessible.Indicator
                    Accessible.name: "Unread"
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: notifData ? timeAgo(notifData.timestamp) : ""
                    font.pixelSize: SentinelTheme.fontSmall - 1
                    color: SentinelTheme.textMuted

                    Accessible.role: Accessible.StaticText
                    Accessible.name: notifData ? "Time: " + timeAgo(notifData.timestamp) : ""
                }

                Text {
                    visible: notifData && notifData.pinned
                    text: "\uD83D\uDCCC"
                    font.pixelSize: SentinelTheme.fontSmall

                    Accessible.role: Accessible.Graphic
                    Accessible.name: "Pinned"
                }

                Text {
                    visible: notifData && notifData.snoozed
                    text: "\u23F0"
                    font.pixelSize: SentinelTheme.fontSmall

                    Accessible.role: Accessible.Graphic
                    Accessible.name: "Snoozed until " + (notifData.snoozeUntil ? new Date(notifData.snoozeUntil).toLocaleString() : "later")
                }
            }

            Text {
                text: notifData ? notifData.title : ""
                font.pixelSize: SentinelTheme.fontBody
                font.bold: notifData ? !notifData.read : false
                color: SentinelTheme.textPrimary
                elide: Text.ElideRight
                maximumLineCount: 1
                Layout.fillWidth: true

                Accessible.role: Accessible.StaticText
                Accessible.name: notifData ? notifData.title : ""
            }

            Text {
                text: notifData ? notifData.body : ""
                font.pixelSize: SentinelTheme.fontSmall
                color: SentinelTheme.textMuted
                elide: Text.ElideRight
                maximumLineCount: notifData && notifData.archived ? 1 : 2
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                visible: notifData && notifData.body.length > 0

                Accessible.role: Accessible.StaticText
                Accessible.name: notifData ? notifData.body : ""
            }
        }

        ColumnLayout {
            spacing: 4
            visible: mouseArea.containsMouse

            SentinelButton {
                text: notifData && notifData.pinned ? "\uD83D\uDCCC" : "\uD83D\uDCCD"
                implicitWidth: 28
                implicitHeight: 28
                flat: true
                tooltipText: notifData && notifData.pinned ? "Unpin" : "Pin"
                Accessible.name: notifData && notifData.pinned ? "Unpin notification" : "Pin notification"
                onClicked: {
                    if (notifData && viewModel) {
                        viewModel.pinNotification(notifData.id)
                    }
                }
            }

            SentinelButton {
                text: "\u2713"
                implicitWidth: 28
                implicitHeight: 28
                flat: true
                tooltipText: "Mark read"
                visible: notifData && !notifData.read && !notifData.archived
                Accessible.name: "Mark notification as read"
                onClicked: {
                    if (notifData && viewModel) {
                        viewModel.markNotificationRead(notifData.id)
                    }
                }
            }

            SentinelButton {
                id: snoozeBtn
                text: "\u23F0"
                implicitWidth: 28
                implicitHeight: 28
                flat: true
                tooltipText: "Snooze"
                Accessible.name: "Snooze notification"
                visible: notifData && !notifData.archived
                onClicked: snoozeMenu.open()

                Menu {
                    id: snoozeMenu
                    y: -snoozeMenu.height

                    MenuItem {
                        text: "5 minutes"
                        onTriggered: {
                            if (notifData && viewModel) viewModel.snoozeNotification(notifData.id, 5)
                        }
                    }
                    MenuItem {
                        text: "15 minutes"
                        onTriggered: {
                            if (notifData && viewModel) viewModel.snoozeNotification(notifData.id, 15)
                        }
                    }
                    MenuItem {
                        text: "1 hour"
                        onTriggered: {
                            if (notifData && viewModel) viewModel.snoozeNotification(notifData.id, 60)
                        }
                    }
                    MenuItem {
                        text: "4 hours"
                        onTriggered: {
                            if (notifData && viewModel) viewModel.snoozeNotification(notifData.id, 240)
                        }
                    }
                    MenuItem {
                        text: "Until tomorrow"
                        onTriggered: {
                            if (notifData && viewModel) viewModel.snoozeNotification(notifData.id, 1440)
                        }
                    }
                }
            }

            SentinelButton {
                text: "\uD83D\uDCC1"
                implicitWidth: 28
                implicitHeight: 28
                flat: true
                tooltipText: notifData && notifData.archived ? "Unarchive" : "Archive"
                Accessible.name: notifData && notifData.archived ? "Unarchive notification" : "Archive notification"
                onClicked: {
                    if (notifData && viewModel) {
                        if (notifData.archived) {
                            root.remove(notifData.id)
                        } else {
                            viewModel.archiveNotification(notifData.id)
                        }
                    }
                }
            }

            SentinelButton {
                text: "\u00D7"
                implicitWidth: 28
                implicitHeight: 28
                flat: true
                tooltipText: "Remove"
                Accessible.name: "Remove notification"
                onClicked: {
                    if (notifData && viewModel) {
                        viewModel.removeNotificationById(notifData.id)
                    }
                }
            }
        }
    }
}