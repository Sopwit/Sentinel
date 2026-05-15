import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: memoryPage
    required property var viewModel

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.spaceXl
        spacing: SentinelTheme.spaceMd

        SectionTitle {
            title: "Runtime Memory"
            subtitle: "Local key-value memory with dedicated persistence. Settings and chat history remain separate."
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            TextField {
                id: memoryKey
                Layout.preferredWidth: 220
                placeholderText: "key"
                color: SentinelTheme.textPrimary
                placeholderTextColor: SentinelTheme.textPlaceholder
            }

            TextField {
                id: memoryValue
                Layout.fillWidth: true
                placeholderText: "value"
                color: SentinelTheme.textPrimary
                placeholderTextColor: SentinelTheme.textPlaceholder
            }

            Button {
                text: "Store"
                enabled: memoryKey.text.trim().length > 0
                onClicked: {
                    memoryPage.viewModel.remember(memoryKey.text, memoryValue.text)
                    memoryKey.clear()
                    memoryValue.clear()
                }
            }
        }

        ListView {
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
    }
}
