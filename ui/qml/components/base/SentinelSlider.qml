// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import Sentinel.Desktop

Slider {
    id: control

    property color accent: SentinelTheme.calmAccent
    property string suffix: ""
    property int decimals: 2

    implicitWidth: 160
    implicitHeight: 36
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 140
        implicitHeight: 6
        width: control.availableWidth
        height: implicitHeight
        radius: 3
        color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.70)
        border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
        border.width: 1

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            color: control.accent
            radius: 3

            Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
        }
    }

    handle: Rectangle {
        id: handleRect
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 18
        implicitHeight: 18
        radius: 9
        color: control.pressed
               ? control.accent
               : (control.hovered ? SentinelTheme.withAlpha(control.accent, 0.25) : SentinelTheme.backgroundRaised)
        border.color: control.accent
        border.width: 2

        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
        Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }

        Rectangle {
            anchors.centerIn: parent
            width: 6
            height: 6
            radius: 3
            color: control.accent
            visible: !control.pressed
        }
    }
}
