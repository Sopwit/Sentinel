// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Dialogs
import Sentinel.Desktop

Item {
    id: root
    required property var viewModel
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property int panelPadding: SentinelTheme.spaceLg

    height: implicitHeight
    implicitHeight: visible ? mainLayout.implicitHeight + panelPadding * 2 : 0

    ColumnLayout {
        id: mainLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: root.panelPadding
        spacing: SentinelTheme.spaceMd

        SectionTitle {
            title: qsTr("Brain & Memory")
            subtitle: qsTr("Memory, recall, context, summaries, and continuity remain local and explicit.")
            Layout.fillWidth: true
        }

        SettingCard {
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceSm

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

        SectionTitle {
            title: qsTr("Workspace & Knowledge")
            subtitle: qsTr("Workspace scope for chat context, Brain summaries, attachments, and optional local knowledge.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            SettingToggleRow {
                title: qsTr("Local Knowledge Base")
                subtitle: qsTr("Enable local RAG vector indexing and semantic retrieval for workspace documents.")
                checked: root.viewModel.localKnowledgeBaseEnabled
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.localKnowledgeBaseEnabled = checked
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                spacing: SentinelTheme.spaceSm

                SentinelButton {
                    text: qsTr("Add Document")
                    accent: root.modeAccent
                    enabled: root.viewModel.localKnowledgeBaseEnabled
                    onClicked: knowledgeFileDialog.open()
                }
                SentinelButton {
                    text: qsTr("Reindex")
                    accent: root.modeAccent
                    enabled: root.viewModel.localKnowledgeBaseEnabled
                    onClicked: root.viewModel.reindexKnowledgeBase()
                }
                SentinelButton {
                    text: qsTr("Clear Index")
                    accent: SentinelTheme.warning
                    enabled: root.viewModel.localKnowledgeBaseEnabled
                    onClicked: root.viewModel.clearKnowledgeBase()
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                spacing: SentinelTheme.spaceXs

                InfoRow {
                    compact: root.compact
                    label: qsTr("Index")
                    value: root.viewModel.localKnowledgeBaseStatus
                    Layout.fillWidth: true
                }
                SettingControlRow {
                    title: qsTr("Semantic Provider")
                    subtitle: qsTr("Use the selected local Ollama embedding model for semantic indexing.")
                    accent: root.modeAccent
                    compact: root.compact
                    showDivider: true
                    SentinelComboBox {
                        anchors.fill: parent
                        accent: root.modeAccent
                        model: ["Ollama", "Disabled"]
                        currentIndex: root.viewModel.semanticProvider === "ollama" ? 0 : 1
                        onActivated: (index) => root.viewModel.semanticProvider = index === 0 ? "ollama" : "disabled"
                    }
                }
                SettingControlRow {
                    title: qsTr("Embedding Model")
                    subtitle: qsTr("Model name installed in the local Ollama runtime.")
                    accent: root.modeAccent
                    compact: root.compact
                    SentinelTextField {
                        anchors.fill: parent
                        text: root.viewModel.semanticEmbeddingModel
                        onEditingFinished: root.viewModel.semanticEmbeddingModel = text
                    }
                }
                InfoRow {
                    compact: root.compact
                    label: qsTr("Semantic Provider")
                    value: root.viewModel.selectedSemanticProviderName + " / " + root.viewModel.semanticProviderReadiness
                    Layout.fillWidth: true
                }
                InfoRow {
                    compact: root.compact
                    label: qsTr("Retrieval")
                    value: root.viewModel.semanticRetrievalStatus + " / " + root.viewModel.vectorIndexedItemCount
                    Layout.fillWidth: true
                }
                Label {
                    Layout.fillWidth: true
                    text: root.viewModel.knowledgeBaseDocumentSummaries.join("\n")
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                    visible: text.length > 0
                }
            }

            SettingControlRow {
                title: qsTr("Export Format")
                subtitle: qsTr("Default file format when saving conversation transcripts.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    id: exportFormatCombo
                    accent: root.modeAccent
                    anchors.fill: parent
                    model: ["Markdown", "JSON", "TXT", "DOCX", "PDF"]
                    currentIndex: {
                        var fmt = root.viewModel.exportDefaultFormat
                        var idx = model.indexOf(fmt)
                        return idx >= 0 ? idx : 0
                    }
                    onActivated: (index) => root.viewModel.exportDefaultFormat = model[index]
                }
            }

            SettingControlRow {
                title: qsTr("Attachment Behavior")
                subtitle: qsTr("Handling rule when dragging or pasting new attachments into chat.")
                accent: root.modeAccent
                compact: root.compact

                SentinelComboBox {
                    id: attachmentBehaviorCombo
                    accent: root.modeAccent
                    anchors.fill: parent
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
        }

        SectionTitle {
            title: qsTr("Export Preferences")
            subtitle: qsTr("Detailed options for generated transcript files.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            SettingToggleRow {
                title: qsTr("Include Timestamps in Export")
                subtitle: qsTr("Include UTC message timestamps in exported Markdown/JSON transcripts.")
                checked: root.viewModel.exportIncludeTimestamps
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.exportIncludeTimestamps = checked
            }

            SettingToggleRow {
                title: qsTr("Include Citations in Export")
                subtitle: qsTr("Append source document citations and RAG references to exports.")
                checked: root.viewModel.exportIncludeCitations
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.exportIncludeCitations = checked
            }

            SettingToggleRow {
                title: qsTr("Anonymize Names in Export")
                subtitle: qsTr("Strip sensitive user names and identity tags from transcript exports.")
                checked: root.viewModel.exportAnonymizeNames
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.exportAnonymizeNames = checked
            }

            SettingToggleRow {
                title: qsTr("Include Model Metadata in Export")
                subtitle: qsTr("Attach provider name, model version, and token usage to export headers.")
                checked: root.viewModel.exportIncludeModelMetadata
                accent: root.modeAccent
                compact: root.compact
                onToggled: (checked) => root.viewModel.exportIncludeModelMetadata = checked
            }
        }

        SectionTitle {
            title: qsTr("Profiles")
            subtitle: qsTr("Name your configuration and choose the active skill profile for agent behavior.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            SettingControlRow {
                title: qsTr("Configuration Profile")
                subtitle: qsTr("Name for the active desktop environment profile.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    anchors.fill: parent
                    placeholderText: "Desktop Alpha"
                    text: root.viewModel.configurationProfile
                    onEditingFinished: root.viewModel.configurationProfile = text
                }
            }

            SettingControlRow {
                title: qsTr("Skill Profile")
                subtitle: qsTr("Active agent capabilities and prompt role profile.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    accent: root.modeAccent
                    anchors.fill: parent
                    model: root.viewModel.skillProfileNames
                    currentIndex: root.viewModel.skillProfileNames.indexOf(root.viewModel.selectedSkillProfileName)
                    displayText: currentIndex >= 0 ? currentText : root.viewModel.selectedSkillProfileName
                    onActivated: (index) => {
                        var ids = root.viewModel.skillProfileIds
                        if (index >= 0 && index < ids.length)
                            root.viewModel.selectedSkillProfile = ids[index]
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm

                InfoRow {
                    compact: root.compact
                    label: qsTr("Skill Profile Readiness")
                    value: root.viewModel.selectedSkillProfileReadiness
                    Layout.fillWidth: true
                }
            }
        }
    }

    FileDialog {
        id: knowledgeFileDialog
        title: qsTr("Select a knowledge-base document")
        fileMode: FileDialog.OpenFile
        onAccepted: {
            if (selectedFile)
                root.viewModel.addKnowledgeBaseDocument(selectedFile.toString().replace("file://", ""))
        }
    }
}
