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
    property var voiceFileDialog: null
    property var soundManager: null
    readonly property int panelPadding: SentinelTheme.spaceLg

    height: implicitHeight
    implicitHeight: visible ? mainLayout.implicitHeight + panelPadding * 2 : 0

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: root.panelPadding
        spacing: SentinelTheme.spaceMd

        SectionTitle {
            title: qsTr("AI Settings & Runtimes")
            subtitle: qsTr("Configure and inspect local AI inference runtimes, providers, and endpoints.")
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: root.compact ? 88 : 132
                text: qsTr("Provider")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
            }

            SentinelComboBox {
                id: runtimeProviderCombo
                accent: root.modeAccent
                Layout.fillWidth: true
                implicitHeight: 36
                model: root.viewModel.selectableRuntimeProviderLabels
                currentIndex: {
                    var sel = root.viewModel.selectedRuntimeProvider
                    var ids = root.viewModel.selectableRuntimeProviderIds
                    var idx = ids.indexOf(sel)
                    if (idx < 0 && (sel === "openai" || sel === "claude" ||
                                    sel === "gemini" || sel === "deepseek" ||
                                    sel === "groq"   || sel === "mistral")) {
                        idx = ids.indexOf("cloud-api")
                    }
                    return idx >= 0 ? idx : 0
                }
                displayText: currentIndex >= 0 ? currentText : root.viewModel.activeRuntimeProviderLabel
                onActivated: (index) => {
                    var ids = root.viewModel.selectableRuntimeProviderIds
                    if (index >= 0 && index < ids.length) {
                        var providerId = ids[index]
                        root.viewModel.selectedRuntimeProvider = providerId
                    }
                }
            }
        }

        InfoRow {
            compact: root.compact
            label: qsTr("Runtime Status")
            value: root.viewModel.localInferenceRuntimeState + " (" + root.viewModel.localInferenceHealthSummary + ")"
            Layout.fillWidth: true
        }

        InfoRow {
            compact: root.compact
            label: qsTr("Active Model")
            value: root.viewModel.activeLocalModelName ? root.viewModel.activeLocalModelName : qsTr("None selected")
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Enable Local Chat Inference")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.fillWidth: true
            }
            Switch {
                checked: root.viewModel.localChatInferenceEnabled
                onCheckedChanged: root.viewModel.localChatInferenceEnabled = checked
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Enable Token Streaming")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.fillWidth: true
            }
            Switch {
                checked: root.viewModel.localInferenceStreamingEnabled
                onCheckedChanged: root.viewModel.localInferenceStreamingEnabled = checked
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Ollama Endpoint")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "http://127.0.0.1:11434"
                text: root.viewModel.ollamaEndpoint
                onEditingFinished: root.viewModel.ollamaEndpoint = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Temperature (%1)").arg(root.viewModel.localInferenceTemperature.toFixed(2))
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            Slider {
                Layout.fillWidth: true
                from: 0.0
                to: 1.5
                stepSize: 0.05
                value: root.viewModel.localInferenceTemperature
                onValueChanged: root.viewModel.localInferenceTemperature = value
            }
        }
    }
}
