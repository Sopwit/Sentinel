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
            SentinelComboBox {
                accent: root.modeAccent
                Layout.fillWidth: true
                model: [qsTr("Ask Before Checking"), qsTr("Weekly"), qsTr("On Startup"), qsTr("Never")]
                currentIndex: {
                    var pol = root.viewModel.updateCheckPolicy
                    if (pol === "Weekly") return 1
                    if (pol === "On Startup") return 2
                    if (pol === "Never") return 3
                    return 0
                }
                onActivated: (index) => {
                    var policies = ["Ask Before Checking", "Weekly", "On Startup", "Never"]
                    root.viewModel.updateCheckPolicy = policies[index]
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Notification Policy")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 150
            }
            SentinelComboBox {
                accent: root.modeAccent
                Layout.fillWidth: true
                model: [qsTr("Important Only"), qsTr("All"), qsTr("Custom"), qsTr("Disabled")]
                currentIndex: {
                    var pol = root.viewModel.notificationPolicy
                    if (pol === "All") return 1
                    if (pol === "Custom") return 2
                    if (pol === "Disabled") return 3
                    return 0
                }
                onActivated: (index) => {
                    var policies = ["Important Only", "All", "Custom", "Disabled"]
                    root.viewModel.notificationPolicy = policies[index]
                }
            }
        }

        SectionTitle {
            title: qsTr("Notifications")
            subtitle: qsTr("Choose which events trigger in-app notifications.")
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Model Downloads")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.fillWidth: true
            }
            Switch {
                checked: root.viewModel.notifyModelDownloads
                onCheckedChanged: root.viewModel.notifyModelDownloads = checked
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Model Removals")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.fillWidth: true
            }
            Switch {
                checked: root.viewModel.notifyModelRemovals
                onCheckedChanged: root.viewModel.notifyModelRemovals = checked
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Agent Responses")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.fillWidth: true
            }
            Switch {
                checked: root.viewModel.notifyAgentResponses
                onCheckedChanged: root.viewModel.notifyAgentResponses = checked
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("System Updates")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.fillWidth: true
            }
            Switch {
                checked: root.viewModel.notifySystemUpdates
                onCheckedChanged: root.viewModel.notifySystemUpdates = checked
            }
        }
    }
}
