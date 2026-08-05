// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
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

    readonly property string currentProvider: root.viewModel.selectedRuntimeProvider
    readonly property var modelList: {
        if (root.currentProvider === "ollama") return root.viewModel.ollamaModelNames
        if (root.currentProvider === "lm-studio") return root.viewModel.loadedLMStudioModelNames
        return []
    }

    function hasLocalModels() {
        return root.currentProvider === "ollama" || root.currentProvider === "lm-studio" || root.currentProvider === "llama-cpp-server"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        SectionTitle {
            title: qsTr("AI Model")
            subtitle: qsTr("Select your default AI model to get started.")
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Model")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            SentinelComboBox {
                id: modelCombo
                accent: root.brandAccent
                Layout.fillWidth: true
                implicitHeight: 38
                enabled: root.hasLocalModels() && root.modelList.length > 0
                model: root.modelList
                currentIndex: root.modelList.indexOf(root.viewModel.selectedLocalModel)
                displayText: currentIndex >= 0 ? currentText : root.viewModel.selectedLocalModel
                onActivated: (index) => {
                    if (index >= 0 && index < root.modelList.length) {
                        root.viewModel.selectedLocalModel = root.modelList[index]
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.hasLocalModels() && root.modelList.length === 0
            text: qsTr("No local models found. Download one from the Models page after setup.")
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }

        InfoRow {
            compact: false
            label: qsTr("Active Model")
            value: root.viewModel.activeLocalModelName ? root.viewModel.activeLocalModelName : qsTr("None selected")
            Layout.fillWidth: true
        }

        InfoRow {
            compact: false
            label: qsTr("Model Status")
            value: root.viewModel.selectedLocalModelSummary ? root.viewModel.selectedLocalModelSummary : qsTr("No model selected")
            Layout.fillWidth: true
        }

        OnboardingToggle {
            Layout.fillWidth: true
            accent: root.brandAccent
            label: qsTr("Enable Local Chat Inference")
            caption: qsTr("Allows local runtimes to generate chat responses.")
            checked: root.viewModel.localChatInferenceEnabled
            onToggled: (on) => root.viewModel.localChatInferenceEnabled = on
        }

        OnboardingToggle {
            Layout.fillWidth: true
            accent: root.brandAccent
            label: qsTr("Enable Token Streaming")
            caption: qsTr("Streams tokens as they are generated for a faster feel.")
            checked: root.viewModel.localInferenceStreamingEnabled
            onToggled: (on) => root.viewModel.localInferenceStreamingEnabled = on
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                text: qsTr("Temperature (%1)").arg(root.viewModel.localInferenceTemperature.toFixed(2))
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
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

        Label {
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceSm
            text: qsTr("Higher temperature produces more creative output; lower is more focused.")
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }
    }
}
