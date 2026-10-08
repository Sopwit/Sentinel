// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: root

    property var viewModel: null
    property int maxVisible: 3
    property real toastWidth: 380
    property real toastSpacing: 8
    property real displayDuration: 4000

    z: 9999

    Accessible.role: Accessible.Grouping
    Accessible.name: "Notification toasts"

    function parseNotification(jsonStr) {
        try { return JSON.parse(jsonStr) }
        catch(e) { return null }
    }

    function priorityColor(priority) {
        switch (priority) {
            case "Critical": return SentinelTheme.liquidGlassLightTheme ? "#ef4444" : "#d66b6b"
            case "High": return SentinelTheme.warning
            case "Low": return SentinelTheme.textMuted
            default: return SentinelTheme.accent
        }
    }

    function priorityIcon(priority) {
        switch (priority) {
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

    ListModel { id: toastQueue }

    // Track notification IDs that have already been shown as toasts
    // to prevent re-showing the same notification on every keystroke.
    property var shownIds: ({})
    property int shownIdLimit: 200

    function trimShownIds() {
        var keys = Object.keys(shownIds)
        if (keys.length > shownIdLimit) {
            shownIds = ({})
        }
    }

    function enqueueToast(notification) {
        if (!notification || !notification.id) return
        if (shownIds[notification.id]) return
        if (viewModel && viewModel.dndEnabled) return
        if (viewModel && viewModel.isChannelMuted && viewModel.isChannelMuted(notification.category)) return

        trimShownIds()
        shownIds[notification.id] = true

        while (toastQueue.count >= root.maxVisible) {
            toastQueue.remove(toastQueue.count - 1, 1)
        }
        toastQueue.insert(0, {
            id: notification.id,
            category: notification.category || "General",
            title: notification.title || "",
            body: notification.body || "",
            priority: notification.priority || "Normal",
            timestamp: notification.timestamp || new Date().toISOString()
        })
        dismissTimer.restart()
    }

    Connections {
        target: root.viewModel
        function onNativeExperienceChanged() {
            const summaries = root.viewModel.notificationFilteredSummaries
            if (summaries.length === 0) return

            for (let i = 0; i < summaries.length; ++i) {
                const latest = root.parseNotification(summaries[i])
                if (!latest || latest.archived || latest.read || latest.snoozed) continue
                root.enqueueToast(latest)
            }
        }
    }

    Timer {
        id: dismissTimer
        interval: 300
        repeat: false
        onTriggered: {
            if (toastQueue.count > 0) {
                root.dismiss(toastQueue.get(toastQueue.count - 1).id)
            }
        }
    }

    function dismiss(id) {
        for (let i = 0; i < toastQueue.count; ++i) {
            if (toastQueue.get(i).id === id) {
                toastQueue.remove(i, 1)
                return
            }
        }
    }

    ColumnLayout {
        id: toastContainer
        anchors.top: parent.top
        anchors.topMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: 16
        spacing: root.toastSpacing
        z: 9999

        Repeater {
            model: toastQueue

            delegate: ShellPanel {
        required property var model
                id: toastItem
                width: root.toastWidth
                implicitHeight: bodyLabel.implicitHeight + 48
                panelColor: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.95)
                borderWidth: 1
                borderColor: SentinelTheme.withAlpha(root.priorityColor(toastItem.model.priority), 0.3)

                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                    shadowBlur: 0.10
                    shadowHorizontalOffset: 2
                    shadowVerticalOffset: 2
                }

                Accessible.role: Accessible.Button
                Accessible.name: toastItem.model.category + ": " + toastItem.model.title + ". " + toastItem.model.body
                Accessible.onPressAction: {
                    fadeOutAnim.stop()
                    root.dismiss(toastItem.model.id)
                    if (root.viewModel) root.viewModel.markNotificationRead(toastItem.model.id)
                }

                property real startX: root.toastWidth + 20
                property real endX: 0
                property real dragX: 0

                transform: Translate {
                    id: slideTransform
                    x: toastItem.startX + toastItem.dragX
                }

                NumberAnimation {
                    target: slideTransform
                    property: "x"
                    from: toastItem.startX
                    to: toastItem.endX
                    duration: MotionTokens.slow
                    easing.type: Easing.OutCubic
                    running: true
                }

                SequentialAnimation {
                    id: fadeOutAnim
                    PauseAnimation { duration: root.displayDuration }
                    NumberAnimation {
                        target: toastItem
                        property: "opacity"
                        from: 1.0
                        to: 0.0
                        duration: MotionTokens.normal
                        easing.type: Easing.InCubic
                    }
                    onFinished: root.dismiss(toastItem.model.id)
                    running: true
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    drag.target: toastItem
                    drag.axis: Drag.XAxis
                    drag.minimumX: -root.toastWidth
                    drag.maximumX: 0
                    onClicked: {
                        fadeOutAnim.stop()
                        root.dismiss(toastItem.model.id)
                        if (root.viewModel) root.viewModel.markNotificationRead(toastItem.model.id)
                    }
                    onReleased: {
                        if (toastItem.dragX < -80) {
                            root.dismiss(toastItem.model.id)
                        } else {
                            toastItem.dragX = 0
                        }
                    }
                }

                onDragXChanged: {
                    if (dragX < -80) {
                        opacity = Math.max(0.3, 1 + dragX / root.toastWidth)
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Rectangle {
                        Layout.preferredWidth: 4
                        Layout.preferredHeight: parent.height
                        radius: 2
                        color: root.priorityColor(toastItem.model.priority)
                        Layout.fillHeight: true

                        Accessible.role: Accessible.Graphic
                        Accessible.name: "Priority: " + (toastItem.model.priority || "Normal")
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        RowLayout {
                            spacing: 6
                            Layout.fillWidth: true

                            TablerGlyph {
                                text: root.categoryIcon(toastItem.model.category)
                                font.pixelSize: SentinelTheme.fontSmall
                                color: root.priorityColor(toastItem.model.priority)
                            }

                            Text {
                                text: toastItem.model.category
                                font.pixelSize: SentinelTheme.fontSmall
                                font.bold: true
                                color: root.priorityColor(toastItem.model.priority)

                                Accessible.role: Accessible.StaticText
                                Accessible.name: "Category: " + toastItem.model.category
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: {
                                    var d = new Date(toastItem.model.timestamp)
                                    return d.toLocaleTimeString(Qt.locale(), Locale.ShortFormat)
                                }
                                font.pixelSize: SentinelTheme.fontSmall - 2
                                color: SentinelTheme.textMuted

                                Accessible.role: Accessible.StaticText
                                Accessible.name: "Time: " + text
                            }
                        }

                        Text {
                            id: titleLabel
                            text: toastItem.model.title
                            font.pixelSize: SentinelTheme.fontBody
                            font.bold: true
                            color: SentinelTheme.textPrimary
                            elide: Text.ElideRight
                            maximumLineCount: 1
                            Layout.fillWidth: true

                            Accessible.role: Accessible.StaticText
                            Accessible.name: toastItem.model.title
                        }

                        Text {
                            id: bodyLabel
                            text: toastItem.model.body
                            font.pixelSize: SentinelTheme.fontSmall
                            color: SentinelTheme.textMuted
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true

                            Accessible.role: Accessible.StaticText
                            Accessible.name: toastItem.model.body
                        }
                    }

                    SentinelButton {
                        iconName: "x"
                        implicitWidth: 24
                        implicitHeight: 24
                        flat: true

                        Accessible.name: "Dismiss notification"

                        onClicked: {
                            fadeOutAnim.stop()
                            root.dismiss(toastItem.model.id)
                        }
                    }
                }
            }
        }
    }
}
