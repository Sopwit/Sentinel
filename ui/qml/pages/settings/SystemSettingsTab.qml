// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: root
    required property var viewModel
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    property var soundManager: null
    readonly property int panelPadding: SentinelTheme.spaceLg

    height: implicitHeight
    implicitHeight: visible ? systemSection.implicitHeight : 0

    function sectionHeight(content) {
        return content.implicitHeight + panelPadding * 2
    }

    Column {
        width: parent.width
        spacing: 0

        Item {
            id: systemSection
            width: parent.width
            height: implicitHeight
            implicitHeight: root.sectionHeight(systemContent)

            ColumnLayout {
                id: systemContent
                x: root.panelPadding
                y: root.panelPadding
                width: parent.width - root.panelPadding * 2
                spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: qsTr("System & Diagnostic Tools")
                    subtitle: qsTr("System integration, notifications, updates, and audio feedback.")
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Version")
                    value: Qt.application.version || qsTr("1.0.0-rc.1")
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Platform")
                    value: Qt.platform.os
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Audio System")
                    value: root.soundManager && root.soundManager.soundEffectsAvailable ? qsTr("Sound effects active") : qsTr("Muted or system audio disabled")
                    Layout.fillWidth: true
                }
            }
        }
    }
}
