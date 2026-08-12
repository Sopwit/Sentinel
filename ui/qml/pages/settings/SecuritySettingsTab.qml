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
    readonly property int panelPadding: SentinelTheme.spaceLg

    height: implicitHeight
    implicitHeight: visible ? mainLayout.implicitHeight + panelPadding * 2 : 0

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: root.panelPadding
        spacing: SentinelTheme.spaceMd

        SectionTitle {
            title: qsTr("Security & Agent Boundaries")
            subtitle: qsTr("Configure system-level permission policies, proxy settings, and sandbox boundaries.")
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: root.compact ? 88 : 132
                text: qsTr("Default Policy")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
            }

            SentinelComboBox {
                id: permissionStateCombo
                accent: root.modeAccent
                Layout.fillWidth: true
                model: root.viewModel.permissionPolicyStateLabels
                currentIndex: root.viewModel.permissionPolicyStateLabels.indexOf(root.viewModel.defaultPermissionPolicyState)
                displayText: currentIndex >= 0 ? currentText : root.viewModel.defaultPermissionPolicyState
                onActivated: (index) => {
                    if (index >= 0 && index < root.viewModel.permissionPolicyStateLabels.length)
                        root.viewModel.defaultPermissionPolicyState = root.viewModel.permissionPolicyStateLabels[index]
                }
            }
        }

        InfoRow {
            compact: root.compact
            label: qsTr("Tool Execution Gateway")
            value: root.viewModel.toolGatewayStatus || qsTr("Inactive")
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Agent Autonomous Mode")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.fillWidth: true
            }
            Switch {
                checked: root.viewModel.agentAutonomousMode
                onCheckedChanged: root.viewModel.agentAutonomousMode = checked
            }
        }

        SectionTitle {
            title: qsTr("Network & Proxy Settings")
            subtitle: qsTr("Configure HTTP/SOCKS proxy for remote endpoints and model APIs.")
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Use Network Proxy")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.fillWidth: true
            }
            Switch {
                checked: root.viewModel.proxyEnabled
                onCheckedChanged: root.viewModel.proxyEnabled = checked
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.proxyEnabled
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Proxy Type")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 100
            }

            SentinelComboBox {
                accent: root.modeAccent
                Layout.fillWidth: true
                model: ["HTTP", "SOCKS5"]
                currentIndex: root.viewModel.proxyType === "SOCKS5" ? 1 : 0
                onActivated: (index) => root.viewModel.proxyType = (index === 1 ? "SOCKS5" : "HTTP")
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.proxyEnabled
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Host & Port")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 100
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "127.0.0.1"
                text: root.viewModel.proxyHost
                onEditingFinished: root.viewModel.proxyHost = text
            }

            SentinelTextField {
                Layout.preferredWidth: 90
                placeholderText: "8080"
                text: root.viewModel.proxyPort ? root.viewModel.proxyPort.toString() : ""
                onEditingFinished: root.viewModel.proxyPort = parseInt(text) || 0
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.proxyEnabled
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Proxy Username")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 100
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "user"
                text: root.viewModel.proxyUser
                onEditingFinished: root.viewModel.proxyUser = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.proxyEnabled
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Proxy Password")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 100
            }

            SentinelTextField {
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: "••••••••"
                text: root.viewModel.proxyPassword
                onEditingFinished: root.viewModel.proxyPassword = text
            }
        }

        SectionTitle {
            title: qsTr("Cloud Providers")
            subtitle: qsTr("Select a cloud provider and configure its API credential. Keys are stored in your local settings.")
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Provider")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelComboBox {
                id: cloudProviderCombo
                accent: root.modeAccent
                Layout.fillWidth: true
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

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.selectedCloudProvider === "OpenAI"
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("OpenAI API Key")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: "sk-..."
                text: root.viewModel.openAiApiKey
                onEditingFinished: root.viewModel.openAiApiKey = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.selectedCloudProvider === "Claude"
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Claude API Key")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: "sk-ant-..."
                text: root.viewModel.claudeApiKey
                onEditingFinished: root.viewModel.claudeApiKey = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.selectedCloudProvider === "Gemini"
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Gemini API Key")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: "AIza..."
                text: root.viewModel.geminiApiKey
                onEditingFinished: root.viewModel.geminiApiKey = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.selectedCloudProvider === "DeepSeek"
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("DeepSeek API Key")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: "sk-..."
                text: root.viewModel.deepseekApiKey
                onEditingFinished: root.viewModel.deepseekApiKey = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.selectedCloudProvider === "Groq"
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Groq API Key")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: "gsk_..."
                text: root.viewModel.groqApiKey
                onEditingFinished: root.viewModel.groqApiKey = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.selectedCloudProvider === "Mistral"
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Mistral API Key")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: "sk-..."
                text: root.viewModel.mistralApiKey
                onEditingFinished: root.viewModel.mistralApiKey = text
            }
        }
    }
}
