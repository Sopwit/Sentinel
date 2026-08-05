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

    ColumnLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        SectionTitle {
            title: qsTr("Capabilities")
            subtitle: qsTr("Overview of local tools, document RAG, and agent capabilities.")
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 76
                radius: SentinelTheme.radiusLg
                color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.spaceMd
                    spacing: 2

                    Label {
                        text: root.viewModel.agentCapabilityEnabledCount
                        color: root.brandAccent
                        font.pixelSize: 22
                        font.bold: true
                    }

                    Label {
                        text: qsTr("Enabled capabilities")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 76
                radius: SentinelTheme.radiusLg
                color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.spaceMd
                    spacing: 2

                    Label {
                        text: root.viewModel.providerCatalogCount
                        color: root.brandAccent
                        font.pixelSize: 22
                        font.bold: true
                    }

                    Label {
                        text: qsTr("Provider catalog entries")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                    }
                }
            }
        }

        InfoRow {
            compact: false
            label: qsTr("Registry")
            value: root.viewModel.agentCapabilityRegistrySummary ? root.viewModel.agentCapabilityRegistrySummary : root.viewModel.agentCapabilityRegistryStatus
            Layout.fillWidth: true
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            clip: true
            contentHeight: capabilityList.implicitHeight

            ColumnLayout {
                id: capabilityList
                width: parent.width
                spacing: SentinelTheme.spaceSm

                Repeater {
                    model: root.viewModel.agentCapabilitySummaries

                    delegate: Rectangle {
                        required property string modelData
                        Layout.fillWidth: true
                        implicitHeight: 44
                        radius: SentinelTheme.radiusMd
                        color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.30)

                        Label {
                            anchors.fill: parent
                            anchors.margins: SentinelTheme.spaceMd
                            text: modelData
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontSmall
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
