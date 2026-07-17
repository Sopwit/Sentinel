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
    preferredWidth:  720
    preferredHeight: 580
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
                spacing: SentinelTheme.spaceMd

                // Category accent bar
                Rectangle {
                    implicitWidth: 4
                    implicitHeight: headerCol.implicitHeight + 4
                    radius: 2
                    color: root.accent
                    opacity: 0.85
                }

                ColumnLayout {
                    id: headerCol
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceXs

                    RowLayout {
                        spacing: SentinelTheme.spaceSm

                        // Category badge
                        Rectangle {
                            implicitHeight: 22
                            implicitWidth: popBadgeLbl.implicitWidth + 14
                            radius: 11
                            color: SentinelTheme.withAlpha(root.accent, 0.16)
                            border.color: SentinelTheme.withAlpha(root.accent, 0.35)
                            border.width: 1
                            Label {
                                id: popBadgeLbl
                                anchors.centerIn: parent
                                text: root.modelInfo ? root.modelInfo.badge : ""
                                font.pixelSize: SentinelTheme.fontTiny
                                color: root.accent
                            }
                        }

                        // Installed chip
                        Rectangle {
                            visible: root.isDone
                            implicitHeight: 22
                            implicitWidth: installedLbl.implicitWidth + 18
                            radius: 11
                            color: SentinelTheme.withAlpha(SentinelTheme.success, 0.14)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.success, 0.35)
                            border.width: 1
                            RowLayout {
                                anchors.centerIn: parent
                                spacing: 4
                                Rectangle { implicitWidth: 6; implicitHeight: 6; radius: 3; color: SentinelTheme.success }
                                Label {
                                    id: installedLbl
                                    text: qsTr("Installed")
                                    font.pixelSize: SentinelTheme.fontTiny
                                    color: SentinelTheme.success
                                }
                            }
                        }

                        // Pulling chip
                        Rectangle {
                            visible: root.activePull
                            implicitHeight: 22
                            implicitWidth: pullingLbl.implicitWidth + 18
                            radius: 11
                            color: SentinelTheme.withAlpha(root.accent, 0.14)
                            border.color: SentinelTheme.withAlpha(root.accent, 0.35)
                            border.width: 1
                            Label {
                                id: pullingLbl
                                text: qsTr("Pulling…")
                                font.pixelSize: SentinelTheme.fontTiny
                                color: root.accent
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // Close button
                        Button {
                            id: closeBtn
                            implicitWidth: 28; implicitHeight: 28
                            flat: true; hoverEnabled: true
                            onClicked: root.close()
                            background: Rectangle {
                                radius: height / 2
                                color: closeBtn.hovered
                                     ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                     : "transparent"
                                Behavior on color { ColorAnimation { duration: 100 } }
                            }
                            contentItem: Label {
                                text: "✕"; font.pixelSize: SentinelTheme.fontSmall
                                color: SentinelTheme.textMuted
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.modelInfo ? root.modelInfo.name : ""
                        font.pixelSize: 22; font.weight: Font.DemiBold
                        color: SentinelTheme.textPrimary
                        elide: Text.ElideRight
                    }

                    Label {
                        text: root.modelInfo ? qsTr("by %1").arg(root.modelInfo.provider) : ""
                        font.pixelSize: SentinelTheme.fontSmall
                        color: SentinelTheme.textPlaceholder
                    }
                }
            }

            // Divider
            Rectangle {
                Layout.fillWidth: true; implicitHeight: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.07)
            }

            // ── Body ──────────────────────────────────────────────────────────
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                clip: true

                ColumnLayout {
                    width: parent.width
                    spacing: SentinelTheme.spaceMd

                    // Horizontal Specs Pills
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceXs

                        // Size pill
                        Rectangle {
                            implicitHeight: 24
                            implicitWidth: sizeLbl.implicitWidth + 20
                            radius: 12
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                            border.width: 1
                            RowLayout {
                                anchors.centerIn: parent; spacing: 4
                                Text { text: "💾"; font.pixelSize: 11 }
                                Label { id: sizeLbl; text: root.effectiveSize; font.pixelSize: SentinelTheme.fontTiny; color: SentinelTheme.textMuted }
                            }
                        }

                        // Context pill
                        Rectangle {
                            implicitHeight: 24
                            implicitWidth: ctxLbl.implicitWidth + 20
                            radius: 12
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                            border.width: 1
                            RowLayout {
                                anchors.centerIn: parent; spacing: 4
                                Text { text: "⚡"; font.pixelSize: 11 }
                                Label { id: ctxLbl; text: qsTr("%1 Context").arg(root.effectiveContext); font.pixelSize: SentinelTheme.fontTiny; color: SentinelTheme.textMuted }
                            }
                        }

                        // Input Type pill
                        Rectangle {
                            implicitHeight: 24
                            implicitWidth: inputLbl.implicitWidth + 20
                            radius: 12
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                            border.width: 1
                            RowLayout {
                                anchors.centerIn: parent; spacing: 4
                                Text { text: "💬"; font.pixelSize: 11 }
                                Label { id: inputLbl; text: root.effectiveInput; font.pixelSize: SentinelTheme.fontTiny; color: SentinelTheme.textMuted }
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    // Description text
                    Label {
                        Layout.fillWidth: true
                        text: root.modelInfo ? root.modelInfo.description : ""
                        font.pixelSize: SentinelTheme.fontControl
                        color: SentinelTheme.textPrimary
                        wrapMode: Text.WordWrap
                        lineHeight: 1.45
                    }

                    // Best For Card
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: bestForCol.implicitHeight + 16
                        radius: SentinelTheme.radiusMd
                        color: SentinelTheme.withAlpha(root.accent, 0.06)
                        border.color: SentinelTheme.withAlpha(root.accent, 0.16)
                        border.width: 1

                        ColumnLayout {
                            id: bestForCol
                            anchors { fill: parent; margins: 10 }
                            spacing: 4

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

                    // Tags selector
                    ColumnLayout {
                        visible: ollamaModelDetailFetcher.tags.length > 0
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            text: qsTr("Available Tags")
                            font.pixelSize: SentinelTheme.fontSmall
                            font.weight: Font.Medium
                            color: SentinelTheme.textMuted
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceXs
                            
                            Repeater {
                                model: ollamaModelDetailFetcher.tags
                                
                                Button {
                                    id: tagBtn
                                    required property var modelData
                                    readonly property bool isSelected: root.selectedTagObj && root.selectedTagObj.tag === modelData.tag
                                    
                                    implicitHeight: 22
                                    implicitWidth: tagBtnLbl.implicitWidth + 14
                                    flat: true
                                    hoverEnabled: true
                                    onClicked: root.selectedTagObj = modelData
                                    
                                    background: Rectangle {
                                        radius: 11
                                        color: tagBtn.isSelected
                                             ? SentinelTheme.withAlpha(root.accent, 0.16)
                                             : (tagBtn.hovered
                                                ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                                : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04))
                                        border.color: tagBtn.isSelected
                                                    ? SentinelTheme.withAlpha(root.accent, 0.45)
                                                    : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                        border.width: 1

                                        Behavior on color { ColorAnimation { duration: 150 } }
                                        Behavior on border.color { ColorAnimation { duration: 150 } }
                                    }
                                    
                                    contentItem: Label {
                                        id: tagBtnLbl
                                        text: tagBtn.modelData.tag + " (" + tagBtn.modelData.size + ")"
                                        font.pixelSize: SentinelTheme.fontTiny
                                        font.weight: tagBtn.isSelected ? Font.Medium : Font.Normal
                                        color: tagBtn.isSelected ? root.accent : SentinelTheme.textMuted
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }
                        }
                    }

                    // Pull command & path row (for local Ollama runner only)
                    ColumnLayout {
                        visible: root.modelInfo && root.modelInfo.ollamaId !== "" && !root.isLMStudio
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: qsTr("Local Developer Command")
                            font.pixelSize: SentinelTheme.fontSmall
                            font.weight: Font.Medium
                            color: SentinelTheme.textMuted
                        }

                        RowLayout {
                            spacing: SentinelTheme.spaceXs

                            Rectangle {
                                implicitHeight: cmdLbl.implicitHeight + 8
                                implicitWidth: cmdLbl.implicitWidth + 20
                                radius: SentinelTheme.radiusSm
                                color: SentinelTheme.withAlpha(SentinelTheme.backgroundDeep, 0.70)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                border.width: 1
                                Label {
                                    id: cmdLbl
                                    anchors.centerIn: parent
                                    text: root.ollamaPullCmd
                                    font.pixelSize: SentinelTheme.fontSmall
                                    font.family: "monospace"
                                    color: SentinelTheme.textMuted
                                }
                            }

                            Button {
                                id: copyBtn
                                implicitWidth: 58
                                implicitHeight: cmdLbl.implicitHeight + 8
                                flat: true; hoverEnabled: true
                                onClicked: {
                                    root.copyToClipboard(root.ollamaPullCmd)
                                    copyFeedback.start()
                                }
                                background: Rectangle {
                                    radius: SentinelTheme.radiusSm
                                    color: copyBtn.hovered
                                         ? SentinelTheme.withAlpha(root.accent, 0.12)
                                         : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
                                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                    border.width: 1
                                    Behavior on color { ColorAnimation { duration: 100 } }
                                }
                                contentItem: Label {
                                    text: copyFeedback.running ? qsTr("✓") : qsTr("Copy")
                                    font.pixelSize: SentinelTheme.fontTiny
                                    color: copyFeedback.running ? SentinelTheme.success : root.accent
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    Behavior on color { ColorAnimation { duration: 150 } }
                                }
                            }
                        }
                    }

                    // Tags
                    Flow {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceXs
                        visible: root.modelInfo && root.modelInfo.tags && root.modelInfo.tags.length > 0

                        Label { text: qsTr("Tags: "); font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPlaceholder }

                        Repeater {
                            model: root.modelInfo ? root.modelInfo.tags : []
                            Rectangle {
                                required property string modelData
                                implicitHeight: 20; implicitWidth: popTagLbl.implicitWidth + 12
                                radius: 10
                                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.12)
                                border.width: 1
                                Label { id: popTagLbl; anchors.centerIn: parent; text: modelData; font.pixelSize: SentinelTheme.fontTiny; color: SentinelTheme.textMuted }
                            }
                        }
                    }

                    // Installed banner
                    Rectangle {
                        Layout.fillWidth: true
                        visible: root.isDone
                        implicitHeight: installedNoteRow.implicitHeight + SentinelTheme.spaceSm * 2
                        radius: SentinelTheme.radiusMd
                        color: SentinelTheme.withAlpha(SentinelTheme.success, 0.08)
                        border.color: SentinelTheme.withAlpha(SentinelTheme.success, 0.22)
                        border.width: 1

                        RowLayout {
                            id: installedNoteRow
                            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: SentinelTheme.spaceMd }
                            spacing: SentinelTheme.spaceSm
                            Rectangle { implicitWidth: 8; implicitHeight: 8; radius: 4; color: SentinelTheme.success }
                            Label {
                                Layout.fillWidth: true
                                text: root.isLMStudio
                                    ? qsTr("This model is loaded and ready for inference in LM Studio.")
                                    : qsTr("This model is installed on your device at %1").arg(root.ollamaModelPath)
                                font.pixelSize: SentinelTheme.fontSmall
                                color: SentinelTheme.success
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    // Error banner
                    Rectangle {
                        Layout.fillWidth: true
                        visible: root.pullError.length > 0
                        implicitHeight: errorNoteRow.implicitHeight + SentinelTheme.spaceSm * 2
                        radius: SentinelTheme.radiusMd
                        color: SentinelTheme.errorSurface
                        border.color: SentinelTheme.errorBorder
                        border.width: 1

                        RowLayout {
                            id: errorNoteRow
                            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: SentinelTheme.spaceMd }
                            spacing: SentinelTheme.spaceSm
                            Rectangle { implicitWidth: 8; implicitHeight: 8; radius: 4; color: "#ef4444" }
                            Label {
                                Layout.fillWidth: true
                                text: root.pullError
                                font.pixelSize: SentinelTheme.fontSmall
                                color: "#ef4444"
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    // Loading indicator for details
                    RowLayout {
                        visible: ollamaModelDetailFetcher.fetching
                        spacing: SentinelTheme.spaceXs
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: SentinelTheme.spaceMd
                        
                        Rectangle {
                            implicitWidth: 6; implicitHeight: 6; radius: 3
                            color: root.accent
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.2; to: 1.0; duration: 500; easing.type: Easing.InOutQuad }
                                NumberAnimation { from: 1.0; to: 0.2; duration: 500; easing.type: Easing.InOutQuad }
                            }
                        }
                        Label {
                            text: qsTr("Loading model details…")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontTiny
                        }
                    }

                    // Error fetching details banner
                    Rectangle {
                        Layout.fillWidth: true
                        visible: ollamaModelDetailFetcher.errorText !== ""
                        implicitHeight: fetchErrorRow.implicitHeight + SentinelTheme.spaceSm * 2
                        radius: SentinelTheme.radiusMd
                        color: SentinelTheme.errorSurface
                        border.color: SentinelTheme.errorBorder
                        border.width: 1

                        RowLayout {
                            id: fetchErrorRow
                            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: SentinelTheme.spaceMd }
                            spacing: SentinelTheme.spaceSm
                            Rectangle { implicitWidth: 8; implicitHeight: 8; radius: 4; color: "#ef4444" }
                            Label {
                                Layout.fillWidth: true
                                text: ollamaModelDetailFetcher.errorText
                                font.pixelSize: SentinelTheme.fontSmall
                                color: "#ef4444"
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    // Readme divider
                    Rectangle {
                        Layout.fillWidth: true; implicitHeight: 1
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.07)
                        visible: ollamaModelDetailFetcher.readme !== ""
                        Layout.topMargin: SentinelTheme.spaceMd
                    }

                    // Readme header
                    Label {
                        text: qsTr("About this Model")
                        font.pixelSize: SentinelTheme.fontControl
                        font.weight: Font.DemiBold
                        color: SentinelTheme.textPrimary
                        visible: ollamaModelDetailFetcher.readme !== ""
                    }

                    // Readme content
                    TextArea {
                        Layout.fillWidth: true
                        text: ollamaModelDetailFetcher.readme
                        textFormat: Text.MarkdownText
                        readOnly: true
                        selectByMouse: true
                        wrapMode: Text.WordWrap
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        background: null
                        visible: ollamaModelDetailFetcher.readme !== ""
                        activeFocusOnTab: false
                    }
                }
            }

            // Divider
            Rectangle {
                Layout.fillWidth: true; implicitHeight: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.07)
            }

            // ── Bottom action bar ─────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceSm

                // Live pull progress (inline in action bar)
                ColumnLayout {
                    visible: root.activePull
                    Layout.fillWidth: true
                    spacing: 4

                    Rectangle {
                        Layout.fillWidth: true; implicitHeight: 5; radius: 3
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                        Rectangle {
                            width: parent.width * root.pullProgress
                            height: parent.height; radius: parent.radius
                            color: root.accent
                            Behavior on width { NumberAnimation { duration: 80 } }
                        }
                    }

                    Label {
                        text: root.pullStatus.length > 0
                            ? root.pullStatus
                            : qsTr("Pulling via Ollama… %1%").arg(Math.round(root.pullProgress * 100))
                        font.pixelSize: SentinelTheme.fontTiny
                        color: SentinelTheme.textPlaceholder
                        elide: Text.ElideRight
                    }
                }

                Item { Layout.fillWidth: true; visible: !root.activePull }

                // Cancel pull
                Button {
                    id: cancelPullBtn
                    visible: root.activePull
                    implicitHeight: 34
                    implicitWidth: cancelPullLbl.implicitWidth + 24
                    flat: true; hoverEnabled: true
                    onClicked: ollamaPuller.cancel()
                    background: Rectangle {
                        radius: height / 2
                        color: cancelPullBtn.hovered
                             ? SentinelTheme.withAlpha("#ef4444", 0.12)
                             : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                        border.color: SentinelTheme.withAlpha("#ef4444", 0.25)
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 100 } }
                    }
                    contentItem: Label {
                        id: cancelPullLbl
                        text: qsTr("Cancel")
                        font.pixelSize: SentinelTheme.fontSmall
                        color: "#ef4444"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                // Close
                Button {
                    id: cancelBtn
                    implicitHeight: 34
                    implicitWidth: cancelLbl.implicitWidth + 28
                    flat: true; hoverEnabled: true
                    onClicked: root.close()
                    background: Rectangle {
                        radius: height / 2
                        color: cancelBtn.hovered
                             ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
                             : "transparent"
                        border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 100 } }
                    }
                    contentItem: Label {
                        id: cancelLbl
                        text: qsTr("Close")
                        font.pixelSize: SentinelTheme.fontSmall
                        color: SentinelTheme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                // Download via Ollama
                Button {
                    id: actionBtn
                    visible: root.modelInfo && root.modelInfo.downloadable && root.modelInfo.ollamaId !== ""
                    enabled: !root.isLMStudio && !root.activePull && !root.isDone
                    implicitHeight: 34
                    implicitWidth: actionLbl.implicitWidth + 28
                    hoverEnabled: true
                    onClicked: root.downloadRequested(root.modelInfo.ollamaId)

                    scale: actionBtn.down ? 0.96 : actionBtn.hovered ? 1.02 : 1.0
                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

                    background: Rectangle {
                        radius: height / 2
                        color: actionBtn.enabled
                             ? (actionBtn.down
                                ? SentinelTheme.withAlpha(root.accent, 0.85)
                                : actionBtn.hovered
                                  ? root.accent
                                  : SentinelTheme.withAlpha(root.accent, 0.16))
                             : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
                        border.color: actionBtn.enabled
                                    ? (actionBtn.hovered ? root.accent : SentinelTheme.withAlpha(root.accent, 0.35))
                                    : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                        border.width: 1
                        Rectangle {
                            anchors.top: parent.top; anchors.topMargin: 1
                            anchors.left: parent.left; anchors.leftMargin: 8
                            anchors.right: parent.right; anchors.rightMargin: 8
                            height: 1
                            color: SentinelTheme.withAlpha("#ffffff", actionBtn.enabled ? 0.45 : 0.10)
                            radius: 1
                        }
                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on border.color { ColorAnimation { duration: 150 } }
                    }

                    contentItem: Label {
                        id: actionLbl
                        text: root.isDone
                            ? (root.isLMStudio ? qsTr("✓  Loaded") : qsTr("✓  Installed"))
                            : root.activePull ? qsTr("Pulling…")
                            : (root.isLMStudio ? qsTr("Downloads via LM Studio only") : qsTr("↓  Download via Ollama"))
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

                // External link for non-Ollama
                Button {
                    id: externalLinkBtn
                    visible: root.modelInfo && (!root.modelInfo.downloadable || root.modelInfo.ollamaId === "")
                    implicitHeight: 34
                    implicitWidth: extLbl.implicitWidth + 24
                    flat: true
                    hoverEnabled: true
                    onClicked: {
                        if (root.modelInfo && root.modelInfo.externalUrl) {
                            Qt.openUrlExternally(root.modelInfo.externalUrl)
                        } else {
                            Qt.openUrlExternally("https://lmstudio.ai/models")
                        }
                    }
                    background: Rectangle {
                        radius: height / 2
                        color: externalLinkBtn.hovered
                             ? root.accent
                             : "transparent"
                        border.color: externalLinkBtn.hovered
                                    ? root.accent
                                    : SentinelTheme.withAlpha(root.accent, 0.20)
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
