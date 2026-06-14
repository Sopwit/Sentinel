import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: settingsPage
    required property var viewModel
    readonly property bool compact: width < 820
    readonly property int panelPadding: SentinelTheme.spaceLg
    readonly property bool developerMode: viewModel.developerModeEnabled
    readonly property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property string uiSelfCheck: "modal-ready rail-scroll-sync advanced-gated voice-path-wrap profiles-metadata permissions-policy tool-gateway agent-runtime bottom-safe-scroll"
    readonly property var themeChoices: ["Sentinel Dark", "Midnight", "Aurora", "Graphite", "System Adaptive"]
    readonly property var notificationPolicies: ["Disabled", "Important Only", "All", "Custom"]
    readonly property var updatePolicies: ["Never", "Ask Before Checking", "Weekly", "On Startup"]
    readonly property var densityChoices: ["Compact", "Comfortable", "Large"]
    readonly property var categories: developerMode
                                    ? ["General", "Appearance", "Accessibility", "AI", "Models", "Voice", "Brain", "Permissions", "Tools", "Agents", "Workspace", "Notifications", "Updates", "Diagnostics", "Advanced"]
                                    : ["General", "Appearance", "Accessibility", "AI", "Models", "Voice", "Brain", "Permissions", "Tools", "Agents", "Workspace", "Notifications", "Updates", "Diagnostics"]
    property string activeCategory: "General"
    property bool programmaticScroll: false

    function sectionHeight(content) {
        return content.implicitHeight + panelPadding * 2
    }

    function sectionFor(category) {
        if (category === "Appearance")
            return appearanceSection
        if (category === "Accessibility")
            return accessibilitySection
        if (category === "AI")
            return localAiSection
        if (category === "Models")
            return modelSection
        if (category === "Voice")
            return voiceSection
        if (category === "Permissions")
            return permissionsSection
        if (category === "Tools")
            return toolsSection
        if (category === "Agents")
            return agentsSection
        if (category === "Workspace")
            return workspaceSection
        if (category === "Brain")
            return brainSection
        if (category === "Notifications")
            return notificationsSection
        if (category === "Updates")
            return updatesSection
        if (category === "Diagnostics")
            return diagnosticsSection
        if (category === "Advanced")
            return developerSection
        return generalSection
    }

    function categoryLabel(category) {
        if (category === "General")
            return qsTr("General")
        if (category === "Appearance")
            return qsTr("Appearance")
        if (category === "Accessibility")
            return qsTr("Accessibility")
        if (category === "AI")
            return qsTr("AI")
        if (category === "Models")
            return qsTr("Models")
        if (category === "Voice")
            return qsTr("Voice")
        if (category === "Permissions")
            return qsTr("Permissions")
        if (category === "Tools")
            return qsTr("Tools")
        if (category === "Agents")
            return qsTr("Agents")
        if (category === "Workspace")
            return qsTr("Workspace")
        if (category === "Brain")
            return qsTr("Brain")
        if (category === "Notifications")
            return qsTr("Notifications")
        if (category === "Updates")
            return qsTr("Updates")
        if (category === "Diagnostics")
            return qsTr("Diagnostics")
        if (category === "Advanced")
            return qsTr("Advanced")
        return category
    }

    function jumpTo(category) {
        activeCategory = category
        var target = sectionFor(category)
        programmaticScroll = true
        settingsFlick.contentY = Math.max(0, Math.min(target.y,
                                                      settingsFlick.contentHeight - settingsFlick.height))
        scrollSettledTimer.restart()
    }

    function voiceReadinessLine() {
        var piperReady = viewModel.piperSynthesisStatus === "Ready Metadata"
        var whisperReady = viewModel.whisperTranscriptionStatus === "Ready Metadata"
        var prefix = piperReady && whisperReady
                     ? qsTr("Voice prepared, activation disabled. ")
                     : qsTr("Voice readiness metadata. ")
        return prefix + viewModel.voiceConfigurationReadinessSummary
    }

    onDeveloperModeChanged: {
        if (!developerMode && activeCategory === "Advanced")
            jumpTo("General")
    }

    RowLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        ShellPanel {
            Layout.preferredWidth: settingsPage.compact ? 132 : 172
            Layout.fillHeight: true
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.036)
            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: SentinelTheme.spaceMd
                spacing: SentinelTheme.spaceSm

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Settings")
                    color: SentinelTheme.textPrimary
                    font.pixelSize: SentinelTheme.fontCard
                    maximumLineCount: 1
                    elide: Text.ElideRight
                }

                Repeater {
                    model: settingsPage.categories

                    Button {
                        id: navButton
                        required property string modelData
                        readonly property bool active: settingsPage.activeCategory === navButton.modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        text: settingsPage.categoryLabel(modelData)
                        hoverEnabled: true
                        focusPolicy: Qt.StrongFocus
                        onClicked: settingsPage.jumpTo(modelData)

                        contentItem: Text {
                            text: navButton.text
                            leftPadding: SentinelTheme.spaceMd
                            color: navButton.active
                                   ? SentinelTheme.textPrimary
                                   : SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall
                            verticalAlignment: Text.AlignVCenter
                            maximumLineCount: 1
                            elide: Text.ElideRight
                        }

                        background: Rectangle {
                            radius: SentinelTheme.radiusMd
                            color: InteractionTokens.surfaceColor(navButton.hovered, navButton.down, navButton.active,
                                                                   settingsPage.modeAccent)
                            border.color: InteractionTokens.borderColor(navButton.activeFocus, navButton.hovered,
                                                                         navButton.active,
                                                                         settingsPage.modeAccent)

                            Rectangle {
                                width: 3
                                height: navButton.active ? parent.height - SentinelTheme.spaceSm : 0
                                radius: 2
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                color: settingsPage.modeAccent
                                opacity: navButton.active ? 0.86 : 0.0

                                Behavior on height {
                                    NumberAnimation {
                                        duration: MotionTokens.fast
                                        easing.type: MotionTokens.enter
                                    }
                                }

                                Behavior on opacity {
                                    NumberAnimation {
                                        duration: MotionTokens.fast
                                        easing.type: MotionTokens.standard
                                    }
                                }
                            }

                            Behavior on color {
                                ColorAnimation {
                                    duration: MotionTokens.fast
                                    easing.type: MotionTokens.standard
                                }
                            }

                            Behavior on border.color {
                                ColorAnimation {
                                    duration: MotionTokens.fast
                                    easing.type: MotionTokens.standard
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

            Behavior on contentY {
                enabled: settingsPage.programmaticScroll
                NumberAnimation {
                    duration: MotionTokens.duration(MotionTokens.slow, settingsPage.viewModel.currentModeName)
                    easing.type: MotionTokens.enter
                }
            }

            onContentYChanged: {
                if (settingsPage.programmaticScroll)
                    return
                var y = contentY + Math.min(settingsFlick.height * 0.36, 180)
                var atBottom = contentY + settingsFlick.height >= settingsFlick.contentHeight - SentinelTheme.spaceSm
                if (developerMode && (y >= developerSection.y || atBottom))
                    activeCategory = "Advanced"
                else if (y >= diagnosticsSection.y)
                    activeCategory = "Diagnostics"
                else if (y >= updatesSection.y)
                    activeCategory = "Updates"
                else if (y >= notificationsSection.y)
                    activeCategory = "Notifications"
                else if (y >= workspaceSection.y)
                    activeCategory = "Workspace"
                else if (y >= toolsSection.y)
                    activeCategory = "Tools"
                else if (y >= permissionsSection.y)
                    activeCategory = "Permissions"
                else if (y >= brainSection.y)
                    activeCategory = "Brain"
                else if (y >= voiceSection.y)
                    activeCategory = "Voice"
                else if (y >= modelSection.y)
                    activeCategory = "Models"
                else if (y >= localAiSection.y)
                    activeCategory = "AI"
                else if (y >= accessibilitySection.y)
                    activeCategory = "Accessibility"
                else if (y >= appearanceSection.y)
                    activeCategory = "Appearance"
                else
                    activeCategory = "General"
            }

            Timer {
                id: scrollSettledTimer
                interval: MotionTokens.slow + 30
                onTriggered: settingsPage.programmaticScroll = false
            }

            Column {
                id: settingsColumn
                width: settingsFlick.width
                spacing: SentinelTheme.spaceLg

                ShellPanel {
                    id: generalSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(generalContent)

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

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Theme")
                            value: settingsPage.viewModel.themeName
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Profile")
                            value: settingsPage.viewModel.configurationProfile
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                text: qsTr("Language")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                            }

                            ComboBox {
                                id: languageCombo
                                Layout.fillWidth: true
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
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: InteractionTokens.borderColor(languageCombo.activeFocus,
                                                                                 languageCombo.hovered,
                                                                                 languageCombo.popup.visible,
                                                                                 settingsPage.modeAccent)
                                }

                                delegate: ItemDelegate {
                                    id: languageOption
                                    width: languageCombo.width
                                    text: settingsPage.viewModel.languageDisplayName(modelData)
                                    highlighted: languageCombo.highlightedIndex === index

                                    contentItem: Text {
                                        text: languageOption.text
                                        color: languageOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontSmall
                                        verticalAlignment: Text.AlignVCenter
                                        maximumLineCount: 1
                                        elide: Text.ElideRight
                                    }

                                    background: Rectangle {
                                        color: InteractionTokens.surfaceColor(languageOption.highlighted, false, false,
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

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Settings are explicit and local. Disabled runtime features stay inactive until a later phase enables them. Language changes are persisted and may require restart.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall
                            wrapMode: Text.WordWrap
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 2
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            SentinelButton {
                                text: qsTr("Replay Onboarding")
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.replayOnboarding()
                            }

                            SentinelButton {
                                text: qsTr("Save Recovery Draft")
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.seedRecoveryDraft(qsTr("Recovered local draft"))
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: SentinelTheme.radiusMd
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.030)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.055)
                            implicitHeight: companionToggleRow.implicitHeight + SentinelTheme.spaceMd

                            RowLayout {
                                id: companionToggleRow
                                x: SentinelTheme.spaceSm
                                y: SentinelTheme.spaceXs
                                width: parent.width - SentinelTheme.spaceSm * 2
                                spacing: SentinelTheme.spaceSm

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Label {
                                        Layout.fillWidth: true
                                        text: qsTr("Show Sentinel in menu bar / system tray")
                                        color: SentinelTheme.textPrimary
                                        font.pixelSize: SentinelTheme.fontBody
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: settingsPage.viewModel.companionStatus
                                              + qsTr(" / ")
                                              + settingsPage.viewModel.companionAvailability
                                              + qsTr(". Uses native tray/menu bar integration when the platform exposes it.")
                                        color: SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontSmall
                                        wrapMode: Text.WordWrap
                                    }
                                }

                                Switch {
                                    id: companionSwitch
                                    checked: settingsPage.viewModel.companionEnabled
                                    hoverEnabled: true
                                    onToggled: settingsPage.viewModel.companionEnabled = checked

                                    indicator: Rectangle {
                                        implicitWidth: 46
                                        implicitHeight: 24
                                        x: companionSwitch.leftPadding
                                        y: parent.height / 2 - height / 2
                                        radius: height / 2
                                        color: companionSwitch.checked
                                               ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.18)
                                               : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                        border.color: companionSwitch.activeFocus
                                                      ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                      : companionSwitch.hovered
                                                        ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                      : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)

                                        Rectangle {
                                            x: companionSwitch.checked ? parent.width - width - 3 : 3
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: 18
                                            height: 18
                                            radius: 9
                                            color: companionSwitch.checked
                                                   ? settingsPage.modeAccent
                                                   : SentinelTheme.textMuted

                                            Behavior on x {
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

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Companion")
                            value: settingsPage.viewModel.companionSafetyBoundary
                            Layout.fillWidth: true
                            valueMaximumLineCount: 3
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: SentinelTheme.radiusMd
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.030)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.055)
                            implicitHeight: developerToggleRow.implicitHeight + SentinelTheme.spaceMd

                            RowLayout {
                                id: developerToggleRow
                                x: SentinelTheme.spaceSm
                                y: SentinelTheme.spaceXs
                                width: parent.width - SentinelTheme.spaceSm * 2
                                spacing: SentinelTheme.spaceSm

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Label {
                                        Layout.fillWidth: true
                                        text: qsTr("Advanced")
                                        color: SentinelTheme.textPrimary
                                        font.pixelSize: SentinelTheme.fontBody
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: qsTr("Shows read-only diagnostics only. It does not enable tools, voice execution, cloud providers, or runtime authority.")
                                        color: SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontSmall
                                        wrapMode: Text.WordWrap
                                    }
                                }

                                Switch {
                                    id: developerSwitch
                                    checked: settingsPage.viewModel.developerModeEnabled
                                    hoverEnabled: true
                                    onToggled: settingsPage.viewModel.developerModeEnabled = checked

                                    indicator: Rectangle {
                                        implicitWidth: 46
                                        implicitHeight: 24
                                        x: developerSwitch.leftPadding
                                        y: parent.height / 2 - height / 2
                                        radius: height / 2
                                        color: developerSwitch.checked
                                               ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.18)
                                               : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                        border.color: developerSwitch.activeFocus
                                                      ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.46)
                                                      : developerSwitch.hovered
                                                        ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.24)
                                                      : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)

                                        Behavior on color {
                                            ColorAnimation {
                                                duration: MotionTokens.fast
                                                easing.type: MotionTokens.standard
                                            }
                                        }

                                        Behavior on border.color {
                                            ColorAnimation {
                                                duration: MotionTokens.fast
                                                easing.type: MotionTokens.standard
                                            }
                                        }

                                        Rectangle {
                                            x: developerSwitch.checked ? parent.width - width - 3 : 3
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: 18
                                            height: 18
                                            radius: 9
                                            color: developerSwitch.checked
                                                   ? settingsPage.modeAccent
                                                   : SentinelTheme.textMuted

                                            Behavior on x {
                                                NumberAnimation {
                                                    duration: MotionTokens.fast
                                                    easing.type: MotionTokens.enter
                                                }
                                            }

                                            Behavior on color {
                                                ColorAnimation {
                                                    duration: MotionTokens.fast
                                                    easing.type: MotionTokens.standard
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                ShellPanel {
                    id: appearanceSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(appearanceContent)

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
                                text: qsTr("Theme")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                            }

                            ComboBox {
                                id: themeCombo
                                Layout.fillWidth: true
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
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: InteractionTokens.borderColor(themeCombo.activeFocus,
                                                                                 themeCombo.hovered,
                                                                                 themeCombo.popup.visible,
                                                                                 settingsPage.modeAccent)
                                }
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            Repeater {
                                model: settingsPage.themeChoices

                                StatusChip {
                                    required property string modelData
                                    label: qsTr("Theme")
                                    value: modelData
                                    accent: modelData === settingsPage.viewModel.themeName
                                            ? settingsPage.modeAccent
                                            : SentinelTheme.textMuted
                                    selected: modelData === settingsPage.viewModel.themeName
                                    muted: modelData !== settingsPage.viewModel.themeName
                                }
                            }
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Scope")
                            value: qsTr("Theme selection is persisted presentation metadata only; it does not change providers, permissions, models, or runtime behavior.")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 3
                        }
                    }
                }

                ShellPanel {
                    id: accessibilitySection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(accessibilityContent)

                    ColumnLayout {
                        id: accessibilityContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Accessibility")
                            subtitle: qsTr("Keyboard-first controls, persistent comfort preferences, and clear focus states.")
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 3
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            CheckBox {
                                text: qsTr("Reduced motion")
                                checked: settingsPage.viewModel.reducedMotionEnabled
                                onToggled: settingsPage.viewModel.reducedMotionEnabled = checked
                                Layout.fillWidth: true
                            }

                            CheckBox {
                                text: qsTr("High contrast")
                                checked: settingsPage.viewModel.highContrastEnabled
                                onToggled: settingsPage.viewModel.highContrastEnabled = checked
                                Layout.fillWidth: true
                            }

                            ComboBox {
                                Layout.fillWidth: true
                                model: settingsPage.densityChoices
                                currentIndex: settingsPage.densityChoices.indexOf(settingsPage.viewModel.uiDensity)
                                onActivated: settingsPage.viewModel.uiDensity = currentText
                            }
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Support")
                            value: settingsPage.viewModel.accessibilitySummaries.join(" / ")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 8
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Discoverability")
                            value: qsTr("Command Palette opens with Ctrl/Cmd+K. Settings, notifications, updates, exports, workspaces, themes, and controlled task planner are reachable from the palette.")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 4
                        }
                    }
                }

                ShellPanel {
                    id: localAiSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(localAiContent)

                    ColumnLayout {
                        id: localAiContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("AI")
                            subtitle: qsTr("Local-first provider readiness. Future API providers are disabled placeholders.")
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: qsTr("Readiness")
                                value: settingsPage.viewModel.activeRuntimeReadinessState
                                accent: settingsPage.viewModel.activeRuntimeReadinessState === "ready"
                                        ? SentinelTheme.success
                                        : SentinelTheme.textMuted
                                muted: settingsPage.viewModel.activeRuntimeReadinessState !== "ready"
                            }

                            StatusChip {
                                label: qsTr("Provider")
                                value: settingsPage.viewModel.activeRuntimeProviderLabel
                                accent: SentinelTheme.accentTertiary
                            }

                            StatusChip {
                                label: qsTr("Scope")
                                value: settingsPage.viewModel.activeRuntimeLocalOnlySummary
                                accent: SentinelTheme.calmAccent
                                muted: settingsPage.viewModel.activeRuntimeLocalOnlySummary !== "Local Only"
                            }

                            StatusChip {
                                label: qsTr("API Keys")
                                value: qsTr("Not stored")
                                accent: SentinelTheme.textMuted
                                muted: true
                            }
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Cloud Providers")
                            value: qsTr("Not configured / disabled")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Credentials")
                            value: settingsPage.viewModel.providerCredentialRegistrySummary
                            Layout.fillWidth: true
                            valueMaximumLineCount: 3
                        }

                        SectionTitle {
                            title: qsTr("Credential Security")
                            subtitle: qsTr("OS secret-store readiness only; cloud execution remains disabled.")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Store")
                            value: settingsPage.viewModel.credentialStoreSummary
                            Layout.fillWidth: true
                            valueMaximumLineCount: 3
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("OS Backend")
                            value: settingsPage.viewModel.credentialStoreBackendSummary
                            Layout.fillWidth: true
                            valueMaximumLineCount: 2
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Actions")
                            value: settingsPage.viewModel.credentialActionReadiness
                            Layout.fillWidth: true
                            valueMaximumLineCount: 3
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 3
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            SentinelButton {
                                text: qsTr("Add API Key")
                                enabled: false
                                Layout.fillWidth: true
                            }

                            SentinelButton {
                                text: qsTr("Update API Key")
                                enabled: false
                                Layout.fillWidth: true
                            }

                            SentinelButton {
                                text: qsTr("Remove API Key")
                                enabled: false
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                text: qsTr("Provider")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                            }

                            ComboBox {
                                id: runtimeProviderCombo
                                Layout.fillWidth: true
                                hoverEnabled: true
                                model: settingsPage.viewModel.selectableRuntimeProviderLabels
                                currentIndex: settingsPage.viewModel.selectableRuntimeProviderIds.indexOf(settingsPage.viewModel.selectedRuntimeProvider)
                                displayText: currentIndex >= 0 ? currentText : settingsPage.viewModel.activeRuntimeProviderLabel
                                onActivated: {
                                    if (index >= 0 && index < settingsPage.viewModel.selectableRuntimeProviderIds.length)
                                        settingsPage.viewModel.selectedRuntimeProvider = settingsPage.viewModel.selectableRuntimeProviderIds[index]
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
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: InteractionTokens.borderColor(runtimeProviderCombo.activeFocus,
                                                                                 runtimeProviderCombo.hovered,
                                                                                 runtimeProviderCombo.popup.visible,
                                                                                 settingsPage.modeAccent)
                                }

                                delegate: ItemDelegate {
                                    id: runtimeProviderOption
                                    width: runtimeProviderCombo.width
                                    text: modelData
                                    highlighted: runtimeProviderCombo.highlightedIndex === index

                                    contentItem: Text {
                                        text: runtimeProviderOption.text
                                        color: runtimeProviderOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontSmall
                                        verticalAlignment: Text.AlignVCenter
                                        maximumLineCount: 1
                                        elide: Text.ElideRight
                                    }

                                    background: Rectangle {
                                        color: InteractionTokens.surfaceColor(runtimeProviderOption.highlighted, false, false,
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

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Active Model")
                            value: settingsPage.viewModel.activeRuntimeModelLabel
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Readiness")
                            value: settingsPage.viewModel.activeRuntimeReadinessSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Endpoint")
                            value: settingsPage.viewModel.ollamaEndpoint
                            Layout.fillWidth: true
                        }

                        Repeater {
                            model: settingsPage.viewModel.runtimeProviderCardSummaries

                            Rectangle {
                                required property string modelData
                                Layout.fillWidth: true
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.030)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                implicitHeight: runtimeProviderCardLabel.implicitHeight + SentinelTheme.spaceMd

                                Label {
                                    id: runtimeProviderCardLabel
                                    x: SentinelTheme.spaceSm
                                    y: SentinelTheme.spaceXs
                                    width: parent.width - SentinelTheme.spaceSm * 2
                                    text: modelData
                                    color: modelData.indexOf("disabled") >= 0
                                           ? SentinelTheme.textMuted
                                           : SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontSmall
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }

                        Repeater {
                            model: settingsPage.viewModel.providerCredentialReadinessSummaries

                            Rectangle {
                                required property string modelData
                                Layout.fillWidth: true
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.024)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.050)
                                implicitHeight: credentialCardLabel.implicitHeight + SentinelTheme.spaceMd

                                Label {
                                    id: credentialCardLabel
                                    x: SentinelTheme.spaceSm
                                    y: SentinelTheme.spaceXs
                                    width: parent.width - SentinelTheme.spaceSm * 2
                                    text: modelData
                                    color: modelData.indexOf("execution disabled") >= 0
                                           ? SentinelTheme.textMuted
                                           : SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontSmall
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }

                ShellPanel {
                    id: modelSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(modelContent)

                    ColumnLayout {
                        id: modelContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Models")
                            subtitle: qsTr("Model Library foundation and local-first selection metadata.")
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: modelCombo
                            Layout.fillWidth: true
                            enabled: settingsPage.viewModel.ollamaModelCount > 0
                            hoverEnabled: true
                            model: settingsPage.viewModel.ollamaModelNames
                            currentIndex: settingsPage.viewModel.ollamaModelNames.indexOf(settingsPage.viewModel.selectedLocalModel)
                            displayText: currentIndex >= 0 ? currentText
                                                           : settingsPage.viewModel.selectedLocalModelStatus
                                                             + qsTr(" / No model selected")
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
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                border.color: InteractionTokens.borderColor(modelCombo.activeFocus,
                                                                             modelCombo.hovered,
                                                                             modelCombo.popup.visible,
                                                                             settingsPage.modeAccent)
                            }

                            delegate: ItemDelegate {
                                id: modelOption
                                width: modelCombo.width
                                text: modelData
                                highlighted: modelCombo.highlightedIndex === index

                                contentItem: Text {
                                    text: modelOption.text
                                    color: modelOption.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    verticalAlignment: Text.AlignVCenter
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 2
                                }

                                background: Rectangle {
                                    color: InteractionTokens.surfaceColor(modelOption.highlighted, false, false,
                                                                           settingsPage.modeAccent)
                                }
                            }

                            popup.background: Rectangle {
                                radius: SentinelTheme.radiusLg
                                color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                border.color: InteractionTokens.borderColor(false, true, false,
                                                                             settingsPage.modeAccent)
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

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Provider")
                            value: settingsPage.viewModel.activeRuntimeProviderLabel
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Readiness")
                            value: settingsPage.viewModel.activeRuntimeReadinessState
                                   + " - " + settingsPage.viewModel.localChatSendAvailabilitySummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Scope")
                            value: settingsPage.viewModel.activeRuntimeLocalOnlySummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Selected")
                            value: settingsPage.viewModel.selectedLocalModelSummary
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            visible: settingsPage.viewModel.selectedModelCapabilityLabels.length > 0
                            spacing: SentinelTheme.spaceSm

                            Repeater {
                                model: settingsPage.viewModel.selectedModelCapabilityLabels

                                StatusChip {
                                    required property string modelData
                                    label: qsTr("Capability")
                                    value: modelData
                                    accent: settingsPage.modeAccent
                                    selected: true
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: settingsPage.developerMode
                            spacing: SentinelTheme.spaceXs

                            InfoRow {
                                compact: settingsPage.compact
                                label: qsTr("Registry")
                                value: settingsPage.viewModel.modelRegistrySummary
                                Layout.fillWidth: true
                            }

                            Repeater {
                                model: Math.min(3, settingsPage.viewModel.modelRegistryModelSummaries.length)

                                InfoRow {
                                    required property int index
                                    compact: settingsPage.compact
                                    label: qsTr("Model Metadata")
                                    value: settingsPage.viewModel.modelRegistryModelSummaries[index]
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        SectionTitle {
                            title: qsTr("Model Library")
                            subtitle: qsTr("Installed, discoverable, and recommended local AI metadata.")
                            Layout.fillWidth: true
                        }

                        Repeater {
                            model: Math.min(4, settingsPage.viewModel.modelLibraryInstalledSummaries.length)

                            InfoRow {
                                required property int index
                                compact: settingsPage.compact
                                label: index === 0 ? qsTr("Installed") : qsTr("Installed Model")
                                value: settingsPage.viewModel.modelLibraryInstalledSummaries[index]
                                Layout.fillWidth: true
                            }
                        }

                        Repeater {
                            model: Math.min(4, settingsPage.viewModel.modelLibraryRecommendedSummaries.length)

                            InfoRow {
                                required property int index
                                compact: settingsPage.compact
                                label: index === 0 ? qsTr("Recommended") : qsTr("Recommendation")
                                value: settingsPage.viewModel.modelLibraryRecommendedSummaries[index]
                                Layout.fillWidth: true
                            }
                        }

                        SectionTitle {
                            title: qsTr("Provider Status")
                            subtitle: qsTr("Discovery metadata only. No background probing or hidden network calls.")
                            Layout.fillWidth: true
                        }

                        Repeater {
                            model: settingsPage.viewModel.providerDiscoverySummaries

                            InfoRow {
                                required property string modelData
                                compact: settingsPage.compact
                                label: qsTr("Provider")
                                value: modelData
                                Layout.fillWidth: true
                            }
                        }

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

                        SectionTitle {
                            title: qsTr("Model Advisor")
                            subtitle: qsTr("Deterministic local recommendations from static metadata.")
                            Layout.fillWidth: true
                        }

                        Repeater {
                            model: Math.min(3, settingsPage.viewModel.modelAdvisorRecommendationSummaries.length)

                            InfoRow {
                                required property int index
                                compact: settingsPage.compact
                                label: qsTr("Advisor")
                                value: settingsPage.viewModel.modelAdvisorRecommendationSummaries[index]
                                Layout.fillWidth: true
                            }
                        }

                        Repeater {
                            model: Math.min(2, settingsPage.viewModel.modelAdvisorAvoidSummaries.length)

                            InfoRow {
                                required property int index
                                compact: settingsPage.compact
                                label: qsTr("Avoid")
                                value: settingsPage.viewModel.modelAdvisorAvoidSummaries[index]
                                Layout.fillWidth: true
                            }
                        }

                        SectionTitle {
                            title: qsTr("Downloads")
                            subtitle: qsTr("Download center foundation. Actions are disabled.")
                            Layout.fillWidth: true
                        }

                        Repeater {
                            model: Math.min(3, settingsPage.viewModel.downloadsCenterSummaries.length)

                            InfoRow {
                                required property int index
                                compact: settingsPage.compact
                                label: qsTr("Download")
                                value: settingsPage.viewModel.downloadsCenterSummaries[index]
                                Layout.fillWidth: true
                            }
                        }

                        SectionTitle {
                            title: qsTr("Benchmark")
                            subtitle: qsTr("Manual/placeholder performance metadata only.")
                            Layout.fillWidth: true
                        }

                        Repeater {
                            model: Math.min(3, settingsPage.viewModel.benchmarkHubSummaries.length)

                            InfoRow {
                                required property int index
                                compact: settingsPage.compact
                                label: qsTr("Benchmark")
                                value: settingsPage.viewModel.benchmarkHubSummaries[index]
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                ShellPanel {
                    id: chatSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(chatContent)

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

                        CheckBox {
                            id: localChatToggle
                            Layout.fillWidth: true
                            text: qsTr("Local chat inference")
                            hoverEnabled: true
                            leftPadding: localChatToggle.indicator.width + SentinelTheme.spaceSm
                            checked: settingsPage.viewModel.localChatInferenceEnabled
                            onToggled: settingsPage.viewModel.localChatInferenceEnabled = checked
                            contentItem: Text {
                                text: localChatToggle.text
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                            }
                            indicator: Rectangle {
                                implicitWidth: 18
                                implicitHeight: 18
                                x: localChatToggle.leftPadding
                                y: parent.height / 2 - height / 2
                                radius: SentinelTheme.radiusSm
                                color: localChatToggle.checked
                                       ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.16)
                                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.045)
                                border.color: InteractionTokens.borderColor(localChatToggle.activeFocus,
                                                                             localChatToggle.hovered,
                                                                             localChatToggle.checked,
                                                                             settingsPage.modeAccent)
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 8
                                    height: 8
                                    radius: 4
                                    visible: localChatToggle.checked
                                    color: settingsPage.modeAccent
                                }

                                Behavior on color {
                                    ColorAnimation {
                                        duration: MotionTokens.fast
                                        easing.type: MotionTokens.standard
                                    }
                                }
                            }
                        }

                        CheckBox {
                            id: streamingToggle
                            Layout.fillWidth: true
                            text: qsTr("Local response streaming")
                            hoverEnabled: true
                            leftPadding: streamingToggle.indicator.width + SentinelTheme.spaceSm
                            checked: settingsPage.viewModel.localInferenceStreamingEnabled
                            onToggled: settingsPage.viewModel.localInferenceStreamingEnabled = checked
                            contentItem: Text {
                                text: streamingToggle.text
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                            }
                            indicator: Rectangle {
                                implicitWidth: 18
                                implicitHeight: 18
                                x: streamingToggle.leftPadding
                                y: parent.height / 2 - height / 2
                                radius: SentinelTheme.radiusSm
                                color: streamingToggle.checked
                                       ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.16)
                                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.045)
                                border.color: InteractionTokens.borderColor(streamingToggle.activeFocus,
                                                                             streamingToggle.hovered,
                                                                             streamingToggle.checked,
                                                                             settingsPage.modeAccent)
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 8
                                    height: 8
                                    radius: 4
                                    visible: streamingToggle.checked
                                    color: settingsPage.modeAccent
                                }

                                Behavior on color {
                                    ColorAnimation {
                                        duration: MotionTokens.fast
                                        easing.type: MotionTokens.standard
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                text: qsTr("Timeout")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                            }

                            SpinBox {
                                id: timeoutSpin
                                Layout.preferredWidth: 150
                                from: 1000
                                to: 300000
                                stepSize: 1000
                                value: settingsPage.viewModel.localInferenceTimeoutMs
                                editable: true
                                onValueModified: settingsPage.viewModel.localInferenceTimeoutMs = value

                                contentItem: TextInput {
                                    text: timeoutSpin.textFromValue(timeoutSpin.value, timeoutSpin.locale)
                                    color: SentinelTheme.textPrimary
                                    selectionColor: SentinelTheme.withAlpha(settingsPage.modeAccent, 0.34)
                                    selectedTextColor: SentinelTheme.textPrimary
                                    horizontalAlignment: Qt.AlignHCenter
                                    verticalAlignment: Qt.AlignVCenter
                                    font.pixelSize: SentinelTheme.fontBody
                                    validator: timeoutSpin.validator
                                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: InteractionTokens.borderColor(timeoutSpin.activeFocus,
                                                                                 timeoutSpin.hovered,
                                                                                 false,
                                                                                 settingsPage.modeAccent)
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("ms")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                            }
                        }

                        CheckBox {
                            id: contextToggle
                            Layout.fillWidth: true
                            text: qsTr("Use local memory/context in chat")
                            hoverEnabled: true
                            leftPadding: contextToggle.indicator.width + SentinelTheme.spaceSm
                            checked: settingsPage.viewModel.promptContextInjectionEnabled
                            onToggled: settingsPage.viewModel.promptContextInjectionEnabled = checked
                            contentItem: Text {
                                text: contextToggle.text
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                            }
                            indicator: Rectangle {
                                implicitWidth: 18
                                implicitHeight: 18
                                x: contextToggle.leftPadding
                                y: parent.height / 2 - height / 2
                                radius: SentinelTheme.radiusSm
                                color: contextToggle.checked
                                       ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.16)
                                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.045)
                                border.color: InteractionTokens.borderColor(contextToggle.activeFocus,
                                                                             contextToggle.hovered,
                                                                             contextToggle.checked,
                                                                             settingsPage.modeAccent)
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 8
                                    height: 8
                                    radius: 4
                                    visible: contextToggle.checked
                                    color: settingsPage.modeAccent
                                }

                                Behavior on color {
                                    ColorAnimation {
                                        duration: MotionTokens.fast
                                        easing.type: MotionTokens.standard
                                    }
                                }
                            }
                        }

                        CheckBox {
                            id: contextReasoningVisibilityToggle
                            Layout.fillWidth: true
                            text: qsTr("Show context reasoning")
                            hoverEnabled: true
                            focusPolicy: Qt.StrongFocus
                            leftPadding: contextReasoningVisibilityToggle.indicator.width + SentinelTheme.spaceSm
                            checked: settingsPage.viewModel.contextExplainabilityVisible
                            onToggled: settingsPage.viewModel.contextExplainabilityVisible = checked
                            contentItem: Text {
                                text: contextReasoningVisibilityToggle.text
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.WordWrap
                            }
                            indicator: Rectangle {
                                implicitWidth: 18
                                implicitHeight: 18
                                x: contextReasoningVisibilityToggle.leftPadding
                                y: parent.height / 2 - height / 2
                                radius: SentinelTheme.radiusSm
                                color: contextReasoningVisibilityToggle.checked
                                       ? SentinelTheme.withAlpha(settingsPage.modeAccent, 0.16)
                                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.045)
                                border.color: InteractionTokens.borderColor(contextReasoningVisibilityToggle.activeFocus,
                                                                             contextReasoningVisibilityToggle.hovered,
                                                                             contextReasoningVisibilityToggle.checked,
                                                                             settingsPage.modeAccent)
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 8
                                    height: 8
                                    radius: 4
                                    visible: contextReasoningVisibilityToggle.checked
                                    color: settingsPage.modeAccent
                                }

                                Behavior on color {
                                    ColorAnimation {
                                        duration: MotionTokens.fast
                                        easing.type: MotionTokens.standard
                                    }
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: !settingsPage.viewModel.contextExplainabilityVisible
                            text: qsTr("Context reasoning is hidden from UI; runtime behavior is unchanged.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall
                            wrapMode: Text.WordWrap
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Routing")
                            value: settingsPage.viewModel.localChatInferenceStatus + " / "
                                   + settingsPage.viewModel.localChatInferenceSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Context")
                            value: (settingsPage.viewModel.promptContextInjectionEnabled ? "On / " : "Off / ")
                                   + settingsPage.viewModel.promptContextInjectionStatus
                                   + " / "
                                   + settingsPage.viewModel.conversationSalienceIncludedCount
                                   + " adaptive items"
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Summary Injection")
                            value: (settingsPage.viewModel.promptContextInjectionEnabled ? "Enabled / " : "Disabled / ")
                                   + settingsPage.viewModel.summaryContinuityStatus
                                   + " / "
                                   + settingsPage.viewModel.conversationSummaryInjectionSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Explainability")
                            value: (settingsPage.viewModel.contextExplainabilityVisible ? "Visible / " : "Hidden / ")
                                   + settingsPage.viewModel.contextReasoningSummary
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

                ShellPanel {
                    id: voiceSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(voiceContent)

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
                                Layout.fillWidth: true
                                text: qsTr("Piper binary")
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                            }

                            SentinelTextField {
                                id: piperBinaryField
                                Layout.fillWidth: true
                                text: settingsPage.viewModel.piperBinaryPath
                                placeholderText: qsTr("Piper binary path")
                                onEditingFinished: settingsPage.viewModel.piperBinaryPath = text
                            }

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Piper model")
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                            }

                            SentinelTextField {
                                id: piperModelField
                                Layout.fillWidth: true
                                text: settingsPage.viewModel.piperModelPath
                                placeholderText: qsTr("Piper .onnx model path")
                                onEditingFinished: settingsPage.viewModel.piperModelPath = text
                            }

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
                                onEditingFinished: settingsPage.viewModel.whisperBinaryPath = text
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
                                placeholderText: qsTr("Whisper model folder or model file")
                                onEditingFinished: settingsPage.viewModel.whisperModelPath = text
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

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Readiness")
                            value: settingsPage.voiceReadinessLine()
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Whisper")
                            value: settingsPage.viewModel.whisperTranscriptionStatus
                                   + " / "
                                   + settingsPage.viewModel.whisperTranscriptionReadinessSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Piper")
                            value: settingsPage.viewModel.piperSynthesisStatus
                                   + " / "
                                   + settingsPage.viewModel.piperSynthesisReadinessSummary
                            Layout.fillWidth: true
                        }
                    }
                }

                ShellPanel {
                    id: brainSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(privacyContent)

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

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: qsTr("Memory")
                                value: settingsPage.viewModel.memoryStatus
                                accent: settingsPage.viewModel.memoryStatus === "Available"
                                        ? SentinelTheme.success
                                        : SentinelTheme.textMuted
                                muted: settingsPage.viewModel.memoryStatus !== "Available"
                            }

                            StatusChip {
                                label: qsTr("Recall")
                                value: settingsPage.viewModel.memoryRecallStatus
                                accent: SentinelTheme.calmAccent
                                muted: settingsPage.viewModel.memoryRecallResultCount === 0
                            }

                            StatusChip {
                                label: qsTr("Context")
                                value: settingsPage.viewModel.promptContextInjectionEnabled ? qsTr("opt-in on")
                                                                                            : qsTr("opt-in off")
                                accent: settingsPage.viewModel.promptContextInjectionEnabled
                                        ? SentinelTheme.success
                                        : SentinelTheme.textMuted
                                muted: !settingsPage.viewModel.promptContextInjectionEnabled
                            }
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

                ShellPanel {
                    id: permissionsSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(permissionsContent)

                    ColumnLayout {
                        id: permissionsContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Permissions")
                            subtitle: qsTr("Central authority posture. These states are metadata only and do not enable execution.")
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: qsTr("Status")
                                value: settingsPage.viewModel.permissionPolicyStatus
                                accent: SentinelTheme.textMuted
                                muted: true
                            }

                            StatusChip {
                                label: qsTr("Default")
                                value: settingsPage.viewModel.defaultPermissionPolicyState
                                accent: SentinelTheme.textMuted
                                muted: true
                            }

                            StatusChip {
                                label: qsTr("Domains")
                                value: String(settingsPage.viewModel.permissionPolicyDomainIds.length)
                                accent: SentinelTheme.calmAccent
                                muted: false
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                text: qsTr("Default")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                            }

                            ComboBox {
                                id: permissionStateCombo
                                Layout.fillWidth: true
                                hoverEnabled: true
                                model: settingsPage.viewModel.permissionPolicyStateLabels
                                currentIndex: settingsPage.viewModel.permissionPolicyStateLabels.indexOf(settingsPage.viewModel.defaultPermissionPolicyState)
                                displayText: currentIndex >= 0 ? currentText : settingsPage.viewModel.defaultPermissionPolicyState
                                onActivated: {
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

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Boundary")
                            value: settingsPage.viewModel.permissionPolicySummary
                            Layout.fillWidth: true
                            valueMaximumLineCount: 4
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("States")
                            value: settingsPage.viewModel.permissionPolicyStateLabels.join(" / ")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 2
                        }

                        Repeater {
                            model: settingsPage.viewModel.permissionPolicyDomainSummaries

                            Rectangle {
                                required property string modelData
                                Layout.fillWidth: true
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.024)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.050)
                                implicitHeight: permissionDomainLabel.implicitHeight + SentinelTheme.spaceMd

                                Label {
                                    id: permissionDomainLabel
                                    x: SentinelTheme.spaceSm
                                    y: SentinelTheme.spaceXs
                                    width: parent.width - SentinelTheme.spaceSm * 2
                                    text: modelData
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }

                ShellPanel {
                    id: toolsSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(toolsContent)

                    ColumnLayout {
                        id: toolsContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Tools")
                            subtitle: qsTr("Gateway readiness only. No tool can run from this section.")
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: qsTr("Status")
                                value: settingsPage.viewModel.toolGatewayStatus
                                accent: SentinelTheme.textMuted
                                muted: true
                            }

                            StatusChip {
                                label: qsTr("Tools")
                                value: String(settingsPage.viewModel.toolGatewayToolCount)
                                accent: settingsPage.modeAccent
                                selected: true
                            }

                            StatusChip {
                                label: qsTr("Metadata")
                                value: String(settingsPage.viewModel.toolGatewayMetadataSafeCount)
                                accent: SentinelTheme.calmAccent
                            }

                            StatusChip {
                                label: qsTr("Unavailable")
                                value: String(settingsPage.viewModel.toolGatewayUnavailableCount)
                                accent: SentinelTheme.textMuted
                                muted: true
                            }

                            StatusChip {
                                label: qsTr("Refused")
                                value: String(settingsPage.viewModel.toolGatewayRefusedCount)
                                accent: settingsPage.viewModel.toolGatewayRefusedCount > 0
                                        ? SentinelTheme.warning
                                        : SentinelTheme.textMuted
                                muted: settingsPage.viewModel.toolGatewayRefusedCount === 0
                            }
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Permission")
                            value: settingsPage.viewModel.toolGatewayPermissionPosture
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Boundary")
                            value: settingsPage.viewModel.toolGatewaySummary
                            Layout.fillWidth: true
                            valueMaximumLineCount: 4
                        }

                        Repeater {
                            model: settingsPage.viewModel.toolGatewayToolSummaries

                            Rectangle {
                                required property string modelData
                                Layout.fillWidth: true
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.024)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.050)
                                implicitHeight: toolGatewayLabel.implicitHeight + SentinelTheme.spaceMd

                                Label {
                                    id: toolGatewayLabel
                                    x: SentinelTheme.spaceSm
                                    y: SentinelTheme.spaceXs
                                    width: parent.width - SentinelTheme.spaceSm * 2
                                    text: modelData
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }

                ShellPanel {
                    id: agentsSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(agentsContent)

                    ColumnLayout {
                        id: agentsContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Agents")
                            subtitle: qsTr("Agent execution is disabled. Sentinel can only prepare dry-run plan metadata.")
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: qsTr("Status")
                                value: settingsPage.viewModel.agentRuntimeStatus
                                accent: SentinelTheme.textMuted
                                muted: true
                            }

                            StatusChip {
                                label: qsTr("Agents")
                                value: String(settingsPage.viewModel.agentRuntimeAgentCount)
                                accent: settingsPage.modeAccent
                                selected: true
                            }

                            StatusChip {
                                label: qsTr("Ready")
                                value: String(settingsPage.viewModel.agentRuntimeReadyAgentCount)
                                accent: SentinelTheme.calmAccent
                            }

                            StatusChip {
                                label: qsTr("Approval")
                                value: settingsPage.viewModel.agentRuntimeApprovalPosture
                                accent: SentinelTheme.warning
                                muted: false
                            }
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Boundary")
                            value: settingsPage.viewModel.agentRuntimeSummary
                            Layout.fillWidth: true
                            valueMaximumLineCount: 4
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Plan Preview")
                            value: settingsPage.viewModel.agentPlanGoalSummary + " / "
                                   + settingsPage.viewModel.agentPlanApprovalState
                            Layout.fillWidth: true
                            valueMaximumLineCount: 4
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Refusal")
                            value: settingsPage.viewModel.agentPlanRefusalReason
                            Layout.fillWidth: true
                            valueMaximumLineCount: 4
                        }

                        Repeater {
                            model: settingsPage.viewModel.agentRuntimeReadinessSummaries

                            Rectangle {
                                required property string modelData
                                Layout.fillWidth: true
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.024)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.050)
                                implicitHeight: agentReadinessLabel.implicitHeight + SentinelTheme.spaceMd

                                Label {
                                    id: agentReadinessLabel
                                    x: SentinelTheme.spaceSm
                                    y: SentinelTheme.spaceXs
                                    width: parent.width - SentinelTheme.spaceSm * 2
                                    text: modelData
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }

                ShellPanel {
                    id: profilesSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(profilesContent)

                    ColumnLayout {
                        id: profilesContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Profiles")
                            subtitle: qsTr("Assistant presentation profiles. Runtime authority is unchanged.")
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: qsTr("Selected")
                                value: settingsPage.viewModel.selectedSkillProfileName
                                accent: SentinelTheme.calmAccent
                                muted: false
                            }

                            StatusChip {
                                label: qsTr("Readiness")
                                value: settingsPage.viewModel.selectedSkillProfileReadiness
                                accent: SentinelTheme.textMuted
                                muted: true
                            }

                            StatusChip {
                                label: qsTr("Authority")
                                value: qsTr("Unchanged")
                                accent: SentinelTheme.textMuted
                                muted: true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                text: qsTr("Selected")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                            }

                            ComboBox {
                                id: profileCombo
                                Layout.fillWidth: true
                                hoverEnabled: true
                                model: settingsPage.viewModel.skillProfileNames
                                currentIndex: settingsPage.viewModel.skillProfileIds.indexOf(settingsPage.viewModel.selectedSkillProfile)
                                displayText: currentIndex >= 0 ? currentText : settingsPage.viewModel.selectedSkillProfileName
                                onActivated: {
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

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Summary")
                            value: settingsPage.viewModel.selectedSkillProfileSummary
                            Layout.fillWidth: true
                            valueMaximumLineCount: 3
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Description")
                            value: settingsPage.viewModel.selectedSkillProfileDescription
                            Layout.fillWidth: true
                            valueMaximumLineCount: 4
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Boundary")
                            value: settingsPage.viewModel.selectedSkillProfilePolicyPosture
                            Layout.fillWidth: true
                            valueMaximumLineCount: 4
                        }

                        Repeater {
                            model: settingsPage.viewModel.skillProfileReadinessChecks

                            Rectangle {
                                required property string modelData
                                Layout.fillWidth: true
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.024)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.050)
                                implicitHeight: profileCheckLabel.implicitHeight + SentinelTheme.spaceMd

                                Label {
                                    id: profileCheckLabel
                                    x: SentinelTheme.spaceSm
                                    y: SentinelTheme.spaceXs
                                    width: parent.width - SentinelTheme.spaceSm * 2
                                    text: modelData
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }

                ShellPanel {
                    id: workspaceSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(workspaceContent)

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

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: qsTr("Access")
                                value: settingsPage.viewModel.selectedWorkspaceAccessState
                                accent: SentinelTheme.textMuted
                                muted: true
                            }

                            StatusChip {
                                label: qsTr("Readiness")
                                value: settingsPage.viewModel.workspaceReadinessStatus
                                accent: SentinelTheme.textMuted
                                muted: true
                            }

                            StatusChip {
                                label: qsTr("Mode")
                                value: qsTr("Workspace only")
                                accent: SentinelTheme.calmAccent
                                muted: false
                            }

                            StatusChip {
                                label: qsTr("Permission")
                                value: settingsPage.viewModel.workspacePermissionPosture
                                accent: SentinelTheme.textMuted
                                muted: true
                            }
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

                                    Label {
                                        Layout.fillWidth: true
                                        text: settingsPage.viewModel.localKnowledgeBaseStatus
                                              + qsTr(" / indexing is manual only / document scope is workspace only")
                                        color: SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontSmall
                                        wrapMode: Text.WordWrap
                                    }
                                }

                                Switch {
                                    checked: settingsPage.viewModel.localKnowledgeBaseEnabled
                                    hoverEnabled: true
                                    onToggled: settingsPage.viewModel.localKnowledgeBaseEnabled = checked
                                }
                            }
                        }

                        Repeater {
                            model: settingsPage.viewModel.privacyCenterSummaries

                            InfoRow {
                                required property string modelData
                                compact: settingsPage.compact
                                label: qsTr("Privacy")
                                value: modelData
                                Layout.fillWidth: true
                            }
                        }

                        Repeater {
                            model: settingsPage.viewModel.exportCenterSummaries

                            InfoRow {
                                required property string modelData
                                compact: settingsPage.compact
                                label: qsTr("Export")
                                value: modelData
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.preferredWidth: settingsPage.compact ? 88 : 132
                                text: qsTr("Selected")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                elide: Text.ElideRight
                            }

                            ComboBox {
                                id: workspaceCombo
                                Layout.fillWidth: true
                                hoverEnabled: true
                                model: settingsPage.viewModel.workspaceNames
                                currentIndex: settingsPage.viewModel.workspaceIds.indexOf(settingsPage.viewModel.selectedWorkspaceId)
                                displayText: currentIndex >= 0 ? currentText : settingsPage.viewModel.selectedWorkspaceName
                                onActivated: {
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

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Boundary")
                            value: settingsPage.viewModel.workspaceReadinessSummary
                            Layout.fillWidth: true
                            valueMaximumLineCount: 4
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Root")
                            value: settingsPage.viewModel.selectedWorkspaceRootSummary
                            Layout.fillWidth: true
                            valueMaximumLineCount: 3
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Posture")
                            value: settingsPage.viewModel.workspacePermissionPosture
                                   + " / "
                                   + settingsPage.viewModel.workspacePermissionPostures.join(" / ")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 3
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Policy")
                            value: settingsPage.viewModel.workspacePermissionSummary
                            Layout.fillWidth: true
                            valueMaximumLineCount: 3
                        }

                        Repeater {
                            model: settingsPage.viewModel.workspaceReadinessChecks

                            Rectangle {
                                required property string modelData
                                Layout.fillWidth: true
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.024)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.050)
                                implicitHeight: workspaceCheckLabel.implicitHeight + SentinelTheme.spaceMd

                                Label {
                                    id: workspaceCheckLabel
                                    x: SentinelTheme.spaceSm
                                    y: SentinelTheme.spaceXs
                                    width: parent.width - SentinelTheme.spaceSm * 2
                                    text: modelData
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }

                        Repeater {
                            model: settingsPage.viewModel.workspaceActionPlaceholders

                            Rectangle {
                                required property string modelData
                                Layout.fillWidth: true
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.48)
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.045)
                                implicitHeight: workspaceActionLabel.implicitHeight + SentinelTheme.spaceMd

                                Label {
                                    id: workspaceActionLabel
                                    x: SentinelTheme.spaceSm
                                    y: SentinelTheme.spaceXs
                                    width: parent.width - SentinelTheme.spaceSm * 2
                                    text: modelData
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 2
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            SentinelButton {
                                text: qsTr("Choose Workspace")
                                enabled: false
                                Layout.fillWidth: true
                            }

                            SentinelButton {
                                text: qsTr("Clear Workspace")
                                enabled: false
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                ShellPanel {
                    id: notificationsSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(notificationsContent)

                    ColumnLayout {
                        id: notificationsContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Notifications")
                            subtitle: qsTr("In-app notification center. No background polling or hidden notifications.")
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            Repeater {
                                model: [qsTr("Disabled"), qsTr("Important Only"), qsTr("All"), qsTr("Custom")]

                                StatusChip {
                                    required property string modelData
                                    label: qsTr("Policy")
                                    value: modelData
                                    accent: modelData === settingsPage.viewModel.notificationPolicy ? settingsPage.modeAccent
                                                                                                     : SentinelTheme.textMuted
                                    selected: modelData === settingsPage.viewModel.notificationPolicy
                                    muted: modelData !== settingsPage.viewModel.notificationPolicy
                                }
                            }
                        }

                        ComboBox {
                            id: notificationPolicyCombo
                            Layout.fillWidth: true
                            model: settingsPage.notificationPolicies
                            currentIndex: settingsPage.notificationPolicies.indexOf(settingsPage.viewModel.notificationPolicy)
                            onActivated: settingsPage.viewModel.notificationPolicy = currentText
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 2
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            SentinelTextField {
                                Layout.fillWidth: true
                                placeholderText: qsTr("Search notifications")
                                text: settingsPage.viewModel.notificationSearchQuery
                                onTextChanged: settingsPage.viewModel.notificationSearchQuery = text
                            }

                            ComboBox {
                                Layout.fillWidth: true
                                model: settingsPage.viewModel.notificationCategories
                                currentIndex: settingsPage.viewModel.notificationCategories.indexOf(settingsPage.viewModel.notificationCategoryFilter)
                                onActivated: settingsPage.viewModel.notificationCategoryFilter = currentText
                            }
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Center")
                            value: settingsPage.viewModel.notificationFilteredSummaries.join(" / ")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 8
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Categories")
                            value: settingsPage.viewModel.notificationCategories.join(", ")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 3
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Lifecycle")
                            value: settingsPage.viewModel.notificationLifecycleSummaries.join(" / ")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 4
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 4
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            SentinelButton {
                                text: qsTr("Pin Update")
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.pinNotification("updates-manual")
                            }

                            SentinelButton {
                                text: qsTr("Mark Security Read")
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.markNotificationRead("security-privacy")
                            }

                            SentinelButton {
                                text: qsTr("Archive Workspace")
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.archiveNotification("workspace-active")
                            }

                            SentinelButton {
                                text: qsTr("Clear Archived")
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.clearArchivedNotifications()
                            }
                        }
                    }
                }

                ShellPanel {
                    id: updatesSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(updatesContent)

                    ColumnLayout {
                        id: updatesContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Updates")
                            subtitle: qsTr("About and update UX foundation. No network check, auto-download, auto-install, or telemetry.")
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 2
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            InfoRow {
                                compact: true
                                label: qsTr("Version")
                                value: Qt.application.version.length > 0 ? Qt.application.version : qsTr("Desktop Alpha")
                                Layout.fillWidth: true
                            }

                            InfoRow {
                                compact: true
                                label: qsTr("Qt")
                                value: qsTr("Qt 6 native shell")
                                Layout.fillWidth: true
                            }

                            InfoRow {
                                compact: true
                                label: qsTr("Platform")
                                value: settingsPage.viewModel.companionPlatformCapability
                                Layout.fillWidth: true
                            }

                            InfoRow {
                                compact: true
                                label: qsTr("Build")
                                value: settingsPage.viewModel.aboutSentinelSummaries.join(" / ")
                                Layout.fillWidth: true
                                valueMaximumLineCount: 6
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            Repeater {
                                model: [qsTr("Never"), qsTr("Ask Before Checking"), qsTr("Weekly"), qsTr("On Startup")]

                                StatusChip {
                                    required property string modelData
                                    label: qsTr("Check Policy")
                                    value: modelData
                                    accent: modelData === settingsPage.viewModel.updateCheckPolicy ? settingsPage.modeAccent
                                                                                                    : SentinelTheme.textMuted
                                    selected: modelData === settingsPage.viewModel.updateCheckPolicy
                                    muted: modelData !== settingsPage.viewModel.updateCheckPolicy
                                }
                            }
                        }

                        ComboBox {
                            id: updatePolicyCombo
                            Layout.fillWidth: true
                            model: settingsPage.updatePolicies
                            currentIndex: settingsPage.updatePolicies.indexOf(settingsPage.viewModel.updateCheckPolicy)
                            onActivated: settingsPage.viewModel.updateCheckPolicy = currentText
                        }

                        SentinelButton {
                            text: qsTr("Check for Updates")
                            enabled: true
                            Layout.preferredWidth: 180
                            onClicked: settingsPage.viewModel.checkForUpdates()
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("State")
                            value: settingsPage.viewModel.updateWorkflowState
                            Layout.fillWidth: true
                            valueMaximumLineCount: 3
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Release Notes")
                            value: settingsPage.viewModel.releaseNotesSummaries.join(" / ")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 6
                        }

                        SentinelButton {
                            text: qsTr("Confirm Download")
                            enabled: true
                            Layout.preferredWidth: 180
                            onClicked: settingsPage.viewModel.confirmUpdateDownload()
                        }
                    }
                }

                ShellPanel {
                    id: diagnosticsSection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(diagnosticsContent)

                    ColumnLayout {
                        id: diagnosticsContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: qsTr("Diagnostics")
                            subtitle: qsTr("Local product diagnostics and export preview. No telemetry upload.")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Diagnostics")
                            value: settingsPage.viewModel.diagnosticsCenterSummaries.join(" / ")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 8
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Export Preview")
                            value: settingsPage.viewModel.exportPreviewSummaries.join(" / ")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 6
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Brain Insights")
                            value: settingsPage.viewModel.brainInsightSummaries.join(" / ")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 8
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Recovery")
                            value: settingsPage.viewModel.recoveryReliabilitySummaries.join(" / ")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 6
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Privacy Guarantees")
                            value: settingsPage.viewModel.privacyCenterSummaries.join(" / ")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 8
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Polish")
                            value: settingsPage.viewModel.productPolishSummaries.join(" / ")
                            Layout.fillWidth: true
                            valueMaximumLineCount: 6
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 4
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            SentinelButton {
                                text: qsTr("Preview Markdown")
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.prepareExportPreview("conversations", "Markdown")
                            }

                            SentinelButton {
                                text: qsTr("Preview JSON")
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.prepareExportPreview("workspace summaries", "JSON")
                            }

                            SentinelButton {
                                text: qsTr("Export Diagnostics TXT")
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.exportDiagnostics("txt")
                            }

                            SentinelButton {
                                text: qsTr("Export Diagnostics JSON")
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.exportDiagnostics("json")
                            }
                        }
                    }
                }

                ShellPanel {
                    id: developerSection
                    width: parent.width
                    visible: settingsPage.developerMode
                    implicitHeight: visible ? settingsPage.sectionHeight(developerContent) : 0

                    ColumnLayout {
                        id: developerContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceMd

                        SectionTitle {
                            title: qsTr("Advanced")
                            subtitle: qsTr("Diagnostics, logs, runtime status, and trace viewer.")
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm
                            Repeater {
                                model: [qsTr("Diagnostics"), qsTr("Logs"), qsTr("Runtime Status"), qsTr("Trace Viewer")]
                                StatusChip {
                                    required property string modelData
                                    label: qsTr("Section")
                                    value: modelData
                                    accent: SentinelTheme.textMuted
                                    muted: true
                                }
                            }
                        }

                        SectionTitle {
                            title: qsTr("Semantic / Vector Readiness")
                            subtitle: qsTr("Grouped summaries only. Semantic authority remains disabled unless separately scoped.")
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 2
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            InfoRow {
                                compact: true
                                label: qsTr("Semantic")
                                value: settingsPage.viewModel.semanticRetrievalStatus
                                       + " / "
                                       + settingsPage.viewModel.semanticActivationReadiness
                                Layout.fillWidth: true
                            }

                            InfoRow {
                                compact: true
                                label: qsTr("Provider")
                                value: settingsPage.viewModel.selectedSemanticProviderName
                                       + " / "
                                       + settingsPage.viewModel.semanticProviderMode
                                Layout.fillWidth: true
                            }

                            InfoRow {
                                compact: true
                                label: qsTr("Vector")
                                value: settingsPage.viewModel.vectorPersistenceStatus
                                       + " / "
                                       + settingsPage.viewModel.vectorPersistenceIndexedItemCount
                                       + " indexed"
                                Layout.fillWidth: true
                            }

                            InfoRow {
                                compact: true
                                label: qsTr("Prompt Authority")
                                value: settingsPage.viewModel.semanticPromptAuthorityStatus
                                       + " / non-authoritative"
                                Layout.fillWidth: true
                            }
                        }

                        SectionTitle {
                            title: qsTr("Runtime Diagnostics")
                            subtitle: qsTr("Presentation-only metadata; no permission changes.")
                            Layout.fillWidth: true
                        }

                        SectionTitle {
                            title: qsTr("Profile Boundary")
                            subtitle: qsTr("Read-only profile metadata. Profiles do not grant runtime authority.")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Profile")
                            value: settingsPage.viewModel.selectedSkillProfileName + " / "
                                   + settingsPage.viewModel.selectedSkillProfileReadiness + " / "
                                   + settingsPage.viewModel.selectedSkillProfileSummary
                            Layout.fillWidth: true
                        }

                        SectionTitle {
                            title: qsTr("Workspace Boundary")
                            subtitle: qsTr("Read-only workspace diagnostics. Access remains disabled.")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Capabilities")
                            value: settingsPage.viewModel.skillProfileCapabilitySummaries.join("\n")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Diagnostics")
                            value: settingsPage.viewModel.skillProfileDeveloperDiagnostics.join("\n")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Workspace")
                            value: settingsPage.viewModel.selectedWorkspaceName + " / "
                                   + settingsPage.viewModel.workspacePermissionPosture + " / "
                                   + settingsPage.viewModel.workspaceReadinessSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Boundary")
                            value: settingsPage.viewModel.workspaceBoundaryDiagnostics.join("\n")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: qsTr("Routing")
                            value: settingsPage.viewModel.currentRoutingMode + " / "
                                   + settingsPage.viewModel.modelRoutingStatus
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Runtime Safety")
                            value: settingsPage.viewModel.runtimeSafetyDecision + " / "
                                   + settingsPage.viewModel.runtimeSafetySummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Credentials")
                            value: settingsPage.viewModel.providerCredentialSafetySummaries.join("\n")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Credential Store")
                            value: settingsPage.viewModel.credentialStoreSafetySummary + "\n"
                                   + settingsPage.viewModel.credentialStoreTraceSummaries.join("\n")
                            Layout.fillWidth: true
                        }

                        SectionTitle {
                            title: qsTr("Companion Boundary")
                            subtitle: qsTr("Native shell presentation with explicit safety boundaries.")
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 2
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            InfoRow {
                                compact: true
                                label: qsTr("State")
                                value: settingsPage.viewModel.companionStatus
                                       + " / "
                                       + settingsPage.viewModel.companionAvailability
                                Layout.fillWidth: true
                            }

                            InfoRow {
                                compact: true
                                label: qsTr("Preference")
                                value: settingsPage.viewModel.companionEnabled
                                       ? qsTr("Enabled preference")
                                       : qsTr("Disabled preference")
                                Layout.fillWidth: true
                            }
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 4
                            label: qsTr("Platform")
                            value: settingsPage.viewModel.companionPlatformCapability
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Actions")
                            value: settingsPage.viewModel.companionActionSummaries.join("\n")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Traces")
                            value: settingsPage.viewModel.companionTraceSummaries.join("\n")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Permissions")
                            value: settingsPage.viewModel.runtimePermissionDecision + " / "
                                   + settingsPage.viewModel.runtimePermissionSummary
                            Layout.fillWidth: true
                        }

                        SectionTitle {
                            title: qsTr("Permission Policy")
                            subtitle: qsTr("Central policy diagnostics. States do not grant execution in this build.")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 4
                            label: qsTr("Registry")
                            value: settingsPage.viewModel.defaultPermissionPolicyState + " / "
                                   + settingsPage.viewModel.permissionPolicySummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 10
                            label: qsTr("Domains")
                            value: settingsPage.viewModel.permissionPolicyDeveloperDiagnostics.join("\n")
                            Layout.fillWidth: true
                        }

                        SectionTitle {
                            title: qsTr("Tool Gateway")
                            subtitle: qsTr("Descriptor, posture, and refusal diagnostics only.")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 4
                            label: qsTr("Registry")
                            value: settingsPage.viewModel.toolGatewayPermissionPosture + " / "
                                   + settingsPage.viewModel.toolGatewaySummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 12
                            label: qsTr("Tools")
                            value: settingsPage.viewModel.toolGatewayDeveloperDiagnostics.join("\n")
                            Layout.fillWidth: true
                        }

                        SectionTitle {
                            title: qsTr("Agent Runtime")
                            subtitle: qsTr("Dry-run plan diagnostics. Approval cannot execute actions.")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 6
                            label: qsTr("Runtime")
                            value: settingsPage.viewModel.agentRuntimeStatus + " / "
                                   + settingsPage.viewModel.agentRuntimeApprovalPosture + "\n"
                                   + settingsPage.viewModel.agentRuntimeSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 12
                            label: qsTr("Plan")
                            value: settingsPage.viewModel.agentPlanSteps.join("\n")
                                   + "\n" + settingsPage.viewModel.agentPlanRequiredTools.join("\n")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 12
                            label: qsTr("Traces")
                            value: settingsPage.viewModel.agentRuntimeDeveloperDiagnostics.join("\n")
                                   + "\n" + settingsPage.viewModel.agentPlanDiagnostics.join("\n")
                            Layout.fillWidth: true
                        }

                        SectionTitle {
                            title: qsTr("Voice Boundary")
                            subtitle: qsTr("No microphone, playback, Piper, or Whisper execution is enabled here.")
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Pipeline")
                            value: settingsPage.viewModel.voicePipelineSessionStatus + " / "
                                   + settingsPage.viewModel.voicePipelineSessionSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Safety")
                            value: settingsPage.viewModel.voiceRuntimeSafetyStatus + " / "
                                   + settingsPage.viewModel.voiceRuntimeSafetySummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: qsTr("Audio File")
                            value: settingsPage.viewModel.audioFileSessionStatus + " / "
                                   + settingsPage.viewModel.audioFileSessionReadinessSummary
                            Layout.fillWidth: true
                        }
                    }
                }

                Item {
                    width: parent.width
                    height: SentinelTheme.space2Xl
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
