// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: root
    required property var viewModel
    readonly property var modes: ["local", "cloud", "hybrid"]

    ColumnLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        SectionTitle {
            title: qsTr("Processing preference")
            subtitle: qsTr("Start locally or configure a cloud provider later. No account or API key is required to continue.")
            Layout.fillWidth: true
        }

        SentinelComboBox {
            Layout.fillWidth: true
            model: [qsTr("Local"), qsTr("Cloud"), qsTr("Hybrid")]
            currentIndex: Math.max(0, root.modes.indexOf(root.viewModel.onboardingProcessingMode))
            onActivated: (index) => root.viewModel.onboardingProcessingMode = root.modes[index]
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: root.viewModel.onboardingProcessingMode === "local"
                  ? qsTr("Local mode uses configured local runtimes. If none is ready, you can finish setup and configure one later.")
                  : qsTr("Cloud requests use the provider you choose and require its credentials. Permissions still apply to tools and files.")
            color: SentinelTheme.textMuted
        }

        Item { Layout.fillHeight: true }
    }
}
