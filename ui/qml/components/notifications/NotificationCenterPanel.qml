// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts
import Sentinel.Desktop

ShellPanel {
    id: root

    property var viewModel: null
    property string activeFilter: "All"
    property string searchQuery: ""
    property QtObject currentGroup: null
    property var expandedGroups: ({})

    signal closeRequested()

    implicitWidth: 480
    implicitHeight: 600
    panelColor: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.97)
    borderWidth: 1
    borderColor: SentinelTheme.withAlpha(SentinelTheme.accent, 0.12)
    radius: SentinelTheme.radiusLg

    Accessible.role: Accessible.Dialog
    Accessible.name: "Notification center"

    Keys.onEscapePressed: root.closeRequested()
    Keys.onUpPressed: notificationList.decrementCurrentIndex()
    Keys.onDownPressed: notificationList.incrementCurrentIndex()
    Keys.onReturnPressed: {
        var idx = notificationList.currentIndex
        if (idx >= 0 && idx < notificationModel.count) {
            var item = notificationModel.get(idx)
            if (item && !item.read && viewModel) {
                viewModel.markNotificationRead(item.id)
            }
        }
    }

    function parseNotification(jsonStr) {
        try { return JSON.parse(jsonStr) }
        catch(e) { return null }
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

    function toggleGroup(category) {
        if (root.expandedGroups[category]) {
            root.expandedGroups[category] = false
        } else {
            root.expandedGroups[category] = true
        }
        root.expandedGroups = root.expandedGroups
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Header ───────────────────────────────────────────────
        ShellPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            panelColor: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 12
                spacing: 8

                Text {
                    text: qsTr("Notifications")
                    font.pixelSize: SentinelTheme.fontCard
                    font.bold: true
                    color: SentinelTheme.textPrimary

                    Accessible.role: Accessible.StaticText
                    Accessible.name: qsTr("Notifications")
                }

                Text {
                    text: {
                        var unreadCount = root.viewModel ? root.viewModel.unreadNotificationCount : 0
                        return unreadCount > 0 ? qsTr("(%1 unread)").arg(unreadCount) : ""
                    }
                    font.pixelSize: SentinelTheme.fontSmall
                    color: SentinelTheme.accent
                    visible: (root.viewModel ? root.viewModel.unreadNotificationCount : 0) > 0
                }

                Item { Layout.fillWidth: true }

                // ── DND toggle ───────────────────────────────────
                SentinelButton {
                    iconName: root.viewModel && root.viewModel.dndEnabled ? "volume-off" : "volume-2"
                    implicitWidth: 32
                    implicitHeight: 32
                    flat: true
                    tooltipText: root.viewModel && root.viewModel.dndEnabled ? qsTr("Do Not Disturb is on") : qsTr("Do Not Disturb is off")
                    Accessible.name: root.viewModel && root.viewModel.dndEnabled ? qsTr("Disable do not disturb") : qsTr("Enable do not disturb")
                    highlighted: root.viewModel && root.viewModel.dndEnabled
                    onClicked: {
                        if (root.viewModel) root.viewModel.dndEnabled = !root.viewModel.dndEnabled
                    }
                }

                SentinelButton {
                    text: qsTr("Mark all read")
                    flat: true
                    font.pixelSize: SentinelTheme.fontSmall
                    Accessible.name: qsTr("Mark all notifications as read")
                    onClicked: {
                        if (root.viewModel) root.viewModel.markAllNotificationsRead()
                    }
                }

                SentinelButton {
                    text: qsTr("Clear archived")
                    flat: true
                    font.pixelSize: SentinelTheme.fontSmall
                    Accessible.name: qsTr("Clear all archived notifications")
                    onClicked: {
                        if (root.viewModel) root.viewModel.clearArchivedNotifications()
                    }
                }

                SentinelButton {
                    iconName: "x"
                    implicitWidth: 32
                    implicitHeight: 32
                    flat: true
                    font.pixelSize: SentinelTheme.fontCard
                    Accessible.name: qsTr("Close notification center")
                    onClicked: root.closeRequested()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
        }

        // ── Search + Category filters ───────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 12
            Layout.bottomMargin: 8
            spacing: 8

            SentinelTextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Search notifications...")
                Accessible.name: qsTr("Search notifications")
                onTextChanged: {
                    root.searchQuery = text
                    if (root.viewModel) root.viewModel.notificationSearchQuery = text
                }
            }

            Repeater {
                model: root.viewModel ? root.viewModel.notificationCategories : ["All"]

                delegate: SentinelButton {
        required property var modelData
        id: delegateScope1
                    text: delegateScope1.modelData === "All" ? qsTr("All") : delegateScope1.modelData
                    iconName: delegateScope1.modelData === "All" ? "" : root.categoryIcon(delegateScope1.modelData)
                    flat: true
                    font.pixelSize: SentinelTheme.fontSmall
                    font.bold: root.activeFilter === delegateScope1.modelData
                    highlighted: root.activeFilter === delegateScope1.modelData
                    Accessible.name: "Filter by category: " + delegateScope1.modelData
                    onClicked: {
                        root.activeFilter = delegateScope1.modelData
                        if (root.viewModel) root.viewModel.notificationCategoryFilter = delegateScope1.modelData
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
        }

        // ── Notification list (grouped + keyboard navigable) ────
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            clip: true

            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ListView {
                id: notificationList
                model: ListModel { id: notificationModel }
                delegate: notificationDelegate
                spacing: 2
                boundsBehavior: Flickable.StopAtBounds
                focus: true

                Accessible.role: Accessible.List
                Accessible.name: "Notification list"

                function refreshNotifications() {
                    notificationModel.clear()
                    var summaries = root.viewModel ? root.viewModel.notificationFilteredSummaries : []
                    var groups = {}
                    for (var i = 0; i < summaries.length; ++i) {
                        var n = root.parseNotification(summaries[i])
                        if (!n) continue
                        if (n.snoozed) {
                            var now = new Date().getTime()
                            if (n.snoozeUntil && n.snoozeUntil > now) continue
                        }
                        var cat = n.category || "Other"
                        if (!groups[cat]) groups[cat] = []
                        groups[cat].push(n)
                    }
                    var catKeys = Object.keys(groups)
                    for (var g = 0; g < catKeys.length; ++g) {
                        var cg = catKeys[g]
                        var expanded = root.expandedGroups[cg] !== false
                        notificationModel.append({
                            type: "groupHeader",
                            groupName: cg,
                            groupCount: groups[cg].length,
                            expanded: expanded
                        })
                        if (expanded) {
                            var items = groups[cg]
                            for (var k = 0; k < items.length; ++k) {
                                var ni = items[k]
                                notificationModel.append({
                                    type: "notification",
                                    id: ni.id,
                                    category: ni.category,
                                    title: ni.title,
                                    body: ni.body,
                                    priority: ni.priority || "Normal",
                                    timestamp: ni.timestamp || 0,
                                    pinned: ni.pinned || false,
                                    archived: ni.archived || false,
                                    read: ni.read || false,
                                    snoozed: ni.snoozed || false,
                                    snoozeUntil: ni.snoozeUntil || 0
                                })
                            }
                        }
                    }
                }

                Component.onCompleted: refreshNotifications()
            }

            Connections {
                target: root.viewModel
                function onNativeExperienceChanged() {
                    notificationList.refreshNotifications()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
        }

        // ── Footer: DND schedule + channel settings + lifecycle ───
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            spacing: 6

            // ── Channel mute toggles ──────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: qsTr("Channels:")
                    font.pixelSize: SentinelTheme.fontSmall - 1
                    color: SentinelTheme.textMuted
                    Accessible.role: Accessible.StaticText
                    Accessible.name: qsTr("Notification channels")
                }

                Repeater {
                    model: root.viewModel ? root.viewModel.notificationCategories : []

                    delegate: SentinelButton {
        required property var modelData
        id: delegateScope2
                        property bool muted: root.viewModel ? root.viewModel.isChannelMuted(delegateScope2.modelData) : false
                        text: delegateScope2.modelData
                        iconName: root.categoryIcon(delegateScope2.modelData)
                        flat: true
                        font.pixelSize: SentinelTheme.fontTiny
                        highlighted: !muted
                        Accessible.name: delegateScope2.modelData + " channel, " + (muted ? "muted" : "active")
                        onClicked: {
                            if (root.viewModel) {
                                root.viewModel.setChannelMuted(delegateScope2.modelData, !muted)
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: {
                        var summaries = root.viewModel ? root.viewModel.notificationLifecycleSummaries : []
                        return summaries.length > 0 ? summaries.join(" \u00B7 ") : ""
                    }
                    font.pixelSize: SentinelTheme.fontSmall - 1
                    color: SentinelTheme.textMuted
                    elide: Text.ElideRight
                    Layout.fillWidth: true

                    Accessible.role: Accessible.StaticText
                    Accessible.name: text
                }

                SentinelButton {
                    text: qsTr("Settings")
                    flat: true
                    font.pixelSize: SentinelTheme.fontSmall
                    Accessible.name: qsTr("Open notification settings")
                    onClicked: {
                        root.closeRequested()
                        if (root.viewModel) root.viewModel.currentPage = "Settings"
                    }
                }
            }
        }
    }

    // ── Delegate component ──────────────────────────────────────
    Component {
        id: notificationDelegate

        Loader {
            id: notificationLoader
            required property var model
            width: parent ? parent.width : 480
            sourceComponent: notificationLoader.model.type === "groupHeader" ? groupHeaderComponent : notificationItemComponent
            Binding {
                target: notificationLoader.item
                property: "row"
                value: notificationLoader.model
                when: notificationLoader.status === Loader.Ready
            }
        }
    }

    Component {
        id: groupHeaderComponent

        ShellPanel {
            id: groupHeader
            property var row: ({})
            activeFocusOnTab: true
            Keys.onReturnPressed: root.toggleGroup(groupHeader.row.groupName)
            Keys.onSpacePressed: root.toggleGroup(groupHeader.row.groupName)
            height: 36
            panelColor: "transparent"
            borderWidth: 0

            Accessible.role: Accessible.Button
            Accessible.name: groupHeader.row.groupName + " group, " + groupHeader.row.groupCount + " notifications"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 6

                TablerGlyph {
                    text: root.categoryIcon(groupHeader.row.groupName)
                    font.pixelSize: SentinelTheme.fontSmall
                    color: SentinelTheme.textPrimary
                }

                Text {
                    text: groupHeader.row.groupName
                    font.pixelSize: SentinelTheme.fontSmall
                    font.bold: true
                    color: SentinelTheme.textPrimary
                }

                Rectangle {
                    Layout.preferredWidth: 18
                    Layout.preferredHeight: 18
                    radius: 9
                    color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.15)
                    visible: groupHeader.row.groupCount > 0

                    layer.enabled: true
                    layer.effect: MultiEffect {
                        shadowEnabled: true
                        shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                        shadowVerticalOffset: 1
                        shadowBlur: 0.06
                        shadowOpacity: 1.0
                    }

                    Text {
                        anchors.centerIn: parent
                        text: groupHeader.row.groupCount
                        font.pixelSize: SentinelTheme.fontTiny
                        font.bold: true
                        color: SentinelTheme.accent
                    }
                }

                Item { Layout.fillWidth: true }

                TablerGlyph {
                    text: groupHeader.row.expanded ? "chevron-down" : "chevron-right"
                    font.pixelSize: 10
                    color: SentinelTheme.textMuted
                }

                TapHandler {
                    onTapped: root.toggleGroup(groupHeader.row.groupName)
                }
            }
        }
    }

    Component {
        id: notificationItemComponent

        NotificationItemDelegate {
            id: notificationRow
            property var row: ({})
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            viewModel: root.viewModel
            notifData: ({
                id: notificationRow.row.id,
                category: notificationRow.row.category,
                title: notificationRow.row.title,
                body: notificationRow.row.body,
                priority: notificationRow.row.priority,
                timestamp: notificationRow.row.timestamp,
                pinned: notificationRow.row.pinned,
                archived: notificationRow.row.archived,
                read: notificationRow.row.read,
                snoozed: notificationRow.row.snoozed,
                snoozeUntil: notificationRow.row.snoozeUntil
            })

            onRemove: function(id) {
                if (viewModel) viewModel.removeNotificationById(id)
            }
        }
    }
}
