// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: root

    property string symbol: ""
    property string tooltip: ""
    property color hoverColor: "transparent"
    property color hoverTextColor: SentinelTheme.textPrimary

    signal clicked()

    width: 36
    height: 30
    radius: 6
    color: mouse.containsMouse || mouse.pressed
           ? (root.hoverColor !== "transparent" ? root.hoverColor : Qt.rgba(root.textColor.r, root.textColor.g, root.textColor.b, 0.08))
           : "transparent"

    readonly property color textColor: mouse.containsMouse ? root.hoverTextColor : SentinelTheme.textMuted

    Behavior on color {
        ColorAnimation { duration: MotionTokens.fast; easing.type: MotionTokens.standard }
    }

    Text {
        anchors.centerIn: parent
        text: root.symbol
        color: root.textColor
        font.pixelSize: 14
        font.weight: Font.Normal
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.ArrowCursor
        onClicked: root.clicked()
    }

    ToolTip.visible: mouse.containsMouse && root.tooltip.length > 0
    ToolTip.text: root.tooltip
    ToolTip.delay: 600
}
