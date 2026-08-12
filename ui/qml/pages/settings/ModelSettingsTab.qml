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
                text: qsTr("Routing Mode")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelComboBox {
                id: routingModeCombo
                accent: root.modeAccent
                Layout.fillWidth: true
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

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("LM Studio Endpoint")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "http://127.0.0.1:1234"
                text: root.viewModel.lmStudioEndpoint
                onEditingFinished: root.viewModel.lmStudioEndpoint = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("llama.cpp Endpoint")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "http://127.0.0.1:8080"
                text: root.viewModel.llamaCppEndpoint
                onEditingFinished: root.viewModel.llamaCppEndpoint = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Cloud API Endpoint")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "https://api.openai.com/v1"
                text: root.viewModel.cloudApiEndpoint
                onEditingFinished: root.viewModel.cloudApiEndpoint = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Timeout (ms)")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SpinBox {
                Layout.fillWidth: true
                from: 1000
                to: 300000
                stepSize: 1000
                value: root.viewModel.localInferenceTimeoutMs
                onValueModified: root.viewModel.localInferenceTimeoutMs = value
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Top-P (%1)").arg(root.viewModel.localInferenceTopP.toFixed(2))
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            Slider {
                Layout.fillWidth: true
                from: 0.1
                to: 1.0
                stepSize: 0.05
                value: root.viewModel.localInferenceTopP
                onValueChanged: root.viewModel.localInferenceTopP = value
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Max Tokens")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SpinBox {
                Layout.fillWidth: true
                from: 64
                to: 32768
                stepSize: 64
                value: root.viewModel.localInferenceMaxTokens
                onValueModified: root.viewModel.localInferenceMaxTokens = value
            }
        }

        SectionTitle {
            title: qsTr("Voice & Text To Speech")
            subtitle: qsTr("Configure local Piper and Whisper runtime paths for TTS and transcription.")
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("TTS Engine")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }
            SentinelComboBox {
                accent: root.modeAccent
                Layout.fillWidth: true
                model: ["Piper", "Kokoro"]
                currentIndex: root.viewModel.selectedTtsEngine === "Kokoro" ? 1 : 0
                onActivated: (index) => root.viewModel.selectedTtsEngine = (index === 1 ? "Kokoro" : "Piper")
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.selectedTtsEngine === "Kokoro"
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Kokoro Model Path")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "/path/to/kokoro.onnx"
                text: root.viewModel.kokoroModelPath
                onEditingFinished: root.viewModel.kokoroModelPath = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.selectedTtsEngine === "Kokoro"
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Kokoro Voice")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "af_bella"
                text: root.viewModel.kokoroVoice
                onEditingFinished: root.viewModel.kokoroVoice = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.selectedTtsEngine === "Piper"
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Piper Binary Path")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "/path/to/piper"
                text: root.viewModel.piperBinaryPath
                onEditingFinished: root.viewModel.piperBinaryPath = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.viewModel.selectedTtsEngine === "Piper"
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Piper Model Path")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "/path/to/voice.onnx"
                text: root.viewModel.piperModelPath
                onEditingFinished: root.viewModel.piperModelPath = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Whisper Binary Path")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "/path/to/whisper"
                text: root.viewModel.whisperBinaryPath
                onEditingFinished: root.viewModel.whisperBinaryPath = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Whisper Model Path")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 132
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "/path/to/model.bin"
                text: root.viewModel.whisperModelPath
                onEditingFinished: root.viewModel.whisperModelPath = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Enable Piper File Output")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.fillWidth: true
            }
            Switch {
                checked: root.viewModel.piperFileOutputExecutionEnabled
                onCheckedChanged: root.viewModel.piperFileOutputExecutionEnabled = checked
            }
        }
    }
}
