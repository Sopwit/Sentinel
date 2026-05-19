import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ScrollView {
    id: memoryPage
    required property var viewModel
    readonly property bool compact: width < 720
    readonly property int panelPadding: SentinelTheme.spaceLg
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    contentWidth: availableWidth

    Item {
        width: memoryPage.availableWidth
        implicitHeight: memoryColumn.implicitHeight

        Column {
            id: memoryColumn
            width: parent.width
            spacing: SentinelTheme.contentSpacing(memoryPage.width)

            SectionTitle {
                width: parent.width
                title: "Runtime Memory"
                subtitle: "Key-value memory, chat history status, and future semantic memory stay separate."
            }

            ShellPanel {
                width: parent.width
                implicitHeight: memoryStatusColumn.implicitHeight + memoryPage.panelPadding * 2

                ColumnLayout {
                    id: memoryStatusColumn
                    x: memoryPage.panelPadding
                    y: memoryPage.panelPadding
                    width: parent.width - memoryPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Key-value Store"
                        value: memoryPage.viewModel.memoryStatus + " (" + memoryPage.viewModel.memoryMaintenanceStatus + ")"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Chat History"
                        value: memoryPage.viewModel.chatHistoryStatus + " (" + memoryPage.viewModel.chatMaintenanceStatus + ")"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Semantic Memory"
                        value: "Future placeholder only. No embeddings, vector search, or autonomous recall."
                        Layout.fillWidth: true
                    }
                }
            }

            ShellPanel {
                width: parent.width
                implicitHeight: candidateColumn.implicitHeight + memoryPage.panelPadding * 2

                ColumnLayout {
                    id: candidateColumn
                    x: memoryPage.panelPadding
                    y: memoryPage.panelPadding
                    width: parent.width - memoryPage.panelPadding * 2
                    spacing: SentinelTheme.spaceSm

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Memory Candidates"
                        value: memoryPage.viewModel.memoryCandidateCount + " total / "
                               + memoryPage.viewModel.pendingMemoryCandidateCount + " pending / "
                               + memoryPage.viewModel.approvedMemoryCandidateCount + " approved / "
                               + memoryPage.viewModel.rejectedMemoryCandidateCount + " rejected / "
                               + memoryPage.viewModel.archivedMemoryCandidateCount + " archived / "
                               + memoryPage.viewModel.committedMemoryCandidateCount + " committed"
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Last Review"
                        value: memoryPage.viewModel.lastMemoryCandidateReviewStatus + " - "
                               + memoryPage.viewModel.lastMemoryCandidateReviewSummary
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "Approved means reviewed metadata, not committed long-term memory."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Commit Readiness"
                        value: memoryPage.viewModel.memoryCommitReadinessStatus + " - "
                               + memoryPage.viewModel.memoryCommitReadinessSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Commit Target"
                        value: memoryPage.viewModel.memoryCommitPlanCount + " planned / "
                               + memoryPage.viewModel.memoryCommitTargetSummary
                        Layout.fillWidth: true
                    }

                    InfoRow {
                        compact: memoryPage.compact
                        label: "Last Commit"
                        value: memoryPage.viewModel.lastMemoryCommitStatus + " - "
                               + memoryPage.viewModel.lastMemoryCommitResultSummary
                        Layout.fillWidth: true
                    }

                    Repeater {
                        model: memoryPage.viewModel.memoryCommitReadinessChecks

                        Label {
                            required property string modelData
                            text: modelData
                            color: SentinelTheme.textMuted
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }

                    Repeater {
                        model: memoryPage.viewModel.memoryCandidateIds

                        ColumnLayout {
                            required property int index
                            required property string modelData
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceXs

                            readonly property string candidateId: modelData
                            readonly property string candidateState: memoryPage.viewModel.memoryCandidateReviewStates[index]
                            readonly property string candidateCommitStatus: memoryPage.viewModel.memoryCandidateCommitStatuses[index]
                            readonly property string candidateSummary: memoryPage.viewModel.memoryCandidateSummaries[index]
                            readonly property string commitSummary: memoryPage.viewModel.memoryCommitCandidateSummaries[index]

                            InfoRow {
                                compact: memoryPage.compact
                                label: "Candidate"
                                value: candidateSummary
                                Layout.fillWidth: true
                            }

                            InfoRow {
                                compact: memoryPage.compact
                                label: "Commit Plan"
                                value: commitSummary
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                visible: candidateState !== "Archived"
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceXs

                                SentinelButton {
                                    text: "Approve"
                                    enabled: candidateState === "Pending Review"
                                    Layout.preferredWidth: 96
                                    onClicked: memoryPage.viewModel.approveMemoryCandidate(candidateId)
                                }

                                SentinelButton {
                                    text: "Reject"
                                    enabled: candidateState === "Pending Review"
                                    Layout.preferredWidth: 96
                                    onClicked: memoryPage.viewModel.rejectMemoryCandidate(candidateId)
                                }

                                SentinelButton {
                                    text: "Reset"
                                    enabled: (candidateState === "Approved" || candidateState === "Rejected")
                                             && candidateCommitStatus !== "Committed"
                                    Layout.preferredWidth: 96
                                    onClicked: memoryPage.viewModel.resetMemoryCandidate(candidateId)
                                }

                                SentinelButton {
                                    visible: candidateState === "Approved"
                                    text: "Commit"
                                    enabled: candidateCommitStatus !== "Committed"
                                    Layout.preferredWidth: 96
                                    onClicked: memoryPage.viewModel.requestMemoryCandidateCommit(candidateId)
                                }

                                Item {
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }

                    Label {
                        visible: memoryPage.viewModel.memoryCandidateCount === 0
                        text: "No memory candidates pending review."
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            GridLayout {
                width: parent.width
                columns: memoryPage.compact ? 1 : 3
                columnSpacing: SentinelTheme.spaceSm
                rowSpacing: SentinelTheme.spaceSm

                SentinelTextField {
                    id: memoryKey
                    Layout.fillWidth: true
                    Layout.preferredWidth: memoryPage.compact ? -1 : 220
                    placeholderText: "key"
                }

                SentinelTextField {
                    id: memoryValue
                    Layout.fillWidth: true
                    placeholderText: "value"
                }

                SentinelButton {
                    text: "Store"
                    enabled: memoryKey.text.trim().length > 0
                    Layout.fillWidth: memoryPage.compact
                    onClicked: {
                        memoryPage.viewModel.remember(memoryKey.text, memoryValue.text)
                        memoryKey.clear()
                        memoryValue.clear()
                    }
                }
            }

            ListView {
                id: memoryList
                width: parent.width
                height: Math.min(420, Math.max(220, memoryPage.height - 360))
                clip: true
                spacing: SentinelTheme.spaceSm
                model: memoryPage.viewModel.memoryEntries

                delegate: Rectangle {
                    id: memoryDelegate
                    required property string modelData

                    width: ListView.view.width
                    radius: SentinelTheme.radiusSm
                    color: SentinelTheme.surfaceMuted
                    border.color: SentinelTheme.accentBorderSubtle
                    implicitHeight: memoryText.implicitHeight + 18

                    Text {
                        id: memoryText
                        x: SentinelTheme.spaceSm
                        y: SentinelTheme.spaceSm
                        width: parent.width - SentinelTheme.spaceSm * 2
                        text: memoryDelegate.modelData
                        color: SentinelTheme.textPrimary
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Label {
                width: parent.width
                visible: memoryList.count === 0
                text: "No key-value memory entries stored."
                color: SentinelTheme.textMuted
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
