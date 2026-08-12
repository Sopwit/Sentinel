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
            title: qsTr("Voice & Audio Settings")
            subtitle: qsTr("Configure local speech-to-text (Whisper) and text-to-speech (Piper, Kokoro) runtimes and paths.")
            Layout.fillWidth: true
        }

        SettingCard {
            title: qsTr("Text-to-Speech Engine")

            SettingControlRow {
                title: qsTr("TTS Engine")
                subtitle: qsTr("Primary local text-to-speech synthesis backend.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    accent: root.modeAccent
                    anchors.fill: parent
                    model: ["Piper", "Kokoro"]
                    currentIndex: root.viewModel.selectedTtsEngine === "Kokoro" ? 1 : 0
                    onActivated: (index) => root.viewModel.selectedTtsEngine = (index === 1 ? "Kokoro" : "Piper")
                }
            }

            SettingControlRow {
                visible: root.viewModel.selectedTtsEngine === "Kokoro"
                title: qsTr("Kokoro Model Path")
                subtitle: qsTr("Path to Kokoro ONNX model file.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    anchors.fill: parent
                    placeholderText: "/path/to/kokoro.onnx"
                    text: root.viewModel.kokoroModelPath
                    onEditingFinished: root.viewModel.kokoroModelPath = text
                }
            }

            SettingControlRow {
                visible: root.viewModel.selectedTtsEngine === "Kokoro"
                title: qsTr("Kokoro Voice")
                subtitle: qsTr("Voice ID preset for Kokoro TTS.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    anchors.fill: parent
                    placeholderText: "af_bella"
                    text: root.viewModel.kokoroVoice
                    onEditingFinished: root.viewModel.kokoroVoice = text
                }
            }

            SettingControlRow {
                visible: root.viewModel.selectedTtsEngine === "Piper"
                title: qsTr("Piper Binary Path")
                subtitle: qsTr("Path to local Piper executable.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    anchors.fill: parent
                    placeholderText: "/path/to/piper"
                    text: root.viewModel.piperBinaryPath
                    onEditingFinished: root.viewModel.piperBinaryPath = text
                }
            }

            SettingControlRow {
                visible: root.viewModel.selectedTtsEngine === "Piper"
                title: qsTr("Piper Model Path")
                subtitle: qsTr("Path to Piper voice ONNX model file.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    anchors.fill: parent
                    placeholderText: "/path/to/voice.onnx"
                    text: root.viewModel.piperModelPath
                    onEditingFinished: root.viewModel.piperModelPath = text
                }
            }

            SettingToggleRow {
                title: qsTr("Enable Piper File Output")
                subtitle: qsTr("Save synthesized audio responses to local WAV output files for review.")
                checked: root.viewModel.piperFileOutputExecutionEnabled
                accent: root.modeAccent
                compact: root.compact
                onToggled: (checked) => root.viewModel.piperFileOutputExecutionEnabled = checked
            }
        }

        SectionTitle {
            title: qsTr("Speech-to-Text")
            subtitle: qsTr("Local speech recognition settings (Whisper).")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            SettingControlRow {
                title: qsTr("Whisper Binary Path")
                subtitle: qsTr("Path to local whisper.cpp executable.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    anchors.fill: parent
                    placeholderText: "/path/to/whisper"
                    text: root.viewModel.whisperBinaryPath
                    onEditingFinished: root.viewModel.whisperBinaryPath = text
                }
            }

            SettingControlRow {
                title: qsTr("Whisper Model Path")
                subtitle: qsTr("Path to whisper model binary file (e.g. ggml-base.en.bin).")
                accent: root.modeAccent
                compact: root.compact

                SentinelTextField {
                    anchors.fill: parent
                    placeholderText: "/path/to/model.bin"
                    text: root.viewModel.whisperModelPath
                    onEditingFinished: root.viewModel.whisperModelPath = text
                }
            }
        }
    }
}
