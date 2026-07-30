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
    readonly property bool isCloud: {
        var p = root.viewModel.selectedRuntimeProvider
        return p === "cloud-api" || p === "openai" || p === "claude" || p === "gemini" || p === "deepseek" || p === "groq" || p === "mistral"
    }

    height: implicitHeight
    implicitHeight: visible ? localAiSection.implicitHeight : 0

    function sectionHeight(content) {
        return content.implicitHeight + panelPadding * 2
    }

    Column {
        width: parent.width
        spacing: 0

        Item {
            id: localAiSection
            width: parent.width
            height: implicitHeight
            implicitHeight: root.sectionHeight(localAiContent)

            ColumnLayout {
                id: localAiContent
                x: root.panelPadding
                y: root.panelPadding
                width: parent.width - root.panelPadding * 2
                spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: qsTr("AI Settings & Runtimes")
                    subtitle: qsTr("Configure and inspect local AI inference runtimes and providers.")
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceMd

                    Label {
                        Layout.preferredWidth: root.compact ? 88 : 132
                        Layout.alignment: Qt.AlignVCenter
                        text: qsTr("Provider")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    ComboBox {
                        id: runtimeProviderCombo
                        Layout.fillWidth: true
                        implicitHeight: 36
                        hoverEnabled: true
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
            }
        }
    }
}
