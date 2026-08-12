// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

Rectangle {
    id: root

    property string title: ""
    property string subtitle: ""
    property bool checked: false
    property color accent: SentinelTheme.calmAccent
    property bool compact: false
    property bool interactive: true
    property bool showDivider: false

    signal toggled(bool checked)

    Layout.fillWidth: true
    implicitHeight: visible ? Math.max(50, contentLayout.implicitHeight + SentinelTheme.spaceSm * 2) : 0
    radius: SentinelTheme.radiusMd
    color: cardArea.containsMouse
           ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
           : "transparent"

    Behavior on color { ColorAnimation { duration: MotionTokens.fast } }

    MouseArea {
        id: cardArea
        anchors.fill: parent
        hoverEnabled: root.interactive
        enabled: root.interactive
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.checked = !root.checked
            root.toggled(root.checked)
        }
    }

    RowLayout {
        id: contentLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: SentinelTheme.spaceMd
        anchors.rightMargin: SentinelTheme.spaceMd
        anchors.topMargin: SentinelTheme.spaceSm
        spacing: SentinelTheme.spaceMd

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: root.title
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                font.weight: Font.Medium
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                visible: root.subtitle.length > 0
                text: root.subtitle
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                wrapMode: Text.WordWrap
            }
        }

        SentinelSwitch {
            id: switchControl
            Layout.alignment: Qt.AlignVCenter
            checked: root.checked
            accent: root.accent
            onToggled: {
                root.checked = checked
                root.toggled(checked)
            }
        }
    }

    Rectangle {
        visible: root.showDivider && root.visible
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: SentinelTheme.spaceMd
        anchors.rightMargin: SentinelTheme.spaceMd
        height: 1
        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
    }
}
