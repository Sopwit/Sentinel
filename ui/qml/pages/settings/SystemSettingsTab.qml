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
        spacing: SentinelTheme.spaceMd

        SectionTitle {
            title: qsTr("System & Diagnostic Tools")
            subtitle: qsTr("System integration, notifications, updates, and audio feedback.")
            Layout.fillWidth: true
        }

        SettingCard {
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceSm

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
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
            }

            SettingToggleRow {
                title: qsTr("Enable Sound Effects")
                subtitle: qsTr("Play subtle audio cues for completions, notifications, and key interactions.")
                checked: root.soundManager ? root.soundManager.enabled : true
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => {
                    if (root.soundManager) {
                        root.soundManager.enabled = checked
                    }
                }
            }

            SettingToggleRow {
                title: qsTr("Enable Companion Service")
                subtitle: qsTr("Run background tray companion for system-wide shortcuts and quick assistant access.")
                checked: root.viewModel.companionEnabled
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.companionEnabled = checked
            }

            SettingToggleRow {
                title: qsTr("Developer Diagnostics Mode")
                subtitle: qsTr("Display telemetry metrics, debug execution logs, and detailed diagnostic counters.")
                checked: root.viewModel.developerModeEnabled
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.developerModeEnabled = checked
            }

            SettingControlRow {
                title: qsTr("Update Policy")
                subtitle: qsTr("Frequency for checking software updates and security releases.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    accent: root.modeAccent
                    anchors.fill: parent
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

            SettingControlRow {
                title: qsTr("Notification Policy")
                subtitle: qsTr("Filter level for system popups and banner alerts.")
                accent: root.modeAccent
                compact: root.compact

                SentinelComboBox {
                    accent: root.modeAccent
                    anchors.fill: parent
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
        }

        SectionTitle {
            title: qsTr("Notifications")
            subtitle: qsTr("Choose which background events trigger desktop and toast notifications.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            SettingToggleRow {
                title: qsTr("Model Downloads")
                subtitle: qsTr("Notify when local LLM downloads or weight verifications complete.")
                checked: root.viewModel.notifyModelDownloads
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.notifyModelDownloads = checked
            }

            SettingToggleRow {
                title: qsTr("Model Removals")
                subtitle: qsTr("Alert when model files or cached weights are purged.")
                checked: root.viewModel.notifyModelRemovals
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.notifyModelRemovals = checked
            }

            SettingToggleRow {
                title: qsTr("Agent Responses")
                subtitle: qsTr("Notify when autonomous agent tasks finish background execution.")
                checked: root.viewModel.notifyAgentResponses
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.notifyAgentResponses = checked
            }

            SettingToggleRow {
                title: qsTr("System Updates")
                subtitle: qsTr("Alert when new Sentinel system versions or security updates are ready.")
                checked: root.viewModel.notifySystemUpdates
                accent: root.modeAccent
                compact: root.compact
                onToggled: (checked) => root.viewModel.notifySystemUpdates = checked
            }
        }
    }
}
