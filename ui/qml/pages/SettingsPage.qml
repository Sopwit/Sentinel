import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: settingsPage
    required property var viewModel

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.spaceXl
        spacing: SentinelTheme.spaceMd

        SectionTitle {
            title: "Settings Foundation"
            subtitle: "JSON-backed local settings for desktop shell preferences. Runtime tests still use InMemorySettingsStore."
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 12

            Label {
                text: "Theme"
                color: SentinelTheme.textMuted
            }

            TextField {
                Layout.fillWidth: true
                text: settingsPage.viewModel.themeName
                color: SentinelTheme.textPrimary
                onEditingFinished: settingsPage.viewModel.setThemeName(text)
            }

            Label {
                text: "Config Profile"
                color: SentinelTheme.textMuted
            }

            TextField {
                Layout.fillWidth: true
                text: settingsPage.viewModel.configurationProfile
                color: SentinelTheme.textPrimary
                onEditingFinished: settingsPage.viewModel.setConfigurationProfile(text)
            }
        }

        SectionTitle {
            title: "Local Data Maintenance"
            subtitle: "Settings are stored separately and are not deleted by memory/chat clear actions."
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 8

            Label {
                text: "Memory Store"
                color: SentinelTheme.textMuted
            }

            Label {
                text: settingsPage.viewModel.memoryStatus + " (" + settingsPage.viewModel.memoryMaintenanceStatus + ")"
                color: SentinelTheme.textPrimary
            }

            Label {
                text: "Chat History"
                color: SentinelTheme.textMuted
            }

            Label {
                text: settingsPage.viewModel.chatHistoryStatus + " (" + settingsPage.viewModel.chatMaintenanceStatus + ")"
                color: SentinelTheme.textPrimary
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            Button {
                text: "Clear Local Memory"
                enabled: settingsPage.viewModel.memoryStatus === "Available"
                onClicked: clearMemoryDialog.open()
            }

            Button {
                text: "Clear Chat History"
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
