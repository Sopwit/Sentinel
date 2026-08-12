// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

SpinBox {
    id: control

    property color accent: SentinelTheme.calmAccent

    implicitWidth: 160
    implicitHeight: 36
    hoverEnabled: true
    editable: true
    focusPolicy: Qt.StrongFocus

    contentItem: TextInput {
        z: 2
        text: control.displayText
        font.pixelSize: SentinelTheme.fontBody
        font.weight: Font.Medium
        color: SentinelTheme.textPrimary
        selectionColor: control.accent
        selectedTextColor: "#ffffff"
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhDigitsOnly
    }

    up.indicator: Rectangle {
        x: control.mirrored ? 0 : parent.width - width
        height: parent.height
        implicitWidth: 32
        implicitHeight: 36
        radius: SentinelTheme.radiusMd
        color: control.up.pressed
               ? SentinelTheme.withAlpha(control.accent, 0.25)
               : control.up.hovered
                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                 : "transparent"

        Text {
            text: "+"
            font.pixelSize: SentinelTheme.fontBody
            font.bold: true
            color: control.up.hovered ? control.accent : SentinelTheme.textMuted
            anchors.centerIn: parent
        }
    }

    down.indicator: Rectangle {
        x: control.mirrored ? parent.width - width : 0
        height: parent.height
        implicitWidth: 32
        implicitHeight: 36
        radius: SentinelTheme.radiusMd
        color: control.down.pressed
               ? SentinelTheme.withAlpha(control.accent, 0.25)
               : control.down.hovered
                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                 : "transparent"

        Text {
            text: "-"
            font.pixelSize: SentinelTheme.fontBody
            font.bold: true
            color: control.down.hovered ? control.accent : SentinelTheme.textMuted
            anchors.centerIn: parent
        }
    }

    background: Rectangle {
        radius: SentinelTheme.radiusMd
        color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.60)
        border.color: control.activeFocus
                      ? control.accent
                      : control.hovered
                        ? SentinelTheme.withAlpha(control.accent, 0.40)
                        : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
        border.width: control.activeFocus ? 2 : 1

        Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }
    }
}
