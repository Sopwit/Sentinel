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
    readonly property int panelPadding: SentinelTheme.spaceLg
    property string activeTaskId: ""
    readonly property bool taskRunning: root.viewModel.controlledTaskActiveSummary.indexOf("[Running]") >= 0

    height: implicitHeight
    implicitHeight: visible ? mainLayout.implicitHeight + panelPadding * 2 : 0

    function createControlledPlan() {
        const goal = controlledGoal.text.trim()
        if (goal.length === 0)
            return
        root.activeTaskId = root.viewModel.planControlledAgentTask(goal)
    }

    ColumnLayout {
        id: mainLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: root.panelPadding
        spacing: SentinelTheme.spaceMd

        // ------------------------------------------------------------------
        // 1. Permissions and safety
        // ------------------------------------------------------------------

        SectionTitle {
            title: qsTr("Permissions & Safety")
            subtitle: qsTr("Set the safety rules that govern every tool and agent action.")
            Layout.fillWidth: true
        }

        SettingCard {
            title: qsTr("Execution Safety")
            subtitle: qsTr("Start with the default policy, then decide how much freedom approved agents get.")

            SettingControlRow {
                title: qsTr("Default Policy")
                subtitle: qsTr("How much permission tools and agents are granted by default.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    id: permissionStateCombo
                    accent: root.modeAccent
                    anchors.fill: parent
                    model: root.viewModel.permissionPolicyStateLabels
                    currentIndex: root.viewModel.permissionPolicyStateLabels.indexOf(root.viewModel.defaultPermissionPolicyState)
                    displayText: currentIndex >= 0 ? currentText : root.viewModel.defaultPermissionPolicyState
                    onActivated: (index) => {
                        if (index >= 0 && index < root.viewModel.permissionPolicyStateLabels.length)
                            root.viewModel.defaultPermissionPolicyState = root.viewModel.permissionPolicyStateLabels[index]
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm

                InfoRow {
                    compact: root.compact
                    label: qsTr("Tool Gateway")
                    value: root.viewModel.toolGatewayStatus || qsTr("Inactive")
                    Layout.fillWidth: true
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
            }

            SettingToggleRow {
                title: qsTr("Agent Autonomous Mode")
                subtitle: qsTr("Let approved plans continue without asking at every step.")
                checked: root.viewModel.agentAutonomousMode
                accent: root.modeAccent
                compact: root.compact
                onToggled: (checked) => root.viewModel.agentAutonomousMode = checked
            }
        }

        // ------------------------------------------------------------------
        // 2. Controlled agent tasks
        // ------------------------------------------------------------------

        SectionTitle {
            title: qsTr("Controlled Agent Tasks")
            subtitle: qsTr("Three steps: describe the task, review its plan, then approve and run it.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            title: qsTr("1. Describe the Task")
            subtitle: qsTr("The plan is generated from your description.")

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceSm

                    SentinelTextField {
                        id: controlledGoal
                        Layout.fillWidth: true
                        placeholderText: qsTr("Example: summarize the selected workspace files")
                        onAccepted: root.createControlledPlan()
                    }

                    SentinelButton {
                        text: qsTr("Create Plan")
                        accent: root.modeAccent
                        enabled: controlledGoal.text.trim().length > 0
                        onClicked: root.createControlledPlan()
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.activeTaskId.length === 0
                    text: qsTr("The generated plan appears in the next two steps once created.")
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                }
            }
        }

        SettingCard {
            visible: root.activeTaskId.length > 0
            title: qsTr("2. Review the Plan")
            subtitle: qsTr("Check what the agent will do before approving it.")

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceXs

                InfoRow {
                    compact: root.compact
                    label: qsTr("Status")
                    value: root.viewModel.controlledTaskActiveSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Current Step")
                    value: root.viewModel.controlledTaskCurrentStep
                    visible: root.viewModel.controlledTaskCurrentStep.indexOf("No visible step") !== 0
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Progress")
                    value: root.viewModel.controlledTaskProgressSummary
                    visible: root.viewModel.controlledTaskProgressSummary.indexOf("No running") !== 0
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: SentinelTheme.spaceXs
                    height: 1
                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Plan Steps")
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                }

                Repeater {
                    model: root.viewModel.controlledTaskPlanSteps

                    Label {
                        required property string modelData
                        Layout.fillWidth: true
                        Layout.leftMargin: SentinelTheme.spaceSm
                        text: modelData
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        SettingCard {
            visible: root.activeTaskId.length > 0
            title: qsTr("3. Approve & Run")
            subtitle: qsTr("Approve the plan, start the task, then run each step. Every step executes for real through the sandbox.")

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceSm

                    SentinelButton {
                        visible: !root.taskRunning
                        text: qsTr("Approve")
                        accent: SentinelTheme.success
                        onClicked: root.viewModel.approveControlledAgentTask(root.activeTaskId, "approve")
                    }

                    SentinelButton {
                        visible: !root.taskRunning
                        text: qsTr("Deny")
                        accent: SentinelTheme.warning
                        onClicked: root.viewModel.denyControlledAgentTask(root.activeTaskId)
                    }

                    SentinelButton {
                        visible: !root.taskRunning
                        text: qsTr("Start")
                        accent: root.modeAccent
                        onClicked: root.viewModel.startControlledAgentTask(root.activeTaskId)
                    }

                    SentinelButton {
                        visible: root.taskRunning
                        Layout.fillWidth: true
                        text: qsTr("Run Current Step")
                        accent: root.modeAccent
                        onClicked: root.viewModel.executeControlledAgentStep(root.activeTaskId)
                    }
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Approval")
                    value: root.viewModel.latestApprovalSummary
                    visible: root.viewModel.latestApprovalSummary.length > 0
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Sandbox")
                    value: root.viewModel.latestSandboxSummary
                    visible: root.viewModel.latestSandboxSummary.length > 0
                    Layout.fillWidth: true
                }
            }
        }

        // ------------------------------------------------------------------
        // 3. Connections
        // ------------------------------------------------------------------

        SectionTitle {
            title: qsTr("Network & Proxy")
            subtitle: qsTr("Route remote endpoints and model APIs through an HTTP/SOCKS proxy.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            SettingToggleRow {
                title: qsTr("Use Network Proxy")
                subtitle: qsTr("Route cloud model API and remote web requests through a proxy server.")
                checked: root.viewModel.proxyEnabled
                accent: root.modeAccent
                compact: root.compact
                showDivider: root.viewModel.proxyEnabled
                onToggled: (checked) => root.viewModel.proxyEnabled = checked
            }

            SettingControlRow {
                visible: root.viewModel.proxyEnabled
                title: qsTr("Proxy Type")
                subtitle: qsTr("Protocol used by the proxy gateway.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    accent: root.modeAccent
                    anchors.fill: parent
                    model: ["HTTP", "SOCKS5"]
                    currentIndex: root.viewModel.proxyType === "SOCKS5" ? 1 : 0
                    onActivated: (index) => root.viewModel.proxyType = (index === 1 ? "SOCKS5" : "HTTP")
                }
            }

            SettingControlRow {
                visible: root.viewModel.proxyEnabled
                title: qsTr("Host & Port")
                subtitle: qsTr("Hostname/IP address and port number.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                RowLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceSm

                    SentinelTextField {
                        Layout.fillWidth: true
                        placeholderText: "127.0.0.1"
                        text: root.viewModel.proxyHost
                        onEditingFinished: root.viewModel.proxyHost = text
                    }

                    SentinelTextField {
                        Layout.preferredWidth: 80
                        placeholderText: "8080"
                        text: root.viewModel.proxyPort ? root.viewModel.proxyPort.toString() : ""
                        onEditingFinished: root.viewModel.proxyPort = parseInt(text) || 0
                    }
                }
            }

            SettingControlRow {
                visible: root.viewModel.proxyEnabled
                title: qsTr("Proxy Username")
                subtitle: qsTr("Optional proxy authentication username.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    anchors.fill: parent
                    placeholderText: "user"
                    text: root.viewModel.proxyUser
                    onEditingFinished: root.viewModel.proxyUser = text
                }
            }

            SettingControlRow {
                visible: root.viewModel.proxyEnabled
                title: qsTr("Proxy Password")
                subtitle: qsTr("Optional proxy authentication password.")
                accent: root.modeAccent
                compact: root.compact

                SentinelTextField {
                    anchors.fill: parent
                    echoMode: TextInput.Password
                    placeholderText: "••••••••"
                    text: root.viewModel.proxyPassword
                    onEditingFinished: root.viewModel.proxyPassword = text
                }
            }
        }

        SectionTitle {
            title: qsTr("Web Search")
            subtitle: qsTr("Configure the provider used by the approved web-search tool.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            SettingControlRow {
                title: qsTr("Search Provider")
                subtitle: qsTr("Search provider for agent web searches. DuckDuckGo needs no API key; Exa and Parallel require one.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    anchors.fill: parent
                    accent: root.modeAccent
                    model: [qsTr("DuckDuckGo (no API key)"), "Exa", "Parallel"]
                    currentIndex: root.viewModel.webSearchProvider === "exa" ? 1
                                  : (root.viewModel.webSearchProvider === "parallel" ? 2 : 0)
                    onActivated: (index) => {
                        root.viewModel.webSearchProvider = index === 1 ? "exa"
                                      : (index === 2 ? "parallel" : "duckduckgo")
                    }
                }
            }

            SettingControlRow {
                title: qsTr("Search API Key")
                subtitle: qsTr("Credential sent only to the selected search provider. Leave empty to use the keyless DuckDuckGo fallback.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    anchors.fill: parent
                    echoMode: TextInput.Password
                    text: root.viewModel.webSearchApiKey
                    onEditingFinished: root.viewModel.webSearchApiKey = text
                }
            }

            SettingControlRow {
                title: qsTr("Maximum Results")
                subtitle: qsTr("Limit the number of search results supplied to an approved request.")
                accent: root.modeAccent
                compact: root.compact

                SentinelSpinBox {
                    anchors.fill: parent
                    from: 1
                    to: 20
                    value: root.viewModel.webSearchMaxResults
                    onValueModified: root.viewModel.webSearchMaxResults = value
                }
            }
        }
    }
}
