// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
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
    property color accent: SentinelTheme.calmAccent
    default property alias cardContent: cardColumn.children

    Layout.fillWidth: true
    implicitHeight: visible ? (mainLayout.implicitHeight + SentinelTheme.spaceXs * 2) : 0
    radius: SentinelTheme.radiusLg
    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.45)
    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
    border.width: 1

    ColumnLayout {
        id: mainLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: SentinelTheme.spaceXs
        spacing: 0

        ColumnLayout {
            visible: root.title.length > 0
            Layout.fillWidth: true
            Layout.leftMargin: SentinelTheme.spaceMd
            Layout.rightMargin: SentinelTheme.spaceMd
            Layout.topMargin: SentinelTheme.spaceSm
            Layout.bottomMargin: SentinelTheme.spaceSm
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: root.title
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                font.weight: Font.DemiBold
            }

            Label {
                Layout.fillWidth: true
                visible: root.subtitle.length > 0
                text: root.subtitle
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: SentinelTheme.spaceXs
                height: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
            }
        }

        ColumnLayout {
            id: cardColumn
            Layout.fillWidth: true
            spacing: 0
        }
    }
}
