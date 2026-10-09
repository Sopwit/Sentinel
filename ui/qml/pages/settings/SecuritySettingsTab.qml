// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: root
    required property var viewModel
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property int panelPadding: SentinelTheme.spaceLg
    property string activeTaskId: ""
    property var displayedPermissionGrants: []
    property bool confirmClearPermissions: false
    onVisibleChanged: if (visible) displayedPermissionGrants = viewModel.persistentPermissionGrants
    readonly property bool taskAwaitingApproval: root.viewModel.controlledTaskActiveSummary.indexOf("[Waiting Approval]") >= 0
    readonly property bool taskRunning: root.viewModel.controlledTaskActiveSummary.indexOf("[Running]") >= 0 || root.taskAwaitingApproval

    property var retentionSettings: []
    property string retentionStatus: ""
    property string pendingPreferenceId: ""
    property var pendingPreferenceValue: null
    property double pendingPreferenceDeadline: 0
    function refreshRetention() {
        const rows = root.viewModel.productSettings().filter(function(row) { return row.id.indexOf("privacy.retention.") === 0 })
        if (JSON.stringify(rows) !== JSON.stringify(root.retentionSettings)) root.retentionSettings = rows
        if (root.pendingPreferenceId.length > 0) {
            const confirmed = rows.some(function(row) { return row.id === root.pendingPreferenceId && row.value === root.pendingPreferenceValue })
            if (confirmed || Date.now() > root.pendingPreferenceDeadline) {
                root.retentionStatus = confirmed ? qsTr("Preference saved.") : qsTr("The change was not confirmed. The currently stored value is shown; check the connection and try again.")
                root.pendingPreferenceId = ""
            }
        }
    }
    function retentionTitle(id) {
        const titles = { "chat": qsTr("Chat history"), "agentRuns": qsTr("Agent runs"), "diagnostics": qsTr("Diagnostics"), "modelSourceCache": qsTr("Model catalog cache"), "modelOperations": qsTr("Model operation history") }
        return titles[id.replace("privacy.retention.", "")] || id
    }
    function retentionLabel(value) {
        return value === "Keep" ? qsTr("Keep until deleted") : qsTr("%1 days").arg(value.replace("d", ""))
    }
    Timer { interval: 1500; repeat: true; running: root.visible; triggeredOnStart: true; onTriggered: root.refreshRetention() }

    height: implicitHeight
    implicitHeight: visible ? mainLayout.implicitHeight + panelPadding * 2 : 0

    function createControlledPlan() {
        const goal = controlledGoal.text.trim()
        if (goal.length === 0)
            return
        root.activeTaskId = root.viewModel.planControlledAgentTask(goal)
    }

    ColumnLayout {
        id: mainLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: root.panelPadding
        spacing: SentinelTheme.spaceMd

        SectionTitle {
            title: qsTr("Data retention")
            subtitle: qsTr("Choose how long local records are kept. Expired records may be permanently removed during maintenance; model weight files are not deleted by these policies.")
            Layout.fillWidth: true
        }
        SettingCard {
            Label {
                visible: root.retentionSettings.length === 0
                Layout.fillWidth: true
                Layout.margins: SentinelTheme.spaceMd
                text: root.viewModel.daemonConnected ? qsTr("Loading retention policies…") : qsTr("Connect to the daemon to load retention policies.")
                color: SentinelTheme.textMuted
                wrapMode: Text.WordWrap
            }
            Repeater {
                model: root.retentionSettings
                SettingControlRow {
                    id: retentionRow
                    required property var modelData
                    title: root.retentionTitle(modelData.id)
                    compact: root.compact
                    subtitle: modelData.enabled ? "" : qsTr("This policy is unavailable in the current storage configuration.")
                    SentinelComboBox {
                        anchors.fill: parent
                        enabled: retentionRow.modelData.enabled
                        model: retentionRow.modelData.allowedValues
                        currentIndex: model.indexOf(retentionRow.modelData.value)
                        delegateTextResolver: (value) => root.retentionLabel(value)
                        displayText: root.retentionLabel(retentionRow.modelData.value)
                        onActivated: (index) => {
                            const result = root.viewModel.setProductSetting(retentionRow.modelData.id, model[index])
                            if (result.code === "Pending") {
                                root.pendingPreferenceId = retentionRow.modelData.id
                                root.pendingPreferenceValue = model[index]
                                root.pendingPreferenceDeadline = Date.now() + 10000
                            }
                            root.retentionStatus = result.accepted ? qsTr("Retention policy saved.") : result.code === "Pending" ? qsTr("Waiting for the daemon to confirm the change…") : qsTr("Could not save the retention policy. Check the daemon connection and try again.")
                            root.refreshRetention()
                        }
                    }
                }
            }
            Label {
                visible: root.retentionStatus.length > 0
                text: root.retentionStatus
                Layout.fillWidth: true
                Layout.margins: SentinelTheme.spaceMd
                color: SentinelTheme.textMuted
                wrapMode: Text.WordWrap
            }
        }

        // ------------------------------------------------------------------
        // 1. Permissions and safety
        // ------------------------------------------------------------------

        SectionTitle {
            title: qsTr("Permissions & Safety")
            subtitle: qsTr("Set the safety rules that govern every tool and agent action.")
            Layout.fillWidth: true
        }

        SettingCard {
            title: qsTr("Execution Safety")
            subtitle: qsTr("Start with the default policy, then decide how much freedom approved agents get.")

            SettingControlRow {
                title: qsTr("Default Policy")
                subtitle: qsTr("How much permission tools and agents are granted by default.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    id: permissionStateCombo
                    accent: root.modeAccent
                    anchors.fill: parent
                    model: root.viewModel.permissionPolicyStateLabels
                    currentIndex: root.viewModel.permissionPolicyStateLabels.indexOf(root.viewModel.defaultPermissionPolicyState)
                    displayText: currentIndex >= 0 ? currentText : root.viewModel.defaultPermissionPolicyState
                    onActivated: (index) => {
                        if (index >= 0 && index < root.viewModel.permissionPolicyStateLabels.length)
                            root.viewModel.defaultPermissionPolicyState = root.viewModel.permissionPolicyStateLabels[index]
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm

                InfoRow {
                    compact: root.compact
                    label: qsTr("Tool Gateway")
                    value: root.viewModel.toolGatewayStatus || qsTr("Inactive")
                    Layout.fillWidth: true
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
            }

            SettingToggleRow {
                title: qsTr("Agent Autonomous Mode")
                subtitle: qsTr("Let approved plans continue without asking at every step.")
                checked: root.viewModel.agentAutonomousMode
                accent: root.modeAccent
                compact: root.compact
                onToggled: (checked) => root.viewModel.agentAutonomousMode = checked
            }
        }

        SettingCard {
            title: qsTr("Persistent Permissions")
            subtitle: qsTr("Approvals that remain after restarting Sentinel.")

            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: SentinelTheme.spaceMd
                spacing: SentinelTheme.spaceSm

                Label {
                    Layout.fillWidth: true
                    visible: root.displayedPermissionGrants.length === 0
                    text: qsTr("No persistent permissions")
                    color: SentinelTheme.textMuted
                }

                Repeater {
                    model: root.displayedPermissionGrants
                    delegate: RowLayout {
        id: delegateScope1
                        required property var modelData
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: delegateScope1.modelData.scope || qsTr("General access")
                            elide: Text.ElideMiddle
                            color: SentinelTheme.textPrimary
                        }
                        Label {
                            text: delegateScope1.modelData.createdAt
                            color: SentinelTheme.textMuted
                        }
                        SentinelButton {
                            text: qsTr("Revoke")
                            accent: root.modeAccent
                            onClicked: {
                                if (root.viewModel.revokePersistentPermission(delegateScope1.modelData.id))
                                    root.displayedPermissionGrants = root.viewModel.persistentPermissionGrants
                            }
                        }
                    }
                }

                RowLayout {
                    visible: root.displayedPermissionGrants.length > 0
                    Layout.fillWidth: true
                    SentinelButton {
                        text: root.confirmClearPermissions ? qsTr("Confirm Clear") : qsTr("Clear All")
                        accent: root.modeAccent
                        onClicked: {
                            if (!root.confirmClearPermissions) {
                                root.confirmClearPermissions = true
                                return
                            }
                            if (root.viewModel.clearPersistentPermissions())
                                root.displayedPermissionGrants = root.viewModel.persistentPermissionGrants
                            root.confirmClearPermissions = false
                        }
                    }
                    SentinelButton {
                        visible: root.confirmClearPermissions
                        text: qsTr("Cancel")
                        accent: root.modeAccent
                        onClicked: root.confirmClearPermissions = false
                    }
                }
            }
        }

        // ------------------------------------------------------------------
        // 2. Controlled agent tasks
        // ------------------------------------------------------------------

        SectionTitle {
            title: qsTr("Controlled Agent Tasks")
            subtitle: qsTr("Three steps: describe the task, review its plan, then approve and run it.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            title: qsTr("1. Describe the Task")
            subtitle: qsTr("The plan is generated from your description.")

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceSm

                    SentinelTextField {
                        id: controlledGoal
                        Layout.fillWidth: true
                        placeholderText: qsTr("Example: summarize the selected workspace files")
                        onAccepted: root.createControlledPlan()
                    }

                    SentinelButton {
                        text: qsTr("Create Plan")
                        accent: root.modeAccent
                        enabled: controlledGoal.text.trim().length > 0
                        onClicked: root.createControlledPlan()
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.activeTaskId.length === 0
                    text: qsTr("The generated plan appears in the next two steps once created.")
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                }
            }
        }

        SettingCard {
            visible: root.activeTaskId.length > 0
            title: qsTr("2. Review the Plan")
            subtitle: qsTr("Check what the agent will do before approving it.")

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceXs

                InfoRow {
                    compact: root.compact
                    label: qsTr("Status")
                    value: root.viewModel.controlledTaskActiveSummary
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Current Step")
                    value: root.viewModel.controlledTaskCurrentStep
                    visible: root.viewModel.controlledTaskCurrentStep.indexOf("No visible step") !== 0
                    Layout.fillWidth: true
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Progress")
                    value: root.viewModel.controlledTaskProgressSummary
                    visible: root.viewModel.controlledTaskProgressSummary.indexOf("No running") !== 0
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: SentinelTheme.spaceXs
                    Layout.preferredHeight: 1
                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Plan Steps")
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                }

                Repeater {
                    model: root.viewModel.controlledTaskPlanSteps

                    Label {
                        required property string modelData
                        Layout.fillWidth: true
                        Layout.leftMargin: SentinelTheme.spaceSm
                        text: modelData
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        SettingCard {
            visible: root.activeTaskId.length > 0
            title: qsTr("3. Approve & Run")
            subtitle: qsTr("Approve the task, start its agent session, then review any tool approval requests.")

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceSm

                    SentinelButton {
                        visible: !root.taskRunning
                        text: qsTr("Approve")
                        accent: SentinelTheme.success
                        onClicked: root.viewModel.approveControlledAgentTask(root.activeTaskId, "approve")
                    }

                    SentinelButton {
                        visible: !root.taskRunning
                        text: qsTr("Deny")
                        accent: SentinelTheme.warning
                        onClicked: root.viewModel.denyControlledAgentTask(root.activeTaskId)
                    }

                    SentinelButton {
                        visible: !root.taskRunning
                        text: qsTr("Start")
                        accent: root.modeAccent
                        onClicked: root.viewModel.startControlledAgentTask(root.activeTaskId)
                    }

                    SentinelButton {
                        visible: root.taskRunning
                        Layout.fillWidth: true
                        text: root.taskAwaitingApproval ? qsTr("Approve Tool & Continue") :
                              root.viewModel.controlledTaskSessionActive ? qsTr("Agent Running") : qsTr("Run Agent Task")
                        enabled: !root.viewModel.controlledTaskSessionActive || root.taskAwaitingApproval
                        accent: root.modeAccent
                        onClicked: root.viewModel.executeControlledAgentStep(root.activeTaskId)
                    }

                    SentinelButton {
                        visible: root.taskRunning
                        text: qsTr("Cancel")
                        accent: SentinelTheme.warning
                        onClicked: root.viewModel.cancelControlledAgentTask(root.activeTaskId)
                    }
                }

            }
        }

        // ------------------------------------------------------------------
        // 3. Connections
        // ------------------------------------------------------------------

        SectionTitle {
            title: qsTr("Network & Proxy")
            subtitle: qsTr("Route remote endpoints and model APIs through an HTTP/SOCKS proxy.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            SettingToggleRow {
                title: qsTr("Use Network Proxy")
                subtitle: qsTr("Route cloud model API and remote web requests through a proxy server.")
                checked: root.viewModel.proxyEnabled
                accent: root.modeAccent
                compact: root.compact
                showDivider: root.viewModel.proxyEnabled
                onToggled: (checked) => root.viewModel.proxyEnabled = checked
            }

            SettingControlRow {
                visible: root.viewModel.proxyEnabled
                title: qsTr("Proxy Type")
                subtitle: qsTr("Protocol used by the proxy gateway.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    accent: root.modeAccent
                    anchors.fill: parent
                    model: ["HTTP", "SOCKS5"]
                    currentIndex: root.viewModel.proxyType === "SOCKS5" ? 1 : 0
                    onActivated: (index) => root.viewModel.proxyType = (index === 1 ? "SOCKS5" : "HTTP")
                }
            }

            SettingControlRow {
                visible: root.viewModel.proxyEnabled
                title: qsTr("Host & Port")
                subtitle: qsTr("Hostname/IP address and port number.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                RowLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceSm

                    SentinelTextField {
                        Layout.fillWidth: true
                        placeholderText: "127.0.0.1"
                        text: root.viewModel.proxyHost
                        onEditingFinished: root.viewModel.proxyHost = text
                    }

                    SentinelTextField {
                        Layout.preferredWidth: 80
                        placeholderText: "8080"
                        text: root.viewModel.proxyPort ? root.viewModel.proxyPort.toString() : ""
                        onEditingFinished: root.viewModel.proxyPort = parseInt(text) || 0
                    }
                }
            }

            SettingControlRow {
                visible: root.viewModel.proxyEnabled
                title: qsTr("Proxy Username")
                subtitle: qsTr("Optional proxy authentication username.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    anchors.fill: parent
                    placeholderText: "user"
                    text: root.viewModel.proxyUser
                    onEditingFinished: root.viewModel.proxyUser = text
                }
            }

            SettingControlRow {
                visible: root.viewModel.proxyEnabled
                title: qsTr("Proxy Password")
                subtitle: qsTr("Optional proxy authentication password.")
                accent: root.modeAccent
                compact: root.compact

                SentinelTextField {
                    anchors.fill: parent
                    echoMode: TextInput.Password
                    placeholderText: root.viewModel.proxyPasswordConfigured ? qsTr("Configured") : qsTr("Not configured")
                    onEditingFinished: {
                        if (text.length > 0) {
                            root.viewModel.proxyPassword = text
                            text = ""
                        }
                    }
                }
            }
        }

        SectionTitle {
            title: qsTr("Web Search")
            subtitle: qsTr("Configure the provider used by the approved web-search tool.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            SettingControlRow {
                title: qsTr("Search Provider")
                subtitle: qsTr("Search provider for agent web searches. DuckDuckGo needs no API key; Exa and Parallel require one.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelComboBox {
                    anchors.fill: parent
                    accent: root.modeAccent
                    model: [qsTr("DuckDuckGo (no API key)"), "Exa", "Parallel"]
                    currentIndex: root.viewModel.webSearchProvider === "exa" ? 1
                                  : (root.viewModel.webSearchProvider === "parallel" ? 2 : 0)
                    onActivated: (index) => {
                        root.viewModel.webSearchProvider = index === 1 ? "exa"
                                      : (index === 2 ? "parallel" : "duckduckgo")
                    }
                }
            }

            SettingControlRow {
                title: qsTr("Search API Key")
                subtitle: qsTr("Credential sent only to the selected search provider. Leave empty to use the keyless DuckDuckGo fallback.")
                accent: root.modeAccent
                compact: root.compact
                showDivider: true

                SentinelTextField {
                    anchors.fill: parent
                    echoMode: TextInput.Password
                    text: root.viewModel.webSearchApiKey
                    onEditingFinished: root.viewModel.webSearchApiKey = text
                }
            }

            SettingControlRow {
                title: qsTr("Maximum Results")
                subtitle: qsTr("Limit the number of search results supplied to an approved request.")
                accent: root.modeAccent
                compact: root.compact

                SentinelSpinBox {
                    anchors.fill: parent
                    from: 1
                    to: 20
                    value: root.viewModel.webSearchMaxResults
                    onValueModified: root.viewModel.webSearchMaxResults = value
                }
            }
        }
    }
}
