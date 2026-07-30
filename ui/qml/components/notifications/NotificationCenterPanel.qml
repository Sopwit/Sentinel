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
            case "Tasks": return "\u26A1"
            case "Models": return "\uD83E\uDDE0"
            case "Updates": return "\uD83D\uDD04"
            case "Brain": return "\uD83D\uDCA1"
            case "Workspace": return "\uD83D\uDCC1"
            case "Security": return "\uD83D\uDEE1\uFE0F"
            default: return "\uD83D\uDD14"
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
                        var unreadCount = viewModel ? viewModel.unreadNotificationCount : 0
                        return unreadCount > 0 ? qsTr("(%1 unread)").arg(unreadCount) : ""
                    }
                    font.pixelSize: SentinelTheme.fontSmall
                    color: SentinelTheme.accent
                    visible: (viewModel ? viewModel.unreadNotificationCount : 0) > 0
                }

                Item { Layout.fillWidth: true }

                // ── DND toggle ───────────────────────────────────
                SentinelButton {
                    text: viewModel && viewModel.dndEnabled ? "\uD83D\uDD07" : "\uD83D\uDD0A"
                    implicitWidth: 32
                    implicitHeight: 32
                    flat: true
                    tooltipText: viewModel && viewModel.dndEnabled ? qsTr("Do Not Disturb is on") : qsTr("Do Not Disturb is off")
                    Accessible.name: viewModel && viewModel.dndEnabled ? qsTr("Disable do not disturb") : qsTr("Enable do not disturb")
                    highlighted: viewModel && viewModel.dndEnabled
                    onClicked: {
                        if (viewModel) viewModel.dndEnabled = !viewModel.dndEnabled
                    }
                }

                SentinelButton {
                    text: qsTr("Mark all read")
                    flat: true
                    font.pixelSize: SentinelTheme.fontSmall
                    Accessible.name: qsTr("Mark all notifications as read")
                    onClicked: {
                        if (viewModel) viewModel.markAllNotificationsRead()
                    }
                }

                SentinelButton {
                    text: qsTr("Clear archived")
                    flat: true
                    font.pixelSize: SentinelTheme.fontSmall
                    Accessible.name: qsTr("Clear all archived notifications")
                    onClicked: {
                        if (viewModel) viewModel.clearArchivedNotifications()
                    }
                }

                SentinelButton {
                    text: "\u00D7"
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
            height: 1
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
                placeholderText: "Search notifications..."
                Accessible.name: "Search notifications"
                onTextChanged: {
                    root.searchQuery = text
                    if (viewModel) viewModel.notificationSearchQuery = text
                }
            }

            Repeater {
                model: viewModel ? viewModel.notificationCategories : ["All"]

                delegate: SentinelButton {
                    text: modelData === "All" ? "All" : root.categoryIcon(modelData) + " " + modelData
                    flat: true
                    font.pixelSize: SentinelTheme.fontSmall
                    font.bold: root.activeFilter === modelData
                    highlighted: root.activeFilter === modelData
                    Accessible.name: "Filter by category: " + modelData
                    onClicked: {
                        root.activeFilter = modelData
                        if (viewModel) viewModel.notificationCategoryFilter = modelData
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
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

                function populate() {
                    notificationModel.clear()
                    var summaries = viewModel ? viewModel.notificationFilteredSummaries : []
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

                Component.onCompleted: populate()
            }

            Connections {
                target: viewModel
                function onNativeExperienceChanged() {
                    notificationList.populate()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
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
                    model: viewModel ? viewModel.notificationCategories : []

                    delegate: SentinelButton {
                        property bool muted: viewModel ? viewModel.isChannelMuted(modelData) : false
                        text: root.categoryIcon(modelData) + " " + modelData + (muted ? " \uD83D\uDD07" : "")
                        flat: true
                        font.pixelSize: SentinelTheme.fontTiny
                        highlighted: !muted
                        Accessible.name: modelData + " channel, " + (muted ? "muted" : "active")
                        onClicked: {
                            if (viewModel) {
                                viewModel.setChannelMuted(modelData, !muted)
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
                        var summaries = viewModel ? viewModel.notificationLifecycleSummaries : []
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
                        if (viewModel) viewModel.currentPage = "Settings"
                    }
                }
            }
        }
    }

    // ── Delegate component ──────────────────────────────────────
    Component {
        id: notificationDelegate

        Loader {
            width: parent ? parent.width : 480
            sourceComponent: model.type === "groupHeader" ? groupHeaderComponent : notificationItemComponent
        }
    }

    Component {
        id: groupHeaderComponent

        ShellPanel {
            height: 36
            panelColor: "transparent"
            borderWidth: 0

            Accessible.role: Accessible.Button
            Accessible.name: model.groupName + " group, " + model.groupCount + " notifications"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 6

                Text {
                    text: root.categoryIcon(model.groupName) + " " + model.groupName
                    font.pixelSize: SentinelTheme.fontSmall
                    font.bold: true
                    color: SentinelTheme.textPrimary
                }

                Rectangle {
                    width: 18
                    height: 18
                    radius: 9
                    color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.15)
                    visible: model.groupCount > 0

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
                        text: model.groupCount
                        font.pixelSize: SentinelTheme.fontTiny
                        font.bold: true
                        color: SentinelTheme.accent
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: model.expanded ? "\u25BC" : "\u25B6"
                    font.pixelSize: 10
                    color: SentinelTheme.textMuted
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.toggleGroup(model.groupName)
                }
            }
        }
    }

    Component {
        id: notificationItemComponent

        NotificationItemDelegate {
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            viewModel: root.viewModel
            notifData: ({
                id: model.id,
                category: model.category,
                title: model.title,
                body: model.body,
                priority: model.priority,
                timestamp: model.timestamp,
                pinned: model.pinned,
                archived: model.archived,
                read: model.read,
                snoozed: model.snoozed,
                snoozeUntil: model.snoozeUntil
            })

            onRemove: function(id) {
                if (viewModel) viewModel.removeNotificationById(id)
            }
        }
    }
}