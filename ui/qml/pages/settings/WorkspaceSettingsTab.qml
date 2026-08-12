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
                        id: exportFormatCombo
                        accent: root.modeAccent
                        Layout.fillWidth: true
                        model: ["Markdown", "JSON", "TXT", "DOCX", "PDF"]
                        currentIndex: {
                            var fmt = root.viewModel.exportDefaultFormat
                            var idx = model.indexOf(fmt)
                            return idx >= 0 ? idx : 0
                        }
                        onActivated: (index) => root.viewModel.exportDefaultFormat = model[index]
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Attachment Behavior")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontBody
                        Layout.preferredWidth: 150
                    }
                    SentinelComboBox {
                        id: attachmentBehaviorCombo
                        accent: root.modeAccent
                        Layout.fillWidth: true
                        model: [qsTr("Manual Attachments Only"), qsTr("Replace Existing Attachment"), qsTr("Paste Attachment Enabled")]
                        currentIndex: {
                            var behavior = root.viewModel.attachmentBehavior
                            if (behavior === "Replace Existing Attachment") return 1
                            if (behavior === "Paste Attachment Enabled") return 2
                            return 0
                        }
                        onActivated: (index) => {
                            var behaviors = ["Manual Attachments Only", "Replace Existing Attachment", "Paste Attachment Enabled"]
                            root.viewModel.attachmentBehavior = behaviors[index]
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

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Anonymize Names in Export")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontBody
                        Layout.fillWidth: true
                    }
                    Switch {
                        checked: root.viewModel.exportAnonymizeNames
                        onCheckedChanged: root.viewModel.exportAnonymizeNames = checked
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Include Model Metadata in Export")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontBody
                        Layout.fillWidth: true
                    }
                    Switch {
                        checked: root.viewModel.exportIncludeModelMetadata
                        onCheckedChanged: root.viewModel.exportIncludeModelMetadata = checked
                    }
                }
                SectionTitle {
                    title: qsTr("Profiles")
                    subtitle: qsTr("Name your configuration and choose the active skill profile for agent behavior.")
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Configuration Profile")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontBody
                        Layout.preferredWidth: 150
                    }
                    SentinelTextField {
                        Layout.fillWidth: true
                        placeholderText: "Desktop Alpha"
                        text: root.viewModel.configurationProfile
                        onEditingFinished: root.viewModel.configurationProfile = text
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Skill Profile")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontBody
                        Layout.preferredWidth: 150
                    }
                    SentinelComboBox {
                        accent: root.modeAccent
                        Layout.fillWidth: true
                        model: root.viewModel.skillProfileNames
                        currentIndex: root.viewModel.skillProfileNames.indexOf(root.viewModel.selectedSkillProfileName)
                        displayText: currentIndex >= 0 ? currentText : root.viewModel.selectedSkillProfileName
                        onActivated: (index) => {
                            if (index >= 0 && index < root.viewModel.skillProfileNames.length)
                                root.viewModel.selectedSkillProfile = root.viewModel.skillProfileNames[index]
                        }
                    }
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Skill Profile Readiness")
                    value: root.viewModel.selectedSkillProfileReadiness
                    Layout.fillWidth: true
                }
            }
        }
    }
}
