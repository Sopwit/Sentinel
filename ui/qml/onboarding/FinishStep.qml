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
    signal configureModelsRequested
    readonly property string runtimeState: viewModel.activeRuntimeReadinessState.toLowerCase()
    readonly property bool modelRunnable: viewModel.daemonConnected && (runtimeState === "ready" || runtimeState === "busy")
    readonly property bool inferenceReady: viewModel.daemonConnected && runtimeState === "ready"
    property color brandAccent: SentinelTheme.modeAccent(viewModel.currentModeName)

    ScrollView {
        id: finishScroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ColumnLayout {
            width: finishScroll.availableWidth
            height: Math.max(implicitHeight, finishScroll.availableHeight)
            spacing: SentinelTheme.spaceLg

            SectionTitle {
                title: qsTr("Preferences saved")
                subtitle: qsTr("Click Finish to open Sentinel. Inference readiness is checked separately.")
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(76, savedConfigurationRow.implicitHeight + SentinelTheme.spaceLg * 2)
                radius: SentinelTheme.radiusLg
                color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                RowLayout {
                    id: savedConfigurationRow
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.spaceLg
                    spacing: SentinelTheme.spaceMd

                    Image {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: 44
                        source: "qrc:/icons/dev.sentinel.Sentinel.png"
                        fillMode: Image.PreserveAspectFit
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: qsTr("Your configuration has been saved locally.")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontBody
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: qsTr("You can change anything anytime from Settings.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall
                        }
                    }
                }
            }

            Label {
                Layout.topMargin: SentinelTheme.spaceMd
                text: qsTr("Summary")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                font.bold: true
            }

            InfoRow {
                compact: false
                label: qsTr("Processing")
                value: root.viewModel.onboardingProcessingMode === "local" ? qsTr("Local") : root.viewModel.onboardingProcessingMode === "cloud" ? qsTr("Cloud") : qsTr("Hybrid")
                Layout.fillWidth: true
            }

            InfoRow {
                compact: false
                label: qsTr("Provider")
                value: root.viewModel.onboardingSnapshot.providerId || qsTr("Set up later")
                Layout.fillWidth: true
            }

            InfoRow {
                compact: false
                label: qsTr("Model")
                value: root.viewModel.onboardingSnapshot.modelId || qsTr("Set up later")
                Layout.fillWidth: true
            }

            InfoRow {
                compact: false
                label: qsTr("Language")
                value: root.viewModel.languageDisplayName(root.viewModel.appLanguage)
                Layout.fillWidth: true
            }

            InfoRow {
                compact: false
                label: qsTr("Theme")
                value: SentinelTheme.localizedThemeName(root.viewModel.themeName) || qsTr("Default")
                Layout.fillWidth: true
            }

            InfoRow {
                Layout.fillWidth: true
                compact: false
                label: qsTr("Daemon")
                value: root.viewModel.daemonConnected ? qsTr("Connected") : qsTr("Disconnected")
            }
            InfoRow {
                Layout.fillWidth: true
                compact: false
                label: qsTr("Provider available")
                value: root.modelRunnable ? qsTr("Confirmed by daemon") : qsTr("Not confirmed")
            }
            InfoRow {
                Layout.fillWidth: true
                compact: false
                label: qsTr("Model runnable")
                value: root.modelRunnable ? qsTr("Confirmed by daemon") : qsTr("Not confirmed")
            }
            SentinelButton {
                visible: !root.inferenceReady
                text: qsTr("Finish and configure models")
                onClicked: root.configureModelsRequested()
            }
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 0
            }

            Label {
                Layout.fillWidth: true
                objectName: "onboardingReadinessSummary"
                text: root.inferenceReady ? qsTr("The daemon confirms the selected provider and model are ready for inference.") : qsTr("Preferences are saved. Inference is not ready yet. Connect the daemon and configure a runnable model in Settings → Models & Providers.")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontBody
                wrapMode: Text.WordWrap
            }
        }
    }
}
