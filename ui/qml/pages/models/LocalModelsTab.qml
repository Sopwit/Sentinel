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
    property string searchQuery: ""
    signal openModelDetail(var modelData)

    implicitHeight: mainLayout.implicitHeight

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: SentinelTheme.spaceMd

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            Label {
                text: qsTr("Installed Runtimes & Models")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontSection
                font.bold: true
                Layout.fillWidth: true
            }

            StatusChip {
                label: root.viewModel.selectedRuntimeProvider
                accent: root.modeAccent
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            contentData: Flow {
                width: parent.width
                spacing: SentinelTheme.spaceMd

                Repeater {
                    model: root.viewModel.installedOllamaModelNames

                    delegate: ShellPanel {
                        id: card
                        required property string modelData
                        width: root.compact ? parent.width : (parent.width - SentinelTheme.spaceMd) / 2
                        implicitHeight: 120
                        radius: SentinelTheme.radiusLg
                        color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.6)
                        border.color: SentinelTheme.withAlpha(root.modeAccent, 0.15)

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: SentinelTheme.spaceMd
                            spacing: SentinelTheme.spaceSm

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceSm

                                Label {
                                    text: card.modelData
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontCard
                                    font.bold: true
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }

                                StatusChip {
                                    label: qsTr("Local")
                                    accent: root.modeAccent
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceSm

                                SentinelButton {
                                    text: qsTr("Select")
                                    accent: root.modeAccent
                                    onClicked: root.viewModel.selectedOllamaModel = card.modelData
                                }

                                SentinelButton {
                                    text: qsTr("Delete")
                                    accent: SentinelTheme.statusError
                                    onClicked: ollamaPuller.removeModel(card.modelData)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
