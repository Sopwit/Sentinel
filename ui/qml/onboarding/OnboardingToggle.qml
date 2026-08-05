// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

RowLayout {
    id: root

    property string label: ""
    property string caption: ""
    property bool checked: false
    property color accent: SentinelTheme.calmAccent

    signal toggled(bool on)

    spacing: SentinelTheme.spaceMd

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        Label {
            Layout.fillWidth: true
            text: root.label
            color: SentinelTheme.textPrimary
            font.pixelSize: SentinelTheme.fontBody
            font.bold: true
        }

        Label {
            Layout.fillWidth: true
            visible: root.caption.length > 0
            text: root.caption
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }
    }

    Switch {
        id: switchControl
        checked: root.checked
        hoverEnabled: true
        onToggled: root.toggled(checked)

        indicator: Rectangle {
            implicitWidth: 46
            implicitHeight: 24
            x: switchControl.leftPadding
            y: switchControl.height / 2 - height / 2
            radius: height / 2
            color: switchControl.checked
                   ? SentinelTheme.withAlpha(root.accent, 0.18)
                   : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
            border.color: switchControl.activeFocus
                          ? SentinelTheme.withAlpha(root.accent, 0.46)
                          : switchControl.hovered
                            ? SentinelTheme.withAlpha(root.accent, 0.24)
                          : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)

            Rectangle {
                x: switchControl.checked ? parent.width - width - 3 : 3
                y: parent.height / 2 - height / 2
                width: 18
                height: 18
                radius: height / 2
                color: switchControl.checked
                       ? root.accent
                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.40)

                Behavior on x { NumberAnimation { duration: MotionTokens.fast } }
            }
        }

        background: Item {}
    }
}
