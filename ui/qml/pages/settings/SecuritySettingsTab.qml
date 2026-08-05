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
    }
}
