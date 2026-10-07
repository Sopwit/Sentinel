// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: root
    required property var viewModel
    property var speech: viewModel.speechSettingsState()

    Connections {
        target: root.viewModel
        function onVoiceConfigurationChanged() { root.speech = root.viewModel.speechSettingsState() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        SectionTitle {
            title: qsTr("Optional speech setup")
            subtitle: qsTr("Speech is optional. Only configured runtime and device information appears here.")
            Layout.fillWidth: true
        }

        InfoRow {
            Layout.fillWidth: true
            label: qsTr("Speech recognition")
            value: root.speech.sttAvailable
                   ? root.speech.sttProviderId + " / " + root.speech.sttModelId
                   : qsTr("No ready STT runtime")
        }

        InfoRow {
            Layout.fillWidth: true
            label: qsTr("Speech synthesis")
            value: root.speech.ttsAvailable
                   ? root.speech.ttsProviderId + " / " + root.speech.ttsModelId
                   : qsTr("No ready TTS runtime")
        }

        RowLayout {
            Layout.fillWidth: true
            visible: !!root.speech.inputDeviceIds && root.speech.inputDeviceIds.length > 0
            Label {
                text: qsTr("Microphone")
                color: SentinelTheme.textMuted
            }
            SentinelComboBox {
                Layout.fillWidth: true
                model: root.speech.inputDeviceIds || []
                currentIndex: Math.max(0, model.indexOf(root.speech.selectedInputId))
                onActivated: (index) => {
                    root.viewModel.setProductSetting("speech.input-device", model[index])
                    root.speech = root.viewModel.speechSettingsState()
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("You can finish setup now and configure speech later in Settings.")
            color: SentinelTheme.textMuted
            wrapMode: Text.WordWrap
        }

        Item { Layout.fillHeight: true }
    }
}
