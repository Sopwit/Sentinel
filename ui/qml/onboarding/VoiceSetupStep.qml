// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: root
    required property var viewModel
    property color brandAccent: SentinelTheme.modeAccent(viewModel.currentModeName)

    ColumnLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        SectionTitle {
            title: qsTr("Voice Setup")
            subtitle: qsTr("Configure text-to-speech and speech-to-text engines for voice interaction.")
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("TTS Engine")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            SentinelComboBox {
                id: ttsCombo
                accent: root.brandAccent
                Layout.fillWidth: true
                implicitHeight: 38
                model: [qsTr("Piper"), qsTr("Kokoro")]
                currentIndex: root.viewModel.selectedTtsEngine === "Kokoro" ? 1 : 0
                onActivated: (index) => root.viewModel.selectedTtsEngine = index === 1 ? "Kokoro" : "Piper"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Piper Binary")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "/usr/bin/piper"
                text: root.viewModel.piperBinaryPath
                onEditingFinished: root.viewModel.piperBinaryPath = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Piper Model")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "/path/to/model.onnx"
                text: root.viewModel.piperModelPath
                onEditingFinished: root.viewModel.piperModelPath = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Whisper Binary")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "/usr/bin/whisper-cli"
                text: root.viewModel.whisperBinaryPath
                onEditingFinished: root.viewModel.whisperBinaryPath = text
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Whisper Model")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "/path/to/ggml-model.bin"
                text: root.viewModel.whisperModelPath
                onEditingFinished: root.viewModel.whisperModelPath = text
            }
        }

        InfoRow {
            compact: false
            label: qsTr("Text to Speech")
            value: root.viewModel.textToSpeechSummary ? root.viewModel.textToSpeechSummary : qsTr("Not configured")
            Layout.fillWidth: true
        }

        InfoRow {
            compact: false
            label: qsTr("Speech to Text")
            value: root.viewModel.speechToTextSummary ? root.viewModel.speechToTextSummary : qsTr("Not configured")
            Layout.fillWidth: true
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceSm
            text: qsTr("Voice is optional. You can adjust engines and paths later in Settings.")
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }
    }
}
