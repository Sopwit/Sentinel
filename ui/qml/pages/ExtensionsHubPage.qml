// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

FocusScope {
    id: hub
    objectName: "extensionsHubPage"
    required property var viewModel
    property string section: "plugins"
    property string query: ""
    property string selectedId: ""
    property var snapshot: ({phase: "loading", entries: [], tools: [], policy: []})
    property var detail: ({phase: "unavailable"})
    readonly property var sections: [
        {key: "plugins", title: qsTr("Plugins")}, {key: "mcp", title: qsTr("MCP Servers")},
        {key: "skills", title: qsTr("Skills")}, {key: "tools", title: qsTr("Tools")}]
    readonly property var rows: {
        if (snapshot.phase !== "ready") return []
        const needle = query.trim().toLocaleLowerCase()
        if (section === "tools") return (snapshot.tools || []).filter(function(id) {
            return id.toLocaleLowerCase().indexOf(needle) >= 0
        }).map(function(id) { return {id: id, name: id, type: "tools"} })
        return (snapshot.entries || []).filter(function(row) {
            return row.type === hub.section && (row.name + " " + row.id).toLocaleLowerCase().indexOf(needle) >= 0
        })
    }
    signal settingsRequested(string category)
    function reload() {
        if (!visible) return
        snapshot = viewModel.extensionsHubSnapshot()
        detail = selectedId.length > 0 ? viewModel.extensionsHubDetail(selectedId) : {phase: "unavailable"}
    }
    function openSection(key) {
        section = key
        selectedId = ""
        query = ""
        Qt.callLater(function() {
            const index = sections.findIndex(function(item) { return item.key === key })
            if (index >= 0) sectionButtons.itemAt(index).forceActiveFocus(Qt.TabFocusReason)
        })
        reload()
    }
    onVisibleChanged: if (visible) reload()
    Component.onCompleted: reload()
    Connections {
        target: hub.viewModel
        function onRuntimeProjectionChanged() { hub.reload() }
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceMd
        RowLayout {
            Layout.fillWidth: true
            SectionTitle {
                Layout.fillWidth: true
                title: qsTr("Extensions")
                subtitle: qsTr("Plugins, MCP servers, skills and tools from your daemon.")
            }
            SentinelButton {
                objectName: "extensionsRefresh"
                text: qsTr("Refresh")
                enabled: hub.viewModel.daemonConnected && hub.snapshot.phase !== "loading"
                Accessible.name: qsTr("Refresh extension metadata")
                onClicked: { hub.viewModel.refreshExtensionsHub(); hub.reload() }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceXs
            Repeater {
                id: sectionButtons
                model: hub.sections
                delegate: SentinelButton {
                    required property var modelData
                    objectName: "extensionsSection_" + modelData.key
                    Layout.fillWidth: true
                    text: modelData.title
                    checkable: true
                    checked: hub.section === modelData.key
                    highlighted: checked
                    Accessible.name: modelData.title
                    Accessible.description: qsTr("Extensions section")
                    background: Rectangle {
                        radius: SentinelTheme.radiusMd
                        color: parent.checked ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.16) : SentinelTheme.backgroundRaised
                        border.color: parent.activeFocus ? SentinelTheme.accent : SentinelTheme.separator
                        border.width: parent.activeFocus ? 2 : 1
                    }
                    onClicked: hub.openSection(modelData.key)
                }
            }
        }
        Label {
            objectName: "extensionsStatus"
            Layout.fillWidth: true
            text: hub.snapshot.message || qsTr("Loading extension metadata…")
            color: hub.snapshot.phase === "error" ? SentinelTheme.error : SentinelTheme.textMuted
            wrapMode: Text.WordWrap
            Accessible.name: text
        }
        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            visible: hub.snapshot.phase === "loading"
            running: visible && hub.visible
            Accessible.name: qsTr("Loading extension metadata")
        }
        SentinelTextField {
            id: search
            objectName: "extensionsSearch"
            Layout.fillWidth: true
            placeholderText: qsTr("Filter this section by name or ID…")
            Accessible.name: qsTr("Filter extensions")
            enabled: hub.snapshot.phase === "ready"
            text: hub.query
            onTextEdited: hub.query = text
        }
        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true
            ColumnLayout {
                width: scroll.availableWidth
                spacing: SentinelTheme.spaceMd
                EmptyState {
                    objectName: "extensionsEmpty"
                    Layout.fillWidth: true
                    visible: hub.rows.length === 0 && hub.snapshot.phase !== "loading"
                    compact: true
                    icon: hub.section === "tools" ? "tool" : "puzzle"
                    title: hub.snapshot.phase === "disconnected" ? qsTr("Daemon disconnected")
                         : hub.snapshot.phase === "error" ? qsTr("Metadata unavailable")
                         : hub.query.length > 0 ? qsTr("No matching items") : qsTr("No items reported")
                    description: hub.snapshot.phase === "disconnected" ? qsTr("Connect the daemon and refresh to inspect extension state.")
                        : hub.snapshot.phase !== "ready" ? qsTr("Refresh metadata or review the daemon connection details.")
                        : hub.query.length > 0 ? qsTr("Try a different name or ID.")
                        : qsTr("No inventory was reported for this section. Desktop cannot verify whether discovery is supported or whether no items are installed.")
                }
                Label {
                    Layout.fillWidth: true
                    text: hub.section === "tools"
                        ? qsTr("Listed tools are discovered identities. Per-tool descriptions, permission requirements and runtime eligibility are not supplied by this Desktop contract. Execution always goes through the permission gateway.")
                        : hub.section === "mcp"
                        ? qsTr("Configured or enabled does not mean connected. Connection details, capabilities, failure details and supported management actions are not supplied by this Desktop contract.")
                        : hub.section === "skills"
                        ? qsTr("Discovered skill metadata does not establish execution eligibility. Presentation profiles in Settings are separate from discovered skills.")
                        : qsTr("Availability and effective enablement are separate. Plugin descriptions and supported configuration actions are not supplied by this Desktop contract.")
                    color: SentinelTheme.textMuted
                    wrapMode: Text.WordWrap
                }
                Repeater {
                    model: hub.rows
                    delegate: Rectangle {
                        id: rowCard
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: rowContent.implicitHeight + SentinelTheme.spaceMd * 2
                        radius: SentinelTheme.radiusMd
                        color: SentinelTheme.backgroundRaised
                        border.color: SentinelTheme.separator
                        ColumnLayout {
                            id: rowContent
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: SentinelTheme.spaceMd
                            spacing: SentinelTheme.spaceSm
                            Label {
                                Layout.fillWidth: true
                                text: rowCard.modelData.name
                                textFormat: Text.PlainText
                                color: SentinelTheme.textPrimary
                                font.bold: true
                                wrapMode: Text.WrapAnywhere
                                Accessible.name: text
                            }
                            Label {
                                Layout.fillWidth: true
                                text: rowCard.modelData.id
                                textFormat: Text.PlainText
                                visible: rowCard.modelData.name !== rowCard.modelData.id
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WrapAnywhere
                            }
                            Label {
                                Layout.fillWidth: true
                                text: hub.section === "tools" ? qsTr("Discovered · Execution eligibility not reported")
                                    : !rowCard.modelData.enabledKnown ? qsTr("Effective enablement not reported")
                                    : rowCard.modelData.enabled ? qsTr("Effectively enabled")
                                    : qsTr("Effectively disabled")
                                wrapMode: Text.WordWrap
                                color: SentinelTheme.textMuted
                            }
                            Label {
                                Layout.fillWidth: true
                                visible: !!rowCard.modelData.failure
                                text: qsTr("Reported issue: %1").arg(rowCard.modelData.failure || "")
                                textFormat: Text.PlainText
                                wrapMode: Text.WordWrap
                                color: SentinelTheme.error
                            }
                            SentinelButton {
                                visible: hub.section !== "tools"
                                text: hub.selectedId === rowCard.modelData.id ? qsTr("Hide details") : qsTr("Inspect availability")
                                Accessible.name: text + " — " + rowCard.modelData.name
                                onClicked: { hub.selectedId = hub.selectedId === rowCard.modelData.id ? "" : rowCard.modelData.id; hub.reload() }
                            }
                            Label {
                                Layout.fillWidth: true
                                visible: hub.selectedId === rowCard.modelData.id
                                text: hub.detail.phase === "loading" ? qsTr("Loading availability…")
                                    : hub.detail.phase !== "ready" ? qsTr("Availability details unavailable")
                                    : (hub.detail.available === undefined ? qsTr("Availability not reported")
                                        : hub.detail.available ? qsTr("Available") : qsTr("Unavailable"))
                                      + (hub.detail.version ? " · " + qsTr("Version %1").arg(hub.detail.version) : "")
                                      + (hub.detail.requirements && hub.detail.requirements.length > 0 ? "\n" + qsTr("Requirements: %1").arg(hub.detail.requirements.join(", ")) : "")
                                textFormat: Text.PlainText
                                wrapMode: Text.WordWrap
                                color: SentinelTheme.textMuted
                            }
                        }
                    }
                }
                SectionTitle {
                    Layout.fillWidth: true
                    visible: hub.section === "tools" && hub.snapshot.phase === "ready"
                    title: qsTr("Registry policy metadata")
                    subtitle: qsTr("Existing registry summaries describe policy contracts; they are not per-tool execution grants.")
                }
                Label {
                    Layout.fillWidth: true
                    visible: hub.section === "tools" && hub.snapshot.phase === "ready" && (hub.snapshot.policy || []).length === 0
                    text: qsTr("Registry policy details are not reported by this daemon.")
                    color: SentinelTheme.textMuted
                    wrapMode: Text.WordWrap
                }
                Repeater {
                    model: hub.section === "tools" && hub.snapshot.phase === "ready" ? (hub.snapshot.policy || []) : []
                    delegate: Label {
                        required property string modelData
                        Layout.fillWidth: true
                        text: modelData
                        textFormat: Text.PlainText
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
        SentinelButton {
            text: qsTr("Privacy & Permissions")
            Accessible.name: qsTr("Open existing privacy and permission settings")
            onClicked: hub.settingsRequested("Security")
        }
    }
}
