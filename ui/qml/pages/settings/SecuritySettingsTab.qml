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

    height: implicitHeight
    implicitHeight: visible ? mainLayout.implicitHeight + panelPadding * 2 : 0

    ColumnLayout {
        id: mainLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: root.panelPadding
        spacing: SentinelTheme.spaceMd

        SectionTitle {
            title: qsTr("Security & Agent Boundaries")
            subtitle: qsTr("Set the safety rules for tools and agents. Start with the policy, then review agent tasks before they run.")
            Layout.fillWidth: true
        }

        SettingCard {
            title: qsTr("Execution Safety")
            subtitle: qsTr("These controls apply to every tool and agent action.")

            SettingControlRow {
                title: qsTr("Default Policy")
                subtitle: qsTr("Choose how much permission is granted by default.")
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

        SectionTitle {
            title: qsTr("Controlled Agent Tasks")
            subtitle: qsTr("Describe a task, review the plan, then approve and start it.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            title: qsTr("Review Before Running")
            subtitle: qsTr("Nothing starts until you approve the generated plan.")

            SettingControlRow {
                title: qsTr("Task Description")
                subtitle: qsTr("Describe one bounded task for the agent.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    id: controlledGoal
                    anchors.fill: parent
                    placeholderText: qsTr("Example: summarize the selected workspace files")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                spacing: SentinelTheme.spaceSm

                SentinelButton {
                    text: qsTr("Create Plan")
                    accent: root.modeAccent
                    enabled: controlledGoal.text.trim().length > 0
                    onClicked: root.viewModel.planControlledAgentTask(controlledGoal.text.trim())
                }
                SentinelButton {
                    text: qsTr("Approve")
                    accent: SentinelTheme.success
                    enabled: root.viewModel.agentPlanId.length > 0
                    onClicked: root.viewModel.approveControlledAgentTask(root.viewModel.agentPlanId, "approve")
                }
                SentinelButton {
                    text: qsTr("Start")
                    accent: root.modeAccent
                    enabled: root.viewModel.agentPlanId.length > 0
                    onClicked: root.viewModel.startControlledAgentTask(root.viewModel.agentPlanId)
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                spacing: SentinelTheme.spaceXs

                InfoRow {
                    compact: root.compact
                    label: qsTr("Plan Status")
                    value: root.viewModel.agentPlanApprovalState + " / " + root.viewModel.agentPlanEstimatedRisk
                    Layout.fillWidth: true
                }
                InfoRow {
                    compact: root.compact
                    label: qsTr("Permissions")
                    value: root.viewModel.agentPlanRequiredPermissions.join(", ") || qsTr("None declared")
                    Layout.fillWidth: true
                }
                Label {
                    Layout.fillWidth: true
                    text: root.viewModel.agentPlanSteps.join("\n")
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                    visible: text.length > 0
                }
                Label {
                    Layout.fillWidth: true
                    text: root.viewModel.latestApprovalSummary + "\n" + root.viewModel.latestSandboxSummary
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                    visible: root.viewModel.latestApprovalSummary.length > 0 || root.viewModel.latestSandboxSummary.length > 0
                }
            }
        }

        SectionTitle {
            title: qsTr("Network & Proxy Settings")
            subtitle: qsTr("Configure HTTP/SOCKS proxy for remote endpoints and model APIs.")
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
            title: qsTr("Cloud Providers")
            subtitle: qsTr("Select a cloud provider and configure its API credentials. Keys are saved to local settings.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
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
                subtitle: qsTr("API-backed search provider for agent web searches.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                SentinelComboBox {
                    anchors.fill: parent
                    accent: root.modeAccent
                    model: ["Exa", "Parallel"]
                    currentIndex: root.viewModel.webSearchProvider === "parallel" ? 1 : 0
                    onActivated: (index) => root.viewModel.webSearchProvider = index === 1 ? "parallel" : "exa"
                }
            }
            SettingControlRow {
                title: qsTr("Search API Key")
                subtitle: qsTr("Credential sent only to the selected search provider.")
                accent: root.modeAccent
                compact: root.compact
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

        SettingCard {
            SettingControlRow {
                title: qsTr("Active Cloud Provider")
                subtitle: qsTr("Primary provider for cloud LLM inference.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    id: cloudProviderCombo
                    accent: root.modeAccent
                    anchors.fill: parent
                    model: ["OpenAI", "Claude", "Gemini", "DeepSeek", "Groq", "Mistral"]
                    currentIndex: root.viewModel.selectedCloudProvider === "OpenAI" ? 0
                        : root.viewModel.selectedCloudProvider === "Claude" ? 1
                        : root.viewModel.selectedCloudProvider === "Gemini" ? 2
                        : root.viewModel.selectedCloudProvider === "DeepSeek" ? 3
                        : root.viewModel.selectedCloudProvider === "Groq" ? 4
                        : root.viewModel.selectedCloudProvider === "Mistral" ? 5 : 0
                    onActivated: (index) => {
                        var providers = ["OpenAI", "Claude", "Gemini", "DeepSeek", "Groq", "Mistral"]
                        root.viewModel.selectedCloudProvider = providers[index]
                    }
                }
            }

            SettingControlRow {
                visible: root.viewModel.selectedCloudProvider === "OpenAI"
                title: qsTr("OpenAI API Key")
                subtitle: qsTr("Authentication key for GPT-4, GPT-4o, and OpenAI endpoints.")
                accent: root.modeAccent
                compact: root.compact

                SentinelTextField {
                    anchors.fill: parent
                    echoMode: TextInput.Password
                    placeholderText: "sk-..."
                    text: root.viewModel.openAiApiKey
                    onEditingFinished: root.viewModel.openAiApiKey = text
                }
            }

            SettingControlRow {
                visible: root.viewModel.selectedCloudProvider === "Claude"
                title: qsTr("Claude API Key")
                subtitle: qsTr("Authentication key for Anthropic Claude 3.5 Sonnet and Opus models.")
                accent: root.modeAccent
                compact: root.compact

                SentinelTextField {
                    anchors.fill: parent
                    echoMode: TextInput.Password
                    placeholderText: "sk-ant-..."
                    text: root.viewModel.claudeApiKey
                    onEditingFinished: root.viewModel.claudeApiKey = text
                }
            }

            SettingControlRow {
                visible: root.viewModel.selectedCloudProvider === "Gemini"
                title: qsTr("Gemini API Key")
                subtitle: qsTr("Authentication key for Google Gemini Pro and Flash models.")
                accent: root.modeAccent
                compact: root.compact

                SentinelTextField {
                    anchors.fill: parent
                    echoMode: TextInput.Password
                    placeholderText: "AIza..."
                    text: root.viewModel.geminiApiKey
                    onEditingFinished: root.viewModel.geminiApiKey = text
                }
            }

            SettingControlRow {
                visible: root.viewModel.selectedCloudProvider === "DeepSeek"
                title: qsTr("DeepSeek API Key")
                subtitle: qsTr("Authentication key for DeepSeek V3 and DeepSeek R1 reasoning endpoints.")
                accent: root.modeAccent
                compact: root.compact

                SentinelTextField {
                    anchors.fill: parent
                    echoMode: TextInput.Password
                    placeholderText: "sk-..."
                    text: root.viewModel.deepseekApiKey
                    onEditingFinished: root.viewModel.deepseekApiKey = text
                }
            }

            SettingControlRow {
                visible: root.viewModel.selectedCloudProvider === "Groq"
                title: qsTr("Groq API Key")
                subtitle: qsTr("Authentication key for ultra-fast Groq LPU inference.")
                accent: root.modeAccent
                compact: root.compact

                SentinelTextField {
                    anchors.fill: parent
                    echoMode: TextInput.Password
                    placeholderText: "gsk_..."
                    text: root.viewModel.groqApiKey
                    onEditingFinished: root.viewModel.groqApiKey = text
                }
            }

            SettingControlRow {
                visible: root.viewModel.selectedCloudProvider === "Mistral"
                title: qsTr("Mistral API Key")
                subtitle: qsTr("Authentication key for Mistral Large, Codestral, and NeMo endpoints.")
                accent: root.modeAccent
                compact: root.compact

                SentinelTextField {
                    anchors.fill: parent
                    echoMode: TextInput.Password
                    placeholderText: "sk-..."
                    text: root.viewModel.mistralApiKey
                    onEditingFinished: root.viewModel.mistralApiKey = text
                }
            }
        }
    }
}
