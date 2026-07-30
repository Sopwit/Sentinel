// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Window
import Sentinel.Desktop

Item {
    id: root

    property string title: qsTr("Sentinel Desktop Alpha")
    property string modeName: ""
    property bool frameless: false
    property bool maximized: false
    property bool compact: false

    signal minimizeRequested()
    signal maximizeRequested()
    signal closeRequested()

    height: root.frameless ? (root.compact ? 40 : 48) : 0
    visible: root.frameless

    Rectangle {
        anchors.fill: parent
        color: SentinelTheme.backgroundBase
        opacity: 0.96

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: SentinelTheme.spaceMd
            anchors.rightMargin: SentinelTheme.spaceXs
            spacing: SentinelTheme.spaceSm

            Image {
                Layout.alignment: Qt.AlignVCenter
                source: "qrc:/icons/dev.sentinel.Sentinel.png"
                sourceSize.width: 20
                sourceSize.height: 20
                visible: status === Image.Ready
            }

            Label {
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true
                text: root.title
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontSmall
                font.weight: Font.Medium
                elide: Text.ElideRight
            }

            TitleBarButton {
                symbol: "\u{2014}"
                tooltip: qsTr("Minimize")
                onClicked: root.minimizeRequested()
            }

            TitleBarButton {
                symbol: root.maximized ? "\u{25A1}" : "\u{25A2}"
                tooltip: root.maximized ? qsTr("Restore") : qsTr("Maximize")
                onClicked: root.maximizeRequested()
            }

            TitleBarButton {
                symbol: "\u2715"
                tooltip: qsTr("Close")
                hoverColor: "#ef4444"
                hoverTextColor: "#ffffff"
                onClicked: root.closeRequested()
            }
        }
    }

    // Window drag region
    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.rightMargin: 100
        enabled: root.frameless
        cursorShape: Qt.ArrowCursor

        property real lastMouseX: 0
        property real lastMouseY: 0

        onPressed: function(event) {
            lastMouseX = event.screenX
            lastMouseY = event.screenY
        }

        onPositionChanged: function(event) {
            if (pressed) {
                var dx = event.screenX - lastMouseX
                var dy = event.screenY - lastMouseY
                var win = root.Window.window
                if (win) {
                    win.x += dx
                    win.y += dy
                }
                lastMouseX = event.screenX
                lastMouseY = event.screenY
            }
        }

        onDoubleClicked: {
            root.maximizeRequested()
        }
    }
}
