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
    property string profileDraft: ""
    property string savedProfile: ""
    property string profileStatus: ""
    property string pendingProfile: ""
    property bool profileSavePending: false
    property double profileDeadline: 0
    Component.onCompleted: { savedProfile = viewModel.responseProfileInstructions; profileDraft = savedProfile }
    Timer {
        interval: 1000; repeat: true; running: root.visible
        onTriggered: {
            const stored = root.viewModel.responseProfileInstructions
            if (root.profileSavePending && stored === root.pendingProfile) {
                root.savedProfile = stored
                root.profileStatus = qsTr("Response profile saved. It applies to new interactive Chat and Agent requests.")
                root.profileSavePending = false
            } else if (root.profileSavePending && Date.now() > root.profileDeadline) {
                root.profileStatus = qsTr("The change was not confirmed. Check the daemon connection and retry.")
                root.profileSavePending = false
            }
        }
    }
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

        SettingCard {
            title: qsTr("Response profile")
            subtitle: qsTr("Visible instructions for answer style and working approach. Model selection and permissions remain separate. Empty instructions disable this profile. Scheduled tasks do not inherit it.")
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: SentinelTheme.spaceMd
                SentinelComboBox {
                    Layout.fillWidth: true
                    model: [qsTr("Start from a template…"), qsTr("Developer"), qsTr("Learning"), qsTr("Research"), qsTr("Planning")]
                    onActivated: function(index) {
                        const templates = ["", qsTr("Explain implementation choices clearly. Prefer small, maintainable changes and describe relevant verification."), qsTr("Explain concepts step by step with concrete examples. Adapt depth to my questions and make assumptions explicit."), qsTr("Separate verified facts from inference. Compare evidence and identify uncertainty. Never invent sources."), qsTr("Turn goals into practical steps. State dependencies and tradeoffs clearly. Ask before committing to external actions.")]
                        if (index > 0) root.profileDraft = templates[index]
                    }
                }
                TextArea {
                    id: responseProfileEditor
                    objectName: "responseProfileEditor"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 140
                    text: root.profileDraft
                    onTextChanged: root.profileDraft = text
                    wrapMode: TextEdit.Wrap
                    color: SentinelTheme.textPrimary
                    placeholderText: qsTr("Describe how you want Sentinel to respond…")
                    Accessible.name: qsTr("Response profile instructions")
                    background: Rectangle { color: SentinelTheme.surface; radius: SentinelTheme.radiusSm; border.color: responseProfileEditor.activeFocus ? root.modeAccent : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.3) }
                }
                Label { text: qsTr("%1 / 2000 characters").arg(root.profileDraft.length); color: SentinelTheme.textMuted }
                Button {
                    text: root.profileSavePending ? qsTr("Saving…") : qsTr("Save response profile")
                    enabled: !root.profileSavePending && root.profileDraft.length <= 2000 && root.profileDraft.trim() !== root.viewModel.responseProfileInstructions && root.viewModel.daemonConnected
                    onClicked: {
                        root.pendingProfile = root.profileDraft.trim()
                        root.profileSavePending = true
                        root.profileDeadline = Date.now() + 10000
                        root.profileStatus = qsTr("Waiting for the daemon to confirm…")
                        root.viewModel.responseProfileInstructions = root.pendingProfile
                    }
                }
                Label { Layout.fillWidth: true; text: root.profileStatus; visible: text.length > 0; wrapMode: Text.Wrap; color: SentinelTheme.textMuted }
            }
        }

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
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceSm

                SettingControlRow {
                    title: qsTr("Workspace")
                    subtitle: qsTr("Agent context and tool authorization are scoped to the active workspace.")
                    accent: root.modeAccent
                    compact: root.compact
                    showDivider: true

                    SentinelComboBox {
                        anchors.fill: parent
                        accent: root.modeAccent
                        model: root.viewModel.workspaceNames
                        currentIndex: root.viewModel.workspaceIds.indexOf(root.viewModel.selectedWorkspaceId)
                        onActivated: function(index) {
                            if (index >= 0 && index < root.viewModel.workspaceIds.length)
                                root.viewModel.selectedWorkspaceId = root.viewModel.workspaceIds[index]
                        }
                    }
                }

                SettingControlRow {
                    title: qsTr("Workspace name")
                    subtitle: qsTr("Rename the selected workspace without changing its folder or permissions.")
                    compact: root.compact
                    SentinelTextField {
                        anchors.fill: parent
                        text: root.viewModel.selectedWorkspaceName
                        Accessible.name: qsTr("Workspace name")
                        onEditingFinished: {
                            if (text.trim().length > 0)
                                root.viewModel.renameWorkspace(root.viewModel.selectedWorkspaceId, text.trim())
                            text = Qt.binding(function() { return root.viewModel.selectedWorkspaceName })
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    SentinelTextField { id: newWorkspaceName; Layout.fillWidth: true; placeholderText: qsTr("New workspace name"); Accessible.name: placeholderText }
                    SentinelButton {
                        text: qsTr("Create")
                        enabled: newWorkspaceName.text.trim().length > 0
                        onClicked: {
                            var id = root.viewModel.createWorkspace(newWorkspaceName.text.trim(), "Personal")
                            if (id.length > 0) { root.viewModel.selectedWorkspaceId = id; newWorkspaceName.clear() }
                        }
                    }
                }
                SentinelButton {
                    text: qsTr("Duplicate workspace")
                    onClicked: {
                        var id = root.viewModel.duplicateWorkspace(root.viewModel.selectedWorkspaceId)
                        if (id.length > 0) root.viewModel.selectedWorkspaceId = id
                    }
                }
                InfoRow {
                    compact: root.compact
                    label: qsTr("Folder")
                    value: root.viewModel.selectedWorkspaceRootSummary
                    Layout.fillWidth: true
                    valueMaximumLineCount: 3
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceSm

                    SentinelButton {
                        text: qsTr("Choose Folder")
                        accent: root.modeAccent
                        onClicked: workspaceFolderDialog.open()
                    }
                    SentinelButton {
                        text: qsTr("Clear Folder")
                        accent: SentinelTheme.warning
                        enabled: root.viewModel.selectedWorkspaceRootPath.length > 0
                        onClicked: root.viewModel.clearWorkspaceRoot()
                    }
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Last Workspace Action")
                    value: root.viewModel.workspaceLastActionStatus + ": " + root.viewModel.workspaceLastActionSummary
                    Layout.fillWidth: true
                    valueMaximumLineCount: 3
                }
            }
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


    }

    FolderDialog {
        id: workspaceFolderDialog
        title: qsTr("Choose Workspace Folder")
        onAccepted: {
            if (selectedFolder)
                root.viewModel.openWorkspaceFolder(selectedFolder.toString().replace("file://", ""))
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
