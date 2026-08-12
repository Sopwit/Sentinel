// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: root

    property string message: ""
    property string severity: "Error"
    property bool show: false
    property string actionLabel: ""
    property bool actionEnabled: true
    property string modeName: ""

    signal actionTriggered()
    signal dismissed()

    implicitHeight: show ? 48 : 0
    clip: true

    readonly property string bannerHex: {
        switch (root.severity) {
        case "Error":   return "#ef4444"
        case "Warning": return "#e7b76a"
        case "Info":    return "#38bdf8"
        default:        return "#ef4444"
        }
    }

    readonly property color bannerColor: root.bannerHex
    readonly property color bgColor: Qt.rgba(root.bannerColor.r, root.bannerColor.g, root.bannerColor.b, 0.12)
    readonly property color borderColor: Qt.rgba(root.bannerColor.r, root.bannerColor.g, root.bannerColor.b, 0.35)
    readonly property color textColor: Qt.rgba(root.bannerColor.r, root.bannerColor.g, root.bannerColor.b, 0.9)

    Behavior on implicitHeight {
        NumberAnimation {
            duration: MotionTokens.duration(MotionTokens.normal, root.modeName)
            easing.type: MotionTokens.enter
        }
    }

    Rectangle {
        id: bannerBg
        anchors.fill: parent
        radius: SentinelTheme.radiusMd
        color: root.bgColor
        border.color: root.borderColor
        border.width: 1
        visible: root.show

        layer.enabled: true
        layer.smooth: false
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.12)
            shadowVerticalOffset: 2
            shadowBlur: 0.2
            shadowOpacity: 1.0
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: SentinelTheme.spaceLg
            anchors.rightMargin: SentinelTheme.spaceSm
            anchors.topMargin: SentinelTheme.spaceSm
            anchors.bottomMargin: SentinelTheme.spaceSm
            spacing: SentinelTheme.spaceMd

            Rectangle {
                width: 4
                Layout.fillHeight: true
                radius: 2
                color: root.bannerColor
            }

            Label {
                id: msgLabel
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: root.message
                color: root.textColor
                font.pixelSize: SentinelTheme.fontSmall
                font.weight: Font.Medium
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }

            SentinelButton {
                id: actionBtn
                Layout.alignment: Qt.AlignVCenter
                visible: root.actionLabel.length > 0
                text: root.actionLabel
                enabled: root.actionEnabled
                implicitHeight: 30
                onClicked: root.actionTriggered()
            }

            Item {
                width: 4
                height: 1
                visible: actionBtn.visible
            }

            Item {
                Layout.alignment: Qt.AlignVCenter
                width: 28
                height: 28

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: closeMouse.containsMouse
                           ? Qt.rgba(root.bannerColor.r, root.bannerColor.g, root.bannerColor.b, 0.15)
                           : "transparent"

                    Behavior on color {
                        ColorAnimation {
                            duration: MotionTokens.fast
                            easing.type: MotionTokens.standard
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "\u2715"
                        color: root.textColor
                        font.pixelSize: 12
                        opacity: closeMouse.containsMouse ? 0.9 : 0.6
                    }
                }

                MouseArea {
                    id: closeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.dismissed()
                }
            }
        }
    }
}
