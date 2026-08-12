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
    property bool compact: false
    property bool showDivider: false
    property int controlWidth: compact ? 160 : 220
    default property alias controlContent: controlContainer.children

    Layout.fillWidth: true
    implicitHeight: Math.max(50, contentLayout.implicitHeight + SentinelTheme.spaceSm * 2)
    radius: SentinelTheme.radiusMd
    color: cardArea.hovered
           ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.03)
           : "transparent"

    Behavior on color { ColorAnimation { duration: MotionTokens.fast } }

    HoverHandler {
        id: cardArea
    }

    RowLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.leftMargin: SentinelTheme.spaceMd
        anchors.rightMargin: SentinelTheme.spaceMd
        anchors.topMargin: SentinelTheme.spaceSm
        anchors.bottomMargin: SentinelTheme.spaceSm
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

        Item {
            id: controlContainer
            Layout.preferredWidth: root.controlWidth
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            implicitHeight: childrenRect.height
        }
    }

    Rectangle {
        visible: root.showDivider
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: SentinelTheme.spaceMd
        anchors.rightMargin: SentinelTheme.spaceMd
        height: 1
        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
    }
}
