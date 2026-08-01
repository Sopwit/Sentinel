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
    implicitHeight: visible ? systemContent.implicitHeight + panelPadding * 2 : 0

    ColumnLayout {
        id: systemContent
        anchors.fill: parent
        anchors.margins: root.panelPadding
        spacing: SentinelTheme.spaceSm

        SectionTitle {
            title: qsTr("System & Diagnostic Tools")
            subtitle: qsTr("System integration, notifications, updates, and audio feedback.")
            Layout.fillWidth: true
        }

        InfoRow {
            compact: root.compact
            label: qsTr("Version")
            value: Qt.application.version || qsTr("1.0.0-rc.7")
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

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Enable Sound Effects")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.fillWidth: true
            }
            Switch {
                checked: root.soundManager ? root.soundManager.enabled : true
                onCheckedChanged: {
                    if (root.soundManager) {
                        root.soundManager.enabled = checked
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Enable Companion Service")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.fillWidth: true
            }
            Switch {
                checked: root.viewModel.companionEnabled
                onCheckedChanged: root.viewModel.companionEnabled = checked
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Developer Diagnostics Mode")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.fillWidth: true
            }
            Switch {
                checked: root.viewModel.developerModeEnabled
                onCheckedChanged: root.viewModel.developerModeEnabled = checked
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Update Policy")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 150
            }
            ComboBox {
                Layout.fillWidth: true
                model: [qsTr("Ask Before Checking"), qsTr("Automatic Background Checks"), qsTr("Disabled")]
                currentIndex: {
                    var pol = root.viewModel.updateCheckPolicy
                    if (pol === "Automatic") return 1
                    if (pol === "Disabled") return 2
                    return 0
                }
                onActivated: (index) => {
                    var policies = ["Ask Before Checking", "Automatic", "Disabled"]
                    root.viewModel.updateCheckPolicy = policies[index]
                }
            }
        }
    }
}
