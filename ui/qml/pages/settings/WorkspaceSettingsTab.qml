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
    readonly property int panelPadding: SentinelTheme.spaceLg

    height: implicitHeight
    implicitHeight: visible ? brainSection.implicitHeight + workspaceSection.implicitHeight : 0

    function sectionHeight(content) {
        return content.implicitHeight + panelPadding * 2
    }

    Column {
        width: parent.width
        spacing: 0

        Item {
            id: brainSection
            width: parent.width
            height: implicitHeight
            implicitHeight: root.sectionHeight(privacyContent)

            ColumnLayout {
                id: privacyContent
                x: root.panelPadding
                y: root.panelPadding
                width: parent.width - root.panelPadding * 2
                spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: qsTr("Brain & Memory")
                    subtitle: qsTr("Memory, recall, context, summaries, and continuity remain local and explicit.")
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Memory")
                    value: root.viewModel.memoryStatus + " (" + root.viewModel.memoryMaintenanceStatus + ")"
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Continuity")
                    value: root.viewModel.summaryContinuityStatus + " / " + root.viewModel.summaryContinuityContributionSummary
                    Layout.fillWidth: true
                    valueMaximumLineCount: 3
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Chat History")
                    value: root.viewModel.conversationHistorySummaryText + " / " + root.viewModel.chatMaintenanceStatus
                    Layout.fillWidth: true
                }
            }
        }

        Item {
            id: workspaceSection
            width: parent.width
            height: implicitHeight
            implicitHeight: root.sectionHeight(workspaceContent)

            ColumnLayout {
                id: workspaceContent
                x: root.panelPadding
                y: root.panelPadding
                width: parent.width - root.panelPadding * 2
                spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: qsTr("Workspace & Knowledge")
                    subtitle: qsTr("Workspace scope for chat context, Brain summaries, attachments, and optional local knowledge.")
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: SentinelTheme.radiusMd
                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.030)
                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.055)
                    implicitHeight: ragToggleRow.implicitHeight + SentinelTheme.spaceMd

                    RowLayout {
                        id: ragToggleRow
                        x: SentinelTheme.spaceSm
                        y: SentinelTheme.spaceXs
                        width: parent.width - SentinelTheme.spaceSm * 2
                        spacing: SentinelTheme.spaceSm

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Local Knowledge Base")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                            }
                        }

                        Switch {
                            checked: root.viewModel.localKnowledgeBaseEnabled
                            hoverEnabled: true
                            onToggled: root.viewModel.localKnowledgeBaseEnabled = checked
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Export Format")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontBody
                        Layout.preferredWidth: 150
                    }
                    SentinelComboBox {
                        accent: root.modeAccent
                        Layout.fillWidth: true
                        model: [qsTr("Markdown"), qsTr("JSON"), qsTr("Plain Text")]
                        currentIndex: {
                            var fmt = root.viewModel.exportDefaultFormat
                            if (fmt === "JSON") return 1
                            if (fmt === "Plain Text") return 2
                            return 0
                        }
                        onActivated: (index) => {
                            var fmts = ["Markdown", "JSON", "Plain Text"]
                            root.viewModel.exportDefaultFormat = fmts[index]
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Include Timestamps in Export")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontBody
                        Layout.fillWidth: true
                    }
                    Switch {
                        checked: root.viewModel.exportIncludeTimestamps
                        onCheckedChanged: root.viewModel.exportIncludeTimestamps = checked
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Include Citations in Export")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontBody
                        Layout.fillWidth: true
                    }
                    Switch {
                        checked: root.viewModel.exportIncludeCitations
                        onCheckedChanged: root.viewModel.exportIncludeCitations = checked
                    }
                }
            }
        }
    }
}
