import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: settingsPage
    required property var viewModel
    readonly property bool compact: width < 720

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.pageMargin(settingsPage.width)
        spacing: SentinelTheme.contentSpacing(settingsPage.width)

        SectionTitle {
            title: "Settings Foundation"
            subtitle: "JSON-backed local settings for desktop shell preferences. Runtime tests still use InMemorySettingsStore."
        }

        GridLayout {
            Layout.fillWidth: true
            columns: settingsPage.compact ? 1 : 2
            columnSpacing: SentinelTheme.spaceSm
            rowSpacing: SentinelTheme.spaceSm

            Label {
                text: "Theme"
                color: SentinelTheme.textMuted
            }

            SentinelTextField {
                id: themeField
                Layout.fillWidth: true
                text: settingsPage.viewModel.themeName
                onEditingFinished: settingsPage.viewModel.setThemeName(text)
            }

            Label {
                text: "Config Profile"
                color: SentinelTheme.textMuted
            }

            SentinelTextField {
                id: profileField
                Layout.fillWidth: true
                text: settingsPage.viewModel.configurationProfile
                onEditingFinished: settingsPage.viewModel.setConfigurationProfile(text)
            }
        }

        SectionTitle {
            title: "Local Data Maintenance"
            subtitle: "Settings are stored separately and are not deleted by memory/chat clear actions."
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            InfoRow {
                compact: settingsPage.compact
                label: "Memory Store"
                value: settingsPage.viewModel.memoryStatus + " (" + settingsPage.viewModel.memoryMaintenanceStatus + ")"
            }

            InfoRow {
                compact: settingsPage.compact
                label: "Chat History"
                value: settingsPage.viewModel.chatHistoryStatus + " (" + settingsPage.viewModel.chatMaintenanceStatus + ")"
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: settingsPage.compact ? 1 : 2
            columnSpacing: SentinelTheme.spaceSm
            rowSpacing: SentinelTheme.spaceSm

            SentinelButton {
                text: "Clear Local Memory"
                enabled: settingsPage.viewModel.memoryStatus === "Available"
                Layout.fillWidth: settingsPage.compact
                onClicked: clearMemoryDialog.open()
            }

            SentinelButton {
                text: "Clear Chat History"
                Layout.fillWidth: settingsPage.compact
                onClicked: clearChatDialog.open()
            }
        }

        Label {
            Layout.fillWidth: true
            text: "Future settings should remain local-first and pass through AppSettings rather than being implemented in QML."
            color: SentinelTheme.textMuted
            wrapMode: Text.WordWrap
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Dialog {
        id: clearMemoryDialog
        title: "Clear local memory?"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent

        Label {
            text: "This clears local memory entries only. Settings are kept."
            color: SentinelTheme.textPrimary
            wrapMode: Text.WordWrap
            width: 320
        }

        onAccepted: settingsPage.viewModel.clearMemory()
    }

    Dialog {
        id: clearChatDialog
        title: "Clear chat history?"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent

        Label {
            text: "This clears local chat history. Settings are kept."
            color: SentinelTheme.textPrimary
            wrapMode: Text.WordWrap
            width: 320
        }

        onAccepted: settingsPage.viewModel.clearChat()
    }
}
