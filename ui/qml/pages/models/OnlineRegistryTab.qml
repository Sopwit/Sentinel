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
    property var modelCatalog: []
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
                text: qsTr("Online Model Library & Registry")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontSection
                font.bold: true
                Layout.fillWidth: true
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
                    model: root.modelCatalog

                    delegate: ShellPanel {
                        id: card
                        required property var modelData
                        width: root.compact ? parent.width : (parent.width - SentinelTheme.spaceMd) / 2
                        implicitHeight: 140
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
                                    text: card.modelData.name
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontCard
                                    font.bold: true
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }

                                StatusChip {
                                    label: card.modelData.badge || card.modelData.category
                                    accent: root.modeAccent
                                }
                            }

                            Label {
                                text: card.modelData.description || ""
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceSm

                                SentinelButton {
                                    text: qsTr("Details")
                                    onClicked: root.openModelDetail(card.modelData)
                                }

                                SentinelButton {
                                    text: qsTr("Pull Model")
                                    visible: card.modelData.downloadable === true
                                    accent: root.modeAccent
                                    onClicked: root.viewModel.pullOllamaModel(card.modelData.ollamaId)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
