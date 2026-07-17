import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

// ── ModelDetailPopup ──────────────────────────────────────────────────────────
// Full model detail popup with real Ollama pull integration.
// Reads ollamaPuller and shellViewModel from QML context.
// ─────────────────────────────────────────────────────────────────────────────
SentinelOverlayModal {
    id: root

    // ── Public API ────────────────────────────────────────────────────────────
    required property var modelInfo    // catalog entry JS object
    property bool installed: false     // detected from shellViewModel.ollamaModelNames
    property bool activePull: false    // ollamaPuller.pulling && activeModel matches
    property real pullProgress: 0.0   // 0..1
    property string pullStatus: ""
    property string pullError: ""

    signal downloadRequested(string modelId)

    // ── Geometry ──────────────────────────────────────────────────────────────
    preferredWidth:  500
    preferredHeight: 340
    accent: root.modelInfo ? Qt.color(root.modelInfo.badgeColor) : SentinelTheme.accent
    modeName: "Sentinel"

    // ── Helpers ───────────────────────────────────────────────────────────────
    readonly property bool isDone: installed
    readonly property bool isLMStudio: shellViewModel.selectedRuntimeProvider === "lm-studio"

    // Ollama model install path (platform default)
    readonly property string ollamaModelPath: {
        if (Qt.platform.os === "windows") return "%USERPROFILE%\\.ollama\\models"
        return "~/.ollama/models"
    }

    // Dynamic detail states
    property var selectedTagObj: null

    readonly property string effectiveOllamaId: {
        if (selectedTagObj) {
            return selectedTagObj.fullTag
        }
        return modelInfo ? modelInfo.ollamaId : ""
    }

    readonly property string effectiveSize: {
        if (installed && modelInfo && modelInfo.ollamaId) {
            var localDetails = shellViewModel.getLocalModelDetails(modelInfo.ollamaId)
            if (localDetails && localDetails.sizeFormatted && localDetails.sizeFormatted !== "—") {
                return localDetails.sizeFormatted
            }
        }
        if (selectedTagObj) {
            return selectedTagObj.size
        }
        return modelInfo ? modelInfo.size : "—"
    }

    readonly property string effectiveModifiedAt: {
        if (installed && modelInfo && modelInfo.ollamaId) {
            var localDetails = shellViewModel.getLocalModelDetails(modelInfo.ollamaId)
            if (localDetails && localDetails.modifiedAt) {
                return localDetails.modifiedAt
            }
        }
        return ""
    }

    readonly property string effectiveContext: {
        if (modelInfo && modelInfo.context !== undefined) {
            return modelInfo.context
        }
        if (modelInfo && modelInfo.id) {
            var idLower = modelInfo.id.toLowerCase()
            if (idLower.indexOf("llama3.3") !== -1 || idLower.indexOf("llama3.2") !== -1 || idLower.indexOf("qwen2.5") !== -1 || idLower.indexOf("deepseek-r1") !== -1 || idLower.indexOf("phi-3.5")) {
                return "128K"
            }
            if (idLower.indexOf("phi4") !== -1 || idLower.indexOf("phi-4") !== -1) {
                return "16K"
            }
            if (idLower.indexOf("gemma2") !== -1 || idLower.indexOf("gemma-2") !== -1 || idLower.indexOf("llama3") !== -1) {
                return "8K"
            }
            if (idLower.indexOf("llava") !== -1) {
                return "4K"
            }
            if (modelInfo.category === "LLM" || modelInfo.category === "Think") {
                return "8K"
            }
        }
        return "—"
    }

    readonly property string effectiveInput: {
        if (modelInfo && modelInfo.input !== undefined) {
            return modelInfo.input
        }
        if (modelInfo && modelInfo.id) {
            var idLower = modelInfo.id.toLowerCase()
            if (idLower.indexOf("llava") !== -1 || modelInfo.category === "Vision" || (modelInfo.tags && modelInfo.tags.indexOf("vision") !== -1)) {
                return "Text / Image"
            }
            if (modelInfo.category === "STT") {
                return "Audio"
            }
            if (modelInfo.category === "Runtime") {
                return "—"
            }
        }
        return "Text"
    }

    readonly property string ollamaPullCmd: {
        return "ollama pull " + effectiveOllamaId
    }

    onModelInfoChanged: {
        selectedTagObj = null
        if (modelInfo && modelInfo.ollamaId && modelInfo.ollamaId !== "") {
            var baseName = modelInfo.ollamaId.split(':')[0]
            ollamaModelDetailFetcher.fetchDetails(baseName)
        } else {
            ollamaModelDetailFetcher.cancel()
        }
    }

    onOpened: {
        if (modelInfo && modelInfo.ollamaId && modelInfo.ollamaId !== "") {
            var baseName = modelInfo.ollamaId.split(':')[0]
            ollamaModelDetailFetcher.fetchDetails(baseName)
        }
    }

    onClosed: {
        ollamaModelDetailFetcher.cancel()
    }

    Connections {
        target: ollamaModelDetailFetcher
        function onTagsChanged() {
            var tags = ollamaModelDetailFetcher.tags
            if (tags && tags.length > 0) {
                var defaultTag = "latest"
                if (root.modelInfo && root.modelInfo.ollamaId) {
                    var parts = root.modelInfo.ollamaId.split(':')
                    if (parts.length > 1) {
                        defaultTag = parts[1]
                    }
                }
                
                var found = null
                for (var i = 0; i < tags.length; i++) {
                    if (tags[i].tag === defaultTag) {
                        found = tags[i]
                        break
                    }
                }
                if (!found && tags.length > 0) {
                    found = tags[0]
                }
                root.selectedTagObj = found
            } else {
                root.selectedTagObj = null
            }
        }
    }

    // Clipboard copy helper
    TextInput {
        id: clipboardHelper
        visible: false
    }

    function copyToClipboard(txt) {
        clipboardHelper.text = txt
        clipboardHelper.selectAll()
        clipboardHelper.copy()
    }

    // ── Content ───────────────────────────────────────────────────────────────
    contentItem: Item {
        anchors.fill: parent

        ColumnLayout {
            anchors { fill: parent; margins: SentinelTheme.spaceLg }
            spacing: SentinelTheme.spaceMd

            // ── Header ────────────────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceSm

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: root.modelInfo ? root.modelInfo.name : ""
                        font.pixelSize: 20
                        font.weight: Font.DemiBold
                        color: SentinelTheme.textPrimary
                        elide: Text.ElideRight
                    }

                    Label {
                        text: root.modelInfo ? qsTr("by %1").arg(root.modelInfo.provider) : ""
                        font.pixelSize: SentinelTheme.fontSmall
                        color: SentinelTheme.textMuted
                    }
                }

                Button {
                    id: closeBtn
                    implicitWidth: 26; implicitHeight: 26
                    flat: true; hoverEnabled: true
                    onClicked: root.close()
                    background: Rectangle {
                        radius: height / 2
                        color: closeBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06) : "transparent"
                        Behavior on color { ColorAnimation { duration: 100 } }
                    }
                    contentItem: Label {
                        text: "×"
                        font.pixelSize: 18
                        font.bold: true
                        color: SentinelTheme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            // Divider
            Rectangle {
                Layout.fillWidth: true; implicitHeight: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
            }

            // ── Body (Concise Promo Text) ─────────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: SentinelTheme.spaceMd

                // Horizontal Premium Specs (Typography-based)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceLg

                    // Size
                    ColumnLayout {
                        spacing: 2
                        Label {
                            text: qsTr("SIZE")
                            font.pixelSize: SentinelTheme.fontTiny
                            font.weight: Font.Bold
                            color: SentinelTheme.textPlaceholder
                        }
                        Label {
                            text: root.effectiveSize
                            font.pixelSize: SentinelTheme.fontSmall
                            font.weight: Font.DemiBold
                            color: SentinelTheme.textPrimary
                        }
                    }

                    // Vertical Separator
                    Rectangle {
                        Layout.preferredHeight: 24
                        implicitWidth: 1
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                        Layout.alignment: Qt.AlignVCenter
                    }

                    // Context Window
                    ColumnLayout {
                        spacing: 2
                        Label {
                            text: qsTr("CONTEXT")
                            font.pixelSize: SentinelTheme.fontTiny
                            font.weight: Font.Bold
                            color: SentinelTheme.textPlaceholder
                        }
                        Label {
                            text: qsTr("%1 tokens").arg(root.effectiveContext)
                            font.pixelSize: SentinelTheme.fontSmall
                            font.weight: Font.DemiBold
                            color: SentinelTheme.textPrimary
                        }
                    }

                    // Vertical Separator
                    Rectangle {
                        Layout.preferredHeight: 24
                        implicitWidth: 1
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                        Layout.alignment: Qt.AlignVCenter
                    }

                    // Input Type
                    ColumnLayout {
                        spacing: 2
                        Label {
                            text: qsTr("INPUT TYPE")
                            font.pixelSize: SentinelTheme.fontTiny
                            font.weight: Font.Bold
                            color: SentinelTheme.textPlaceholder
                        }
                        Label {
                            text: root.effectiveInput
                            font.pixelSize: SentinelTheme.fontSmall
                            font.weight: Font.DemiBold
                            color: SentinelTheme.textPrimary
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                Label {
                    Layout.fillWidth: true
                    text: root.modelInfo ? root.modelInfo.description : ""
                    font.pixelSize: SentinelTheme.fontControl
                    color: SentinelTheme.textPrimary
                    wrapMode: Text.WordWrap
                    lineHeight: 1.45
                }

                // Highlighted Recommendation Box
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: bestForCol.implicitHeight + 16
                    radius: 8
                    color: SentinelTheme.withAlpha(root.accent, 0.05)
                    border.color: SentinelTheme.withAlpha(root.accent, 0.14)
                    border.width: 1

                    ColumnLayout {
                        id: bestForCol
                        anchors { fill: parent; margins: 10 }
                        spacing: 2

                        RowLayout {
                            spacing: 4
                            Text { text: "💡"; font.pixelSize: 12 }
                            Label {
                                text: qsTr("Best For")
                                font.pixelSize: SentinelTheme.fontSmall
                                font.weight: Font.Bold
                                color: root.accent
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.modelInfo && root.modelInfo.bestFor ? root.modelInfo.bestFor : qsTr("General-purpose local AI tasks.")
                            font.pixelSize: SentinelTheme.fontSmall
                            color: SentinelTheme.textMuted
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                // Progress Indicator (Active download only)
                ColumnLayout {
                    visible: root.activePull
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceXs

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Downloading model…")
                            font.pixelSize: SentinelTheme.fontSmall
                            color: root.accent
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text: Math.round(root.pullProgress * 100) + "%"
                            font.pixelSize: SentinelTheme.fontSmall
                            font.weight: Font.DemiBold
                            color: root.accent
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 4
                        radius: 2
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)

                        Rectangle {
                            width: parent.width * root.pullProgress
                            height: parent.height
                            radius: parent.radius
                            color: root.accent
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }

            // ── Footer ────────────────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceMd

                // Cancel download
                Button {
                    id: cancelPullBtn
                    visible: root.activePull
                    implicitHeight: 32
                    implicitWidth: 80
                    hoverEnabled: true
                    onClicked: root.cancelRequested()
                    background: Rectangle {
                        radius: 6
                        color: cancelPullBtn.hovered
                             ? SentinelTheme.withAlpha("#ef4444", 0.08)
                             : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.03)
                        border.color: SentinelTheme.withAlpha("#ef4444", 0.20)
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 100 } }
                    }
                    contentItem: Label {
                        text: qsTr("Cancel")
                        font.pixelSize: SentinelTheme.fontSmall
                        color: "#ef4444"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Item { Layout.fillWidth: true }

                // Download Button
                Button {
                    id: actionBtn
                    visible: root.modelInfo && root.modelInfo.downloadable && root.modelInfo.ollamaId !== ""
                    enabled: !root.isLMStudio && !root.activePull && !root.isDone
                    implicitHeight: 32
                    implicitWidth: actionLbl.implicitWidth + 24
                    hoverEnabled: true
                    onClicked: root.downloadRequested(root.modelInfo.ollamaId)
                    scale: actionBtn.down ? 0.97 : 1.0

                    background: Rectangle {
                        radius: 6
                        color: actionBtn.enabled
                             ? (actionBtn.hovered ? root.accent : SentinelTheme.withAlpha(root.accent, 0.12))
                             : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                        border.color: actionBtn.enabled
                                    ? root.accent
                                    : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on border.color { ColorAnimation { duration: 150 } }
                    }

                    contentItem: Label {
                        id: actionLbl
                        text: root.isDone
                            ? (root.isLMStudio ? qsTr("✓  Loaded") : qsTr("✓  Installed"))
                            : root.activePull ? qsTr("Downloading…")
                            : (root.isLMStudio ? qsTr("Use LM Studio App") : qsTr("Install Model"))
                        font.pixelSize: SentinelTheme.fontSmall
                        font.weight: Font.Medium
                        color: actionBtn.enabled
                             ? (actionBtn.hovered ? "#ffffff" : root.accent)
                             : SentinelTheme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                }

                // External website button
                Button {
                    id: externalLinkBtn
                    visible: root.modelInfo && (!root.modelInfo.downloadable || root.modelInfo.ollamaId === "")
                    implicitHeight: 32
                    implicitWidth: extLbl.implicitWidth + 24
                    hoverEnabled: true
                    onClicked: {
                        if (root.modelInfo && root.modelInfo.externalUrl) {
                            Qt.openUrlExternally(root.modelInfo.externalUrl)
                        } else {
                            Qt.openUrlExternally("https://lmstudio.ai/models")
                        }
                    }
                    background: Rectangle {
                        radius: 6
                        color: externalLinkBtn.hovered ? root.accent : "transparent"
                        border.color: externalLinkBtn.hovered ? root.accent : SentinelTheme.withAlpha(root.accent, 0.20)
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on border.color { ColorAnimation { duration: 150 } }
                    }
                    contentItem: Label {
                        id: extLbl
                        text: qsTr("Visit provider website ↗")
                        font.pixelSize: SentinelTheme.fontSmall
                        color: externalLinkBtn.hovered ? "#ffffff" : root.accent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                }
            }
        }
    }

    // ── Copy feedback timer ───────────────────────────────────────────────────
    Timer {
        id: copyFeedback
        interval: 1600
        repeat: false
    }
}
