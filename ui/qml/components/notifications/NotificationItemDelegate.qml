// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

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

    height: notifData ? Math.max(100, contentColumn.implicitHeight + 24) : 0
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
            if (root.notifData && !root.notifData.read && root.viewModel) {
                root.viewModel.markNotificationRead(root.notifData.id)
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
            case "Critical": return "alert-triangle"
            case "High": return "arrow-up"
            case "Low": return "arrow-down"
            default: return "circle"
        }
    }

    function categoryIcon(cat) {
        switch (cat) {
            case "Tasks": return "bolt"
            case "Models": return "brain"
            case "Updates": return "refresh"
            case "Brain": return "bulb"
            case "Workspace": return "folder"
            case "Security": return "shield"
            default: return "bell"
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
            Layout.preferredWidth: 3
            Layout.preferredHeight: parent.height
            radius: 1.5
            color: root.notifData ? root.priorityColor(root.notifData.priority) : "transparent"
            Layout.fillHeight: true

            Accessible.role: Accessible.Graphic
            Accessible.name: "Priority: " + (root.notifData ? root.notifData.priority : "Normal")
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            RowLayout {
                spacing: 8
                Layout.fillWidth: true

                TablerGlyph {
                    text: root.notifData ? root.categoryIcon(root.notifData.category) : ""
                    font.pixelSize: SentinelTheme.fontSmall
                    color: root.notifData ? root.priorityColor(root.notifData.priority) : SentinelTheme.textMuted
                }

                Text {
                    text: root.notifData ? root.notifData.category : ""
                    font.pixelSize: SentinelTheme.fontSmall
                    font.bold: true
                    color: root.notifData ? root.priorityColor(root.notifData.priority) : SentinelTheme.textMuted

                    Accessible.role: Accessible.StaticText
                    Accessible.name: root.notifData ? "Category: " + root.notifData.category : ""
                }

                Rectangle {
                    visible: root.notifData && !root.notifData.read && !root.notifData.archived
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: 8
                    radius: 4
                    color: SentinelTheme.accent

                    Accessible.role: Accessible.Indicator
                    Accessible.name: "Unread"
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: root.notifData ? root.timeAgo(root.notifData.timestamp) : ""
                    font.pixelSize: SentinelTheme.fontSmall - 1
                    color: SentinelTheme.textMuted

                    Accessible.role: Accessible.StaticText
                    Accessible.name: root.notifData ? "Time: " + root.timeAgo(root.notifData.timestamp) : ""
                }

                TablerGlyph {
                    visible: root.notifData && root.notifData.pinned
                    text: "pin"
                    font.pixelSize: SentinelTheme.fontSmall

                    Accessible.role: Accessible.Graphic
                    Accessible.name: "Pinned"
                }

                TablerGlyph {
                    visible: root.notifData && root.notifData.snoozed
                    text: "alarm"
                    font.pixelSize: SentinelTheme.fontSmall

                    Accessible.role: Accessible.Graphic
                    Accessible.name: "Snoozed until " + (root.notifData.snoozeUntil ? new Date(root.notifData.snoozeUntil).toLocaleString() : "later")
                }
            }

            Text {
                text: root.notifData ? root.notifData.title : ""
                font.pixelSize: SentinelTheme.fontBody
                font.bold: root.notifData ? !root.notifData.read : false
                color: SentinelTheme.textPrimary
                elide: Text.ElideRight
                maximumLineCount: 1
                Layout.fillWidth: true

                Accessible.role: Accessible.StaticText
                Accessible.name: root.notifData ? root.notifData.title : ""
            }

            Text {
                text: root.notifData ? root.notifData.body : ""
                font.pixelSize: SentinelTheme.fontSmall
                color: SentinelTheme.textMuted
                elide: Text.ElideRight
                maximumLineCount: root.notifData && root.notifData.archived ? 1 : 2
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                visible: root.notifData && root.notifData.body.length > 0

                Accessible.role: Accessible.StaticText
                Accessible.name: root.notifData ? root.notifData.body : ""
            }
        }

        ColumnLayout {
            spacing: 4
            visible: true
            Layout.preferredWidth: 32

            SentinelButton {
                iconName: root.notifData && root.notifData.pinned ? "pin" : "map-pin"
                implicitWidth: 28
                implicitHeight: 28
                flat: true
                tooltipText: root.notifData && root.notifData.pinned ? qsTr("Unpin") : qsTr("Pin")
                Accessible.name: root.notifData && root.notifData.pinned ? qsTr("Unpin notification") : qsTr("Pin notification")
                onClicked: {
                    if (root.notifData && root.viewModel) {
                        root.viewModel.pinNotification(root.notifData.id)
                    }
                }
            }

            SentinelButton {
                iconName: "check"
                objectName: "notificationMarkReadButton"
                implicitWidth: 28
                implicitHeight: 28
                flat: true
                tooltipText: qsTr("Mark read")
                visible: root.notifData && !root.notifData.read && !root.notifData.archived
                Accessible.name: qsTr("Mark notification as read")
                onClicked: {
                    if (root.notifData && root.viewModel) {
                        root.viewModel.markNotificationRead(root.notifData.id)
                    }
                }
            }

            SentinelButton {
                id: snoozeBtn
                iconName: "alarm"
                implicitWidth: 28
                implicitHeight: 28
                flat: true
                tooltipText: qsTr("Snooze")
                Accessible.name: qsTr("Snooze notification")
                visible: root.notifData && !root.notifData.archived
                onClicked: snoozeMenu.open()

                Menu {
                    id: snoozeMenu
                    y: -snoozeMenu.height

                    MenuItem {
                        text: qsTr("5 minutes")
                        onTriggered: {
                            if (root.notifData && root.viewModel) root.viewModel.snoozeNotification(root.notifData.id, 5)
                        }
                    }
                    MenuItem {
                        text: qsTr("15 minutes")
                        onTriggered: {
                            if (root.notifData && root.viewModel) root.viewModel.snoozeNotification(root.notifData.id, 15)
                        }
                    }
                    MenuItem {
                        text: qsTr("1 hour")
                        onTriggered: {
                            if (root.notifData && root.viewModel) root.viewModel.snoozeNotification(root.notifData.id, 60)
                        }
                    }
                    MenuItem {
                        text: qsTr("4 hours")
                        onTriggered: {
                            if (root.notifData && root.viewModel) root.viewModel.snoozeNotification(root.notifData.id, 240)
                        }
                    }
                    MenuItem {
                        text: qsTr("Until tomorrow")
                        onTriggered: {
                            if (root.notifData && root.viewModel) root.viewModel.snoozeNotification(root.notifData.id, 1440)
                        }
                    }
                }
            }

            SentinelButton {
                objectName: "notificationArchiveButton"
                visible: root.notifData && !root.notifData.archived
                iconName: "folder"
                implicitWidth: 28
                implicitHeight: 28
                flat: true
                tooltipText: qsTr("Archive")
                Accessible.name: qsTr("Archive notification")
                onClicked: {
                    if (root.notifData && root.viewModel)
                        root.viewModel.archiveNotification(root.notifData.id)
                }
            }

            SentinelButton {
                iconName: "x"
                implicitWidth: 28
                implicitHeight: 28
                flat: true
                tooltipText: qsTr("Remove")
                Accessible.name: qsTr("Remove notification")
                onClicked: {
                    if (root.notifData && root.viewModel) {
                        root.viewModel.removeNotificationById(root.notifData.id)
                    }
                }
            }
        }
    }
}
