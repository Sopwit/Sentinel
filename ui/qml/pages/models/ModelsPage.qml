// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Dialogs
import Sentinel.Desktop
import "ModelCatalog.js" as Catalog

Item {
    id: modelsPage

    property var viewModel: shellViewModel
    readonly property bool compact: width < 820
    readonly property int panelPadding: SentinelTheme.spaceLg
    readonly property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    property bool sidebarCollapsed: false
    property string searchQuery: ""

    readonly property var installedOllamaNames: JSON.parse(JSON.stringify(shellViewModel.installedOllamaModelNames || []))
    readonly property var loadedStudioNames: JSON.parse(JSON.stringify(shellViewModel.loadedLMStudioModelNames || []))

    // ── Static model catalog ─────────────────────────────────────────────────
    readonly property var modelCatalog: ggufLibraryFetcher.catalog || []

    FileDialog {
        id: ggufImportDialog
        title: qsTr("Import a local GGUF model")
        nameFilters: [qsTr("GGUF models (*.gguf)")]
        onAccepted: ggufLibraryFetcher.importFile(selectedFile.toString())
    }

    // ── Filter state ─────────────────────────────────────────────────────────
    property string activeCategory: "All"
    property string catalogSource: "all"
    onCatalogSourceChanged: catalogSearchTimer.restart()
    property string discoveryTask: ""
    onDiscoveryTaskChanged: catalogSearchTimer.restart()
    property string listSort: "installed"
    property string discoverySort: "downloads"
    function discoveryQuery() { return JSON.stringify({text: searchQuery, category: activeCategory, sort: discoverySort, task: discoveryTask, ggufOnly: catalogSource === "llamacpp"}) }
    onActiveCategoryChanged: { discoveryTask = ""; catalogSearchTimer.restart() }
    onDiscoverySortChanged: catalogSearchTimer.restart()
    onSearchQueryChanged: catalogSearchTimer.restart()
    Timer { id: catalogSearchTimer; interval: 700; onTriggered: { if (modelsPage.activeCategory !== "Runtime") ggufLibraryFetcher.fetch(modelsPage.discoveryQuery()) } }

    readonly property bool catalogFetching: ollamaLibraryFetcher.fetching || lmStudioLibraryFetcher.fetching || ggufLibraryFetcher.fetching
    function inferredLocalCategory(name) {
        var lower = name.toLowerCase()
        if (lower.indexOf("embed") >= 0) return "Embedding"
        if (lower.indexOf("vision") >= 0 || lower.indexOf("llava") >= 0) return "Vision"
        if (lower.indexOf("deepseek-r1") >= 0 || lower.indexOf("thinking") >= 0) return "Think"
        if (lower.indexOf("whisper") >= 0) return "STT"
        return "LLM"
    }
    function matchesCategory(model, category) {
        return Catalog.matchesCategory(model, category)
    }
    readonly property var allModels: {
        var liveModels = ollamaLibraryFetcher.models || []
        var lmModels = lmStudioLibraryFetcher.models || []
        var ggufModels = ggufLibraryFetcher.models || []
        var list = []
        var seenOllamaIds = {}
        var seenIds = {}

        for (var g = 0; g < ggufModels.length; g++) {
            list.push(ggufModels[g]); seenIds[ggufModels[g].id] = true
        }

        // Add live models first
        for (var i = 0; i < liveModels.length; i++) {
            var m = liveModels[i]
            list.push(m)
            seenIds[m.id] = true
            if (m.ollamaId && m.ollamaId !== "") {
                seenOllamaIds[m.ollamaId.toLowerCase()] = true
            }
        }

        // Add LM Studio models
        for (var k = 0; k < lmModels.length; k++) {
            var lmm = lmModels[k]
            if (!seenIds[lmm.id]) {
                list.push(lmm)
                seenIds[lmm.id] = true
            }
        }

        // Add static catalog models if not already represented by live models
        for (var j = 0; j < modelCatalog.length; j++) {
            var sm = modelCatalog[j]
            var oid = sm.ollamaId ? sm.ollamaId.toLowerCase() : ""
            if (oid !== "" && seenOllamaIds[oid]) {
                continue // Already added from live models
            }
            if (seenIds[sm.id]) {
                continue
            }
            list.push(sm)
            seenIds[sm.id] = true
            if (oid !== "") {
                seenOllamaIds[oid] = true
            }
        }

        // Add local installed Ollama models if they are not already in the list
        var ollamaNames = installedOllamaNames
        for (var n = 0; n < ollamaNames.length; n++) {
            var localName = ollamaNames[n]
            var localOid = localName.toLowerCase()
            var baseLocal = localOid.split(":")[0]
            
            if (seenOllamaIds[localOid] || seenOllamaIds[baseLocal] || seenIds[localOid] || seenIds[baseLocal]) {
                continue
            }
            
            var details = shellViewModel.getLocalModelDetails(localName)
            var sizeStr = (details && details.sizeFormatted) ? details.sizeFormatted : "—"
            
            var category = inferredLocalCategory(localOid)

            var localModelObj = {
                id: localName,
                ollamaId: localName,
                category: category,
                name: localName,
                provider: "Ollama",
                catalogSource: "ollama",
                size: sizeStr,
                description: qsTr("Custom model installed locally via Ollama."),
                badge: qsTr("Installed"),
                badgeColor: "#10b981",
                tags: ["Local", "Ollama"],
                downloadable: false
            }
            
            list.push(localModelObj)
            seenIds[localName] = true
            seenOllamaIds[localOid] = true
        }

        // Add local loaded LM Studio models if they are not already in the list
        var lmNames = loadedStudioNames
        for (var n = 0; n < lmNames.length; n++) {
            var localName = lmNames[n]
            var localOid = localName.toLowerCase()
            var baseLocal = localOid.split(":")[0]
            
            // Allow duplicate names across providers but ensure unique IDs in this view
            var uniqueId = "lmstudio/" + localName
            if (seenIds[uniqueId]) {
                continue
            }
            
            var details = shellViewModel.getLocalModelDetails(localName)
            var sizeStr = (details && details.sizeFormatted) ? details.sizeFormatted : "—"
            
            var category = inferredLocalCategory(localOid)

            var localModelObj = {
                id: uniqueId,
                ollamaId: localName,
                category: category,
                name: localName,
                provider: "LM Studio",
                catalogSource: "lmstudio",
                size: sizeStr,
                description: qsTr("Custom model loaded locally in LM Studio."),
                badge: qsTr("Loaded"),
                badgeColor: "#4f8ef7",
                tags: ["Local", "LM Studio"],
                downloadable: false
            }
            
            list.push(localModelObj)
            seenIds[uniqueId] = true
        }

        return list
    }

    readonly property var categories: {
        var cats = ["All"]
        var standardOrder = ["LLM", "Think", "Vision", "Image", "Video", "STT", "TTS", "Embedding", "Other", "Runtime"]
        
        var presentCats = []
        for (var i = 0; i < allModels.length; i++) {
            var cat = allModels[i].category
            if (cat && presentCats.indexOf(cat) === -1) {
                presentCats.push(cat)
            }
        }

        for (var j = 0; j < standardOrder.length; j++) {
            var std = standardOrder[j]
            if (presentCats.indexOf(std) !== -1) {
                cats.push(std)
            }
        }

        for (var kk = 0; kk < presentCats.length; kk++) {
            var p = presentCats[kk]
            if (cats.indexOf(p) === -1) {
                cats.push(p)
            }
        }

        return cats
    }

    property string ollamaSort: "popular"

    function fetchOllamaModels(force) {
        if (!daemonClient.daemonReachable) return
        ollamaLibraryFetcher.fetch(ollamaSort)
        lmStudioLibraryFetcher.fetch()
        if (force) ggufLibraryFetcher.refreshCatalog(discoveryQuery())
        else ggufLibraryFetcher.fetch(discoveryQuery())
    }

    Component.onCompleted: fetchOllamaModels()
    onVisibleChanged: {
        if (visible) fetchOllamaModels()
    }
    Connections {
        target: daemonClient
        function onDaemonReachableChanged() {
            if (daemonClient.daemonReachable && modelsPage.visible) modelsPage.fetchOllamaModels()
        }
    }

    onOllamaSortChanged: {
        ollamaLibraryFetcher.fetch(ollamaSort)
    }

    readonly property var filteredModels: {
        var baseList = []
        if (activeCategory === "All" || categories.indexOf(activeCategory) === -1) {
            baseList = allModels
        } else {
            baseList = allModels.filter(function(m) { return modelsPage.matchesCategory(m, activeCategory) })
        }

        baseList = baseList.filter(function(m) {
            return (modelsPage.activeCategory === "Runtime" || Catalog.matchesSource(m, modelsPage.catalogSource)) && Catalog.matchesSearch(m, modelsPage.searchQuery)
        })
        var installed = baseList.map(function(m) { return modelsPage.isInstalledOnDevice(m) })
        return Catalog.sortModels(baseList, listSort, installed)
    }

    property var currentModels: []

    onFilteredModelsChanged: {
        // Detach the visible snapshot from QML's live QVariant sequence wrappers.
        var snapshot = JSON.parse(JSON.stringify(filteredModels || []))
        if (JSON.stringify(currentModels) !== JSON.stringify(snapshot)) currentModels = snapshot
    }

    // ── Installed model detection via Ollama / LM Studio ─────────────────────
    function isInstalledOnDevice(model) {
        if (!model) return false
        if (model.gguf) return !!model.installed
        var isLM = (model.catalogSource === "lmstudio" || model.provider === "LM Studio" || (model.id && model.id.indexOf("lmstudio/") !== -1))
        var names = isLM ? modelsPage.loadedStudioNames
                         : modelsPage.installedOllamaNames
        if (!names || names.length === 0) return false

        var candidates = []
        if (model.ollamaId && model.ollamaId !== "") candidates.push(model.ollamaId.toLowerCase())
        if (model.id && model.id !== "") {
            var cleanId = model.id
            if (cleanId.indexOf("lmstudio/") === 0) cleanId = cleanId.substring(9)
            candidates.push(cleanId.toLowerCase())
        }
        if (model.name && model.name !== "") candidates.push(model.name.toLowerCase())

        for (var i = 0; i < names.length; i++) {
            var n = names[i].toLowerCase()
            for (var c = 0; c < candidates.length; c++) {
                var needle = candidates[c]
                if (n === needle) return true

                // Match "llama3.2" against "llama3.2:3b-instruct-q4_K_M"
                if (needle.indexOf(":") === -1 && (n === needle + ":latest")) return true


                // Flexible match for MLX/Hugging Face custom tags

            }
        }
        return false
    }

    // Returns true if ollamaPuller is actively pulling this model
    function isPulling(modelId) {
        if (!ollamaPuller.pulling || !ollamaPuller.activeModel || !modelId) return false
        var activeLower = ollamaPuller.activeModel.toLowerCase()
        var targetLower = modelId.toLowerCase()
        if (activeLower === targetLower) return true

        var activeBase = activeLower.split(":")[0]
        var targetBase = targetLower.split(":")[0]
        return activeBase === targetBase
    }

    function isModelPulling(model) {
        if (!model) return false
        return isPulling(model.ollamaId)
    }

    function categoryIcon(cat) {
        if (cat === "All") return "layout-dashboard"
        if (cat === "LLM") return "message-circle"
        if (cat === "Think") return "brain"
        if (cat === "Vision") return "eye"
        if (cat === "Image") return "palette"
        if (cat === "Video") return "movie"
        if (cat === "STT") return "microphone"
        if (cat === "TTS") return "volume-2"
        if (cat === "Embedding") return "search"
        if (cat === "Runtime") return "settings"
        return "circle"
    }

    function categoryIconSize(cat) {
        if (cat === "All") return 18
        if (cat === "LLM") return 14
        if (cat === "Think") return 14
        if (cat === "Vision") return 14
        if (cat === "Image") return 14
        if (cat === "Video") return 14
        if (cat === "STT") return 14
        if (cat === "TTS") return 14
        if (cat === "Runtime") return 15
        return 14
    }

    function categoryTitle(cat) {
        if (cat === "All") return qsTr("All Models")
        if (cat === "LLM") return qsTr("Text Models")
        if (cat === "Think") return qsTr("Reasoning Models")
        if (cat === "Vision") return qsTr("Vision Models")
        if (cat === "Image") return qsTr("Image Generation")
        if (cat === "Video") return qsTr("Video Generation")
        if (cat === "STT") return qsTr("Speech to Text")
        if (cat === "TTS") return qsTr("Text to Speech")
        if (cat === "Embedding") return qsTr("Embedding Models")
        if (cat === "Other") return qsTr("Other Tasks")
        if (cat === "Runtime") return qsTr("Local Runtimes")
        return cat
    }

    // ── Layout ───────────────────────────────────────────────────────────────
    RowLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        ShellPanel {
            Layout.preferredWidth: modelsPage.sidebarCollapsed ? 64 : (modelsPage.compact ? 196 : 278)
            Layout.fillHeight: true
            color: SentinelTheme.backgroundBase
            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)

            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: modelsPage.sidebarCollapsed ? SentinelTheme.spaceSm : SentinelTheme.spaceMd
                spacing: SentinelTheme.spaceMd

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: SentinelTheme.spaceSm
                    Layout.bottomMargin: SentinelTheme.spaceXs
                    spacing: 0

                    Label {
                        visible: !modelsPage.sidebarCollapsed
                        Layout.fillWidth: true
                        Layout.leftMargin: SentinelTheme.spaceMd
                        text: qsTr("Models")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontTitle
                        font.bold: true
                        maximumLineCount: 1
                        elide: Text.ElideRight
                    }

                    Item {
                        visible: modelsPage.sidebarCollapsed
                        Layout.fillWidth: true
                    }

                    Button {
                        id: collapseBtn
                        Layout.alignment: Qt.AlignVCenter
                        Layout.rightMargin: modelsPage.sidebarCollapsed ? 0 : SentinelTheme.spaceMd
                        implicitHeight: 36
                        implicitWidth: 36
                        flat: true
                        padding: 0
                        onClicked: modelsPage.sidebarCollapsed = !modelsPage.sidebarCollapsed
                        hoverEnabled: true

                        contentItem: Label {
                            text: modelsPage.sidebarCollapsed ? "»" : "«"
                            font.pixelSize: 22
                            font.bold: true
                            color: collapseBtn.hovered ? modelsPage.modeAccent : SentinelTheme.textMuted
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            radius: 8
                            color: collapseBtn.hovered
                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                 : "transparent"
                        }

                        ToolTip.visible: collapseBtn.hovered
                        ToolTip.text: modelsPage.sidebarCollapsed ? qsTr("Expand Sidebar") : qsTr("Collapse Sidebar")
                        ToolTip.delay: 400
                    }

                    Item {
                        visible: modelsPage.sidebarCollapsed
                        Layout.fillWidth: true
                    }
                }

                Flickable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: width
                    contentHeight: sidebarButtonsColumn.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds

                    ColumnLayout {
                        id: sidebarButtonsColumn
                        width: parent.width
                        spacing: modelsPage.sidebarCollapsed ? 6 : SentinelTheme.spaceXs

                        Repeater {
                            model: modelsPage.categories
                            delegate: Button {
                                id: navButton
                                required property var modelData
                                readonly property bool active: modelsPage.activeCategory === modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                hoverEnabled: true
                                focusPolicy: Qt.StrongFocus
                                onClicked: modelsPage.activeCategory = modelData

                                ToolTip.visible: navButton.hovered && modelsPage.sidebarCollapsed
                                ToolTip.text: modelsPage.categoryTitle(modelData)
                                ToolTip.delay: 200

                                contentItem: Item {
                                    // Fixed geometry icon container for exact alignment
                                    Item {
                                        id: iconContainer
                                        width: 24
                                        height: 24
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: modelsPage.sidebarCollapsed ? undefined : parent.left
                                        anchors.leftMargin: modelsPage.sidebarCollapsed ? 0 : SentinelTheme.spaceLg
                                        anchors.horizontalCenter: modelsPage.sidebarCollapsed ? parent.horizontalCenter : undefined

                                        TablerGlyph {
                                            anchors.centerIn: parent
                                            text: modelsPage.categoryIcon(navButton.modelData)
                                            font.family: SentinelTheme.iconFontFamily
                                            color: navButton.active
                                                   ? SentinelTheme.textPrimary
                                                   : (navButton.hovered ? SentinelTheme.textPrimary : SentinelTheme.textMuted)
                                            font.pixelSize: 15
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                    }

                                    // Category Title Label (visible only when expanded)
                                    Text {
                                        visible: !modelsPage.sidebarCollapsed
                                        anchors.left: iconContainer.right
                                        anchors.leftMargin: SentinelTheme.spaceSm
                                        anchors.right: parent.right
                                        anchors.rightMargin: SentinelTheme.spaceMd
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelsPage.categoryTitle(navButton.modelData)
                                        color: navButton.active
                                               ? SentinelTheme.textPrimary
                                               : (navButton.hovered ? SentinelTheme.textPrimary : SentinelTheme.textMuted)
                                        font.pixelSize: SentinelTheme.fontBody
                                        font.bold: navButton.active
                                        maximumLineCount: 1
                                        elide: Text.ElideRight
                                    }
                                }

                                background: Rectangle {
                                    id: navBtnBg
                                    radius: SentinelTheme.radiusMd
                                    color: navButton.active
                                           ? SentinelTheme.withAlpha(modelsPage.modeAccent, 0.14)
                                           : (navButton.hovered
                                              ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
                                              : "transparent")

                                    layer.enabled: navButton.active || navButton.hovered
                                    layer.effect: MultiEffect {
                                        shadowEnabled: true
                                        shadowColor: navButton.active
                                                     ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.20)
                                                     : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                        shadowVerticalOffset: navButton.active ? 2 : 1
                                        shadowBlur: 0.12
                                        shadowOpacity: 1.0
                                    }

                                    Behavior on color {
                                        ColorAnimation {
                                            duration: MotionTokens.fast
                                            easing.type: MotionTokens.standard
                                        }
                                    }

                                    Rectangle {
                                        width: navButton.active ? 3 : 0
                                        height: 20
                                        radius: 1.5
                                        anchors.left: parent.left
                                        anchors.leftMargin: modelsPage.sidebarCollapsed ? 3 : 6
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: modelsPage.modeAccent

                                        Behavior on width {
                                            NumberAnimation {
                                                duration: MotionTokens.fast
                                                easing.type: MotionTokens.enter
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Header Section of the Content Area
            Item {
                Layout.fillWidth: true
                implicitHeight: headerContent.implicitHeight + SentinelTheme.spaceLg * 2

                ColumnLayout {
                    id: headerContent
                    anchors {
                        left: parent.left
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                        leftMargin: SentinelTheme.spaceSm
                        rightMargin: SentinelTheme.spaceSm
                    }
                    spacing: SentinelTheme.spaceXs

                    GridLayout {
                        id: catalogToolbar
                        Layout.fillWidth: true
                        columns: width < 600 ? 1 : 2
                        columnSpacing: SentinelTheme.spaceSm
                        rowSpacing: SentinelTheme.spaceSm

                        Label {
                            text: modelsPage.categoryTitle(modelsPage.activeCategory).toUpperCase()
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontTiny
                            font.letterSpacing: 1.8
                        }

                        SentinelTextField {
                            id: searchField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Search models…")
                            implicitWidth: 180
                            implicitHeight: 26
                            font.pixelSize: SentinelTheme.fontTiny
                            leftPadding: 10
                            rightPadding: 10
                            onTextChanged: modelsPage.searchQuery = text
                            
                            background: Rectangle {
                                id: searchFieldBg
                                radius: 13
                                color: searchField.activeFocus
                                       ? SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.90)
                                       : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
                                border.color: searchField.activeFocus
                                              ? SentinelTheme.withAlpha(modelsPage.modeAccent, 0.46)
                                              : searchField.hovered
                                                ? SentinelTheme.withAlpha(modelsPage.modeAccent, 0.24)
                                              : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                 border.width: 1
                                 Behavior on border.color {
                                     ColorAnimation { duration: 120; easing.type: Easing.InOutQuad }
                                 }
                             }
                        }

                        // Sort controls / Refresh button / count chips
                        Flow {
                            Layout.fillWidth: true
                            Layout.columnSpan: catalogToolbar.columns
                            spacing: SentinelTheme.spaceSm

                            SentinelComboBox {
                                visible: modelsPage.activeCategory !== "Runtime"
                                model: [qsTr("All sources"), "Hugging Face", "llama.cpp · GGUF", "Ollama", "LM Studio"]
                                Accessible.name: qsTr("Model catalog source")
                                currentIndex: ["all", "huggingface", "llamacpp", "ollama", "lmstudio"].indexOf(modelsPage.catalogSource)
                                onActivated: modelsPage.catalogSource = ["all", "huggingface", "llamacpp", "ollama", "lmstudio"][currentIndex]
                            }
                            // Loading Indicator
                            RowLayout {
                                visible: ollamaLibraryFetcher.fetching
                                spacing: SentinelTheme.spaceXs
                                Rectangle {
                                    Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4
                                    color: SentinelTheme.accent
                                    SequentialAnimation on opacity {
                                        loops: Animation.Infinite
                                        NumberAnimation { from: 0.3; to: 1.0; duration: 600; easing.type: Easing.InOutQuad }
                                        NumberAnimation { from: 1.0; to: 0.3; duration: 600; easing.type: Easing.InOutQuad }
                                    }
                                }
                                Label {
                                    text: qsTr("Fetching…")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontTiny
                                }
                            }

                            // Sort Popular / Newest / Refresh
                            RowLayout {
                                id: catalogSourceActions
                                readonly property bool sortRelevant: (modelsPage.catalogSource === "all" || modelsPage.catalogSource === "ollama") && (modelsPage.activeCategory === "All" || modelsPage.activeCategory === "LLM" || modelsPage.activeCategory === "Think" || modelsPage.activeCategory === "Vision")
                                spacing: SentinelTheme.spaceSm

                                Button {
                                    id: sortPopularBtn
                                    visible: catalogSourceActions.sortRelevant
                                    implicitHeight: 22
                                    implicitWidth: contentItem.implicitWidth + 20
                                    flat: true
                                    checkable: true
                                    checked: modelsPage.ollamaSort === "popular"
                                    onClicked: modelsPage.ollamaSort = "popular"
                                    contentItem: Label {
                                        text: qsTr("Ollama: Popular")
                                        font.pixelSize: SentinelTheme.fontTiny
                                        font.weight: sortPopularBtn.checked ? Font.Medium : Font.Normal
                                        color: sortPopularBtn.checked ? SentinelTheme.accent : SentinelTheme.textMuted
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        id: sortPopularBg
                                        radius: 11
                                        color: sortPopularBtn.checked ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.12) : "transparent"
                                        border.color: sortPopularBtn.checked ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.3) : "transparent"
                                        layer.enabled: sortPopularBtn.checked
                                        layer.effect: MultiEffect {
                                            shadowEnabled: true
                                            shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                            shadowVerticalOffset: 1
                                            shadowBlur: 0.06
                                            shadowOpacity: 1.0
                                        }
                                    }
                                }

                                Button {
                                    id: sortNewestBtn
                                    visible: catalogSourceActions.sortRelevant
                                    implicitHeight: 22
                                    implicitWidth: contentItem.implicitWidth + 20
                                    flat: true
                                    checkable: true
                                    checked: modelsPage.ollamaSort === "newest"
                                    onClicked: modelsPage.ollamaSort = "newest"
                                    contentItem: Label {
                                        text: qsTr("Ollama: Newest")
                                        font.pixelSize: SentinelTheme.fontTiny
                                        font.weight: sortNewestBtn.checked ? Font.Medium : Font.Normal
                                        color: sortNewestBtn.checked ? SentinelTheme.accent : SentinelTheme.textMuted
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        id: sortNewestBg
                                        radius: 11
                                        color: sortNewestBtn.checked ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.12) : "transparent"
                                        border.color: sortNewestBtn.checked ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.3) : "transparent"
                                        layer.enabled: sortNewestBtn.checked
                                        layer.effect: MultiEffect {
                                            shadowEnabled: true
                                            shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                            shadowVerticalOffset: 1
                                            shadowBlur: 0.06
                                            shadowOpacity: 1.0
                                        }
                                    }
                                }

                                Button {
                                    text: qsTr("Import GGUF…")
                                    Accessible.name: qsTr("Import a local model for llama.cpp")
                                    implicitHeight: 26
                                    onClicked: ggufImportDialog.open()
                                }

                                Button {
                                    id: refreshBtn
                                    visible: modelsPage.activeCategory !== "Runtime"
                                    implicitHeight: 22
                                    implicitWidth: 54
                                    flat: true
                                    onClicked: modelsPage.fetchOllamaModels(true)
                                    contentItem: Label {
                                        text: qsTr("Refresh")
                                        font.pixelSize: SentinelTheme.fontTiny
                                        color: refreshBtn.hovered ? SentinelTheme.accent : SentinelTheme.textMuted
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        id: refreshBg
                                        radius: 11
                                        color: refreshBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.08) : "transparent"
                                        border.color: refreshBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.2) : "transparent"
                                        layer.enabled: refreshBtn.hovered
                                        layer.effect: MultiEffect {
                                            shadowEnabled: true
                                            shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                            shadowVerticalOffset: 1
                                            shadowBlur: 0.06
                                            shadowOpacity: 1.0
                                        }
                                    }
                                }
                            }

                            // Ollama installed count chip
                            Rectangle {
                                id: installedChip
                                visible: shellViewModel.ollamaModelCount > 0
                                implicitHeight: 22
                                implicitWidth: installedCountLbl.implicitWidth + 16
                                radius: 11
                                color: SentinelTheme.withAlpha(SentinelTheme.success, 0.12)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.success, 0.25)
                                border.width: 1

                                Label {
                                    id: installedCountLbl
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    horizontalAlignment: Text.AlignHCenter
                                    text: "● " + shellViewModel.ollamaModelCount + qsTr(" Ollama installed")
                                    font.pixelSize: SentinelTheme.fontTiny
                                    color: SentinelTheme.success
                                }
                            }

                            // Model count chip
                            Rectangle {
                                id: modelCountChip
                                implicitHeight: 22
                                implicitWidth: countLabel.implicitWidth + 16
                                radius: 11
                                color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.12)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.22)
                                border.width: 1

                                Label {
                                    id: countLabel
                                    anchors.centerIn: parent
                                    text: modelsPage.filteredModels.length + " " + (modelsPage.activeCategory === "All" ? qsTr("models") : modelsPage.activeCategory)
                                    font.pixelSize: SentinelTheme.fontTiny
                                    color: SentinelTheme.accent
                                }
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.topMargin: SentinelTheme.spaceMd
                        text: modelsPage.activeCategory === "Runtime" ? qsTr("Review local inference and voice engines, platform requirements and Sentinel integration. Select a runtime to open its official setup documentation.") : qsTr("Browse Hugging Face repositories and files, Ollama and LM Studio catalogs, or GGUF models for llama.cpp. Select a card for variants and details.")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // Error banner
            Rectangle {
                id: errorBanner
                visible: ollamaLibraryFetcher.errorText.length > 0 && (modelsPage.activeCategory === "All" || modelsPage.activeCategory === "LLM" || modelsPage.activeCategory === "Think" || modelsPage.activeCategory === "Vision")
                Layout.fillWidth: true
                implicitHeight: errLabel.implicitHeight + 20
                color: SentinelTheme.withAlpha(SentinelTheme.errorSurface, 0.4)
                border.color: SentinelTheme.withAlpha("#ef4444", 0.3)
                border.width: 1
                radius: SentinelTheme.radiusLg
                Layout.margins: SentinelTheme.spaceSm

                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.12)
                    shadowVerticalOffset: 2
                    shadowBlur: 0.10
                    shadowOpacity: 1.0
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.spaceSm
                    spacing: SentinelTheme.spaceSm

                    Label {
                        id: errLabel
                        Layout.fillWidth: true
                        text: ollamaLibraryFetcher.errorText
                        color: "#ef4444"
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                    }

                    Button {
                        id: retryBtn
                        implicitHeight: 24
                        implicitWidth: 60
                        flat: true
                        onClicked: modelsPage.fetchOllamaModels(true)
                        contentItem: Label {
                            text: qsTr("Retry")
                            font.pixelSize: SentinelTheme.fontTiny
                            font.weight: Font.Medium
                            color: SentinelTheme.accent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            id: retryBg
                            radius: 12
                            color: retryBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.08) : "transparent"
                            border.color: retryBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.2) : "transparent"

                            layer.enabled: retryBtn.hovered
                            layer.effect: MultiEffect {
                                shadowEnabled: true
                                shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                shadowVerticalOffset: 1
                                shadowBlur: 0.06
                                shadowOpacity: 1.0
                            }
                        }
                    }
                }
            }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: modelsPage.currentModels.length === 0 && !modelsPage.catalogFetching
            z: 1

            EmptyState {
                visible: true
                icon: modelsPage.searchQuery.length > 0 ? "search" : "brain"
                title: modelsPage.searchQuery.length > 0 ? qsTr("No models match your search")
                       : modelsPage.activeCategory === "Installed" ? qsTr("No installed models yet")
                       : qsTr("No models available")
                description: modelsPage.searchQuery.length > 0 ? qsTr("Try a different search term or browse categories.")
                             : modelsPage.activeCategory === "Installed" ? qsTr("Download a model from the catalog to get started.")
                             : qsTr("Refresh the catalog or import a local GGUF model.")
                compact: modelsPage.compact
                anchors.centerIn: parent
            }
        }

        // Shimmer skeleton loading for model grid
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: modelsPage.catalogFetching && modelsPage.currentModels.length === 0
            z: 1

            Flickable {
                anchors.fill: parent
                clip: true
                contentHeight: shimmerColumn.implicitHeight
                interactive: false

            ColumnLayout {
                id: shimmerColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                spacing: SentinelTheme.spaceMd

                Repeater {
                    model: 4

                    Item {
                        Layout.fillWidth: true
                        implicitHeight: 190

                        ShimmerEffect {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            height: 180
                            radius: SentinelTheme.radiusXl
                            active: true
                        }
                    }
                }
            }
        }
        }

        Label {
            Layout.fillWidth: true
            visible: ggufLibraryFetcher.statusText.length > 0 || ggufLibraryFetcher.errorText.length > 0
            text: ggufLibraryFetcher.errorText || ggufLibraryFetcher.statusText
            color: ggufLibraryFetcher.errorText.length > 0 ? SentinelTheme.warning : SentinelTheme.textMuted
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            text: [ggufLibraryFetcher.catalogStatus, ollamaLibraryFetcher.catalogStatus, lmStudioLibraryFetcher.catalogStatus].filter(function(value) { return value.length > 0 }).join("\n")
            color: SentinelTheme.textMuted
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            visible: text.length > 0
            text: [ollamaLibraryFetcher.errorText ? "Ollama: " + ollamaLibraryFetcher.errorText : "", lmStudioLibraryFetcher.errorText ? "LM Studio: " + lmStudioLibraryFetcher.errorText : ""].filter(function(value) { return value.length > 0 }).join("\n")
            color: SentinelTheme.warning
            wrapMode: Text.WordWrap
        }
        ProgressBar { Layout.fillWidth: true; visible: ggufLibraryFetcher.pulling; value: ggufLibraryFetcher.progress }
        BusyIndicator { visible: ggufLibraryFetcher.fetching; running: visible; Layout.alignment: Qt.AlignHCenter }

        Flow {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm
            SentinelButton { visible: modelsPage.activeCategory !== "Runtime" && ["all", "huggingface", "llamacpp"].indexOf(modelsPage.catalogSource) >= 0; text: qsTr("Previous discovery page"); enabled: ggufLibraryFetcher.hasPrevious && !ggufLibraryFetcher.fetching; onClicked: ggufLibraryFetcher.previousPage() }
            SentinelButton { visible: modelsPage.activeCategory !== "Runtime" && ["all", "huggingface", "llamacpp"].indexOf(modelsPage.catalogSource) >= 0; text: qsTr("More from Hugging Face"); enabled: ggufLibraryFetcher.hasMore && !ggufLibraryFetcher.fetching; onClicked: ggufLibraryFetcher.nextPage() }
            SentinelComboBox {
                visible: modelsPage.activeCategory === "Video" || modelsPage.activeCategory === "Image"
                width: Math.min(parent.width, 260)
                model: modelsPage.activeCategory === "Video" ? [qsTr("Text to video"), qsTr("Image to video")] : [qsTr("Text to image"), qsTr("Image to image")]
                currentIndex: modelsPage.discoveryTask.length > 0 ? 1 : 0
                onActivated: modelsPage.discoveryTask = currentIndex === 1 ? (modelsPage.activeCategory === "Video" ? "image-to-video" : "image-to-image") : ""
            }
            SentinelComboBox {
                visible: modelsPage.activeCategory !== "Runtime" && ["all", "huggingface", "llamacpp"].indexOf(modelsPage.catalogSource) >= 0
                width: Math.min(parent.width, 300)
                model: [qsTr("Hugging Face: Most downloaded"), qsTr("Hugging Face: Recently updated")]
                Accessible.name: qsTr("Hugging Face discovery order")
                currentIndex: modelsPage.discoverySort === "lastModified" ? 1 : 0
                onActivated: modelsPage.discoverySort = currentIndex === 1 ? "lastModified" : "downloads"
            }
            SentinelComboBox {
                width: Math.min(parent.width, 300)
                model: [qsTr("List: Installed first"), qsTr("List: Name A–Z"), qsTr("List: Name Z–A"), qsTr("List: Reported downloads"), qsTr("List: Last updated"), qsTr("List: Smallest file")]
                Accessible.name: qsTr("Visible model list order")
                currentIndex: ["installed", "name", "nameDesc", "downloads", "updated", "size"].indexOf(modelsPage.listSort)
                onActivated: modelsPage.listSort = ["installed", "name", "nameDesc", "downloads", "updated", "size"][currentIndex]
            }
            Label {
                width: parent.width
                visible: ["downloads", "updated", "size"].indexOf(modelsPage.listSort) >= 0
                text: qsTr("Entries without reported metadata appear last. Sorting applies to the loaded catalog entries; discovery order selects the next Hugging Face batch.")
                color: SentinelTheme.textMuted
                wrapMode: Text.WordWrap
            }
        }
        // Model grid
        GridView {
            id: modelGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cellWidth: {
                var cols = width < 540 ? 1
                         : width < 900 ? 2
                         : 3
                return Math.floor(width / cols)
            }
            readonly property int columns: Math.max(1, Math.floor(width / cellWidth))
            cellHeight: 210
            model: modelsPage.currentModels
            activeFocusOnTab: true
            keyNavigationWraps: true
            focusPolicy: Qt.StrongFocus

            Keys.onLeftPressed: {
                if (currentIndex % modelGrid.columns > 0)
                    currentIndex = Math.max(0, currentIndex - 1)
            }
            Keys.onRightPressed: {
                if (currentIndex % modelGrid.columns < modelGrid.columns - 1 && currentIndex < count - 1)
                    currentIndex = Math.min(count - 1, currentIndex + 1)
            }
            Keys.onUpPressed: {
                var prev = currentIndex - modelGrid.columns
                if (prev >= 0)
                    currentIndex = prev
            }
            Keys.onDownPressed: {
                var next = currentIndex + modelGrid.columns
                if (next < count)
                    currentIndex = next
            }
            Keys.onReturnPressed: {
                var item = currentItem
                if (item) {
                    item.forceActiveFocus()
                }
            }

            ScrollBar.vertical: ScrollBar {
                id: gridScrollBar
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 4
                    radius: 2
                    color: SentinelTheme.withAlpha(modelsPage.modeAccent, gridScrollBar.active ? 0.34 : 0.18)
                }
                background: Rectangle {
                    color: "transparent"
                }
            }

                add: Transition {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.OutQuad }
                    NumberAnimation { property: "scale";   from: 0.94; to: 1; duration: 220; easing.type: Easing.OutCubic }
                }

                delegate: Item {
                    id: modelDelegate
                    required property var modelData
                    required property int index

                    width: modelGrid.cellWidth
                    height: modelGrid.cellHeight

                    // Detect real installed state + active pull
                    readonly property bool deviceInstalled: modelsPage.isInstalledOnDevice(modelData)
                    readonly property bool activePull: modelsPage.isPulling(modelData.ollamaId)

                    // Reactively track puller progress for this model
                    readonly property real pullProgress: activePull ? ollamaPuller.progress : 0.0
                    readonly property string pullStatus: activePull ? ollamaPuller.statusText : ""

                    // effective installed = on device OR just finished pull
                    readonly property bool effectivelyInstalled: deviceInstalled

                    // ── Card ─────────────────────────────────────────────────
                    Rectangle {
                        id: card
                        anchors {
                            fill: parent
                            margins: SentinelTheme.spaceSm
                        }
                        radius: SentinelTheme.radiusXl

                        color: SentinelTheme.backgroundRaised

                        border.color: modelDelegate.effectivelyInstalled
                                    ? SentinelTheme.withAlpha(SentinelTheme.success, cardArea.containsMouse ? 0.45 : 0.28)
                                    : (cardArea.containsMouse
                                       ? SentinelTheme.withAlpha(modelsPage.modeAccent, 0.35)
                                       : (SentinelTheme.lightTheme
                                          ? SentinelTheme.withAlpha("#e2e8f4", 0.80)
                                          : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)))
                        border.width: 1

                        Behavior on color {
                            ColorAnimation { duration: 140; easing.type: Easing.InOutQuad }
                        }
                        Behavior on border.color {
                            ColorAnimation { duration: 140; easing.type: Easing.InOutQuad }
                        }

                        MouseArea {
                            id: cardArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                detailPopup.modelInfo = modelDelegate.modelData
                                detailPopup.open()
                            }
                        }

                        // ── Content ──────────────────────────────────────────
                        ColumnLayout {
                            anchors { fill: parent; margins: SentinelTheme.cardPadding }
                            spacing: SentinelTheme.spaceXs

                            // Row 1: Badge + Installed + Size
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceSm

                                 Rectangle {
                                    visible: !modelDelegate.effectivelyInstalled
                                    implicitHeight: 20
                                    implicitWidth: badgeLbl.implicitWidth + 12
                                    radius: 10
                                    color: SentinelTheme.withAlpha(badgeColorValue, 0.16)
                                    border.color: SentinelTheme.withAlpha(badgeColorValue, 0.32)
                                    border.width: 1
                                    readonly property color badgeColorValue: modelDelegate.modelData.badgeColor

                                    Label {
                                        id: badgeLbl
                                        anchors.centerIn: parent
                                        text: modelDelegate.modelData.badge
                                        font.pixelSize: SentinelTheme.fontTiny
                                        color: modelDelegate.modelData.badgeColor
                                    }
                                }

                                // Installed badge (on-device)
                                Rectangle {
                                    visible: modelDelegate.effectivelyInstalled
                                    implicitHeight: 20
                                    implicitWidth: devInstalledLbl.implicitWidth + 18
                                    radius: 10
                                    color: SentinelTheme.withAlpha(SentinelTheme.success, 0.14)
                                    border.color: SentinelTheme.withAlpha(SentinelTheme.success, 0.30)
                                    border.width: 1

                                    RowLayout {
                                        anchors.centerIn: parent
                                        spacing: 4
                                        Rectangle { Layout.preferredWidth: 5; Layout.preferredHeight: 5; radius: 3; color: SentinelTheme.success }
                                        Label {
                                            id: devInstalledLbl
                                            text: qsTr("Installed")
                                            font.pixelSize: SentinelTheme.fontTiny
                                            color: SentinelTheme.success
                                        }
                                    }
                                }

                                Item { Layout.fillWidth: true }
                            }

                            // Row 2: Name + Provider
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Label {
                                    Layout.fillWidth: true
                                    text: modelDelegate.modelData.name
                                    font.pixelSize: SentinelTheme.fontControl
                                    font.weight: Font.Medium
                                    color: SentinelTheme.textPrimary
                                    elide: Text.ElideRight
                                }
                                Label {
                                    text: modelDelegate.modelData.provider
                                    font.pixelSize: SentinelTheme.fontTiny
                                    color: SentinelTheme.textPlaceholder
                                }
                            }

                            // Row 3: Description
                            Label {
                                Layout.fillWidth: true
                                text: modelDelegate.modelData.description
                                font.pixelSize: SentinelTheme.fontSmall
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                                Layout.fillHeight: true
                            }



                            // Row 5: Action
                            Item {
                                Layout.fillWidth: true
                                implicitHeight: 32

                                // Download / Details button — opens popup
                                Button {
                                    id: dlBtn
                                    visible: !modelDelegate.activePull && !modelDelegate.effectivelyInstalled
                                    enabled: true
                                    anchors.right: parent.right
                                    implicitHeight: 30
                                    implicitWidth: dlBtnLabel.implicitWidth + 24
                                    hoverEnabled: true
                                    onClicked: {
                                        detailPopup.modelInfo = modelDelegate.modelData
                                        detailPopup.open()
                                    }

                                    scale: dlBtn.down ? 0.97 : (dlBtn.hovered ? 1.02 : 1.0)
                                    Behavior on scale {
                                        NumberAnimation { duration: 100; easing.type: Easing.OutCubic }
                                    }

                                    background: Rectangle {
                                        id: dlBtnBg
                                        radius: height / 2
                                        color: dlBtn.down
                                             ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.32)
                                             : dlBtn.hovered
                                               ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.22)
                                               : SentinelTheme.withAlpha(SentinelTheme.accent, 0.14)
                                        border.color: SentinelTheme.withAlpha(SentinelTheme.accent, dlBtn.hovered ? 0.50 : 0.30)
                                        border.width: 1

                                        layer.enabled: dlBtn.hovered || dlBtn.down
                                        layer.effect: MultiEffect {
                                            shadowEnabled: true
                                            shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.12)
                                            shadowVerticalOffset: 1
                                            shadowBlur: 0.08
                                            shadowOpacity: 1.0
                                        }

                                        Behavior on color { ColorAnimation { duration: 100 } }
                                    }

                                    contentItem: Label {
                                        id: dlBtnLabel
                                        text: modelDelegate.modelData.category === "Runtime" ? qsTr("Setup & compatibility") : qsTr("Details & downloads")
                                        font.pixelSize: SentinelTheme.fontSmall
                                        font.weight: Font.Medium
                                        color: SentinelTheme.accent
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                }

                                // Active pull progress bar
                                ColumnLayout {
                                    visible: modelDelegate.activePull
                                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                                    spacing: 4

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 4; radius: 2
                                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                        Rectangle {
                                            width: parent.width * modelDelegate.pullProgress
                                            height: parent.height; radius: parent.radius
                                            color: SentinelTheme.accent
                                            Behavior on width { NumberAnimation { duration: 80 } }
                                        }
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: modelDelegate.pullStatus.length > 0
                                            ? modelDelegate.pullStatus
                                            : qsTr("Pulling via Ollama… %1%").arg(Math.round(modelDelegate.pullProgress * 100))
                                        font.pixelSize: SentinelTheme.fontTiny
                                        color: SentinelTheme.textPlaceholder
                                        elide: Text.ElideRight
                                    }
                                }

                                // Installed state row
                                RowLayout {
                                    visible: modelDelegate.effectivelyInstalled && !modelDelegate.activePull
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: SentinelTheme.spaceXs

                                    Item { Layout.fillWidth: true }

                                    Button {
                                        id: detailsBtnInstalled
                                        implicitHeight: 24
                                        implicitWidth: detailsInstalledLbl.implicitWidth + 16
                                        flat: true
                                        hoverEnabled: true
                                        onClicked: {
                                            detailPopup.modelInfo = modelDelegate.modelData
                                            detailPopup.open()
                                        }
                                        background: Rectangle {
                                            id: detailsBtnBg
                                            radius: height / 2
                                            color: detailsBtnInstalled.hovered
                                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
                                                 : "transparent"
                                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                            border.width: 1

                                            layer.enabled: detailsBtnInstalled.hovered
                                            layer.effect: MultiEffect {
                                                shadowEnabled: true
                                                shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                                shadowVerticalOffset: 1
                                                shadowBlur: 0.06
                                                shadowOpacity: 1.0
                                            }

                                            Behavior on color { ColorAnimation { duration: 100 } }
                                        }
                                        contentItem: Label {
                                            id: detailsInstalledLbl
                                            text: qsTr("Details")
                                            font.pixelSize: SentinelTheme.fontTiny
                                            color: SentinelTheme.textMuted
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                    }
                                }
                            }
                        }
                    }



                    scale: cardArea.containsMouse ? 1.012 : 1.0
                    Behavior on scale {
                        NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
                    }
                }
            }
        }
    }

    // ── Model Detail Popup ────────────────────────────────────────────────────
    ModelDetailPopup {
        id: detailPopup
        modelInfo: null
        modeName: modelsPage.viewModel ? modelsPage.viewModel.currentModeName : "Sentinel"
        accent: modelInfo ? Qt.color(modelInfo.badgeColor) : modelsPage.modeAccent

        // Installed state is always live (reads shellViewModel directly)
        installed: modelInfo ? modelsPage.isInstalledOnDevice(modelInfo) : false

        // Live pull state for this model
        activePull: modelInfo ? modelsPage.isModelPulling(modelInfo) : false
        pullProgress: activePull ? ollamaPuller.progress : 0.0
        pullStatus:  activePull ? ollamaPuller.statusText : ""
        pullError:   modelInfo && modelsPage.isModelPulling(modelInfo) && !ollamaPuller.pulling
                    ? ollamaPuller.errorText : ""

        onGgufSearchRequested: function(query) {
            modelsPage.activeCategory = "All"
            modelsPage.catalogSource = "llamacpp"
            modelsPage.searchQuery = query
            catalogSearchTimer.restart()
        }
        onDownloadRequested: function(modelId) {
            ollamaPuller.pull(modelId)
        }
        onCancelRequested: {
            ollamaPuller.cancel()
        }
    }


}
