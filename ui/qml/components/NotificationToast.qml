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
            default: return viewModel ? viewModel.currentModeAccent : SentinelTheme.accent
        }
    }

    function priorityIcon(priority) {
        switch (priority) {
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

    ListModel { id: toastQueue }

    Connections {
        target: viewModel
        function onNativeExperienceChanged() {
            const summaries = viewModel.notificationFilteredSummaries
            if (summaries.length === 0) return

            const latest = parseNotification(summaries[0])
            if (!latest || latest.archived || latest.read || latest.snoozed) return

            if (viewModel && viewModel.dndEnabled) return
            if (viewModel && viewModel.isChannelMuted && viewModel.isChannelMuted(latest.category)) return

            for (let i = 0; i < toastQueue.count; ++i) {
                if (toastQueue.get(i).id === latest.id) return
            }

            if (toastQueue.count >= maxVisible) {
                toastQueue.remove(toastQueue.count - 1, 1)
            }

            toastQueue.prepend({
                id: latest.id,
                category: latest.category,
                title: latest.title,
                body: latest.body,
                priority: latest.priority || "Normal",
                timestamp: latest.timestamp || 0
            })

            if (viewModel && viewModel.playNotificationSound) {
                viewModel.playNotificationSound()
            }
            dismissTimer.start()
        }
    }

    Timer {
        id: dismissTimer
        interval: 300
        repeat: false
        onTriggered: {
            if (toastQueue.count > 0) {
                dismiss(toastQueue.get(toastQueue.count - 1).id)
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
                id: toastItem
                width: root.toastWidth
                implicitHeight: bodyLabel.implicitHeight + 48
                panelColor: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.95)
                borderWidth: 1
                borderColor: Qt.alpha(priorityColor(model.priority), 0.3)

                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                    shadowBlur: 0.10
                    shadowHorizontalOffset: 2
                    shadowVerticalOffset: 2
                }

                Accessible.role: Accessible.Button
                Accessible.name: model.category + ": " + model.title + ". " + model.body
                Accessible.onPressAction: {
                    fadeOutAnim.stop()
                    root.dismiss(model.id)
                    if (viewModel) viewModel.markNotificationRead(model.id)
                }

                property real startX: root.toastWidth + 20
                property real endX: 0
                property real dragX: 0

                transform: Translate {
                    id: slideTransform
                    x: startX + dragX
                }

                NumberAnimation {
                    target: slideTransform
                    property: "x"
                    from: startX
                    to: endX
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
                        duration: MotionTokens.medium
                        easing.type: Easing.InCubic
                    }
                    onFinished: root.dismiss(model.id)
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
                        root.dismiss(model.id)
                        if (viewModel) viewModel.markNotificationRead(model.id)
                    }
                    onReleased: {
                        if (toastItem.dragX < -80) {
                            root.dismiss(model.id)
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
                        width: 4
                        height: parent.height
                        radius: 2
                        color: priorityColor(model.priority)
                        Layout.fillHeight: true

                        Accessible.role: Accessible.Graphic
                        Accessible.name: "Priority: " + (model.priority || "Normal")
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        RowLayout {
                            spacing: 6
                            Layout.fillWidth: true

                            Text {
                                text: categoryIcon(model.category) + " " + model.category
                                font.pixelSize: SentinelTheme.fontSmall
                                font.bold: true
                                color: priorityColor(model.priority)

                                Accessible.role: Accessible.StaticText
                                Accessible.name: "Category: " + model.category
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: {
                                    var d = new Date(model.timestamp)
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
                            text: model.title
                            font.pixelSize: SentinelTheme.fontBody
                            font.bold: true
                            color: SentinelTheme.textPrimary
                            elide: Text.ElideRight
                            maximumLineCount: 1
                            Layout.fillWidth: true

                            Accessible.role: Accessible.StaticText
                            Accessible.name: model.title
                        }

                        Text {
                            id: bodyLabel
                            text: model.body
                            font.pixelSize: SentinelTheme.fontSmall
                            color: SentinelTheme.textMuted
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true

                            Accessible.role: Accessible.StaticText
                            Accessible.name: model.body
                        }
                    }

                    SentinelButton {
                        text: "\u00D7"
                        implicitWidth: 24
                        implicitHeight: 24
                        flat: true

                        Accessible.name: "Dismiss notification"

                        onClicked: {
                            fadeOutAnim.stop()
                            root.dismiss(model.id)
                        }
                    }
                }
            }
        }
    }
}
