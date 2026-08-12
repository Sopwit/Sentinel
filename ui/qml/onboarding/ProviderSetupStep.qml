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

    readonly property var providerIds: root.viewModel.selectableRuntimeProviderIds

    function providerIdAt(index) {
        return index >= 0 && index < root.providerIds.length ? root.providerIds[index] : root.viewModel.selectedRuntimeProvider
    }

    function endpointFor(id) {
        switch (id) {
        case "ollama": return root.viewModel.ollamaEndpoint
        case "lm-studio": return root.viewModel.lmStudioEndpoint
        case "llama-cpp-server": return root.viewModel.llamaCppEndpoint
        case "cloud-api": return root.viewModel.cloudApiEndpoint
        default: return ""
        }
    }

    function setEndpointFor(id, value) {
        switch (id) {
        case "ollama": root.viewModel.ollamaEndpoint = value; break
        case "lm-studio": root.viewModel.lmStudioEndpoint = value; break
        case "llama-cpp-server": root.viewModel.llamaCppEndpoint = value; break
        case "cloud-api": root.viewModel.cloudApiEndpoint = value; break
        }
    }

    function syncOnboardingProvider(id) {
        switch (id) {
        case "ollama": root.viewModel.onboardingAiProvider = "Ollama"; break
        case "lm-studio": root.viewModel.onboardingAiProvider = "LM Studio"; break
        case "llama-cpp-server": root.viewModel.onboardingAiProvider = "llama.cpp server"; break
        case "cloud-api": root.viewModel.onboardingAiProvider = "Cloud API"; break
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        SectionTitle {
            title: qsTr("AI Provider")
            subtitle: qsTr("Connect to local runtimes like Ollama or LM Studio, or a cloud provider API.")
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Provider")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            SentinelComboBox {
                id: providerCombo
                accent: root.brandAccent
                Layout.fillWidth: true
                implicitHeight: 38
                model: root.viewModel.selectableRuntimeProviderLabels
                currentIndex: root.providerIds.indexOf(root.viewModel.selectedRuntimeProvider) >= 0
                              ? root.providerIds.indexOf(root.viewModel.selectedRuntimeProvider)
                              : 0
                displayText: currentIndex >= 0 ? currentText : root.viewModel.selectedRuntimeProvider
                onActivated: (index) => {
                    var id = root.providerIdAt(index)
                    root.viewModel.selectedRuntimeProvider = id
                    root.syncOnboardingProvider(id)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Endpoint")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            SentinelTextField {
                Layout.fillWidth: true
                placeholderText: "http://127.0.0.1:11434"
                text: root.endpointFor(root.providerIdAt(providerCombo.currentIndex))
                enabled: providerCombo.currentIndex >= 0
                onEditingFinished: root.setEndpointFor(root.providerIdAt(providerCombo.currentIndex), text)
            }
        }

        InfoRow {
            compact: false
            label: qsTr("Runtime Status")
            value: root.viewModel.localInferenceRuntimeState + " (" + root.viewModel.localInferenceHealthSummary + ")"
            Layout.fillWidth: true
        }

        InfoRow {
            compact: false
            label: qsTr("Active Provider")
            value: root.viewModel.activeRuntimeProviderLabel ? root.viewModel.activeRuntimeProviderLabel : qsTr("Not configured")
            Layout.fillWidth: true
        }

        InfoRow {
            compact: false
            label: qsTr("Readiness")
            value: root.viewModel.activeRuntimeReadinessSummary ? root.viewModel.activeRuntimeReadinessSummary : qsTr("Checking runtime availability…")
            Layout.fillWidth: true
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceSm
            text: qsTr("You can change the provider or endpoints later in Settings > Model Settings.")
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }
    }
}
