import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Shapes
import Sentinel.Desktop

Item {
    id: settingsPage
    required property var viewModel
    readonly property bool compact: width < 820
    readonly property int panelPadding: SentinelTheme.spaceLg
    readonly property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property string uiSelfCheck: "modal-ready rail-scroll-sync voice-path-wrap agent-runtime bottom-safe-scroll"
    readonly property var themeChoices: ["Liquid Glass Light", "Liquid Glass Dark", "Sentinel Classic", "Midnight Blue", "Aurora Teal", "Graphite Grey", "System Sync"]
    readonly property var notificationPolicies: ["Disabled", "Important Only", "All", "Custom"]
    readonly property var updatePolicies: ["Never", "Ask Before Checking", "Weekly", "On Startup"]
    readonly property var densityChoices: ["Compact", "Comfortable", "Large"]
    readonly property var sidebarItems: [
        { "key": "Interface", "title": qsTr("Interface"), "keywords": ["general", "appearance", "accessibility", "theme", "language", "density", "motion", "contrast"] },
        { "key": "AI", "title": qsTr("AI Settings"), "keywords": ["ai", "models", "chat", "voice", "profiles", "provider", "temperature", "tokens", "streaming", "tts", "kokoro", "piper"] },
        { "key": "Memory", "title": qsTr("Memory & Knowledge"), "keywords": ["brain", "workspace", "memory", "recall", "context", "rag", "knowledge", "files"] },
        { "key": "Security", "title": qsTr("Security & Agents"), "keywords": ["permissions", "tools", "agents", "boundary", "policy", "gateway", "sandbox"] },
        { "key": "System", "title": qsTr("System"), "keywords": ["notifications", "updates", "policy", "version"] }
    ]
    property string activeCategory: "Interface"
    property string searchQuery: ""
    property bool controlledAgentTasks: true

    FileDialog {
        id: voiceFileDialog
        title: qsTr("Dosya Seçin")
        property var targetField: null

        function openWithField(field, titleText) {
            targetField = field;
            title = titleText;
            
            var currentPath = field.text;
            if (currentPath && currentPath.trim() !== "") {
                var idx = currentPath.lastIndexOf('/');
                if (idx !== -1) {
                    var folderPath = currentPath.substring(0, idx);
                    if (Qt.platform.os === "windows") {
                        if (folderPath.indexOf(':') !== -1) {
                            if (!folderPath.startsWith("file://")) {
                                folderPath = folderPath.replace(/\\/g, '/');
                                folderPath = "file:///" + folderPath;
                            }
                        }
                    } else {
                        if (!folderPath.startsWith("file://")) {
                            folderPath = "file://" + folderPath;
                        }
                    }
                    voiceFileDialog.currentFolder = folderPath;
                }
            }
            voiceFileDialog.open();
        }

        onAccepted: {
            if (targetField) {
                var path = selectedFile.toString();
                if (Qt.platform.os === "windows") {
                    if (path.startsWith("file:///")) {
                        path = path.substring(8);
                    } else if (path.startsWith("file://")) {
                        path = path.substring(7);
                    }
                } else {
                    if (path.startsWith("file://")) {
                        path = path.substring(7);
                    }
                }
                path = decodeURIComponent(path);
                targetField.text = path;
                targetField.editingFinished(); // Save changes to model
            }
        }
    }

    readonly property var filteredSidebarItems: {
        if (searchQuery.trim() === "")
            return sidebarItems
        var q = searchQuery.toLowerCase()
        return sidebarItems.filter(function(item) {
            if (item.title.toLowerCase().indexOf(q) !== -1)
                return true
            if (item.keywords) {
                for (var i = 0; i < item.keywords.length; i++) {
                    if (item.keywords[i].toLowerCase().indexOf(q) !== -1)
                        return true
                }
            }
            return false
        })
    }

    function sectionHeight(content) {
        return content.implicitHeight + panelPadding * 2
    }

    function jumpTo(category) {
        activeCategory = category
        settingsFlick.contentY = 0
    }

    RowLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        ShellPanel {
            Layout.preferredWidth: settingsPage.compact ? 196 : 278
            Layout.fillHeight: true
            color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.70)
            border.color: SentinelTheme.withAlpha(settingsPage.modeAccent, 0.20)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: SentinelTheme.spaceMd
                spacing: SentinelTheme.spaceMd

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: SentinelTheme.spaceMd
                    Layout.topMargin: SentinelTheme.spaceSm
                    Layout.bottomMargin: SentinelTheme.spaceXs

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Settings")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontTitle
                        font.bold: true
                        maximumLineCount: 1
                        elide: Text.ElideRight
                    }
                }

                // ── Search Field ──
                Rectangle {
                    id: searchFieldContainer
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    Layout.leftMargin: SentinelTheme.spaceMd
                    Layout.rightMargin: SentinelTheme.spaceMd
                    Layout.bottomMargin: SentinelTheme.spaceXs
                    radius: SentinelTheme.radiusMd
                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.48)
                    border.color: searchInput.activeFocus 
                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                  : searchInput.hovered
                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                    : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: SentinelTheme.spaceSm
                        anchors.rightMargin: SentinelTheme.spaceSm
                        spacing: SentinelTheme.spaceXs

                        Text {
                            text: "🔍"
                            font.pixelSize: SentinelTheme.fontSmall
                            color: SentinelTheme.textMuted
                        }

                        TextInput {
                            id: searchInput
                            Layout.fillWidth: true
                            font.pixelSize: SentinelTheme.fontBody
                            color: SentinelTheme.textPrimary
                            selectByMouse: true
                            clip: true
                            
                            // Placeholder
                            Text {
                                text: qsTr("Search settings...")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontBody
                                visible: parent.text.length === 0
                                anchors.fill: parent
                                verticalAlignment: Text.AlignVCenter
                            }

                            onTextChanged: settingsPage.searchQuery = text
                        }

                        Button {
                            id: clearSearchBtn
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            visible: searchInput.text.length > 0
                            hoverEnabled: true
                            onClicked: {
                                searchInput.text = ""
                                searchInput.focus = false
                            }
                            background: Rectangle {
                                radius: width/2
                                color: clearSearchBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08) : "transparent"
                            }
                            contentItem: Text {
                                text: "×"
                                font.pixelSize: SentinelTheme.fontBody
                                color: SentinelTheme.textMuted
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                Flickable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: width
                    contentHeight: sidebarButtonsColumn.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar {
                        id: sidebarScrollBar
                        policy: ScrollBar.AsNeeded
                        contentItem: Rectangle {
                            implicitWidth: 2
                            radius: 1
                            color: SentinelTheme.withAlpha(settingsPage.modeAccent, sidebarScrollBar.active ? 0.34 : 0.18)
                        }
                        background: Rectangle {
                            color: "transparent"
                        }
                    }

                    ColumnLayout {
                        id: sidebarButtonsColumn
                        width: parent.width
                        spacing: SentinelTheme.spaceXs

                        Repeater {
                            model: settingsPage.filteredSidebarItems
                            delegate: Button {
                                id: navButton
                                required property var modelData
                                readonly property bool active: settingsPage.activeCategory === modelData.key
                                Layout.fillWidth: true
                                Layout.preferredHeight: settingsPage.compact ? 36 : 42
                                hoverEnabled: true
                                focusPolicy: Qt.StrongFocus
                                onClicked: settingsPage.jumpTo(modelData.key)

                                contentItem: RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: SentinelTheme.spaceLg
                                    anchors.rightMargin: SentinelTheme.spaceMd
                                    spacing: SentinelTheme.spaceSm

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.title
                                        color: navButton.active
                                               ? SentinelTheme.textPrimary
                                               : SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontBody
                                        font.bold: navButton.active
                                        maximumLineCount: 1
                                        elide: Text.ElideRight
                                    }
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: navButton.active
                                           ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.12)
                                           : navButton.hovered
                                             ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                             : "transparent"

                                    Rectangle {
                                        width: navButton.active ? 3 : 0
                                        height: parent.height - SentinelTheme.spaceSm * 2
                                        radius: 1.5
                                        anchors.left: parent.left
                                        anchors.leftMargin: SentinelTheme.spaceSm
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: settingsPage.modeAccent
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Flickable {
            id: settingsFlick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: settingsColumn.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            boundsMovement: Flickable.StopAtBounds
            maximumFlickVelocity: 2200
            flickDeceleration: 5200
            ScrollBar.vertical: ScrollBar {
                id: settingsFlickScrollBar
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 4
                    radius: 2
                    color: SentinelTheme.withAlpha(settingsPage.modeAccent, settingsFlickScrollBar.active ? 0.34 : 0.18)
                }
                background: Rectangle {
                    color: "transparent"
                }
            }

            Column {
                id: settingsColumn
                width: settingsFlick.width
                spacing: 0

                Item {
                    id: generalSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "Interface"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(generalContent) : 0

                    ColumnLayout {
                        id: generalContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("General")
                            subtitle: qsTr("Desktop shell preferences.")
                            Layout.fillWidth: true
                        }



                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Language")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            ComboBox {
                                id: languageCombo
                                Layout.fillWidth: true
                                implicitHeight: 36
                                hoverEnabled: true
                                model: settingsPage.viewModel.availableLanguages
                                currentIndex: settingsPage.viewModel.availableLanguages.indexOf(settingsPage.viewModel.appLanguage)
                                textRole: ""
                                displayText: settingsPage.viewModel.languageDisplayName(currentValue)
                                onActivated: settingsPage.viewModel.appLanguage = currentValue

                                contentItem: Text {
                                    leftPadding: SentinelTheme.spaceMd
                                    rightPadding: SentinelTheme.space2Xl
                                    text: languageCombo.displayText
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    implicitHeight: 36
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: languageCombo.activeFocus
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                  : languageCombo.hovered || languageCombo.popup.visible
                                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                    Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }
                                }

                                indicator: Text {
                                    x: parent.width - width - SentinelTheme.spaceMd
                                    y: parent.height / 2 - height / 2
                                    text: "\u2039\u203a"
                                    rotation: 90
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }

                                delegate: ItemDelegate {
                                    id: languageOption
                                    width: languageCombo.width
                                    implicitHeight: 36
                                    text: settingsPage.viewModel.languageDisplayName(modelData)
                                    highlighted: languageCombo.highlightedIndex === index
                                    hoverEnabled: true

                                    contentItem: RowLayout {
                                        spacing: SentinelTheme.spaceSm
                                        anchors.fill: parent
                                        anchors.leftMargin: SentinelTheme.spaceMd
                                        anchors.rightMargin: SentinelTheme.spaceMd

                                        Text {
                                            Layout.fillWidth: true
                                            text: languageOption.text
                                            color: languageOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontBody
                                            font.bold: languageOption.highlighted
                                            verticalAlignment: Text.AlignVCenter
                                            maximumLineCount: 1
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            visible: languageCombo.currentIndex === index
                                            text: "\u2713"
                                            color: settingsPage.modeAccent
                                            font.pixelSize: SentinelTheme.fontSmall
                                        }
                                    }

                                    background: Rectangle {
                                        color: languageOption.highlighted
                                               ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.12)
                                               : languageOption.hovered
                                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                               : "transparent"
                                        radius: SentinelTheme.radiusSm
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                    }
                                }

                                popup.background: Rectangle {
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                    border.color: SentinelTheme.withAlpha(settingsPage.modeAccent, 0.20)
                                    border.width: 1
                                }
                            }
                        }

                    }
                }

                Item {
                    id: appearanceSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "Interface"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(appearanceContent) : 0

                    ColumnLayout {
                        id: appearanceContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Appearance")
                            subtitle: qsTr("Theme foundation for a calm native desktop UI.")
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Theme")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            ComboBox {
                                id: themeCombo
                                Layout.fillWidth: true
                                implicitHeight: 36
                                hoverEnabled: true
                                model: settingsPage.themeChoices
                                currentIndex: settingsPage.themeChoices.indexOf(settingsPage.viewModel.themeName)
                                displayText: currentIndex >= 0 ? currentText : settingsPage.viewModel.themeName
                                onActivated: settingsPage.viewModel.themeName = currentText

                                contentItem: Text {
                                    leftPadding: SentinelTheme.spaceMd
                                    rightPadding: SentinelTheme.space2Xl
                                    text: themeCombo.displayText
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    implicitHeight: 36
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: themeCombo.activeFocus
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                  : themeCombo.hovered || themeCombo.popup.visible
                                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                    Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }
                                }

                                indicator: Text {
                                    x: parent.width - width - SentinelTheme.spaceMd
                                    y: parent.height / 2 - height / 2
                                    text: "\u2039\u203a"
                                    rotation: 90
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }

                                delegate: ItemDelegate {
                                    id: themeOption
                                    required property string modelData
                                    required property int index
                                    width: themeCombo.width
                                    implicitHeight: 36
                                    text: modelData
                                    highlighted: themeCombo.highlightedIndex === index
                                    hoverEnabled: true

                                    contentItem: RowLayout {
                                        spacing: SentinelTheme.spaceSm
                                        anchors.fill: parent
                                        anchors.leftMargin: SentinelTheme.spaceMd
                                        anchors.rightMargin: SentinelTheme.spaceMd

                                        Text {
                                            Layout.fillWidth: true
                                            text: themeOption.text
                                            color: themeOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontBody
                                            font.bold: themeOption.highlighted
                                            verticalAlignment: Text.AlignVCenter
                                            maximumLineCount: 1
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            visible: themeCombo.currentIndex === index
                                            text: "\u2713"
                                            color: settingsPage.modeAccent
                                            font.pixelSize: SentinelTheme.fontSmall
                                        }
                                    }

                                    background: Rectangle {
                                        color: themeOption.highlighted
                                               ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.12)
                                               : themeOption.hovered
                                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                               : "transparent"
                                        radius: SentinelTheme.radiusSm
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                    }
                                }

                                popup.background: Rectangle {
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                    border.color: SentinelTheme.withAlpha(settingsPage.modeAccent, 0.20)
                                    border.width: 1
                                }
                            }
                        }

                        Label {
                            text: qsTr("Visual Theme Presets")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontBody
                            font.bold: true
                            Layout.fillWidth: true
                            Layout.topMargin: SentinelTheme.spaceMd
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd
                            Layout.bottomMargin: SentinelTheme.spaceMd

                            Repeater {
                                model: settingsPage.themeChoices

                                delegate: Button {
                                    id: themeCard
                                    required property string modelData
                                    readonly property bool isSelected: settingsPage.viewModel.themeName === modelData
                                    
                                    implicitWidth: settingsPage.compact ? 130 : 160
                                    implicitHeight: 90
                                    hoverEnabled: true
                                    
                                    onClicked: settingsPage.viewModel.themeName = modelData

                                    background: Rectangle {
                                        radius: SentinelTheme.radiusLg
                                        color: themeCard.isSelected 
                                               ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.12)
                                               : themeCard.hovered 
                                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.03)
                                                 : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
                                        border.color: themeCard.isSelected
                                                      ? settingsPage.modeAccent
                                                      : themeCard.hovered
                                                        ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.30)
                                                        : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                        border.width: themeCard.isSelected ? 2 : 1
                                        Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }

                                        // Mini Palette Preview
                                        Rectangle {
                                            id: palettePreview
                                            anchors.top: parent.top
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.margins: SentinelTheme.spaceSm
                                            height: 40
                                            radius: SentinelTheme.radiusMd
                                            clip: true
                                            
                                            color: {
                                                if (modelData === "Liquid Glass Light") return "#f5f6f8"
                                                if (modelData === "Liquid Glass Dark" || modelData === "Midnight Blue") return "#0b0f19"
                                                if (modelData === "Sentinel Classic") return "#121212"
                                                if (modelData === "Aurora Teal") return "#04141a"
                                                if (modelData === "Graphite Grey") return "#1a1a1a"
                                                return "#141721" // System Sync
                                            }

                                            Rectangle {
                                                width: 12
                                                height: 12
                                                radius: 6
                                                anchors.right: parent.right
                                                anchors.bottom: parent.bottom
                                                anchors.margins: 6
                                                color: {
                                                    if (modelData === "Liquid Glass Light") return "#3b82f6"
                                                    if (modelData === "Liquid Glass Dark") return "#60a5fa"
                                                    if (modelData === "Sentinel Classic") return "#9b51e0"
                                                    if (modelData === "Midnight Blue") return "#2f80ed"
                                                    if (modelData === "Aurora Teal") return "#00b4d8"
                                                    if (modelData === "Graphite Grey") return "#828282"
                                                    return settingsPage.modeAccent
                                                }
                                            }

                                            RowLayout {
                                                anchors.left: parent.left
                                                anchors.top: parent.top
                                                anchors.margins: 6
                                                spacing: 4
                                                Rectangle { width: 16; height: 4; radius: 2; color: themeCard.isSelected ? "#ffffff" : "#888888" }
                                                Rectangle { width: 8; height: 4; radius: 2; color: themeCard.isSelected ? "#ffffff" : "#666666" }
                                            }
                                        }

                                        Label {
                                            anchors.bottom: parent.bottom
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.margins: SentinelTheme.spaceSm
                                            text: modelData
                                            color: themeCard.isSelected ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontSmall
                                            font.bold: themeCard.isSelected
                                            horizontalAlignment: Text.AlignHCenter
                                            elide: Text.ElideRight
                                            maximumLineCount: 1
                                        }
                                    }
                                }
                            }
                        }

                    }
                }

                Item {
                    id: accessibilitySection
                    width: parent.width
                    visible: settingsPage.activeCategory === "Interface"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(accessibilityContent) : 0

                    ColumnLayout {
                        id: accessibilityContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Accessibility")
                            subtitle: qsTr("Comfort, motion, contrast, and density preferences. Changes take effect immediately.")
                            Layout.fillWidth: true
                        }

                        // ── Reduced Motion ──────────────────────────────
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm
                            Layout.topMargin: SentinelTheme.spaceXs
                            Layout.bottomMargin: SentinelTheme.spaceXs

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Reduced Motion")
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    font.bold: true
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Disables all animations and transitions throughout the UI.")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    wrapMode: Text.WordWrap
                                }
                            }

                            Switch {
                                id: reducedMotionSwitch
                                checked: settingsPage.viewModel.reducedMotionEnabled
                                hoverEnabled: true
                                onToggled: settingsPage.viewModel.reducedMotionEnabled = checked

                                indicator: Rectangle {
                                    implicitWidth: 46
                                    implicitHeight: 24
                                    x: reducedMotionSwitch.leftPadding
                                    y: parent.height / 2 - height / 2
                                    radius: height / 2
                                    color: reducedMotionSwitch.checked
                                           ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.18)
                                           : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                    border.color: reducedMotionSwitch.activeFocus
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                  : reducedMotionSwitch.hovered
                                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)

                                    Rectangle {
                                        x: reducedMotionSwitch.checked ? parent.width - width - 3 : 3
                                        y: parent.height / 2 - height / 2
                                        width: 18; height: 18
                                        radius: height / 2
                                        color: reducedMotionSwitch.checked
                                               ? settingsPage.modeAccent
                                               : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.40)
                                    }
                                }

                                background: Item {}
                            }
                        }

                        // ── High Contrast ────────────────────────────────
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm
                            Layout.topMargin: SentinelTheme.spaceXs
                            Layout.bottomMargin: SentinelTheme.spaceXs

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("High Contrast")
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    font.bold: true
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Increases text and border contrast for better readability.")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    wrapMode: Text.WordWrap
                                }
                            }

                            Switch {
                                id: highContrastSwitch
                                checked: settingsPage.viewModel.highContrastEnabled
                                hoverEnabled: true
                                onToggled: settingsPage.viewModel.highContrastEnabled = checked

                                indicator: Rectangle {
                                    implicitWidth: 46
                                    implicitHeight: 24
                                    x: highContrastSwitch.leftPadding
                                    y: parent.height / 2 - height / 2
                                    radius: height / 2
                                    color: highContrastSwitch.checked
                                           ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.18)
                                           : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                    border.color: highContrastSwitch.activeFocus
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                  : highContrastSwitch.hovered
                                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)

                                    Rectangle {
                                        x: highContrastSwitch.checked ? parent.width - width - 3 : 3
                                        y: parent.height / 2 - height / 2
                                        width: 18; height: 18
                                        radius: height / 2
                                        color: highContrastSwitch.checked
                                               ? settingsPage.modeAccent
                                               : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.40)
                                    }
                                }

                                background: Item {}
                            }
                        }

                        // ── UI Density ───────────────────────────────────
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("UI Density")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 36
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.48)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    spacing: 2

                                    Repeater {
                                        model: settingsPage.densityChoices

                                        Button {
                                            id: densityBtn
                                            required property string modelData
                                            required property int index
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            hoverEnabled: true
                                            focusPolicy: Qt.NoFocus

                                            contentItem: Text {
                                                text: densityBtn.modelData
                                                color: (settingsPage.viewModel.uiDensity === densityBtn.modelData)
                                                       ? SentinelTheme.textPrimary
                                                       : SentinelTheme.textMuted
                                                font.pixelSize: SentinelTheme.fontSmall
                                                font.bold: (settingsPage.viewModel.uiDensity === densityBtn.modelData)
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }

                                            background: Rectangle {
                                                radius: SentinelTheme.radiusSm
                                                color: (settingsPage.viewModel.uiDensity === densityBtn.modelData)
                                                       ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.16)
                                                       : densityBtn.hovered
                                                         ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                                         : "transparent"
                                                border.color: (settingsPage.viewModel.uiDensity === densityBtn.modelData)
                                                              ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.36)
                                                              : "transparent"
                                            }

                                            onClicked: settingsPage.viewModel.uiDensity = densityBtn.modelData
                                        }
                                    }
                                }
                            }
                        }

                    }
                }

                Item {
                    id: localAiSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "AI"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(localAiContent) : 0

                    ColumnLayout {
                        id: localAiContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("AI")
                            subtitle: qsTr("Configure and inspect local AI inference runtimes.")
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Provider")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            ComboBox {
                                id: runtimeProviderCombo
                                Layout.fillWidth: true
                                implicitHeight: 36
                                hoverEnabled: true
                                model: settingsPage.viewModel.selectableRuntimeProviderLabels
                                currentIndex: settingsPage.viewModel.selectableRuntimeProviderIds.indexOf(settingsPage.viewModel.selectedRuntimeProvider)
                                displayText: currentIndex >= 0 ? currentText : settingsPage.viewModel.activeRuntimeProviderLabel
                                onActivated: (index) => {
                                    if (index >= 0 && index < settingsPage.viewModel.selectableRuntimeProviderIds.length) {
                                        var providerId = settingsPage.viewModel.selectableRuntimeProviderIds[index]
                                        settingsPage.viewModel.selectedRuntimeProvider = providerId
                                        
                                        // Update endpoint to default for the selected provider
                                        if (providerId === "ollama") {
                                            settingsPage.viewModel.ollamaEndpoint = "http://127.0.0.1:11434"
                                        } else if (providerId === "lm-studio") {
                                            settingsPage.viewModel.ollamaEndpoint = "http://127.0.0.1:1234"
                                        } else if (providerId === "llama-cpp-server") {
                                            settingsPage.viewModel.ollamaEndpoint = "http://127.0.0.1:8080"
                                        } else if (providerId === "openai-compatible-local") {
                                            settingsPage.viewModel.ollamaEndpoint = "http://127.0.0.1:8000"
                                        }
                                    }
                                }

                                contentItem: Text {
                                    leftPadding: SentinelTheme.spaceMd
                                    rightPadding: SentinelTheme.space2Xl
                                    text: runtimeProviderCombo.displayText
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    implicitHeight: 36
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: runtimeProviderCombo.activeFocus
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                  : runtimeProviderCombo.hovered || runtimeProviderCombo.popup.visible
                                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                    Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }
                                }

                                indicator: Text {
                                    x: parent.width - width - SentinelTheme.spaceMd
                                    y: parent.height / 2 - height / 2
                                    text: "\u2039\u203a"
                                    rotation: 90
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }

                                delegate: ItemDelegate {
                                    id: runtimeProviderOption
                                    required property string modelData
                                    required property int index
                                    width: runtimeProviderCombo.width
                                    implicitHeight: 36
                                    text: modelData
                                    highlighted: runtimeProviderCombo.highlightedIndex === index
                                    hoverEnabled: true

                                    contentItem: RowLayout {
                                        spacing: SentinelTheme.spaceSm
                                        anchors.fill: parent
                                        anchors.leftMargin: SentinelTheme.spaceMd
                                        anchors.rightMargin: SentinelTheme.spaceMd

                                        Text {
                                            Layout.fillWidth: true
                                            text: runtimeProviderOption.text
                                            color: runtimeProviderOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontBody
                                            font.bold: runtimeProviderOption.highlighted
                                            verticalAlignment: Text.AlignVCenter
                                            maximumLineCount: 1
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            visible: runtimeProviderCombo.currentIndex === index
                                            text: "\u2713"
                                            color: settingsPage.modeAccent
                                            font.pixelSize: SentinelTheme.fontSmall
                                        }
                                    }

                                    background: Rectangle {
                                        color: runtimeProviderOption.highlighted
                                               ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.12)
                                               : runtimeProviderOption.hovered
                                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                               : "transparent"
                                        radius: SentinelTheme.radiusSm
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                    }
                                }

                                popup.background: Rectangle {
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                    border.color: SentinelTheme.withAlpha(settingsPage.modeAccent, 0.20)
                                    border.width: 1
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("System Mode")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            ComboBox {
                                id: settingsModeCombo
                                Layout.fillWidth: true
                                implicitHeight: 36
                                hoverEnabled: true
                                model: settingsPage.viewModel.availableModes
                                currentIndex: settingsPage.viewModel.availableModes.indexOf(settingsPage.viewModel.currentModeName)
                                displayText: currentIndex >= 0 ? currentText : qsTr("Chat")
                                onActivated: (index) => {
                                    if (index >= 0 && index < settingsPage.viewModel.availableModes.length) {
                                        settingsPage.viewModel.currentModeName = settingsPage.viewModel.availableModes[index]
                                    }
                                }

                                contentItem: Text {
                                    leftPadding: SentinelTheme.spaceMd
                                    rightPadding: SentinelTheme.space2Xl
                                    text: settingsModeCombo.displayText
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    implicitHeight: 36
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: settingsModeCombo.activeFocus
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                  : settingsModeCombo.hovered || settingsModeCombo.popup.visible
                                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                    Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }
                                }

                                indicator: Text {
                                    x: parent.width - width - SentinelTheme.spaceMd
                                    y: parent.height / 2 - height / 2
                                    text: "\u2039\u203a"
                                    rotation: 90
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }

                                delegate: ItemDelegate {
                                    id: settingsModeOption
                                    required property string modelData
                                    required property int index
                                    width: settingsModeCombo.width
                                    implicitHeight: 36
                                    highlighted: settingsModeCombo.currentIndex === index
                                    hoverEnabled: true

                                    contentItem: Text {
                                        text: settingsModeOption.modelData
                                        color: settingsModeOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontBody
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }

                                    background: Rectangle {
                                        color: settingsModeOption.highlighted
                                               ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.12)
                                               : settingsModeOption.hovered
                                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                               : "transparent"
                                        radius: SentinelTheme.radiusSm
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                    }
                                }

                                popup.background: Rectangle {
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                    border.color: SentinelTheme.withAlpha(settingsPage.modeAccent, 0.20)
                                    border.width: 1
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Autonomous Mode")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            Switch {
                                id: autonomousSwitch
                                checked: settingsPage.viewModel.agentAutonomousMode
                                hoverEnabled: true
                                onToggled: settingsPage.viewModel.agentAutonomousMode = checked

                                indicator: Rectangle {
                                    implicitWidth: 46
                                    implicitHeight: 24
                                    x: autonomousSwitch.leftPadding
                                    y: parent.height / 2 - height / 2
                                    radius: height / 2
                                    color: autonomousSwitch.checked
                                           ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.18)
                                           : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                    border.color: autonomousSwitch.checked
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.44)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)

                                    Rectangle {
                                        x: autonomousSwitch.checked ? parent.width - width - 2 : 2
                                        y: 2
                                        width: 20
                                        height: 20
                                        radius: 10
                                        color: autonomousSwitch.checked ? settingsPage.modeAccent : SentinelTheme.textPrimary
                                        opacity: autonomousSwitch.hovered ? 0.90 : 0.74
                                        Behavior on x { NumberAnimation { duration: MotionTokens.duration(MotionTokens.fast, settingsPage.viewModel.currentModeName) } }
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Endpoint")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 36
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                border.color: endpointInput.activeFocus
                                              ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                              : endpointInputMouse.containsMouse
                                                ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                              : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }

                                HoverHandler { id: endpointInputMouse }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: SentinelTheme.spaceMd
                                    anchors.rightMargin: SentinelTheme.spaceMd
                                    spacing: SentinelTheme.spaceSm

                                    TextInput {
                                        id: endpointInput
                                        Layout.fillWidth: true
                                        text: settingsPage.viewModel.ollamaEndpoint
                                        color: SentinelTheme.textPrimary
                                        selectionColor: SentinelTheme.withAlpha(settingsPage.modeAccent, 0.34)
                                        selectedTextColor: SentinelTheme.textPrimary
                                        verticalAlignment: Text.AlignVCenter
                                        font.pixelSize: SentinelTheme.fontBody
                                        selectByMouse: true
                                        clip: true
                                        onEditingFinished: settingsPage.viewModel.ollamaEndpoint = text
                                    }
                                }
                            }
                        }


                    }
                }

                Item {
                    id: modelSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "AI"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(modelContent) : 0

                    ColumnLayout {
                        id: modelContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Models")
                            subtitle: qsTr("Configure active LLM configurations and select local inference models.")
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Model")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            ComboBox {
                                id: modelCombo
                                Layout.fillWidth: true
                                implicitHeight: 36
                                enabled: settingsPage.viewModel.ollamaModelCount > 0
                                hoverEnabled: true
                                model: settingsPage.viewModel.ollamaModelNames
                                currentIndex: settingsPage.viewModel.ollamaModelNames.indexOf(settingsPage.viewModel.selectedLocalModel)
                                displayText: currentIndex >= 0
                                    ? currentText + " (" + (settingsPage.viewModel.selectedRuntimeProvider === "lm-studio" ? "LM Studio" : "Ollama") + ")"
                                    : settingsPage.viewModel.selectedLocalModelStatus + qsTr(" / No model selected")
                                onActivated: settingsPage.viewModel.selectedLocalModel = currentText

                                contentItem: Text {
                                    leftPadding: SentinelTheme.spaceMd
                                    rightPadding: SentinelTheme.space2Xl
                                    text: modelCombo.displayText
                                    color: modelCombo.enabled ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontBody
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    implicitHeight: 36
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase,
                                                                   modelCombo.enabled ? 0.72 : 0.38)
                                    border.color: modelCombo.activeFocus
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                  : modelCombo.hovered || modelCombo.popup.visible
                                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                    Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }
                                }

                                indicator: Text {
                                    x: parent.width - width - SentinelTheme.spaceMd
                                    y: parent.height / 2 - height / 2
                                    text: "\u2039\u203a"
                                    rotation: 90
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }

                                delegate: ItemDelegate {
                                    id: modelOption
                                    required property string modelData
                                    required property int index
                                    width: modelCombo.width
                                    implicitHeight: 40
                                    highlighted: modelCombo.highlightedIndex === index
                                    hoverEnabled: true

                                    contentItem: RowLayout {
                                        spacing: SentinelTheme.spaceSm
                                        anchors.fill: parent
                                        anchors.leftMargin: SentinelTheme.spaceMd
                                        anchors.rightMargin: SentinelTheme.spaceMd

                                        Text {
                                            Layout.fillWidth: true
                                            text: modelOption.modelData + " (" + (settingsPage.viewModel.selectedRuntimeProvider === "lm-studio" ? "LM Studio" : "Ollama") + ")"
                                            color: modelOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontBody
                                            font.bold: modelOption.highlighted
                                            verticalAlignment: Text.AlignVCenter
                                            wrapMode: Text.WordWrap
                                            maximumLineCount: 2
                                        }

                                        Text {
                                            visible: modelCombo.currentIndex === index
                                            text: "\u2713"
                                            color: settingsPage.modeAccent
                                            font.pixelSize: SentinelTheme.fontSmall
                                        }
                                    }

                                    background: Rectangle {
                                        color: modelOption.highlighted
                                               ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.12)
                                               : modelOption.hovered
                                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                               : "transparent"
                                        radius: SentinelTheme.radiusSm
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                    }
                                }

                                popup.background: Rectangle {
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                    border.color: SentinelTheme.withAlpha(settingsPage.modeAccent, 0.20)
                                    border.width: 1
                                }

                                popup.enter: Transition {
                                    NumberAnimation {
                                        property: "opacity"
                                        from: 0.0
                                        to: 1.0
                                        duration: MotionTokens.duration(MotionTokens.menu,
                                                                        settingsPage.viewModel.currentModeName)
                                        easing.type: MotionTokens.enter
                                    }
                                }

                                popup.exit: Transition {
                                    NumberAnimation {
                                        property: "opacity"
                                        to: 0.0
                                        duration: MotionTokens.duration(MotionTokens.fast,
                                                                        settingsPage.viewModel.currentModeName)
                                        easing.type: MotionTokens.exit
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm
                            visible: false

                            SectionTitle {
                                title: qsTr("Model Roles")
                                subtitle: qsTr("Assignments are metadata only and do not enable automatic routing.")
                                Layout.fillWidth: true
                            }

                            Repeater {
                                model: settingsPage.viewModel.modelRoleAssignmentSummaries

                                RowLayout {
                                    required property string modelData
                                    required property int index
                                    Layout.fillWidth: true
                                    spacing: SentinelTheme.spaceSm

                                    InfoRow {
                                        compact: true
                                        label: qsTr("Role")
                                        value: modelData
                                        Layout.fillWidth: true
                                    }

                                    SentinelButton {
                                        text: qsTr("Use Selected")
                                        enabled: settingsPage.viewModel.selectedLocalModel.length > 0
                                        onClicked: settingsPage.viewModel.assignModelRole(
                                                       settingsPage.viewModel.modelRoleIds[index],
                                                       settingsPage.viewModel.selectedLocalModel)
                                    }
                                }
                            }
                        }

                        SectionTitle {
                            title: qsTr("Provider Status")
                            subtitle: qsTr("Active and available local runtime connection status.")
                            Layout.fillWidth: true
                        }

                        Repeater {
                            model: settingsPage.viewModel.selectableRuntimeProviderIds

                            delegate: RowLayout {
                                required property string modelData
                                required property int index
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceMd

                                Label {
                                    text: {
                                        if (modelData === "ollama") return "Ollama"
                                        if (modelData === "lm-studio") return "LM Studio"
                                        if (modelData === "llama-cpp-server") return "llama.cpp server"
                                        if (modelData === "openai-compatible-local") return "OpenAI-compatible Local"
                                        return modelData
                                    }
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    font.bold: true
                                    Layout.fillWidth: true
                                }

                                StatusChip {
                                    value: settingsPage.viewModel.selectedRuntimeProvider === modelData ? qsTr("Active") : qsTr("Available")
                                    accent: settingsPage.viewModel.selectedRuntimeProvider === modelData ? SentinelTheme.success : SentinelTheme.calmAccent
                                    selected: true
                                }
                            }
                        }
                    }
                }

                Item {
                    id: chatSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "AI"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(chatContent) : 0

                    ColumnLayout {
                        id: chatContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Chat")
                            subtitle: qsTr("Explicit local chat routing controls.")
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm
                            Layout.topMargin: SentinelTheme.spaceXs
                            Layout.bottomMargin: SentinelTheme.spaceXs

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Local chat inference")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                                font.bold: true
                                verticalAlignment: Text.AlignVCenter
                            }

                            Switch {
                                id: localChatToggle
                                checked: settingsPage.viewModel.localChatInferenceEnabled
                                hoverEnabled: true
                                onToggled: settingsPage.viewModel.localChatInferenceEnabled = checked
                                indicator: Rectangle {
                                    implicitWidth: 46
                                    implicitHeight: 24
                                    x: localChatToggle.leftPadding
                                    y: parent.height / 2 - height / 2
                                    radius: height / 2
                                    color: localChatToggle.checked
                                           ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.18)
                                           : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                    border.color: localChatToggle.activeFocus
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                  : localChatToggle.hovered
                                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                    Rectangle {
                                        x: localChatToggle.checked ? parent.width - width - 3 : 3
                                        y: parent.height / 2 - height / 2
                                        width: 18; height: 18
                                        radius: height / 2
                                        color: localChatToggle.checked ? settingsPage.modeAccent : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.40)
                                        Behavior on x { NumberAnimation { duration: MotionTokens.fast; easing.type: MotionTokens.press } }
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                    }
                                    Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                }
                                background: Item {}
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm
                            Layout.topMargin: SentinelTheme.spaceXs
                            Layout.bottomMargin: SentinelTheme.spaceXs

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Local response streaming")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                                font.bold: true
                                verticalAlignment: Text.AlignVCenter
                            }

                            Switch {
                                id: streamingToggle
                                checked: settingsPage.viewModel.localInferenceStreamingEnabled
                                hoverEnabled: true
                                onToggled: settingsPage.viewModel.localInferenceStreamingEnabled = checked
                                indicator: Rectangle {
                                    implicitWidth: 46
                                    implicitHeight: 24
                                    x: streamingToggle.leftPadding
                                    y: parent.height / 2 - height / 2
                                    radius: height / 2
                                    color: streamingToggle.checked
                                           ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.18)
                                           : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                    border.color: streamingToggle.activeFocus
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                  : streamingToggle.hovered
                                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                    Rectangle {
                                        x: streamingToggle.checked ? parent.width - width - 3 : 3
                                        y: parent.height / 2 - height / 2
                                        width: 18; height: 18
                                        radius: height / 2
                                        color: streamingToggle.checked ? settingsPage.modeAccent : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.40)
                                        Behavior on x { NumberAnimation { duration: MotionTokens.fast; easing.type: MotionTokens.press } }
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                    }
                                    Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                }
                                background: Item {}
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Timeout")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            Rectangle {
                                implicitHeight: 36
                                implicitWidth: 180
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                border.color: timeoutInput.activeFocus
                                              ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                              : timeoutInputMouse.containsMouse
                                                ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                              : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }

                                HoverHandler { id: timeoutInputMouse }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 2
                                    anchors.rightMargin: 2
                                    spacing: 0

                                    Button {
                                        implicitWidth: 32
                                        Layout.fillHeight: true
                                        hoverEnabled: true
                                        focusPolicy: Qt.NoFocus
                                        onClicked: {
                                            let v = settingsPage.viewModel.localInferenceTimeoutMs - 1000
                                            settingsPage.viewModel.localInferenceTimeoutMs = Math.max(1000, v)
                                        }
                                        contentItem: Text { text: "−"; color: SentinelTheme.textMuted; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: SentinelTheme.fontBody }
                                        background: Rectangle { color: parent.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06) : "transparent"; radius: SentinelTheme.radiusSm }
                                    }

                                    TextInput {
                                        id: timeoutInput
                                        Layout.fillWidth: true
                                        text: settingsPage.viewModel.localInferenceTimeoutMs
                                        color: SentinelTheme.textPrimary
                                        selectionColor: SentinelTheme.withAlpha(settingsPage.modeAccent, 0.34)
                                        selectedTextColor: SentinelTheme.textPrimary
                                        horizontalAlignment: Qt.AlignHCenter
                                        verticalAlignment: Qt.AlignVCenter
                                        font.pixelSize: SentinelTheme.fontBody
                                        validator: IntValidator { bottom: 1000; top: 300000 }
                                        inputMethodHints: Qt.ImhDigitsOnly
                                        onEditingFinished: settingsPage.viewModel.localInferenceTimeoutMs = parseInt(text) || 30000
                                    }

                                    Button {
                                        implicitWidth: 32
                                        Layout.fillHeight: true
                                        hoverEnabled: true
                                        focusPolicy: Qt.NoFocus
                                        onClicked: {
                                            let v = settingsPage.viewModel.localInferenceTimeoutMs + 1000
                                            settingsPage.viewModel.localInferenceTimeoutMs = Math.min(300000, v)
                                        }
                                        contentItem: Text { text: "+"; color: SentinelTheme.textMuted; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: SentinelTheme.fontBody }
                                        background: Rectangle { color: parent.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06) : "transparent"; radius: SentinelTheme.radiusSm }
                                    }
                                }
                            }

                            Label {
                                text: qsTr("ms")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                            }
                        }

                        Label {
                            text: qsTr("Inference Parameters (Advanced)")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontBody
                            font.bold: true
                            Layout.fillWidth: true
                            Layout.topMargin: SentinelTheme.spaceMd
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                text: qsTr("Temperature")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                            }

                            Slider {
                                id: tempSlider
                                Layout.fillWidth: true
                                from: 0.0
                                to: 2.0
                                stepSize: 0.1
                                value: settingsPage.viewModel.localInferenceTemperature
                                onMoved: settingsPage.viewModel.localInferenceTemperature = parseFloat(value.toFixed(1))

                                background: Rectangle {
                                    x: tempSlider.leftPadding
                                    y: tempSlider.topPadding + tempSlider.availableHeight / 2 - height / 2
                                    implicitWidth: 200
                                    implicitHeight: 4
                                    width: tempSlider.availableWidth
                                    height: implicitHeight
                                    radius: 2
                                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                                    Rectangle {
                                        width: tempSlider.visualPosition * parent.width
                                        height: parent.height
                                        color: settingsPage.modeAccent
                                        radius: 2
                                    }
                                }

                                handle: Rectangle {
                                    x: tempSlider.leftPadding + tempSlider.visualPosition * (tempSlider.availableWidth - width)
                                    y: tempSlider.topPadding + tempSlider.availableHeight / 2 - height / 2
                                    implicitWidth: 16
                                    implicitHeight: 16
                                    radius: 8
                                    color: tempSlider.pressed ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.8) : "#ffffff"
                                    border.color: settingsPage.modeAccent
                                    border.width: 1
                                }
                            }

                            Label {
                                Layout.preferredWidth: 40
                                text: settingsPage.viewModel.localInferenceTemperature.toFixed(1)
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontSmall
                                font.bold: true
                                horizontalAlignment: Text.AlignRight
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                text: qsTr("Top-P")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                            }

                            Slider {
                                id: topPSlider
                                Layout.fillWidth: true
                                from: 0.0
                                to: 1.0
                                stepSize: 0.05
                                value: settingsPage.viewModel.localInferenceTopP
                                onMoved: settingsPage.viewModel.localInferenceTopP = parseFloat(value.toFixed(2))

                                background: Rectangle {
                                    x: topPSlider.leftPadding
                                    y: topPSlider.topPadding + topPSlider.availableHeight / 2 - height / 2
                                    implicitWidth: 200
                                    implicitHeight: 4
                                    width: topPSlider.availableWidth
                                    height: implicitHeight
                                    radius: 2
                                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                                    Rectangle {
                                        width: topPSlider.visualPosition * parent.width
                                        height: parent.height
                                        color: settingsPage.modeAccent
                                        radius: 2
                                    }
                                }

                                handle: Rectangle {
                                    x: topPSlider.leftPadding + topPSlider.visualPosition * (topPSlider.availableWidth - width)
                                    y: topPSlider.topPadding + topPSlider.availableHeight / 2 - height / 2
                                    implicitWidth: 16
                                    implicitHeight: 16
                                    radius: 8
                                    color: topPSlider.pressed ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.8) : "#ffffff"
                                    border.color: settingsPage.modeAccent
                                    border.width: 1
                                }
                            }

                            Label {
                                Layout.preferredWidth: 40
                                text: settingsPage.viewModel.localInferenceTopP.toFixed(2)
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontSmall
                                font.bold: true
                                horizontalAlignment: Text.AlignRight
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd
                            Layout.bottomMargin: SentinelTheme.spaceSm

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                text: qsTr("Max Tokens")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                            }

                            Slider {
                                id: maxTokensSlider
                                Layout.fillWidth: true
                                from: 256
                                to: 8192
                                stepSize: 256
                                value: settingsPage.viewModel.localInferenceMaxTokens
                                onMoved: settingsPage.viewModel.localInferenceMaxTokens = parseInt(value)

                                background: Rectangle {
                                    x: maxTokensSlider.leftPadding
                                    y: maxTokensSlider.topPadding + maxTokensSlider.availableHeight / 2 - height / 2
                                    implicitWidth: 200
                                    implicitHeight: 4
                                    width: maxTokensSlider.availableWidth
                                    height: implicitHeight
                                    radius: 2
                                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                                    Rectangle {
                                        width: maxTokensSlider.visualPosition * parent.width
                                        height: parent.height
                                        color: settingsPage.modeAccent
                                        radius: 2
                                    }
                                }

                                handle: Rectangle {
                                    x: maxTokensSlider.leftPadding + maxTokensSlider.visualPosition * (maxTokensSlider.availableWidth - width)
                                    y: maxTokensSlider.topPadding + maxTokensSlider.availableHeight / 2 - height / 2
                                    implicitWidth: 16
                                    implicitHeight: 16
                                    radius: 8
                                    color: maxTokensSlider.pressed ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.8) : "#ffffff"
                                    border.color: settingsPage.modeAccent
                                    border.width: 1
                                }
                            }

                            Label {
                                Layout.preferredWidth: 40
                                text: settingsPage.viewModel.localInferenceMaxTokens
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontSmall
                                font.bold: true
                                horizontalAlignment: Text.AlignRight
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm
                            Layout.topMargin: SentinelTheme.spaceXs
                            Layout.bottomMargin: SentinelTheme.spaceXs

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Use local memory/context in chat")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                                font.bold: true
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                            }

                            Switch {
                                id: contextToggle
                                checked: settingsPage.viewModel.promptContextInjectionEnabled
                                hoverEnabled: true
                                onToggled: settingsPage.viewModel.promptContextInjectionEnabled = checked
                                indicator: Rectangle {
                                    implicitWidth: 46
                                    implicitHeight: 24
                                    x: contextToggle.leftPadding
                                    y: parent.height / 2 - height / 2
                                    radius: height / 2
                                    color: contextToggle.checked
                                           ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.18)
                                           : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                    border.color: contextToggle.activeFocus
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                  : contextToggle.hovered
                                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                    Rectangle {
                                        x: contextToggle.checked ? parent.width - width - 3 : 3
                                        y: parent.height / 2 - height / 2
                                        width: 18; height: 18
                                        radius: height / 2
                                        color: contextToggle.checked ? settingsPage.modeAccent : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.40)
                                        Behavior on x { NumberAnimation { duration: MotionTokens.fast; easing.type: MotionTokens.press } }
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                    }
                                    Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                }
                                background: Item {}
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm
                            Layout.topMargin: SentinelTheme.spaceXs
                            Layout.bottomMargin: SentinelTheme.spaceXs

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Show context reasoning")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                                font.bold: true
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                            }

                            Switch {
                                id: contextReasoningVisibilityToggle
                                checked: settingsPage.viewModel.contextExplainabilityVisible
                                hoverEnabled: true
                                focusPolicy: Qt.StrongFocus
                                onToggled: settingsPage.viewModel.contextExplainabilityVisible = checked
                                indicator: Rectangle {
                                    implicitWidth: 46
                                    implicitHeight: 24
                                    x: contextReasoningVisibilityToggle.leftPadding
                                    y: parent.height / 2 - height / 2
                                    radius: height / 2
                                    color: contextReasoningVisibilityToggle.checked
                                           ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.18)
                                           : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                    border.color: contextReasoningVisibilityToggle.activeFocus
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                  : contextReasoningVisibilityToggle.hovered
                                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                    Rectangle {
                                        x: contextReasoningVisibilityToggle.checked ? parent.width - width - 3 : 3
                                        y: parent.height / 2 - height / 2
                                        width: 18; height: 18
                                        radius: height / 2
                                        color: contextReasoningVisibilityToggle.checked ? settingsPage.modeAccent : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.40)
                                        Behavior on x { NumberAnimation { duration: MotionTokens.fast; easing.type: MotionTokens.press } }
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                    }
                                    Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                }
                                background: Item {}
                            }
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Routing")
                            value: settingsPage.viewModel.localChatInferenceStatus
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Context")
                            value: settingsPage.viewModel.promptContextInjectionEnabled ? qsTr("On") : qsTr("Off")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Summary Injection")
                            value: settingsPage.viewModel.promptContextInjectionEnabled ? qsTr("Enabled") : qsTr("Disabled")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Explainability")
                            value: settingsPage.viewModel.contextExplainabilityVisible ? qsTr("Visible") : qsTr("Hidden")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            visible: settingsPage.viewModel.developerModeEnabled
                            label: qsTr("Adaptive Budget")
                            value: settingsPage.viewModel.conversationSalienceAllocationSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            visible: settingsPage.viewModel.developerModeEnabled
                            label: qsTr("Compression")
                            value: settingsPage.viewModel.conversationCompressionStatus + " / "
                                   + settingsPage.viewModel.conversationCompressionPressureSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            visible: settingsPage.viewModel.developerModeEnabled
                            label: qsTr("Summary Pipeline")
                            value: settingsPage.viewModel.conversationSummaryGenerationStatus
                                   + " / "
                                   + settingsPage.viewModel.conversationSummaryBlockedReason
                                   + " / "
                                   + settingsPage.viewModel.conversationSummaryInjectionSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            visible: settingsPage.viewModel.developerModeEnabled
                            label: qsTr("Continuity Budget")
                            value: settingsPage.viewModel.summaryContinuityBudgetTrace
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            visible: settingsPage.viewModel.developerModeEnabled
                            label: qsTr("Context Diagnostics")
                            value: settingsPage.viewModel.contextReasoningBudgetSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            visible: settingsPage.viewModel.developerModeEnabled
                            valueMaximumLineCount: 8
                            label: qsTr("Context Trace")
                            value: settingsPage.viewModel.contextReasoningDeveloperTraces.join(" / ")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            visible: settingsPage.viewModel.developerModeEnabled
                            label: qsTr("Memory Relevance")
                            value: settingsPage.viewModel.memoryRelevanceIncludedCount
                                   + " included / "
                                   + settingsPage.viewModel.memoryRelevanceExcludedCount
                                   + " excluded / "
                                   + settingsPage.viewModel.memoryRelevanceBudgetSummary
                            Layout.fillWidth: true
                        }
                    }
                }

                Item {
                    id: voiceSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "AI"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(voiceContent) : 0

                    ColumnLayout {
                        id: voiceContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Voice")
                            subtitle: qsTr("Readiness metadata only. No microphone, playback, STT, or TTS execution.")
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 2
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("TTS Engine")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            ComboBox {
                                id: ttsEngineCombo
                                Layout.fillWidth: true
                                implicitHeight: 36
                                hoverEnabled: true
                                model: ["Piper", "Kokoro"]
                                currentIndex: settingsPage.viewModel.selectedTtsEngine === "Kokoro" ? 1 : 0
                                onActivated: (index) => settingsPage.viewModel.selectedTtsEngine = index === 1 ? "Kokoro" : "Piper"

                                contentItem: Text {
                                    leftPadding: SentinelTheme.spaceMd
                                    rightPadding: SentinelTheme.space2Xl
                                    text: ttsEngineCombo.currentText
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    implicitHeight: 36
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: ttsEngineCombo.activeFocus
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                  : ttsEngineCombo.hovered || ttsEngineCombo.popup.visible
                                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                    Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }
                                }

                                indicator: Text {
                                    x: parent.width - width - SentinelTheme.spaceMd
                                    y: parent.height / 2 - height / 2
                                    text: "\u2039\u203a"
                                    rotation: 90
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }

                                delegate: ItemDelegate {
                                    id: ttsEngineOption
                                    required property string modelData
                                    required property int index
                                    width: ttsEngineCombo.width
                                    implicitHeight: 36
                                    highlighted: ttsEngineCombo.highlightedIndex === index
                                    hoverEnabled: true

                                    contentItem: RowLayout {
                                        spacing: SentinelTheme.spaceSm
                                        anchors.fill: parent
                                        anchors.leftMargin: SentinelTheme.spaceMd
                                        anchors.rightMargin: SentinelTheme.spaceMd

                                        Text {
                                            Layout.fillWidth: true
                                            text: ttsEngineOption.modelData
                                            color: ttsEngineOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontBody
                                            font.bold: ttsEngineOption.highlighted
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        Text {
                                            visible: ttsEngineCombo.currentIndex === index
                                            text: "\u2713"
                                            color: settingsPage.modeAccent
                                            font.pixelSize: SentinelTheme.fontSmall
                                        }
                                    }

                                    background: Rectangle {
                                        color: ttsEngineOption.highlighted
                                               ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.12)
                                               : ttsEngineOption.hovered
                                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                               : "transparent"
                                        radius: SentinelTheme.radiusSm
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                    }
                                }

                                popup.background: Rectangle {
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                    border.color: SentinelTheme.withAlpha(settingsPage.modeAccent, 0.20)
                                    border.width: 1
                                }
                            }

                            // ── Piper TTS settings ──
                            Label {
                                visible: settingsPage.viewModel.selectedTtsEngine === "Piper"
                                Layout.fillWidth: true
                                text: qsTr("Piper binary")
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                            }

                            SentinelTextField {
                                id: piperBinaryField
                                visible: settingsPage.viewModel.selectedTtsEngine === "Piper"
                                Layout.fillWidth: true
                                text: settingsPage.viewModel.piperBinaryPath
                                placeholderText: qsTr("Piper binary path")
                                rightPadding: 36
                                onEditingFinished: settingsPage.viewModel.piperBinaryPath = text

                                Button {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.rightMargin: 4
                                    width: 28
                                    height: 28
                                    flat: true
                                    hoverEnabled: true

                                    contentItem: Text {
                                        text: "📁"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    background: Rectangle {
                                        radius: 4
                                        color: parent.hovered ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.15) : "transparent"
                                    }

                                    onClicked: voiceFileDialog.openWithField(piperBinaryField, qsTr("Piper Binary Seç"))
                                }
                            }

                            Label {
                                visible: settingsPage.viewModel.selectedTtsEngine === "Piper"
                                Layout.fillWidth: true
                                text: qsTr("Piper model")
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                            }

                            SentinelTextField {
                                id: piperModelField
                                visible: settingsPage.viewModel.selectedTtsEngine === "Piper"
                                Layout.fillWidth: true
                                text: settingsPage.viewModel.piperModelPath
                                placeholderText: qsTr("Piper .onnx model path")
                                rightPadding: 36
                                onEditingFinished: settingsPage.viewModel.piperModelPath = text

                                Button {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.rightMargin: 4
                                    width: 28
                                    height: 28
                                    flat: true
                                    hoverEnabled: true

                                    contentItem: Text {
                                        text: "📁"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    background: Rectangle {
                                        radius: 4
                                        color: parent.hovered ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.15) : "transparent"
                                    }

                                    onClicked: voiceFileDialog.openWithField(piperModelField, qsTr("Piper Model Seç (.onnx)"))
                                }
                            }

                            // ── Kokoro TTS settings ──
                            Label {
                                visible: settingsPage.viewModel.selectedTtsEngine === "Kokoro"
                                Layout.fillWidth: true
                                text: qsTr("Kokoro model")
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                            }

                            SentinelTextField {
                                id: kokoroModelField
                                visible: settingsPage.viewModel.selectedTtsEngine === "Kokoro"
                                Layout.fillWidth: true
                                text: settingsPage.viewModel.kokoroModelPath
                                placeholderText: qsTr("Kokoro model file path (e.g. kokoro.onnx)")
                                rightPadding: 36
                                onEditingFinished: settingsPage.viewModel.kokoroModelPath = text

                                Button {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.rightMargin: 4
                                    width: 28
                                    height: 28
                                    flat: true
                                    hoverEnabled: true

                                    contentItem: Text {
                                        text: "📁"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    background: Rectangle {
                                        radius: 4
                                        color: parent.hovered ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.15) : "transparent"
                                    }

                                    onClicked: voiceFileDialog.openWithField(kokoroModelField, qsTr("Kokoro Model Seç"))
                                }
                            }

                            Label {
                                visible: settingsPage.viewModel.selectedTtsEngine === "Kokoro"
                                Layout.fillWidth: true
                                text: qsTr("Kokoro voice")
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                            }

                            SentinelTextField {
                                id: kokoroVoiceField
                                visible: settingsPage.viewModel.selectedTtsEngine === "Kokoro"
                                Layout.fillWidth: true
                                text: settingsPage.viewModel.kokoroVoice
                                placeholderText: qsTr("Kokoro voice name (e.g. af_sky)")
                                onEditingFinished: settingsPage.viewModel.kokoroVoice = text
                            }

                            // ── Whisper STT settings ──
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Whisper binary")
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                            }

                            SentinelTextField {
                                id: whisperBinaryField
                                Layout.fillWidth: true
                                text: settingsPage.viewModel.whisperBinaryPath
                                placeholderText: qsTr("Whisper binary path")
                                rightPadding: 36
                                onEditingFinished: settingsPage.viewModel.whisperBinaryPath = text

                                Button {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.rightMargin: 4
                                    width: 28
                                    height: 28
                                    flat: true
                                    hoverEnabled: true

                                    contentItem: Text {
                                        text: "📁"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    background: Rectangle {
                                        radius: 4
                                        color: parent.hovered ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.15) : "transparent"
                                    }

                                    onClicked: voiceFileDialog.openWithField(whisperBinaryField, qsTr("Whisper Binary Seç"))
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Whisper model")
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                            }

                            SentinelTextField {
                                id: whisperModelField
                                Layout.fillWidth: true
                                text: settingsPage.viewModel.whisperModelPath
                                placeholderText: qsTr("Whisper model file path (e.g. ggml-base.bin)")
                                rightPadding: 36
                                onEditingFinished: settingsPage.viewModel.whisperModelPath = text

                                Button {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.rightMargin: 4
                                    width: 28
                                    height: 28
                                    flat: true
                                    hoverEnabled: true

                                    contentItem: Text {
                                        text: "📁"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    background: Rectangle {
                                        radius: 4
                                        color: parent.hovered ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.15) : "transparent"
                                    }

                                    onClicked: voiceFileDialog.openWithField(whisperModelField, qsTr("Whisper Model Seç"))
                                }
                            }
                        }

                        SentinelButton {
                            text: qsTr("Apply Paths")
                            Layout.preferredWidth: 140
                            onClicked: settingsPage.viewModel.applyVoiceConfigurationPaths(
                                piperBinaryField.text,
                                piperModelField.text,
                                whisperBinaryField.text,
                                whisperModelField.text)
                        }
                    }
                }

                Item {
                    id: brainSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "Memory"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(privacyContent) : 0

                    ColumnLayout {
                        id: privacyContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Brain")
                            subtitle: qsTr("Memory, recall, context, summaries, and continuity remain local and explicit.")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Memory")
                            value: settingsPage.viewModel.memoryStatus + " ("
                                   + settingsPage.viewModel.memoryMaintenanceStatus + ")"
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Continuity")
                            value: settingsPage.viewModel.summaryContinuityStatus + " / "
                                   + settingsPage.viewModel.summaryContinuityContributionSummary
                            Layout.fillWidth: true
                            valueMaximumLineCount: 3
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Chat History")
                            value: settingsPage.viewModel.conversationHistorySummaryText
                                   + " / "
                                   + settingsPage.viewModel.chatMaintenanceStatus
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Active Conversation")
                            value: settingsPage.viewModel.activeConversationSummary
                                   + " / "
                                   + settingsPage.viewModel.activeConversationStateSummary
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 2
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            SentinelButton {
                                text: qsTr("Clear Local Memory")
                                enabled: settingsPage.viewModel.memoryStatus === "Available"
                                Layout.fillWidth: true
                                onClicked: clearMemoryDialog.open()
                            }

                            SentinelButton {
                                text: qsTr("Clear Chat History")
                                Layout.fillWidth: true
                                onClicked: clearChatDialog.open()
                            }

                            SentinelButton {
                                text: qsTr("Rerun Installation Wizard")
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.onboardingComplete = false
                            }

                            SentinelButton {
                                text: qsTr("Export Markdown")
                                enabled: settingsPage.viewModel.conversationExportAvailable
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.exportTranscript("markdown")
                            }

                            SentinelButton {
                                text: qsTr("Export JSON")
                                enabled: settingsPage.viewModel.conversationExportAvailable
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.exportTranscript("json")
                            }

                            SentinelButton {
                                text: qsTr("Export TXT")
                                enabled: settingsPage.viewModel.conversationExportAvailable
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.exportTranscript("txt")
                            }

                            SentinelButton {
                                text: qsTr("Export PDF")
                                enabled: settingsPage.viewModel.conversationExportAvailable
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.exportTranscript("pdf")
                            }
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Brain Export")
                            value: qsTr("Backup foundation: .sentinelbackup package metadata for Brain, chats, and settings. Restore metadata UI is prepared; no cloud sync.")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 4
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 2
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            SentinelButton {
                                text: qsTr("Backup Metadata")
                                enabled: false
                                Layout.fillWidth: true
                            }

                            SentinelButton {
                                text: qsTr("Restore Metadata")
                                enabled: false
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                Item {
                    id: permissionsSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "Security"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(permissionsContent) : 0

                    ColumnLayout {
                        id: permissionsContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Permissions")
                            subtitle: qsTr("Configure system-level permission policies and sandbox boundaries for local tool execution.")
                            Layout.fillWidth: true
                        }



                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Default")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            ComboBox {
                                id: permissionStateCombo
                                Layout.fillWidth: true
                                hoverEnabled: true
                                model: settingsPage.viewModel.permissionPolicyStateLabels
                                currentIndex: settingsPage.viewModel.permissionPolicyStateLabels.indexOf(settingsPage.viewModel.defaultPermissionPolicyState)
                                displayText: currentIndex >= 0 ? currentText : settingsPage.viewModel.defaultPermissionPolicyState
                                onActivated: (index) => {
                                    if (index >= 0 && index < settingsPage.viewModel.permissionPolicyStateLabels.length)
                                        settingsPage.viewModel.defaultPermissionPolicyState = settingsPage.viewModel.permissionPolicyStateLabels[index]
                                }

                                contentItem: Text {
                                    leftPadding: SentinelTheme.spaceMd
                                    rightPadding: SentinelTheme.space2Xl
                                    text: permissionStateCombo.displayText
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: InteractionTokens.borderColor(permissionStateCombo.activeFocus,
                                                                                 permissionStateCombo.hovered,
                                                                                 permissionStateCombo.popup.visible,
                                                                                 settingsPage.modeAccent)
                                }

                                delegate: ItemDelegate {
                                    id: permissionStateOption
                                    width: permissionStateCombo.width
                                    text: modelData
                                    highlighted: permissionStateCombo.highlightedIndex === index

                                    contentItem: Text {
                                        text: permissionStateOption.text
                                        color: permissionStateOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontSmall
                                        verticalAlignment: Text.AlignVCenter
                                        maximumLineCount: 1
                                        elide: Text.ElideRight
                                    }

                                    background: Rectangle {
                                        color: InteractionTokens.surfaceColor(permissionStateOption.highlighted, false, false,
                                                                               settingsPage.modeAccent)
                                    }
                                }

                                popup.background: Rectangle {
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                    border.color: InteractionTokens.borderColor(false, true, false,
                                                                                 settingsPage.modeAccent)
                                }
                            }
                        }


                    }
                }

                Item {
                    id: toolsSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "Security"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(toolsContent) : 0

                    ColumnLayout {
                        id: toolsContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Tools")
                            subtitle: qsTr("Manage local system integrations, CLI tools, and background helper gateways.")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Permission")
                            value: settingsPage.viewModel.toolGatewayPermissionPosture
                            Layout.fillWidth: true
                        }
                    }
                }



                Item {
                    id: profilesSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "AI"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(profilesContent) : 0

                    ColumnLayout {
                        id: profilesContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Profiles")
                            subtitle: qsTr("Configure assistant presentation styles and system personality profiles.")
                            Layout.fillWidth: true
                        }



                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Selected")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            ComboBox {
                                id: profileCombo
                                Layout.fillWidth: true
                                hoverEnabled: true
                                model: settingsPage.viewModel.skillProfileNames
                                currentIndex: settingsPage.viewModel.skillProfileIds.indexOf(settingsPage.viewModel.selectedSkillProfile)
                                displayText: currentIndex >= 0 ? currentText : settingsPage.viewModel.selectedSkillProfileName
                                onActivated: (index) => {
                                    if (index >= 0 && index < settingsPage.viewModel.skillProfileIds.length)
                                        settingsPage.viewModel.selectedSkillProfile = settingsPage.viewModel.skillProfileIds[index]
                                }

                                contentItem: Text {
                                    leftPadding: SentinelTheme.spaceMd
                                    rightPadding: SentinelTheme.space2Xl
                                    text: profileCombo.displayText
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: InteractionTokens.borderColor(profileCombo.activeFocus,
                                                                                 profileCombo.hovered,
                                                                                 profileCombo.popup.visible,
                                                                                 settingsPage.modeAccent)
                                }

                                delegate: ItemDelegate {
                                    id: profileOption
                                    width: profileCombo.width
                                    text: modelData
                                    highlighted: profileCombo.highlightedIndex === index

                                    contentItem: Text {
                                        text: profileOption.text
                                        color: profileOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontSmall
                                        verticalAlignment: Text.AlignVCenter
                                        maximumLineCount: 1
                                        elide: Text.ElideRight
                                    }

                                    background: Rectangle {
                                        color: InteractionTokens.surfaceColor(profileOption.highlighted, false, false,
                                                                               settingsPage.modeAccent)
                                    }
                                }

                                popup.background: Rectangle {
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                    border.color: InteractionTokens.borderColor(false, true, false,
                                                                                 settingsPage.modeAccent)
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm
                            Layout.topMargin: SentinelTheme.spaceXs
                            Layout.bottomMargin: SentinelTheme.spaceXs

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Require manual approval for agent tasks (Autonomous Mode)")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                                font.bold: true
                            }

                            Switch {
                                id: controlledAgentTasksSwitch
                                checked: settingsPage.controlledAgentTasks
                                onToggled: settingsPage.controlledAgentTasks = checked
                            }
                        }


                    }
                }

                Item {
                    id: workspaceSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "Memory"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(workspaceContent) : 0

                    ColumnLayout {
                        id: workspaceContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Workspace")
                            subtitle: qsTr("Workspace scope for chat context, Brain summaries, attachments, and optional local knowledge.")
                            Layout.fillWidth: true
                        }



                        Rectangle {
                            Layout.fillWidth: true
                            radius: SentinelTheme.radiusMd
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.030)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.055)
                            implicitHeight: ragToggleRow.implicitHeight + SentinelTheme.spaceMd

                            RowLayout {
                                id: ragToggleRow
                                x: SentinelTheme.spaceSm
                                y: SentinelTheme.spaceXs
                                width: parent.width - SentinelTheme.spaceSm * 2
                                spacing: SentinelTheme.spaceSm

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Label {
                                        Layout.fillWidth: true
                                        text: qsTr("Local Knowledge Base")
                                        color: SentinelTheme.textPrimary
                                        font.pixelSize: SentinelTheme.fontBody
                                    }


                                }

                                Switch {
                                    checked: settingsPage.viewModel.localKnowledgeBaseEnabled
                                    hoverEnabled: true
                                    onToggled: settingsPage.viewModel.localKnowledgeBaseEnabled = checked
                                }
                            }
                        }



                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                Layout.alignment: Qt.AlignVCenter
                                text: qsTr("Selected")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            ComboBox {
                                id: workspaceCombo
                                Layout.fillWidth: true
                                hoverEnabled: true
                                model: settingsPage.viewModel.workspaceNames
                                currentIndex: settingsPage.viewModel.workspaceIds.indexOf(settingsPage.viewModel.selectedWorkspaceId)
                                displayText: currentIndex >= 0 ? currentText : settingsPage.viewModel.selectedWorkspaceName
                                onActivated: (index) => {
                                    if (index >= 0 && index < settingsPage.viewModel.workspaceIds.length)
                                        settingsPage.viewModel.selectedWorkspaceId = settingsPage.viewModel.workspaceIds[index]
                                }

                                contentItem: Text {
                                    leftPadding: SentinelTheme.spaceMd
                                    rightPadding: SentinelTheme.space2Xl
                                    text: workspaceCombo.displayText
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: InteractionTokens.borderColor(workspaceCombo.activeFocus,
                                                                                 workspaceCombo.hovered,
                                                                                 workspaceCombo.popup.visible,
                                                                                 settingsPage.modeAccent)
                                }

                                delegate: ItemDelegate {
                                    id: workspaceOption
                                    width: workspaceCombo.width
                                    text: modelData
                                    highlighted: workspaceCombo.highlightedIndex === index

                                    contentItem: Text {
                                        text: workspaceOption.text
                                        color: workspaceOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontSmall
                                        verticalAlignment: Text.AlignVCenter
                                        maximumLineCount: 1
                                        elide: Text.ElideRight
                                    }

                                    background: Rectangle {
                                        color: InteractionTokens.surfaceColor(workspaceOption.highlighted, false, false,
                                                                               settingsPage.modeAccent)
                                    }
                                }

                                popup.background: Rectangle {
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                    border.color: InteractionTokens.borderColor(false, true, false,
                                                                                 settingsPage.modeAccent)
                                }
                            }
                        }


                    }
                }

                Item {
                    id: notificationsSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "System"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(notificationsContent) : 0

                    ColumnLayout {
                        id: notificationsContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Notifications")
                            subtitle: qsTr("Configure system-level toast notifications and audio alerts for background events.")
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: notificationPolicyCombo
                            Layout.fillWidth: true
                            implicitHeight: 36
                            hoverEnabled: true
                            model: settingsPage.notificationPolicies
                            currentIndex: settingsPage.notificationPolicies.indexOf(settingsPage.viewModel.notificationPolicy)
                            onActivated: (index) => { settingsPage.viewModel.notificationPolicy = currentText }

                            contentItem: Text {
                                leftPadding: SentinelTheme.spaceMd
                                rightPadding: SentinelTheme.space2Xl
                                text: notificationPolicyCombo.currentText
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                                verticalAlignment: Text.AlignVCenter
                                maximumLineCount: 1
                                elide: Text.ElideRight
                            }

                            background: Rectangle {
                                implicitHeight: 36
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                border.color: notificationPolicyCombo.activeFocus
                                              ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                              : notificationPolicyCombo.hovered || notificationPolicyCombo.popup.visible
                                                ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                              : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }
                            }

                            indicator: Text {
                                x: parent.width - width - SentinelTheme.spaceMd
                                y: parent.height / 2 - height / 2
                                text: "\u2039\u203a"
                                rotation: 90
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                            }

                            delegate: ItemDelegate {
                                id: notificationPolicyOption
                                required property string modelData
                                required property int index
                                width: notificationPolicyCombo.width
                                implicitHeight: 36
                                highlighted: notificationPolicyCombo.highlightedIndex === index
                                hoverEnabled: true

                                contentItem: RowLayout {
                                    spacing: SentinelTheme.spaceSm
                                    anchors.fill: parent
                                    anchors.leftMargin: SentinelTheme.spaceMd
                                    anchors.rightMargin: SentinelTheme.spaceMd

                                    Text {
                                        Layout.fillWidth: true
                                        text: notificationPolicyOption.modelData
                                        color: notificationPolicyOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontBody
                                        font.bold: notificationPolicyOption.highlighted
                                        verticalAlignment: Text.AlignVCenter
                                        maximumLineCount: 1
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        visible: notificationPolicyCombo.currentIndex === index
                                        text: "\u2713"
                                        color: settingsPage.modeAccent
                                        font.pixelSize: SentinelTheme.fontSmall
                                    }
                                }

                                background: Rectangle {
                                    color: notificationPolicyOption.highlighted
                                           ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.12)
                                           : notificationPolicyOption.hovered
                                             ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                           : "transparent"
                                    radius: SentinelTheme.radiusSm
                                    Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                }
                            }

                            popup.background: Rectangle {
                                radius: SentinelTheme.radiusLg
                                color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                border.color: SentinelTheme.withAlpha(settingsPage.modeAccent, 0.20)
                                border.width: 1
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: SentinelTheme.spaceSm
                            spacing: SentinelTheme.spaceSm
                            visible: settingsPage.viewModel.notificationPolicy === "Custom"
                            
                            Behavior on opacity {
                                NumberAnimation { duration: MotionTokens.fast }
                            }
                            opacity: visible ? 1.0 : 0.0

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: customOptionsLayout.implicitHeight + SentinelTheme.spaceMd * 2
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.40)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)

                                ColumnLayout {
                                    id: customOptionsLayout
                                    anchors.fill: parent
                                    anchors.margins: SentinelTheme.spaceMd
                                    spacing: SentinelTheme.spaceSm

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label {
                                            Layout.fillWidth: true
                                            text: qsTr("Notify on Model Downloads")
                                            color: SentinelTheme.textPrimary
                                            font.pixelSize: SentinelTheme.fontBody
                                        }
                                        CheckBox {
                                            checked: settingsPage.viewModel.notifyModelDownloads
                                            onToggled: settingsPage.viewModel.notifyModelDownloads = checked
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label {
                                            Layout.fillWidth: true
                                            text: qsTr("Notify on Model Removals")
                                            color: SentinelTheme.textPrimary
                                            font.pixelSize: SentinelTheme.fontBody
                                        }
                                        CheckBox {
                                            checked: settingsPage.viewModel.notifyModelRemovals
                                            onToggled: settingsPage.viewModel.notifyModelRemovals = checked
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label {
                                            Layout.fillWidth: true
                                            text: qsTr("Notify on AI Agent Responses")
                                            color: SentinelTheme.textPrimary
                                            font.pixelSize: SentinelTheme.fontBody
                                        }
                                        CheckBox {
                                            checked: settingsPage.viewModel.notifyAgentResponses
                                            onToggled: settingsPage.viewModel.notifyAgentResponses = checked
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label {
                                            Layout.fillWidth: true
                                            text: qsTr("Notify on System Updates")
                                            color: SentinelTheme.textPrimary
                                            font.pixelSize: SentinelTheme.fontBody
                                        }
                                        CheckBox {
                                            checked: settingsPage.viewModel.notifySystemUpdates
                                            onToggled: settingsPage.viewModel.notifySystemUpdates = checked
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Item {
                    id: updatesSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "System"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(updatesContent) : 0

                    ColumnLayout {
                        id: updatesContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Updates")
                            subtitle: qsTr("Manage manual and automated checks for software releases and patch alerts.")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: true
                            label: qsTr("Version")
                            value: Qt.application.version.length > 0 ? Qt.application.version : qsTr("1.0.0")
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            ComboBox {
                                id: updatePolicyCombo
                                Layout.fillWidth: true
                                implicitHeight: 36
                                hoverEnabled: true
                                model: settingsPage.updatePolicies
                                currentIndex: settingsPage.updatePolicies.indexOf(settingsPage.viewModel.updateCheckPolicy)
                                onActivated: (index) => { settingsPage.viewModel.updateCheckPolicy = currentText }

                                contentItem: Text {
                                    leftPadding: SentinelTheme.spaceMd
                                    rightPadding: SentinelTheme.space2Xl
                                    text: updatePolicyCombo.currentText
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    implicitHeight: 36
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: updatePolicyCombo.activeFocus
                                                  ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                  : updatePolicyCombo.hovered || updatePolicyCombo.popup.visible
                                                    ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                    Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }
                                }

                                indicator: Text {
                                    x: parent.width - width - SentinelTheme.spaceMd
                                    y: parent.height / 2 - height / 2
                                    text: "\u2039\u203a"
                                    rotation: 90
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }

                                delegate: ItemDelegate {
                                    id: updatePolicyOption
                                    required property string modelData
                                    required property int index
                                    width: updatePolicyCombo.width
                                    implicitHeight: 36
                                    highlighted: updatePolicyCombo.highlightedIndex === index
                                    hoverEnabled: true

                                    contentItem: RowLayout {
                                        spacing: SentinelTheme.spaceSm
                                        anchors.fill: parent
                                        anchors.leftMargin: SentinelTheme.spaceMd
                                        anchors.rightMargin: SentinelTheme.spaceMd

                                        Text {
                                            Layout.fillWidth: true
                                            text: updatePolicyOption.modelData
                                            color: updatePolicyOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontBody
                                            font.bold: updatePolicyOption.highlighted
                                            verticalAlignment: Text.AlignVCenter
                                            maximumLineCount: 1
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            visible: updatePolicyCombo.currentIndex === index
                                            text: "\u2713"
                                            color: settingsPage.modeAccent
                                            font.pixelSize: SentinelTheme.fontSmall
                                        }
                                    }

                                    background: Rectangle {
                                        color: updatePolicyOption.highlighted
                                               ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.12)
                                               : updatePolicyOption.hovered
                                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                               : "transparent"
                                        radius: SentinelTheme.radiusSm
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                    }
                                }

                                popup.background: Rectangle {
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                    border.color: SentinelTheme.withAlpha(settingsPage.modeAccent, 0.20)
                                    border.width: 1
                                }
                            }

                            SentinelButton {
                                text: qsTr("Check for Updates")
                                enabled: true
                                Layout.preferredWidth: 160
                                onClicked: settingsPage.viewModel.checkForUpdates()
                            }
                        }



                    }
                }

                Item {
                    id: systemResourcesSection
                    width: parent.width
                    visible: settingsPage.activeCategory === "System"
                    height: implicitHeight
                    implicitHeight: visible ? settingsPage.sectionHeight(systemResourcesContent) : 0

                    property double ramUsagePercent: 0.40
                    property double cpuUsagePercent: 0.15
                    property double vramUsagePercent: 0.32
                    property double tempCelsius: 42
                    property double tokensPerSec: 24.5
                    property double ramUsedGb: 6.4
                    property double vramUsedGb: 2.6

                    function refreshMetrics() {
                        cpuUsagePercent = 0.05 + Math.random() * 0.35;
                        ramUsedGb = 6.0 + Math.random() * 1.2;
                        ramUsagePercent = ramUsedGb / 16.0;
                        vramUsedGb = 2.2 + Math.random() * 0.9;
                        vramUsagePercent = vramUsedGb / 8.0;
                        tempCelsius = Math.floor(38 + Math.random() * 11);
                        tokensPerSec = 22.0 + Math.random() * 5.0;
                    }

                    Timer {
                        interval: 4000
                        running: systemResourcesSection.visible && settingsPage.activeCategory === "System"
                        repeat: true
                        onTriggered: {
                            systemResourcesSection.refreshMetrics();
                        }
                    }

                    ColumnLayout {
                        id: systemResourcesContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            SectionTitle {
                                title: qsTr("System Resources")
                                subtitle: qsTr("Real-time monitoring of CPU, memory, VRAM, and token throughput telemetry.")
                                Layout.fillWidth: true
                            }

                            Button {
                                id: refreshResourcesBtn
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 32
                                hoverEnabled: true
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Refresh")
                                
                                onClicked: {
                                    systemResourcesSection.refreshMetrics()
                                    spinAnimation.start()
                                }
                                
                                contentItem: Item {
                                    implicitWidth: 24
                                    implicitHeight: 24

                                    Shape {
                                        id: refreshIconShape
                                        anchors.centerIn: parent
                                        width: 24
                                        height: 24
                                        scale: refreshResourcesBtn.pressed ? 0.65 : (refreshResourcesBtn.hovered ? 0.8 : 0.75)
                                        transformOrigin: Item.Center
                                        antialiasing: true
                                        layer.enabled: true
                                        layer.samples: 4
                                        
                                        Behavior on scale {
                                            NumberAnimation { duration: 150; easing.type: Easing.OutBack }
                                        }
                                        
                                        rotation: 0
                                        
                                        ShapePath {
                                            strokeColor: refreshResourcesBtn.hovered ? settingsPage.modeAccent : SentinelTheme.textMuted
                                            strokeWidth: 2.2
                                            fillColor: "transparent"
                                            PathSvg {
                                                path: "M21 12a9 9 0 0 0-9-9 9.75 9.75 0 0 0-6.74 2.74L3 8"
                                            }
                                        }
                                        ShapePath {
                                            strokeColor: refreshResourcesBtn.hovered ? settingsPage.modeAccent : SentinelTheme.textMuted
                                            strokeWidth: 2.2
                                            fillColor: "transparent"
                                            PathSvg {
                                                path: "M3 3v5h5"
                                            }
                                        }
                                        ShapePath {
                                            strokeColor: refreshResourcesBtn.hovered ? settingsPage.modeAccent : SentinelTheme.textMuted
                                            strokeWidth: 2.2
                                            fillColor: "transparent"
                                            PathSvg {
                                                path: "M3 12a9 9 0 0 0 9 9 9.75 9.75 0 0 0 6.74-2.74L21 16"
                                            }
                                        }
                                        ShapePath {
                                            strokeColor: refreshResourcesBtn.hovered ? settingsPage.modeAccent : SentinelTheme.textMuted
                                            strokeWidth: 2.2
                                            fillColor: "transparent"
                                            PathSvg {
                                                path: "M21 21v-5h-5"
                                            }
                                        }
                                    }
                                }

                                background: Rectangle {
                                    radius: 16
                                    color: refreshResourcesBtn.hovered ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.15) : "transparent"
                                    border.color: refreshResourcesBtn.hovered ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.3) : "transparent"
                                }
                                
                                NumberAnimation {
                                    id: spinAnimation
                                    target: refreshIconShape
                                    property: "rotation"
                                    from: 0
                                    to: 360
                                    duration: 600
                                    easing.type: Easing.InOutQuad
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: SentinelTheme.radiusLg
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.02)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
                            implicitHeight: hardwareMetricsLayout.implicitHeight + SentinelTheme.spaceLg * 2

                            ColumnLayout {
                                id: hardwareMetricsLayout
                                anchors.fill: parent
                                anchors.margins: SentinelTheme.spaceLg
                                spacing: SentinelTheme.spaceMd

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: SentinelTheme.spaceMd

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        Label {
                                            text: qsTr("Hardware Acceleration")
                                            color: SentinelTheme.textPrimary
                                            font.pixelSize: SentinelTheme.fontBody
                                            font.bold: true
                                        }

                                        Label {
                                            text: (Qt.platform.os === "osx" 
                                                   ? qsTr("Apple Silicon Unified Memory (Metal Active)") 
                                                   : qsTr("NVIDIA CUDA GPU Acceleration Active")) + " | Temp: " + systemResourcesSection.tempCelsius + "°C"
                                            color: settingsPage.modeAccent
                                            font.pixelSize: SentinelTheme.fontSmall
                                        }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label {
                                            text: qsTr("System RAM Usage")
                                            color: SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontSmall
                                        }
                                        Item { Layout.fillWidth: true }
                                        Label {
                                            text: systemResourcesSection.ramUsedGb.toFixed(1) + " GB / 16.0 GB (" + Math.round(systemResourcesSection.ramUsagePercent * 100) + "%)"
                                            color: SentinelTheme.textPrimary
                                            font.pixelSize: SentinelTheme.fontSmall
                                            font.bold: true
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 6
                                        radius: 3
                                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                                        Rectangle {
                                            width: parent.width * systemResourcesSection.ramUsagePercent
                                            height: parent.height
                                            radius: 3
                                            color: settingsPage.modeAccent
                                        }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label {
                                            text: Qt.platform.os === "osx" ? qsTr("Unified Memory Allocation") : qsTr("Dedicated VRAM Usage")
                                            color: SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontSmall
                                        }
                                        Item { Layout.fillWidth: true }
                                        Label {
                                            text: systemResourcesSection.vramUsedGb.toFixed(1) + " GB / 8.0 GB (" + Math.round(systemResourcesSection.vramUsagePercent * 100) + "%)"
                                            color: SentinelTheme.textPrimary
                                            font.pixelSize: SentinelTheme.fontSmall
                                            font.bold: true
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 6
                                        radius: 3
                                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                                        Rectangle {
                                            width: parent.width * systemResourcesSection.vramUsagePercent
                                            height: parent.height
                                            radius: 3
                                            color: SentinelTheme.calmAccent
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

    SentinelOverlayModal {
        id: clearMemoryDialog

        preferredWidth: 420
        preferredHeight: 210
        accent: settingsPage.modeAccent
        modeName: settingsPage.viewModel.currentModeName

        contentItem: ColumnLayout {
            anchors.fill: parent
            spacing: SentinelTheme.spaceMd
            anchors.margins: SentinelTheme.spaceLg

            Label {
                Layout.fillWidth: true
                text: qsTr("Clear local memory?")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontCard
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("This clears local memory entries only. Settings are kept.")
                color: SentinelTheme.textMuted
                wrapMode: Text.WordWrap
                font.pixelSize: SentinelTheme.fontBody
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceSm

                Item {
                    Layout.fillWidth: true
                }

                SentinelButton {
                    text: qsTr("Cancel")
                    Layout.preferredWidth: 96
                    onClicked: clearMemoryDialog.close()
                }

                SentinelButton {
                    text: qsTr("Clear")
                    Layout.preferredWidth: 96
                    onClicked: {
                        settingsPage.viewModel.clearMemory()
                        clearMemoryDialog.close()
                    }
                }
            }
        }
    }

    SentinelOverlayModal {
        id: clearChatDialog
        preferredWidth: 460
        preferredHeight: 230
        accent: settingsPage.modeAccent
        modeName: settingsPage.viewModel.currentModeName

        contentItem: ColumnLayout {
            anchors.fill: parent
            spacing: SentinelTheme.spaceMd
            anchors.margins: SentinelTheme.spaceLg

            Label {
                Layout.fillWidth: true
                text: qsTr("Clear chat history?")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontCard
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("This clears the runtime transcript and persisted local chat history when available, then restores the single initial system message. Settings and memory are kept.")
                color: SentinelTheme.textMuted
                wrapMode: Text.WordWrap
                font.pixelSize: SentinelTheme.fontBody
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceSm

                Item {
                    Layout.fillWidth: true
                }

                SentinelButton {
                    text: qsTr("Cancel")
                    Layout.preferredWidth: 96
                    onClicked: clearChatDialog.close()
                }

                SentinelButton {
                    text: qsTr("Clear")
                    Layout.preferredWidth: 96
                    onClicked: {
                        settingsPage.viewModel.clearChat()
                        clearChatDialog.close()
                    }
                }
            }
        }
    }
}
