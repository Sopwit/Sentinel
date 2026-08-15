// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
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

    signal openUpdateRequested()

    height: implicitHeight
    implicitHeight: visible ? systemContent.implicitHeight + panelPadding * 2 : 0

    ColumnLayout {
        id: systemContent
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: root.panelPadding
        spacing: SentinelTheme.spaceMd

        SectionTitle {
            title: qsTr("System & Diagnostic Tools")
            subtitle: qsTr("System integration, notifications, updates, and audio feedback.")
            Layout.fillWidth: true
        }

        SettingCard {
            title: qsTr("Software Updates & System Info")
            subtitle: qsTr("Check for application updates, release notes, and system runtime status.")

            SettingControlRow {
                title: qsTr("Check for Updates")
                subtitle: qsTr("Current version: v%1 (%2)").arg(Qt.application.version || "1.0.0-rc.7").arg(Qt.platform.os)
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                controlWidth: root.compact ? 160 : 200

                SentinelButton {
                    accent: root.modeAccent
                    anchors.fill: parent
                    text: qsTr("Check Updates")
                    onClicked: root.openUpdateRequested()
                }
            }

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

            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
            }

            SettingToggleRow {
                title: qsTr("Enable Sound Effects")
                subtitle: qsTr("Play subtle audio cues for completions, notifications, and key interactions.")
                checked: root.viewModel.soundEffectsEnabled
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.soundEffectsEnabled = checked
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
                subtitle: qsTr("Show read-only permission, tool, agent, notification, and task diagnostics. This does not change execution permissions.")
                checked: root.viewModel.developerModeEnabled
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.developerModeEnabled = checked
            }

            SettingCard {
                visible: root.viewModel.developerModeEnabled
                title: qsTr("Developer Diagnostics")
                subtitle: qsTr("Read-only runtime details for troubleshooting.")

                Label {
                    Layout.fillWidth: true
                    text: root.viewModel.diagnosticsCenterSummaries.join("\n")
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                    leftPadding: SentinelTheme.spaceMd
                    rightPadding: SentinelTheme.spaceMd
                    bottomPadding: SentinelTheme.spaceSm
                }
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
            subtitle: qsTr("Turn each notification type on or off. New notification types can be added here without changing the rest of Settings.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            title: qsTr("Notification Types")
            subtitle: qsTr("Only the events you enable will notify you.")

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
