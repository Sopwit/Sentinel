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
    readonly property string uiSelfCheck: "rail-scroll-sync developer-gated voice-path-wrap bottom-safe-scroll"
    readonly property var categories: developerMode
                                    ? ["General", "Local AI", "Model", "Chat", "Voice", "Privacy / Data", "Developer"]
                                    : ["General", "Local AI", "Model", "Chat", "Voice", "Privacy / Data"]
    property string activeCategory: "General"
    property bool programmaticScroll: false

    function sectionHeight(content) {
        return content.implicitHeight + panelPadding * 2
    }

    function sectionFor(category) {
        if (category === "Local AI")
            return localAiSection
        if (category === "Model")
            return modelSection
        if (category === "Chat")
            return chatSection
        if (category === "Voice")
            return voiceSection
        if (category === "Privacy / Data")
            return privacySection
        if (category === "Developer")
            return developerSection
        return generalSection
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
                     ? "Voice prepared, activation disabled. "
                     : "Voice readiness metadata. "
        return prefix + viewModel.voiceConfigurationReadinessSummary
    }

    onDeveloperModeChanged: {
        if (!developerMode && activeCategory === "Developer")
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
                    text: "Settings"
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
                        text: modelData
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
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 4
                    radius: 2
                    color: SentinelTheme.withAlpha(settingsPage.modeAccent, parent.active ? 0.34 : 0.18)
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
                    activeCategory = "Developer"
                else if (y >= privacySection.y)
                    activeCategory = "Privacy / Data"
                else if (y >= voiceSection.y)
                    activeCategory = "Voice"
                else if (y >= chatSection.y)
                    activeCategory = "Chat"
                else if (y >= modelSection.y)
                    activeCategory = "Model"
                else if (y >= localAiSection.y)
                    activeCategory = "Local AI"
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
                            title: "General"
                            subtitle: "Desktop shell preferences."
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: "Theme"
                            value: settingsPage.viewModel.themeName
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: "Profile"
                            value: settingsPage.viewModel.configurationProfile
                            Layout.fillWidth: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "Settings are explicit and local. Disabled runtime features stay inactive until a later phase enables them."
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall
                            wrapMode: Text.WordWrap
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
                                        text: "Developer Mode"
                                        color: SentinelTheme.textPrimary
                                        font.pixelSize: SentinelTheme.fontBody
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: "Shows advanced read-only metadata only. It does not enable tools, voice execution, cloud providers, or runtime authority."
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
                            title: "Local AI"
                            subtitle: "Local Ollama only. No cloud provider active."
                            Layout.fillWidth: true
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            StatusChip {
                                label: "Health"
                                value: settingsPage.viewModel.ollamaHealthStatus
                                accent: settingsPage.viewModel.ollamaHealthStatus === "Available"
                                        ? SentinelTheme.success
                                        : SentinelTheme.textMuted
                                muted: settingsPage.viewModel.ollamaHealthStatus !== "Available"
                            }

                            StatusChip {
                                label: "Models"
                                value: settingsPage.viewModel.ollamaModelCount.toString()
                                accent: SentinelTheme.accentTertiary
                            }

                            StatusChip {
                                label: "Cloud"
                                value: "inactive"
                                accent: SentinelTheme.textMuted
                                muted: true
                            }
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: "Endpoint"
                            value: "Local loopback Ollama"
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: "Availability"
                            value: settingsPage.viewModel.ollamaModelCount > 0
                                   ? settingsPage.viewModel.ollamaHealthSummary
                                   : "Start Ollama and install/select a local model."
                            Layout.fillWidth: true
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
                            title: "Model"
                            subtitle: "Selection is persisted configuration only."
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
                                                             + " / No model selected"
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
                            label: "Status"
                            value: settingsPage.viewModel.selectedLocalModelStatus
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: "Selected"
                            value: settingsPage.viewModel.selectedLocalModelSummary
                            Layout.fillWidth: true
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
                            title: "Chat"
                            subtitle: "Explicit local chat routing controls."
                            Layout.fillWidth: true
                        }

                        CheckBox {
                            id: localChatToggle
                            Layout.fillWidth: true
                            text: "Local chat inference"
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
                            text: "Local response streaming"
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

                        CheckBox {
                            id: contextToggle
                            Layout.fillWidth: true
                            text: "Use local memory/context in chat"
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

                        InfoRow {
                            compact: settingsPage.compact
                            label: "Routing"
                            value: settingsPage.viewModel.localChatInferenceStatus + " / "
                                   + settingsPage.viewModel.localChatInferenceSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: "Context"
                            value: (settingsPage.viewModel.promptContextInjectionEnabled ? "On / " : "Off / ")
                                   + settingsPage.viewModel.promptContextInjectionStatus
                                   + " / "
                                   + settingsPage.viewModel.retrievalPlanningSelectedSourceCount
                                   + " deterministic sources"
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            visible: settingsPage.viewModel.developerModeEnabled
                            label: "Context Budget"
                            value: settingsPage.viewModel.contextBudgetUsageSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            visible: settingsPage.viewModel.developerModeEnabled
                            label: "Memory Relevance"
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
                            title: "Voice"
                            subtitle: "Readiness metadata only. No microphone, playback, STT, or TTS execution."
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 2
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            Label {
                                Layout.fillWidth: true
                                text: "Piper binary"
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                            }

                            SentinelTextField {
                                id: piperBinaryField
                                Layout.fillWidth: true
                                text: settingsPage.viewModel.piperBinaryPath
                                placeholderText: "Piper binary path"
                                onEditingFinished: settingsPage.viewModel.piperBinaryPath = text
                            }

                            Label {
                                Layout.fillWidth: true
                                text: "Piper model"
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                            }

                            SentinelTextField {
                                id: piperModelField
                                Layout.fillWidth: true
                                text: settingsPage.viewModel.piperModelPath
                                placeholderText: "Piper .onnx model path"
                                onEditingFinished: settingsPage.viewModel.piperModelPath = text
                            }

                            Label {
                                Layout.fillWidth: true
                                text: "Whisper binary"
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                            }

                            SentinelTextField {
                                id: whisperBinaryField
                                Layout.fillWidth: true
                                text: settingsPage.viewModel.whisperBinaryPath
                                placeholderText: "Whisper binary path"
                                onEditingFinished: settingsPage.viewModel.whisperBinaryPath = text
                            }

                            Label {
                                Layout.fillWidth: true
                                text: "Whisper model"
                                color: SentinelTheme.textMuted
                                wrapMode: Text.WordWrap
                            }

                            SentinelTextField {
                                id: whisperModelField
                                Layout.fillWidth: true
                                text: settingsPage.viewModel.whisperModelPath
                                placeholderText: "Whisper model folder or model file"
                                onEditingFinished: settingsPage.viewModel.whisperModelPath = text
                            }
                        }

                        SentinelButton {
                            text: "Apply Paths"
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
                            label: "Readiness"
                            value: settingsPage.voiceReadinessLine()
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: "Whisper"
                            value: settingsPage.viewModel.whisperTranscriptionStatus
                                   + " / "
                                   + settingsPage.viewModel.whisperTranscriptionReadinessSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: "Piper"
                            value: settingsPage.viewModel.piperSynthesisStatus
                                   + " / "
                                   + settingsPage.viewModel.piperSynthesisReadinessSummary
                            Layout.fillWidth: true
                        }
                    }
                }

                ShellPanel {
                    id: privacySection
                    width: parent.width
                    implicitHeight: settingsPage.sectionHeight(privacyContent)

                    ColumnLayout {
                        id: privacyContent
                        x: settingsPage.panelPadding
                        y: settingsPage.panelPadding
                        width: parent.width - settingsPage.panelPadding * 2
                        spacing: SentinelTheme.spaceSm

                        SectionTitle {
                            title: "Privacy / Data"
                            subtitle: "Settings, memory, and chat history remain separate."
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: "Memory"
                            value: settingsPage.viewModel.memoryStatus + " ("
                                   + settingsPage.viewModel.memoryMaintenanceStatus + ")"
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: "Chat History"
                            value: settingsPage.viewModel.conversationHistorySummaryText
                                   + " / "
                                   + settingsPage.viewModel.chatMaintenanceStatus
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: "Active Conversation"
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
                                text: "Clear Local Memory"
                                enabled: settingsPage.viewModel.memoryStatus === "Available"
                                Layout.fillWidth: true
                                onClicked: clearMemoryDialog.open()
                            }

                            SentinelButton {
                                text: "Clear Chat History"
                                Layout.fillWidth: true
                                onClicked: clearChatDialog.open()
                            }

                            SentinelButton {
                                text: "Export Markdown"
                                enabled: settingsPage.viewModel.conversationExportAvailable
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.exportTranscript("markdown")
                            }

                            SentinelButton {
                                text: "Export JSON"
                                enabled: settingsPage.viewModel.conversationExportAvailable
                                Layout.fillWidth: true
                                onClicked: settingsPage.viewModel.exportTranscript("json")
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
                            title: "Developer"
                            subtitle: "Read-only runtime boundary metadata."
                            Layout.fillWidth: true
                        }

                        SectionTitle {
                            title: "Semantic / Vector Readiness"
                            subtitle: "Grouped summaries only. Semantic authority remains disabled unless separately scoped."
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: settingsPage.compact ? 1 : 2
                            columnSpacing: SentinelTheme.spaceSm
                            rowSpacing: SentinelTheme.spaceSm

                            InfoRow {
                                compact: true
                                label: "Semantic"
                                value: settingsPage.viewModel.semanticRetrievalStatus
                                       + " / "
                                       + settingsPage.viewModel.semanticActivationReadiness
                                Layout.fillWidth: true
                            }

                            InfoRow {
                                compact: true
                                label: "Provider"
                                value: settingsPage.viewModel.selectedSemanticProviderName
                                       + " / "
                                       + settingsPage.viewModel.semanticProviderMode
                                Layout.fillWidth: true
                            }

                            InfoRow {
                                compact: true
                                label: "Vector"
                                value: settingsPage.viewModel.vectorPersistenceStatus
                                       + " / "
                                       + settingsPage.viewModel.vectorPersistenceIndexedItemCount
                                       + " indexed"
                                Layout.fillWidth: true
                            }

                            InfoRow {
                                compact: true
                                label: "Prompt Authority"
                                value: settingsPage.viewModel.semanticPromptAuthorityStatus
                                       + " / non-authoritative"
                                Layout.fillWidth: true
                            }
                        }

                        SectionTitle {
                            title: "Runtime Diagnostics"
                            subtitle: "Presentation-only metadata; no permission changes."
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            label: "Routing"
                            value: settingsPage.viewModel.currentRoutingMode + " / "
                                   + settingsPage.viewModel.modelRoutingStatus
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: "Runtime Safety"
                            value: settingsPage.viewModel.runtimeSafetyDecision + " / "
                                   + settingsPage.viewModel.runtimeSafetySummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: "Permissions"
                            value: settingsPage.viewModel.runtimePermissionDecision + " / "
                                   + settingsPage.viewModel.runtimePermissionSummary
                            Layout.fillWidth: true
                        }

                        SectionTitle {
                            title: "Voice Boundary"
                            subtitle: "No microphone, playback, Piper, or Whisper execution is enabled here."
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: "Pipeline"
                            value: settingsPage.viewModel.voicePipelineSessionStatus + " / "
                                   + settingsPage.viewModel.voicePipelineSessionSummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: "Safety"
                            value: settingsPage.viewModel.voiceRuntimeSafetyStatus + " / "
                                   + settingsPage.viewModel.voiceRuntimeSafetySummary
                            Layout.fillWidth: true
                        }

                        InfoRow {
                            compact: settingsPage.compact
                            valueMaximumLineCount: 8
                            label: "Audio File"
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
                text: "Clear local memory?"
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontCard
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: "This clears local memory entries only. Settings are kept."
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
                    text: "Cancel"
                    Layout.preferredWidth: 96
                    onClicked: clearMemoryDialog.close()
                }

                SentinelButton {
                    text: "Clear"
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
                text: "Clear chat history?"
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontCard
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: "This clears the runtime transcript and persisted local chat history when available, then restores the single initial system message. Settings and memory are kept."
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
                    text: "Cancel"
                    Layout.preferredWidth: 96
                    onClicked: clearChatDialog.close()
                }

                SentinelButton {
                    text: "Clear"
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
