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
    property var voiceFileDialog: null
    property var soundManager: null
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
            title: qsTr("AI Settings & Runtimes")
            subtitle: qsTr("Configure and inspect local AI inference runtimes, providers, and endpoints.")
            Layout.fillWidth: true
        }

        SettingCard {
            title: qsTr("Active Runtime Status")

            SettingControlRow {
                title: qsTr("Provider")
                subtitle: qsTr("Active LLM engine backend.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    id: runtimeProviderCombo
                    accent: root.modeAccent
                    anchors.fill: parent
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

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceSm

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
            }
        }

        SettingCard {
            title: qsTr("Inference Features")

            SettingToggleRow {
                title: qsTr("Enable Local Chat Inference")
                subtitle: qsTr("Allow conversations to be processed locally via Ollama, LM Studio, or llama.cpp.")
                checked: root.viewModel.localChatInferenceEnabled
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.localChatInferenceEnabled = checked
            }

            SettingToggleRow {
                title: qsTr("Enable Token Streaming")
                subtitle: qsTr("Stream response tokens in real-time as they are produced by the inference engine.")
                checked: root.viewModel.localInferenceStreamingEnabled
                accent: root.modeAccent
                compact: root.compact
                onToggled: (checked) => root.viewModel.localInferenceStreamingEnabled = checked
            }
        }

        SettingCard {
            title: qsTr("Endpoints & Routing")
            subtitle: qsTr("Configure server URLs and provider routing logic.")

            SettingControlRow {
                title: qsTr("Routing Mode")
                subtitle: qsTr("Strategy for selecting between local models and cloud providers.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    id: routingModeCombo
                    accent: root.modeAccent
                    anchors.fill: parent
                    implicitHeight: 36
                    model: root.viewModel.availableRoutingModes
                    currentIndex: root.viewModel.availableRoutingModes.indexOf(root.viewModel.currentRoutingMode)
                    displayText: currentIndex >= 0 ? currentText : root.viewModel.currentRoutingMode
                    onActivated: (index) => {
                        if (index >= 0 && index < root.viewModel.availableRoutingModes.length)
                            root.viewModel.setRoutingModeByName(root.viewModel.availableRoutingModes[index])
                    }
                }
            }

            SettingControlRow {
                title: qsTr("Ollama Endpoint")
                subtitle: qsTr("Local REST endpoint for Ollama daemon.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    anchors.fill: parent
                    placeholderText: "http://127.0.0.1:11434"
                    text: root.viewModel.ollamaEndpoint
                    onEditingFinished: root.viewModel.ollamaEndpoint = text
                }
            }

            SettingControlRow {
                title: qsTr("LM Studio Endpoint")
                subtitle: qsTr("Local REST endpoint for LM Studio server.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    anchors.fill: parent
                    placeholderText: "http://127.0.0.1:1234"
                    text: root.viewModel.lmStudioEndpoint
                    onEditingFinished: root.viewModel.lmStudioEndpoint = text
                }
            }

            SettingControlRow {
                title: qsTr("llama.cpp Endpoint")
                subtitle: qsTr("Local REST endpoint for llama-server process.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    anchors.fill: parent
                    placeholderText: "http://127.0.0.1:8080"
                    text: root.viewModel.llamaCppEndpoint
                    onEditingFinished: root.viewModel.llamaCppEndpoint = text
                }
            }

            SettingControlRow {
                title: qsTr("Cloud API Endpoint")
                subtitle: qsTr("Custom OpenAI-compatible cloud gateway endpoint.")
                accent: root.modeAccent
                compact: root.compact

                SentinelTextField {
                    anchors.fill: parent
                    placeholderText: "https://api.openai.com/v1"
                    text: root.viewModel.cloudApiEndpoint
                    onEditingFinished: root.viewModel.cloudApiEndpoint = text
                }
            }
        }

        SettingCard {
            title: qsTr("Inference Parameters")
            subtitle: qsTr("Fine-tune sampling temperature, top-p nucleus sampling, and context boundaries.")

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceSm

                Label {
                    text: qsTr("Inference Presets")
                    color: SentinelTheme.textPrimary
                    font.pixelSize: SentinelTheme.fontBody
                    font.weight: Font.Medium
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceSm

                    Repeater {
                        model: [
                            { "name": qsTr("Precise"), "temp": 0.20, "topP": 0.80 },
                            { "name": qsTr("Balanced"), "temp": 0.70, "topP": 0.90 },
                            { "name": qsTr("Creative"), "temp": 1.10, "topP": 0.95 }
                        ]

                        delegate: Button {
                            id: presetBtn
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: 34
                            hoverEnabled: true
                            focusPolicy: Qt.NoFocus

                            readonly property bool isSelected: Math.abs(root.viewModel.localInferenceTemperature - modelData.temp) < 0.08

                            onClicked: {
                                root.viewModel.localInferenceTemperature = modelData.temp
                                root.viewModel.localInferenceTopP = modelData.topP
                            }

                            contentItem: Text {
                                text: presetBtn.modelData.name
                                color: presetBtn.isSelected ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                font.weight: presetBtn.isSelected ? Font.DemiBold : Font.Normal
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusMd
                                color: presetBtn.isSelected
                                       ? SentinelTheme.withAlpha(root.modeAccent, 0.20)
                                       : presetBtn.hovered
                                         ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                         : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
                                border.color: presetBtn.isSelected
                                              ? root.modeAccent
                                              : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                border.width: presetBtn.isSelected ? 1.5 : 1

                                Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }
                                Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
            }

            SettingControlRow {
                title: qsTr("Temperature (%1)").arg(root.viewModel.localInferenceTemperature.toFixed(2))
                subtitle: qsTr("Higher values produce more creative responses, lower values make output more deterministic.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                controlWidth: root.compact ? 150 : 200

                SentinelSlider {
                    accent: root.modeAccent
                    anchors.fill: parent
                    from: 0.0
                    to: 1.5
                    stepSize: 0.05
                    value: root.viewModel.localInferenceTemperature
                    onValueChanged: root.viewModel.localInferenceTemperature = value
                }
            }

            SettingControlRow {
                title: qsTr("Top-P (%1)").arg(root.viewModel.localInferenceTopP.toFixed(2))
                subtitle: qsTr("Nucleus sampling cumulative probability threshold.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                controlWidth: root.compact ? 150 : 200

                SentinelSlider {
                    accent: root.modeAccent
                    anchors.fill: parent
                    from: 0.1
                    to: 1.0
                    stepSize: 0.05
                    value: root.viewModel.localInferenceTopP
                    onValueChanged: root.viewModel.localInferenceTopP = value
                }
            }

            SettingControlRow {
                title: qsTr("Max Tokens")
                subtitle: qsTr("Maximum token generation length per response.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                controlWidth: root.compact ? 150 : 200

                SentinelSpinBox {
                    accent: root.modeAccent
                    anchors.fill: parent
                    from: 64
                    to: 32768
                    stepSize: 64
                    value: root.viewModel.localInferenceMaxTokens
                    onValueModified: root.viewModel.localInferenceMaxTokens = value
                }
            }

            SettingControlRow {
                title: qsTr("Timeout (ms)")
                subtitle: qsTr("Maximum waiting time before timing out requests.")
                accent: root.modeAccent
                compact: root.compact
                controlWidth: root.compact ? 150 : 200

                SentinelSpinBox {
                    accent: root.modeAccent
                    anchors.fill: parent
                    from: 1000
                    to: 300000
                    stepSize: 1000
                    value: root.viewModel.localInferenceTimeoutMs
                    onValueModified: root.viewModel.localInferenceTimeoutMs = value
                }
            }
        }
    }
}
