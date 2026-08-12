// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: root
    required property var viewModel
    property color brandAccent: SentinelTheme.modeAccent(viewModel.currentModeName)

    function buildUseCases() {
        return [
            { id: "Coding", label: qsTr("Coding") },
            { id: "Study", label: qsTr("Study") },
            { id: "Writing", label: qsTr("Writing") },
            { id: "Research", label: qsTr("Research") },
            { id: "Creative", label: qsTr("Creative") },
            { id: "Business", label: qsTr("Business") },
            { id: "Personal Assistant", label: qsTr("Personal Assistant") },
            { id: "General Assistant", label: qsTr("General Assistant") }
        ]
    }

    function buildUpdatePolicies() {
        return [qsTr("Ask Before Checking"), qsTr("Never"), qsTr("Weekly"), qsTr("On Startup")]
    }

    function buildNotificationPolicies() {
        return [qsTr("Disabled"), qsTr("All"), qsTr("Custom"), qsTr("Important Only")]
    }

    property var useCases: buildUseCases()

    property var updatePolicies: buildUpdatePolicies()
    readonly property var updatePolicyIds: ["Ask Before Checking", "Never", "Weekly", "On Startup"]

    property var notificationPolicies: buildNotificationPolicies()
    readonly property var notificationPolicyIds: ["Disabled", "All", "Custom", "Important Only"]

    Connections {
        target: shellViewModel
        function onAppLanguageChanged() {
            root.useCases = root.buildUseCases()
            root.updatePolicies = root.buildUpdatePolicies()
            root.notificationPolicies = root.buildNotificationPolicies()
        }
    }

    function selectUseCase(id) {
        root.viewModel.onboardingUseCase = id
        if (id === "Coding") {
            root.viewModel.selectedSkillProfile = "developer"
        } else if (id === "Study") {
            root.viewModel.selectedSkillProfile = "student"
        } else if (id === "Research") {
            root.viewModel.selectedSkillProfile = "researcher"
        } else if (id === "Personal Assistant") {
            root.viewModel.selectedSkillProfile = "personal-assistant"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        SectionTitle {
            title: qsTr("Preferences")
            subtitle: qsTr("Tell Sentinel how you work and fine-tune your local assistant.")
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("What will you use Sentinel for?")
            color: SentinelTheme.textPrimary
            font.pixelSize: SentinelTheme.fontBody
            font.bold: true
        }

        Flow {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            Repeater {
                model: root.useCases

                delegate: Button {
                    id: useCaseBtn
                    required property var modelData
                    readonly property bool isSelected: root.viewModel.onboardingUseCase === modelData.id

                    implicitWidth: 148
                    implicitHeight: 40
                    hoverEnabled: true
                    focusPolicy: Qt.NoFocus

                    contentItem: Text {
                        text: useCaseBtn.modelData.label
                        color: useCaseBtn.isSelected ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontBody
                        font.bold: useCaseBtn.isSelected
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: SentinelTheme.radiusLg
                        color: useCaseBtn.isSelected
                               ? SentinelTheme.withAlpha(root.brandAccent, 0.14)
                               : useCaseBtn.hovered
                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                 : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
                        border.color: useCaseBtn.isSelected
                                      ? SentinelTheme.withAlpha(root.brandAccent, 0.5)
                                      : useCaseBtn.hovered
                                        ? SentinelTheme.withAlpha(root.brandAccent, 0.28)
                                        : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                        Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }
                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                    }

                    onClicked: root.selectUseCase(useCaseBtn.modelData.id)
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Skill Profile")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            SentinelComboBox {
                accent: root.brandAccent
                Layout.fillWidth: true
                implicitHeight: 38
                model: root.viewModel.skillProfileNames
                currentIndex: root.viewModel.skillProfileIds.indexOf(root.viewModel.selectedSkillProfile) >= 0
                              ? root.viewModel.skillProfileIds.indexOf(root.viewModel.selectedSkillProfile)
                              : 0
                onActivated: (index) => {
                    var ids = root.viewModel.skillProfileIds
                    if (index >= 0 && index < ids.length) {
                        root.viewModel.selectedSkillProfile = ids[index]
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Update Policy")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            SentinelComboBox {
                accent: root.brandAccent
                Layout.fillWidth: true
                implicitHeight: 38
                model: root.updatePolicies
                currentIndex: Math.max(0, root.updatePolicyIds.indexOf(root.viewModel.updateCheckPolicy))
                onActivated: (index) => root.viewModel.updateCheckPolicy = root.updatePolicyIds[index]
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Notifications")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            SentinelComboBox {
                accent: root.brandAccent
                Layout.fillWidth: true
                implicitHeight: 38
                model: root.notificationPolicies
                currentIndex: Math.max(0, root.notificationPolicyIds.indexOf(root.viewModel.notificationPolicy))
                onActivated: (index) => root.viewModel.notificationPolicy = root.notificationPolicyIds[index]
            }
        }

        OnboardingToggle {
            Layout.fillWidth: true
            accent: root.brandAccent
            label: qsTr("Local Knowledge Base")
            caption: qsTr("Enables document indexing and local search on your workspace files.")
            checked: root.viewModel.localKnowledgeBaseEnabled
            onToggled: (on) => root.viewModel.localKnowledgeBaseEnabled = on
        }

        OnboardingToggle {
            Layout.fillWidth: true
            accent: root.brandAccent
            label: qsTr("Prompt Context Injection")
            caption: qsTr("Injects relevant local context into prompts automatically.")
            checked: root.viewModel.promptContextInjectionEnabled
            onToggled: (on) => root.viewModel.promptContextInjectionEnabled = on
        }
    }
}
