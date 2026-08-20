// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
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
    readonly property var cloudProviderNames: ["OpenAI", "Claude", "Gemini", "DeepSeek", "Groq", "Mistral"]
    readonly property string currentProvider: root.viewModel.selectedRuntimeProvider
    readonly property var modelList: {
        if (root.currentProvider === "ollama") return root.viewModel.ollamaModelNames
        if (root.currentProvider === "lm-studio") return root.viewModel.loadedLMStudioModelNames
        // Cloud providers (cloud-api, openai, claude, gemini, deepseek, groq,
        // mistral) and the other OpenAI-compatible local runtimes share the
        // same discovered model-name list.
        return root.viewModel.ollamaModelNames
    }
    readonly property var inferencePresetModel: [
        { "name": qsTr("Precise"), "temp": 0.20, "topP": 0.80 },
        { "name": qsTr("Balanced"), "temp": 0.70, "topP": 0.90 },
        { "name": qsTr("Creative"), "temp": 1.10, "topP": 0.95 }
    ]
    readonly property var inferencePresetLabels: [
        qsTr("Precise"),
        qsTr("Balanced"),
        qsTr("Creative")
    ]

    height: implicitHeight
    implicitHeight: visible ? mainLayout.implicitHeight + panelPadding * 2 : 0

    function hasSelectableModels() {
        return root.modelList.length > 0
    }

    function inferencePresetIndex() {
        var bestIndex = 0
        var bestDistance = 999
        for (var i = 0; i < root.inferencePresetModel.length; ++i) {
            var preset = root.inferencePresetModel[i]
            var distance = Math.abs(root.viewModel.localInferenceTemperature - preset.temp)
                         + Math.abs(root.viewModel.localInferenceTopP - preset.topP)
            if (distance < bestDistance) {
                bestDistance = distance
                bestIndex = i
            }
        }
        return bestDistance < 0.18 ? bestIndex : -1
    }

    function applyInferencePreset(index) {
        if (index < 0 || index >= root.inferencePresetModel.length)
            return
        var preset = root.inferencePresetModel[index]
        root.viewModel.localInferenceTemperature = preset.temp
        root.viewModel.localInferenceTopP = preset.topP
    }

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

            SettingControlRow {
                title: qsTr("Runtime Status")
                subtitle: root.viewModel.localInferenceHealthSummary
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                StatusChip {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: parent.width
                    value: root.viewModel.localInferenceRuntimeState.length > 0
                           ? root.viewModel.localInferenceRuntimeState
                           : qsTr("Unknown")
                    accent: root.modeAccent
                    active: root.viewModel.localInferenceRuntimeState.toLowerCase().indexOf("ready") >= 0
                         || root.viewModel.localInferenceRuntimeState.toLowerCase().indexOf("healthy") >= 0
                    muted: root.viewModel.localInferenceRuntimeState.length === 0
                }
            }

            SettingControlRow {
                title: qsTr("Active Model")
                subtitle: root.hasSelectableModels()
                          ? root.viewModel.selectedLocalModelSummary
                          : qsTr("Model selection is available after the active local runtime reports models.")
                accent: root.modeAccent
                compact: root.compact

                SentinelComboBox {
                    id: activeModelCombo
                    accent: root.modeAccent
                    anchors.fill: parent
                    implicitHeight: 36
                    enabled: root.hasSelectableModels()
                    model: root.hasSelectableModels() ? root.modelList : [qsTr("None selected")]
                    currentIndex: root.hasSelectableModels()
                                  ? root.modelList.indexOf(root.viewModel.selectedLocalModel)
                                  : 0
                    displayText: {
                        if (root.hasSelectableModels())
                            return currentIndex >= 0 ? currentText : root.viewModel.activeLocalModelName
                        return root.viewModel.activeLocalModelName
                               ? root.viewModel.activeLocalModelName
                               : qsTr("None selected")
                    }
                    onActivated: (index) => {
                        if (index >= 0 && index < root.modelList.length)
                            root.viewModel.selectedLocalModel = root.modelList[index]
                    }
                }
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
            title: qsTr("Cloud Providers")
            subtitle: qsTr("Select a cloud provider and configure its API credentials. Keys are saved to local settings.")

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
                    implicitHeight: 36
                    model: root.cloudProviderNames
                    currentIndex: {
                        // Settings stores lowercase ids; the display list is
                        // capitalized, so compare case-insensitively.
                        var stored = (root.viewModel.selectedCloudProvider || "").toLowerCase()
                        for (var i = 0; i < root.cloudProviderNames.length; ++i) {
                            if (root.cloudProviderNames[i].toLowerCase() === stored)
                                return i
                        }
                        return 0
                    }
                    displayText: currentIndex >= 0 ? currentText : root.viewModel.selectedCloudProvider
                    onActivated: (index) => {
                        if (index >= 0 && index < root.cloudProviderNames.length)
                            root.viewModel.selectedCloudProvider = root.cloudProviderNames[index]
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

        SettingCard {
            title: qsTr("Inference Parameters")
            subtitle: qsTr("Fine-tune sampling temperature, top-p nucleus sampling, and context boundaries.")

            SettingControlRow {
                title: qsTr("Inference Presets")
                subtitle: qsTr("Quickly apply paired temperature and top-p values.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    id: inferencePresetCombo
                    accent: root.modeAccent
                    anchors.fill: parent
                    implicitHeight: 36
                    model: root.inferencePresetLabels
                    currentIndex: root.inferencePresetIndex()
                    displayText: currentIndex >= 0 ? currentText : qsTr("Custom")
                    onActivated: (index) => root.applyInferencePreset(index)
                }
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
