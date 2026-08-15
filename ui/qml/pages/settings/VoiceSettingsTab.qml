// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
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
    property string autoDetectStatus: ""
    property string pendingPathTarget: ""
    readonly property int panelPadding: SentinelTheme.spaceLg

    function fileUrlToPath(fileUrl) {
        var value = fileUrl.toString()
        if (value.indexOf("file:///") === 0) {
            return decodeURIComponent(Qt.platform.os === "windows" ? value.substring(8) : value.substring(7))
        }
        if (value.indexOf("file://") === 0) {
            return decodeURIComponent(value.substring(7))
        }
        return decodeURIComponent(value)
    }

    function openPathDialog(target, title, filters) {
        pendingPathTarget = target
        voicePathDialog.title = title
        voicePathDialog.nameFilters = filters
        voicePathDialog.open()
    }

    function applySelectedPath(path) {
        if (pendingPathTarget === "kokoroModel")
            viewModel.kokoroModelPath = path
        else if (pendingPathTarget === "piperBinary")
            viewModel.piperBinaryPath = path
        else if (pendingPathTarget === "piperModel")
            viewModel.piperModelPath = path
        else if (pendingPathTarget === "whisperBinary")
            viewModel.whisperBinaryPath = path
        else if (pendingPathTarget === "whisperModel")
            viewModel.whisperModelPath = path
        pendingPathTarget = ""
    }

    function localizedAutoDetectStatus(result) {
        if (!result.detected)
            return qsTr("No local Piper, Whisper, or Kokoro binaries or model files were detected automatically.")

        var paths = result.paths
        var lines = [qsTr("Auto-detected voice paths:")]
        if (paths.piperBinary)
            lines.push(qsTr("Piper binary: %1").arg(paths.piperBinary))
        if (paths.piperModel)
            lines.push(qsTr("Piper model: %1").arg(paths.piperModel))
        if (paths.kokoroModel)
            lines.push(qsTr("Kokoro model: %1").arg(paths.kokoroModel))
        if (paths.whisperBinary)
            lines.push(qsTr("Whisper binary: %1").arg(paths.whisperBinary))
        if (paths.whisperModel)
            lines.push(qsTr("Whisper model: %1").arg(paths.whisperModel))
        return lines.join("\n- ")
    }

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
            title: qsTr("Voice & Audio Settings")
            subtitle: qsTr("Configure local speech-to-text (Whisper) and text-to-speech (Piper, Kokoro) runtimes and paths.")
            Layout.fillWidth: true
        }

        SettingCard {
            title: qsTr("Auto Detection")

            SettingControlRow {
                title: qsTr("Auto-Detect Voice Engines")
                subtitle: root.autoDetectStatus.length > 0 ? root.autoDetectStatus : qsTr("Automatically scan system PATH and model directories for Piper, Whisper, and Kokoro.")
                accent: root.modeAccent
                compact: root.compact
                controlWidth: root.compact ? 160 : 200

                SentinelButton {
                    accent: root.modeAccent
                    anchors.fill: parent
                    text: qsTr("Auto Detect")
                    onClicked: {
                        root.autoDetectStatus = root.localizedAutoDetectStatus(root.viewModel.autoDetectVoicePathStatus())
                    }
                }
            }
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
                controlWidth: root.compact ? 260 : 420
                showDivider: true

                RowLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceXs

                    SentinelTextField {
                        Layout.fillWidth: true
                        placeholderText: "/path/to/kokoro.onnx"
                        text: root.viewModel.kokoroModelPath
                        onEditingFinished: root.viewModel.kokoroModelPath = text
                    }

                    SentinelButton {
                        Layout.preferredWidth: 92
                        accent: root.modeAccent
                        text: qsTr("Browse")
                        tooltipText: qsTr("Select Kokoro model file")
                        onClicked: root.openPathDialog("kokoroModel", qsTr("Select Kokoro model file"), [qsTr("Model files (*.onnx *.pt *.pth *.bin *.safetensors)"), qsTr("All files (*)")])
                    }
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
                controlWidth: root.compact ? 260 : 420
                showDivider: true

                RowLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceXs

                    SentinelTextField {
                        Layout.fillWidth: true
                        placeholderText: "/path/to/piper"
                        text: root.viewModel.piperBinaryPath
                        onEditingFinished: root.viewModel.piperBinaryPath = text
                    }

                    SentinelButton {
                        Layout.preferredWidth: 92
                        accent: root.modeAccent
                        text: qsTr("Browse")
                        tooltipText: qsTr("Select Piper executable")
                        onClicked: root.openPathDialog("piperBinary", qsTr("Select Piper executable"), [qsTr("Executables (*)"), qsTr("All files (*)")])
                    }
                }
            }

            SettingControlRow {
                visible: root.viewModel.selectedTtsEngine === "Piper"
                title: qsTr("Piper Model Path")
                subtitle: qsTr("Path to Piper voice ONNX model file.")
                accent: root.modeAccent
                compact: root.compact
                controlWidth: root.compact ? 260 : 420
                showDivider: true

                RowLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceXs

                    SentinelTextField {
                        Layout.fillWidth: true
                        placeholderText: "/path/to/voice.onnx"
                        text: root.viewModel.piperModelPath
                        onEditingFinished: root.viewModel.piperModelPath = text
                    }

                    SentinelButton {
                        Layout.preferredWidth: 92
                        accent: root.modeAccent
                        text: qsTr("Browse")
                        tooltipText: qsTr("Select Piper model file")
                        onClicked: root.openPathDialog("piperModel", qsTr("Select Piper model file"), [qsTr("ONNX model files (*.onnx)"), qsTr("All files (*)")])
                    }
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
                controlWidth: root.compact ? 260 : 420
                showDivider: true

                RowLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceXs

                    SentinelTextField {
                        Layout.fillWidth: true
                        placeholderText: "/path/to/whisper"
                        text: root.viewModel.whisperBinaryPath
                        onEditingFinished: root.viewModel.whisperBinaryPath = text
                    }

                    SentinelButton {
                        Layout.preferredWidth: 92
                        accent: root.modeAccent
                        text: qsTr("Browse")
                        tooltipText: qsTr("Select Whisper executable")
                        onClicked: root.openPathDialog("whisperBinary", qsTr("Select Whisper executable"), [qsTr("Executables (*)"), qsTr("All files (*)")])
                    }
                }
            }

            SettingControlRow {
                title: qsTr("Whisper Model Path")
                subtitle: qsTr("Path to whisper model binary file (e.g. ggml-base.en.bin).")
                accent: root.modeAccent
                compact: root.compact
                controlWidth: root.compact ? 260 : 420

                RowLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceXs

                    SentinelTextField {
                        Layout.fillWidth: true
                        placeholderText: "/path/to/model.bin"
                        text: root.viewModel.whisperModelPath
                        onEditingFinished: root.viewModel.whisperModelPath = text
                    }

                    SentinelButton {
                        Layout.preferredWidth: 92
                        accent: root.modeAccent
                        text: qsTr("Browse")
                        tooltipText: qsTr("Select Whisper model file")
                        onClicked: root.openPathDialog("whisperModel", qsTr("Select Whisper model file"), [qsTr("Whisper model files (*.bin *.gguf)"), qsTr("All files (*)")])
                    }
                }
            }

            SettingToggleRow {
                title: qsTr("Enable Whisper Transcription")
                subtitle: qsTr("Allow the configured local Whisper process to transcribe selected audio files.")
                checked: root.viewModel.whisperTranscriptionExecutionEnabled
                accent: root.modeAccent
                compact: root.compact
                onToggled: (checked) => root.viewModel.whisperTranscriptionExecutionEnabled = checked
            }
        }
    }

    FileDialog {
        id: voicePathDialog
        fileMode: FileDialog.OpenFile
        onAccepted: {
            if (selectedFile)
                root.applySelectedPath(root.fileUrlToPath(selectedFile))
        }
    }
}
