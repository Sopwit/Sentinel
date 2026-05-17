import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ScrollView {
    id: memoryPage
    required property var viewModel
    readonly property bool compact: width < 720
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    ColumnLayout {
        width: memoryPage.availableWidth
        spacing: SentinelTheme.contentSpacing(memoryPage.width)

        SectionTitle {
            title: "Runtime Memory"
            subtitle: "Key-value memory, chat history status, and future semantic memory stay separate."
        }

        ShellPanel {
            Layout.fillWidth: true
            implicitHeight: memoryStatusColumn.implicitHeight + SentinelTheme.space2Xl

            ColumnLayout {
                id: memoryStatusColumn
                anchors.fill: parent
                anchors.margins: SentinelTheme.spaceLg
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

        GridLayout {
            Layout.fillWidth: true
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
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(220, memoryPage.height - 360)
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
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.spaceSm
                    text: memoryDelegate.modelData
                    color: SentinelTheme.textPrimary
                    wrapMode: Text.WordWrap
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: memoryList.count === 0
            text: "No key-value memory entries stored."
            color: SentinelTheme.textMuted
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
