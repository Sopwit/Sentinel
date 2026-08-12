// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import Sentinel.Desktop

Switch {
    id: control

    property color accent: SentinelTheme.calmAccent

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    implicitWidth: 42
    implicitHeight: 22

    indicator: Rectangle {
        implicitWidth: 42
        implicitHeight: 22
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: height / 2
        color: control.checked
               ? control.accent
               : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.12)
        border.color: control.activeFocus
                      ? SentinelTheme.withAlpha(control.accent, 0.60)
                      : control.hovered
                        ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.22)
                        : "transparent"
        border.width: 1

        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
        Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }

        Rectangle {
            id: handle
            x: control.checked ? parent.width - width - 3 : 3
            y: parent.height / 2 - height / 2
            width: 16
            height: 16
            radius: 8
            color: "#ffffff"

            Behavior on x { NumberAnimation { duration: MotionTokens.fast; easing.type: MotionTokens.standard } }
        }
    }

    background: Item {}
}
