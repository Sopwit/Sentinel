import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: settingsPage
    required property var viewModel

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        spacing: 14

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
                color: "#82aaa1"
            }

            TextField {
                Layout.fillWidth: true
                text: settingsPage.viewModel.themeName
                color: "#d9fff4"
                onEditingFinished: settingsPage.viewModel.setThemeName(text)
            }

            Label {
                text: "Config Profile"
                color: "#82aaa1"
            }

            TextField {
                Layout.fillWidth: true
                text: settingsPage.viewModel.configurationProfile
                color: "#d9fff4"
                onEditingFinished: settingsPage.viewModel.setConfigurationProfile(text)
            }
        }

        Label {
            Layout.fillWidth: true
            text: "Future settings should remain local-first and pass through AppSettings rather than being implemented in QML."
            color: "#82aaa1"
            wrapMode: Text.WordWrap
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
