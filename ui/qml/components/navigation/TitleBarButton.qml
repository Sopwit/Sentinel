// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects

Rectangle {
    id: root

    property string iconName: ""
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

    Image {
        anchors.centerIn: parent
        width: 16
        height: 16
        source: "qrc:/icons/tabler/" + root.iconName + ".svg"
        sourceSize.width: 16
        sourceSize.height: 16
        layer.enabled: true
        layer.effect: MultiEffect {
            colorization: 1.0
            colorizationColor: root.textColor
        }
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
