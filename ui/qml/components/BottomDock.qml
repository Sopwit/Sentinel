import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: dock
    required property string currentPage
    signal pageRequested(string pageName)

    implicitHeight: 72
    implicitWidth: 260

    readonly property var dockItems: [
        { id: "Dashboard", label: qsTr("Home"),    icon: "⌂" },
        { id: "Models",    label: qsTr("Models"),  icon: "◈" }
    ]

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

    // ── Pill container (Liquid Glass surface) ────────────────────────────────
    Rectangle {
        id: dockPill
        anchors.centerIn: parent
        width: dockRow.implicitWidth + 24
        height: 60
        radius: height / 2

        // Layered glass effect
        color: SentinelTheme.lightTheme
             ? SentinelTheme.withAlpha("#ffffff", 0.70)
             : SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.72)

        border.color: SentinelTheme.lightTheme
                    ? SentinelTheme.withAlpha("#ffffff", 0.88)
                    : SentinelTheme.withAlpha(SentinelTheme.accent, 0.18)
        border.width: 1

        // Inner top sheen — the key Liquid Glass highlight
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 1
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            height: 1
            color: SentinelTheme.lightTheme
                 ? SentinelTheme.withAlpha("#ffffff", 0.95)
                 : SentinelTheme.withAlpha("#ffffff", 0.28)
            radius: 1
        }

        // Frosted inner fill
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: parent.radius - 1
            color: SentinelTheme.lightTheme
                 ? SentinelTheme.withAlpha("#f8faff", 0.52)
                 : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.22)
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

            // Sheen on active indicator
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 1
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                height: 1
                color: SentinelTheme.withAlpha("#ffffff", 0.60)
                radius: 1
            }

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

                        // Icon
                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: tabBtn.modelData.icon
                            font.pixelSize: tabBtn.active ? 18 : 16
                            color: tabBtn.active
                                 ? SentinelTheme.accent
                                 : (tabBtn.hovered
                                    ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.80)
                                    : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.45))

                            Behavior on color {
                                ColorAnimation { duration: MotionTokens.fast; easing.type: MotionTokens.standard }
                            }
                            Behavior on font.pixelSize {
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
