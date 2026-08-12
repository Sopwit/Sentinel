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
            subtitle: qsTr("Configure system-level permission policies, proxy settings, and sandbox boundaries.")
            Layout.fillWidth: true
        }

        SettingCard {
            SettingControlRow {
                title: qsTr("Default Policy")
                subtitle: qsTr("Global permission policy baseline for tool execution and desktop integration.")
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
                    label: qsTr("Tool Execution Gateway")
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
                subtitle: qsTr("Allow AI agents to execute approved tool pipelines without requiring manual approval per step.")
                checked: root.viewModel.agentAutonomousMode
                accent: root.modeAccent
                compact: root.compact
                onToggled: (checked) => root.viewModel.agentAutonomousMode = checked
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
