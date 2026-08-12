// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: dock
    required property string currentPage
    signal pageRequested(string pageName)

    implicitHeight: 72
    implicitWidth: 260

    property var dockItems: buildDockItems()

    function buildDockItems() {
        return [
            { id: "Dashboard", label: qsTr("Home"),   icon: "⌂" },
            { id: "Models",    label: qsTr("Models"), icon: "◈" }
        ]
    }

    Connections {
        target: shellViewModel
        function onAppLanguageChanged() { dock.dockItems = dock.buildDockItems() }
    }

    // ── Outer glow halo ──────────────────────────────────────────────────────
    Rectangle {
        id: dockHalo
        anchors.centerIn: dockPill
        width: dockPill.width + 28
        height: dockPill.height + 28
        radius: height / 2
        color: "transparent"
        border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.14)
        border.width: 1
        opacity: 0.0

        SequentialAnimation on opacity {
            loops: Animation.Infinite
            running: true
            NumberAnimation { to: 1.0; duration: 2400; easing.type: Easing.InOutSine }
            NumberAnimation { to: 0.0; duration: 2400; easing.type: Easing.InOutSine }
        }
    }

    // ── Drop shadow for the dock pill ─────────────────────────────────────────
    // Pill container (Liquid Glass surface)
    Rectangle {
        id: dockPill
        anchors.centerIn: parent
        width: dockRow.implicitWidth + 24
        height: 60
        radius: height / 2

        // Drop shadow (MultiEffect shadow, available since Qt 6.5)
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: SentinelTheme.lightTheme
                         ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.20)
                         : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.55)
            shadowBlur: 0.55
            shadowHorizontalOffset: 0
            shadowVerticalOffset: SentinelTheme.lightTheme ? 3 : 6
            shadowOpacity: 1.0
        }

        // Layered glass effect
        color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.82)

        border.color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.92)
        border.width: 1

        // Frosted inner fill
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: parent.radius - 1
            color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.60)
        }

        Behavior on color {
            ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard }
        }
        Behavior on border.color {
            ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard }
        }

        // ── Active indicator pill (slides between tabs) ──────────────────────
        Rectangle {
            id: activeIndicator
            height: 40
            width: activeIndicatorWidth()
            radius: height / 2
            y: (parent.height - height) / 2

            function activeIndicatorWidth() {
                if (tabRepeater.count === 0) return 90
                // find the active tab button width
                for (let i = 0; i < tabRepeater.count; i++) {
                    if (tabRepeater.itemAt(i) && dock.dockItems[i].id === dock.currentPage) {
                        return tabRepeater.itemAt(i).width - 4
                    }
                }
                return 90
            }

            function activeIndicatorX() {
                if (tabRepeater.count === 0) return 12
                var cumX = 12
                for (let i = 0; i < tabRepeater.count; i++) {
                    if (dock.dockItems[i].id === dock.currentPage) {
                        return cumX + 2
                    }
                    if (tabRepeater.itemAt(i)) {
                        cumX += tabRepeater.itemAt(i).width
                    }
                }
                return 12
            }

            x: activeIndicatorX()
            color: SentinelTheme.lightTheme
                 ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.14)
                 : SentinelTheme.withAlpha(SentinelTheme.accent, 0.20)
            border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.30)
            border.width: 1

            Behavior on x {
                NumberAnimation {
                    duration: MotionTokens.duration(MotionTokens.normal, "")
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on width {
                NumberAnimation {
                    duration: MotionTokens.duration(MotionTokens.normal, "")
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on color {
                ColorAnimation { duration: MotionTokens.fast; easing.type: MotionTokens.standard }
            }
        }

        // ── Tab row ──────────────────────────────────────────────────────────
        RowLayout {
            id: dockRow
            anchors.centerIn: parent
            spacing: 0

            Repeater {
                id: tabRepeater
                model: dock.dockItems

                Button {
                    id: tabBtn
                    required property var modelData
                    required property int index

                    readonly property bool active: dock.currentPage === tabBtn.modelData.id
                    readonly property color tabAccent: SentinelTheme.accent

                    Layout.preferredHeight: 52
                    implicitWidth: tabContent.implicitWidth + 28
                    flat: true
                    hoverEnabled: true
                    focusPolicy: Qt.StrongFocus

                    onClicked: dock.pageRequested(tabBtn.modelData.id)

                    scale: tabBtn.down ? InteractionTokens.pressScale : 1.0
                    Behavior on scale {
                        NumberAnimation {
                            duration: MotionTokens.duration(MotionTokens.fast, "")
                            easing.type: MotionTokens.press
                        }
                    }

                    background: Item {}

                    contentItem: ColumnLayout {
                        id: tabContent
                        spacing: 3
                        anchors.centerIn: parent

                        // Premium vector icon (Canvas-drawn)
                        Item {
                            Layout.alignment: Qt.AlignHCenter
                            width: 22
                            height: 22

                            readonly property color iconColor: tabBtn.active
                                ? SentinelTheme.accent
                                : (tabBtn.hovered
                                    ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.80)
                                    : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.45))

                            Canvas {
                                id: iconCanvas
                                anchors.fill: parent
                                antialiasing: true
                                property string iconId: tabBtn.modelData.id
                                property var c: parent.iconColor

                                onPaint: {
                                    var ctx = getContext("2d")
                                    ctx.reset()
                                    ctx.fillStyle = Qt.rgba(c.r, c.g, c.b, c.a)
                                    ctx.strokeStyle = Qt.rgba(c.r, c.g, c.b, c.a)
                                    ctx.lineWidth = 1.8
                                    ctx.lineJoin = "round"
                                    ctx.lineCap = "round"

                                    var cx = width / 2
                                    var cy = height / 2

                                    if (iconId === "Dashboard") {
                                        ctx.beginPath()
                                        ctx.moveTo(cx, 3)
                                        ctx.lineTo(3, cy + 2)
                                        ctx.stroke()
                                        ctx.beginPath()
                                        ctx.moveTo(cx, 3)
                                        ctx.lineTo(width - 3, cy + 2)
                                        ctx.stroke()
                                        ctx.beginPath()
                                        ctx.rect(cx - 6, cy + 2, 12, 10)
                                        ctx.stroke()
                                        ctx.beginPath()
                                        ctx.rect(cx - 3, cy + 5, 6, 7)
                                        ctx.fill()
                                    } else if (iconId === "Models") {
                                        var r = 2.5
                                        ctx.beginPath()
                                        ctx.arc(cx - 6, cy - 5, r, 0, Math.PI * 2)
                                        ctx.fill()
                                        ctx.beginPath()
                                        ctx.arc(cx + 6, cy - 5, r, 0, Math.PI * 2)
                                        ctx.fill()
                                        ctx.beginPath()
                                        ctx.arc(cx, cy, r, 0, Math.PI * 2)
                                        ctx.fill()
                                        ctx.beginPath()
                                        ctx.arc(cx - 6, cy + 5, r, 0, Math.PI * 2)
                                        ctx.fill()
                                        ctx.beginPath()
                                        ctx.arc(cx + 6, cy + 5, r, 0, Math.PI * 2)
                                        ctx.fill()
                                        ctx.beginPath()
                                        ctx.moveTo(cx - 6, cy - 5)
                                        ctx.lineTo(cx, cy)
                                        ctx.stroke()
                                        ctx.beginPath()
                                        ctx.moveTo(cx + 6, cy - 5)
                                        ctx.lineTo(cx, cy)
                                        ctx.stroke()
                                        ctx.beginPath()
                                        ctx.moveTo(cx - 6, cy + 5)
                                        ctx.lineTo(cx, cy)
                                        ctx.stroke()
                                        ctx.beginPath()
                                        ctx.moveTo(cx + 6, cy + 5)
                                        ctx.lineTo(cx, cy)
                                        ctx.stroke()
                                    }
                                }

                                Connections {
                                    target: tabBtn
                                    function onActiveChanged() { iconCanvas.requestPaint() }
                                    function onHoveredChanged() { iconCanvas.requestPaint() }
                                }
                            }

                            Behavior on scale {
                                NumberAnimation { duration: MotionTokens.fast }
                            }
                        }

                        // Label
                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: tabBtn.modelData.label
                            font.pixelSize: SentinelTheme.fontTiny
                            font.weight: tabBtn.active ? Font.Medium : Font.Normal
                            color: tabBtn.active
                                 ? SentinelTheme.accent
                                 : (tabBtn.hovered
                                    ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.72)
                                    : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.38))

                            Behavior on color {
                                ColorAnimation { duration: MotionTokens.fast; easing.type: MotionTokens.standard }
                            }
                        }
                    }

                    // Hover glow dot
                    Rectangle {
                        id: hoverDot
                        width: 3
                        height: 3
                        radius: 1.5
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 6
                        color: SentinelTheme.accent
                        opacity: tabBtn.active ? 0.85 : (tabBtn.hovered ? 0.40 : 0.0)
                        Behavior on opacity {
                            NumberAnimation { duration: MotionTokens.fast }
                        }
                    }
                }
            }
        }
    }
}
