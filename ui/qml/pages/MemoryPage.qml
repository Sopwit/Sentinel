import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ShellPanel {
    id: memoryPage
    required property var viewModel
    readonly property bool compact: width < 720

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.pageMargin(memoryPage.width)
        spacing: SentinelTheme.contentSpacing(memoryPage.width)

        SectionTitle {
            title: "Runtime Memory"
            subtitle: "Key-value memory remains separate from the metadata-only taxonomy below."
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            InfoRow {
                compact: memoryPage.compact
                label: "Taxonomy Categories"
                value: memoryPage.viewModel.memoryCatalogCount.toString()
            }

            InfoRow {
                compact: memoryPage.compact
                label: "Planner Affinity"
                value: memoryPage.viewModel.currentMemoryAffinitySummary
            }

            Repeater {
                model: memoryPage.viewModel.memoryCatalogSummaries

                InfoRow {
                    compact: memoryPage.compact
                    label: "Memory"
                    value: modelData
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
            Layout.fillHeight: true
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
