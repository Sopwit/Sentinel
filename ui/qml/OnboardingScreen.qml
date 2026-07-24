import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Sentinel.Desktop

Item {
    id: onboarding
    required property var viewModel

    property int step: 0
    property bool active: false
    readonly property int totalSteps: 9
    readonly property color brandAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property bool reducedMotion: viewModel.reducedMotionEnabled

    signal finished()

    opacity: active ? 1.0 : 0.0
    visible: opacity > 0.01
    enabled: active
    z: 300

    function isModelInstalled(ollamaId) {
        if (!ollamaId) return false;
        var names = viewModel.installedOllamaModelNames || [];
        var target = ollamaId.toLowerCase();
        for (var i = 0; i < names.length; i++) {
            var n = names[i].toLowerCase();
            if (n === target) return true;
            var base = target.split(":")[0];
            if (n === base || n.startsWith(base + ":") || n.startsWith(base + "-")) return true;
            if (n.startsWith(target)) return true;
        }
        return false;
    }

    readonly property var recommendedModels: [
        {
            ollamaId: "deepseek-r1:14b",
            name: "DeepSeek R1 14B",
            provider: "DeepSeek",
            size: "9.0 GB",
            description: qsTr("High-performance reasoning model. Distilled from Qwen 2.5 14B, offers exceptionally smart analytical capabilities.")
        },
        {
            ollamaId: "gemma2:9b",
            name: "Gemma 2 9B",
            provider: "Google",
            size: "5.5 GB",
            description: qsTr("Google's highly efficient open-weight powerhouse. Excellent balance of speed, intelligence, and safety.")
        },
        {
            ollamaId: "qwen2.5-coder:7b",
            name: "Qwen 2.5 Coder 7B",
            provider: "Alibaba",
            size: "4.7 GB",
            description: qsTr("State-of-the-art specialist model. Superb at software development, coding, and mathematical reasoning.")
        },
        {
            ollamaId: "deepseek-r1:8b",
            name: "DeepSeek R1 8B",
            provider: "DeepSeek",
            size: "4.7 GB",
            description: qsTr("Extremely popular reasoning model. Bounded yet powerful logic capabilities for medium-spec hardware.")
        }
    ]

    readonly property var stepMeta: [
        { key: "welcome",      title: qsTr("Welcome"),      caption: qsTr("A calm assistant built around you.") },
        { key: "privacy",      title: qsTr("Privacy"),      caption: qsTr("Your data stays on your device.") },
        { key: "appearance",   title: qsTr("Appearance"),   caption: qsTr("Make Sentinel feel like home.") },
        { key: "provider",     title: qsTr("AI Provider"),  caption: qsTr("Connect to models, your way.") },
        { key: "model",        title: qsTr("AI Model"),     caption: qsTr("Choose and download a model.") },
        { key: "settings",     title: qsTr("Preferences"),  caption: qsTr("Fine-tune your local assistant.") },
        { key: "voice",        title: qsTr("Voice Setup"),  caption: qsTr("Configure voice engines and models.") },
        { key: "capabilities", title: qsTr("Capabilities"), caption: qsTr("Everything Sentinel can do.") },
        { key: "finish",       title: qsTr("Finish"),       caption: qsTr("You're ready to begin.") }
    ]

    readonly property var useCases: [
        { id: "Coding",            tag: qsTr("Build & create"),   desc: qsTr("Write, plan, and solve problems with a private copilot.") },
        { id: "Study",             tag: qsTr("Learn & explore"),  desc: qsTr("Summarize, explain, and remember what matters to you.") },
        { id: "Writing",           tag: qsTr("Draft & express"),  desc: qsTr("Letters, reports, stories, and everyday writing.") },
        { id: "General Assistant", tag: qsTr("Daily companion"),  desc: qsTr("A helpful partner for whatever life or work brings.") }
    ]

    readonly property var themes: ["Liquid Glass Light", "Liquid Glass Dark", "Sentinel Classic", "Midnight Blue", "Aurora Teal", "Graphite Grey", "System Sync"]

    readonly property var providers: [
        { id: "Ollama",                   note: qsTr("Popular local runtime, easy to start.") },
        { id: "LM Studio",                note: qsTr("Friendly desktop app for local models.") },
        { id: "llama.cpp server",         note: qsTr("Lightweight server for advanced users.") }
    ]

    readonly property var privacyPoints: [
        { t: qsTr("Local-first"), d: qsTr("Your settings, memory, chats, notes, and history stay on your own device.") },
        { t: qsTr("No tracking"), d: qsTr("Nothing is sent to us. No analytics, no ads, no hidden uploads.") },
        { t: qsTr("You stay in control"), d: qsTr("Updates, exports, and sensitive actions always ask for your permission first.") }
    ]

    readonly property var capabilityPoints: [
        { t: qsTr("Brain"), d: qsTr("Your personal memory: recall, context, summaries, and insights.") },
        { t: qsTr("Workspaces"), d: qsTr("Separate spaces for home, work, study, writing, and more.") },
        { t: qsTr("Tasks"), d: qsTr("Step-by-step plans that check with you before acting.") },
        { t: qsTr("Notifications"), d: qsTr("A tidy in-app center for reminders and updates.") }
    ]

    readonly property var themePalette: ({
        "Liquid Glass Light": { bg: "#f4f6f9", raised: "#ffffff", accent: "#4f8ef7", text: "#0f1724", muted: "#5a6b82" },
        "Liquid Glass Dark":  { bg: "#0e1117", raised: "#161b27", accent: "#7eb8ff", text: "#e8f0ff", muted: "#8899bb" },
        "Sentinel Classic":   { bg: "#10181f", raised: "#18242d", accent: "#79dcff", text: "#eef8ff", muted: "#94abb8" },
        "Midnight Blue":      { bg: "#0a1020", raised: "#111a31", accent: "#8fb4ff", text: "#f0f5ff", muted: "#98a9c8" },
        "Aurora Teal":        { bg: "#121b1d", raised: "#1b2a2d", accent: "#7de0b9", text: "#effbf7", muted: "#9fb8b4" },
        "Graphite Grey":      { bg: "#151719", raised: "#202326", accent: "#d0d7dc", text: "#f2f4f4", muted: "#a7adaf" },
        "System Sync":        { bg: "#11181c", raised: "#1b2327", accent: "#79dcff", text: "#eef8ff", muted: "#94abb8" }
    })

    function paletteFor(name) { return onboarding.themePalette[name] || onboarding.themePalette["Liquid Glass Light"] }

    FileDialog {
        id: voiceFileDialog
        title: qsTr("Select File")
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
                    }
                    path = path.replace(/\//g, '\\');
                } else {
                    if (path.startsWith("file://")) {
                        path = path.substring(7);
                    }
                }
                targetField.text = path;
                targetField.editingFinished(); // Trigger update on viewModel
            }
        }
    }

    Behavior on opacity {
        NumberAnimation {
            duration: reducedMotion ? 0 : MotionTokens.duration(MotionTokens.page, viewModel.currentModeName)
            easing.type: MotionTokens.enter
        }
    }

    // ── Background ──────────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: SentinelTheme.backgroundBase
        Behavior on color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }
    }
    GlowSurface {
        anchors.horizontalCenter: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: parent.height * 1.1
        height: width
        accent: onboarding.brandAccent
        secondaryAccent: SentinelTheme.modeSecondaryAccent(viewModel.currentModeName)
        active: true
        reducedMotion: onboarding.reducedMotion
        opacity: 0.7
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.space2Xl
        spacing: SentinelTheme.space2Xl

        // ── Left rail ──────────────────────────────────────────────────────
        ColumnLayout {
            Layout.preferredWidth: Math.max(260, onboarding.width * 0.26)
            Layout.maximumWidth: 360
            Layout.fillHeight: true
            spacing: SentinelTheme.spaceLg

            RowLayout {
                spacing: SentinelTheme.spaceSm
                Rectangle {
                    Layout.preferredWidth: 30; Layout.preferredHeight: 30; radius: 9
                    color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.16)
                    border.color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.5)
                    border.width: 1
                    StatusPulse {
                        anchors.centerIn: parent
                        size: 8
                        accent: onboarding.brandAccent
                        active: true
                    }
                }
                Label {
                    text: qsTr("Sentinel")
                    color: SentinelTheme.textPrimary
                    font.pixelSize: SentinelTheme.fontBrand
                    font.bold: true
                }
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("A personal assistant that respects your privacy and adapts to how you work, learn, and create.")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontBody
                wrapMode: Text.WordWrap
            }

            Item { Layout.fillHeight: true }

            ColumnLayout {
                spacing: SentinelTheme.spaceLg
                Repeater {
                    model: onboarding.stepMeta
                    RowLayout {
                        required property int index
                        required property var modelData
                        spacing: SentinelTheme.spaceMd

                        Rectangle {
                            Layout.preferredWidth: 26; Layout.preferredHeight: 26; radius: 13
                            color: index < onboarding.step ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.18)
                                 : index === onboarding.step ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.24)
                                 : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.05)
                            border.color: index <= onboarding.step ? onboarding.brandAccent
                                 : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.12)
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }
                            Behavior on border.color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }
                            Label {
                                anchors.centerIn: parent
                                text: index < onboarding.step ? "✓" : (index + 1).toString()
                                color: index <= onboarding.step ? onboarding.brandAccent : SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                font.bold: true
                            }
                        }
                        ColumnLayout {
                            spacing: 2
                            Label {
                                text: modelData.title
                                color: index === onboarding.step ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontControl
                                font.bold: index === onboarding.step
                            }
                            Label {
                                Layout.fillWidth: true
                                text: modelData.caption
                                visible: index === onboarding.step
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontTiny
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            Label {
                Layout.fillWidth: true
                text: qsTr("Takes about a minute. You can change anything later in Settings.")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                wrapMode: Text.WordWrap
            }
        }

        // ── Right content ───────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: SentinelTheme.spaceLg

            StackLayout {
                id: stack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: onboarding.step

                // Step 0 — Welcome
                ScrollView {
                    contentData: ColumnLayout {
                        spacing: SentinelTheme.spaceMd
                        width: stack.width
                        Label {
                            text: qsTr("Welcome to Sentinel")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontDisplay
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Sentinel is a private, local-first assistant for everyone — students, professionals, families, creators, and curious minds. Let's make it yours.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontBody
                            wrapMode: Text.WordWrap
                        }
                        Label {
                            Layout.topMargin: SentinelTheme.spaceMd
                            text: qsTr("What will you mostly use Sentinel for?")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontCard
                            font.bold: true
                        }
                        GridLayout {
                            Layout.topMargin: SentinelTheme.spaceSm
                            Layout.fillWidth: true
                            columns: onboarding.width < 900 ? 1 : 2
                            rowSpacing: SentinelTheme.spaceMd
                            columnSpacing: SentinelTheme.spaceMd
                            Repeater {
                                model: onboarding.useCases
                                Rectangle {
                                    required property var modelData
                                    readonly property bool chosen: viewModel.onboardingUseCase === modelData.id
                                    Layout.fillWidth: true
                                    radius: SentinelTheme.radiusLg
                                    color: chosen ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.12)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.034)
                                    border.color: chosen ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.5)
                                                         : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                    border.width: chosen ? 1.5 : 1
                                    implicitHeight: ucRow.implicitHeight + SentinelTheme.spaceLg * 2
                                    scale: ucHover.hovered && !chosen ? 1.012 : 1.0
                                    Behavior on color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }
                                    Behavior on border.color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }
                                    Behavior on scale { NumberAnimation { duration: MotionTokens.fast; easing.type: MotionTokens.enter } }
                                    ColumnLayout {
                                        id: ucRow
                                        anchors.fill: parent
                                        anchors.margins: SentinelTheme.spaceLg
                                        spacing: SentinelTheme.spaceXs
                                        Label {
                                            text: modelData.id
                                            color: SentinelTheme.textPrimary
                                            font.pixelSize: SentinelTheme.fontCard
                                            font.bold: true
                                        }
                                        Label {
                                            text: modelData.tag
                                            color: chosen ? onboarding.brandAccent : SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontSmall
                                            font.bold: true
                                        }
                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.desc
                                            color: SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontBody
                                            wrapMode: Text.WordWrap
                                        }
                                    }
                                    Rectangle {
                                        visible: chosen
                                        anchors.top: parent.top; anchors.right: parent.right
                                        anchors.margins: SentinelTheme.spaceMd
                                        width: 20; height: 20; radius: 10
                                        color: onboarding.brandAccent
                                        Label {
                                            anchors.centerIn: parent
                                            text: "✓"
                                            color: SentinelTheme.textOnAccent
                                            font.pixelSize: SentinelTheme.fontSmall
                                            font.bold: true
                                        }
                                    }
                                    HoverHandler { id: ucHover }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: viewModel.onboardingUseCase = modelData.id
                                    }
                                }
                            }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }

                // Step 1 — Privacy
                ScrollView {
                    contentData: ColumnLayout {
                        spacing: SentinelTheme.spaceMd
                        width: stack.width
                        Label {
                            text: qsTr("Private by design")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontDisplay
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Sentinel is built so your information can stay entirely on your computer. These are promises we keep, always.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontBody
                            wrapMode: Text.WordWrap
                        }
                        ColumnLayout {
                            Layout.topMargin: SentinelTheme.spaceLg
                            spacing: SentinelTheme.spaceMd
                            Repeater {
                                model: onboarding.privacyPoints
                                Rectangle {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.034)
                                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                    border.width: 1
                                    implicitHeight: pvRow.implicitHeight + SentinelTheme.spaceLg * 2
                                    RowLayout {
                                        id: pvRow
                                        anchors.fill: parent
                                        anchors.margins: SentinelTheme.spaceLg
                                        spacing: SentinelTheme.spaceMd
                                        Rectangle {
                                            Layout.preferredWidth: 36; Layout.preferredHeight: 36; radius: 11
                                            color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.14)
                                            StatusPulse {
                                                anchors.centerIn: parent
                                                size: 9
                                                accent: onboarding.brandAccent
                                                active: true
                                            }
                                        }
                                        ColumnLayout {
                                            spacing: SentinelTheme.spaceXs
                                            Label {
                                                text: modelData.t
                                                color: SentinelTheme.textPrimary
                                                font.pixelSize: SentinelTheme.fontCard
                                                font.bold: true
                                            }
                                            Label {
                                                Layout.fillWidth: true
                                                text: modelData.d
                                                color: SentinelTheme.textMuted
                                                font.pixelSize: SentinelTheme.fontBody
                                                wrapMode: Text.WordWrap
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }

                // Step 2 — Appearance (with live theme previews)
                ScrollView {
                    contentData: ColumnLayout {
                        spacing: SentinelTheme.spaceMd
                        width: stack.width
                        Label {
                            text: qsTr("Choose your look")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontDisplay
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Pick a theme. Each card shows a live preview of how Sentinel will look — and you can change it anytime.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontBody
                            wrapMode: Text.WordWrap
                        }
                        GridLayout {
                            Layout.topMargin: SentinelTheme.spaceMd
                            Layout.fillWidth: true
                            columns: onboarding.width < 900 ? 2 : 3
                            rowSpacing: SentinelTheme.spaceMd
                            columnSpacing: SentinelTheme.spaceMd
                            Repeater {
                                model: onboarding.themes
                                Rectangle {
                                    required property string modelData
                                    readonly property bool chosen: viewModel.themeName === modelData
                                    readonly property var pal: onboarding.paletteFor(modelData)
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 168
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.030)
                                    border.color: chosen ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.6)
                                                         : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.070)
                                    border.width: chosen ? 2 : 1
                                    scale: themeHover.hovered && !chosen ? 1.015 : 1.0
                                    Behavior on border.color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }
                                    Behavior on scale { NumberAnimation { duration: MotionTokens.fast; easing.type: MotionTokens.enter } }

                                    // Mini preview mock
                                    Rectangle {
                                        anchors.fill: parent
                                        anchors.margins: SentinelTheme.spaceSm
                                        radius: SentinelTheme.radiusMd
                                        color: pal.bg
                                        clip: true
                                        Rectangle {
                                            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
                                            height: 16
                                            color: pal.raised
                                            Label {
                                                anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                                                anchors.leftMargin: 6
                                                text: "Sentinel"
                                                color: pal.text
                                                font.pixelSize: 8
                                                font.bold: true
                                            }
                                        }
                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.topMargin: 20
                                            anchors.leftMargin: 6
                                            anchors.rightMargin: 6
                                            anchors.bottomMargin: 6
                                            spacing: 6
                                            Rectangle {
                                                Layout.preferredWidth: 26
                                                Layout.fillHeight: true
                                                radius: 4
                                                color: pal.raised
                                                ColumnLayout {
                                                    anchors.fill: parent
                                                    anchors.margins: 3
                                                    spacing: 3
                                                    Repeater {
                                                        model: 3
                                                        Rectangle {
                                                            Layout.fillWidth: true
                                                            Layout.preferredHeight: 4
                                                            radius: 2
                                                            color: pal.muted
                                                            opacity: 0.6
                                                        }
                                                    }
                                                }
                                            }
                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                spacing: 4
                                                Rectangle {
                                                    Layout.fillWidth: true
                                                    Layout.preferredHeight: 8
                                                    radius: 3
                                                    color: pal.raised
                                                }
                                                Rectangle {
                                                    Layout.fillWidth: true
                                                    Layout.preferredHeight: 18
                                                    radius: 3
                                                    color: pal.raised
                                                    RowLayout {
                                                        anchors.fill: parent
                                                        anchors.leftMargin: 4; anchors.rightMargin: 4
                                                        spacing: 3
                                                        Rectangle {
                                                            Layout.fillWidth: true
                                                            Layout.preferredHeight: 4
                                                            radius: 2
                                                            color: pal.muted
                                                            opacity: 0.5
                                                        }
                                                        Rectangle {
                                                            Layout.preferredWidth: 26
                                                            Layout.preferredHeight: 10
                                                            radius: 5
                                                            color: pal.accent
                                                        }
                                                    }
                                                }
                                                Item { Layout.fillHeight: true }
                                            }
                                        }
                                    }
                                    Label {
                                        anchors.bottom: parent.bottom
                                        anchors.left: parent.left
                                        anchors.leftMargin: SentinelTheme.spaceMd
                                        anchors.bottomMargin: SentinelTheme.spaceMd
                                        text: modelData
                                        color: chosen ? onboarding.brandAccent : SentinelTheme.textPrimary
                                        font.pixelSize: SentinelTheme.fontSmall
                                        font.bold: chosen
                                    }
                                    Rectangle {
                                        visible: chosen
                                        anchors.top: parent.top; anchors.right: parent.right
                                        anchors.margins: SentinelTheme.spaceSm
                                        width: 18; height: 18; radius: 9
                                        color: onboarding.brandAccent
                                        Label {
                                            anchors.centerIn: parent
                                            text: "✓"
                                            color: SentinelTheme.textOnAccent
                                            font.pixelSize: 10
                                            font.bold: true
                                        }
                                    }
                                    HoverHandler { id: themeHover }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: viewModel.themeName = modelData
                                    }
                                }
                            }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }

                // Step 3 — Provider
                ScrollView {
                    contentData: ColumnLayout {
                        spacing: SentinelTheme.spaceMd
                        width: stack.width
                        Label {
                            text: qsTr("Connect your AI")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontDisplay
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Sentinel runs AI privately on your device or a local runtime you choose. Nothing is downloaded now, and you can switch anytime.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontBody
                            wrapMode: Text.WordWrap
                        }
                        ColumnLayout {
                            Layout.topMargin: SentinelTheme.spaceMd
                            spacing: SentinelTheme.spaceMd
                            Repeater {
                                model: onboarding.providers
                                Rectangle {
                                    required property var modelData
                                    readonly property bool chosen: viewModel.onboardingAiProvider === modelData.id
                                    Layout.fillWidth: true
                                    radius: SentinelTheme.radiusLg
                                    color: chosen ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.12)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.034)
                                    border.color: chosen ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.5)
                                                         : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                    border.width: chosen ? 1.5 : 1
                                    implicitHeight: prRow.implicitHeight + SentinelTheme.spaceLg * 2
                                    Behavior on color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }
                                    Behavior on border.color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }
                                    RowLayout {
                                        id: prRow
                                        anchors.fill: parent
                                        anchors.margins: SentinelTheme.spaceLg
                                        spacing: SentinelTheme.spaceMd
                                        ColumnLayout {
                                            spacing: SentinelTheme.spaceXs
                                            Layout.fillWidth: true
                                            Label {
                                                text: modelData.id
                                                color: SentinelTheme.textPrimary
                                                font.pixelSize: SentinelTheme.fontCard
                                                font.bold: true
                                            }
                                            Label {
                                                Layout.fillWidth: true
                                                text: modelData.note
                                                color: SentinelTheme.textMuted
                                                font.pixelSize: SentinelTheme.fontBody
                                                wrapMode: Text.WordWrap
                                            }
                                        }
                                        Rectangle {
                                            visible: chosen
                                            Layout.preferredWidth: 22; Layout.preferredHeight: 22; radius: 11
                                            color: onboarding.brandAccent
                                            Label {
                                                anchors.centerIn: parent
                                                text: "✓"
                                                color: SentinelTheme.textOnAccent
                                                font.pixelSize: SentinelTheme.fontSmall
                                                font.bold: true
                                            }
                                        }
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            viewModel.onboardingAiProvider = modelData.id
                                            if (modelData.id === "Ollama") {
                                                viewModel.selectedRuntimeProvider = "ollama"
                                            } else if (modelData.id === "LM Studio") {
                                                viewModel.selectedRuntimeProvider = "lm-studio"
                                            } else if (modelData.id === "llama.cpp server") {
                                                viewModel.selectedRuntimeProvider = "llama-cpp-server"
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }

                // Step 4 — AI Model Setup (Recommended models & tracking)
                ScrollView {
                    contentData: ColumnLayout {
                        spacing: SentinelTheme.spaceMd
                        width: stack.width
                        Label {
                            text: qsTr("Select AI Model")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontDisplay
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Sentinel requires an AI model to generate responses. Below are the recommended models for your selected provider.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontBody
                            wrapMode: Text.WordWrap
                        }

                        // Ollama Specific Model Selection & Installation
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: viewModel.selectedRuntimeProvider === "ollama"
                            spacing: SentinelTheme.spaceMd

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceMd
                                Repeater {
                                    model: onboarding.recommendedModels
                                    Rectangle {
                                        required property var modelData
                                        readonly property bool installed: onboarding.isModelInstalled(modelData.ollamaId)
                                        readonly property bool activePull: ollamaPuller.pulling && ollamaPuller.activeModel === modelData.ollamaId
                                        readonly property bool selected: viewModel.selectedLocalModel === modelData.ollamaId

                                        Layout.fillWidth: true
                                        radius: SentinelTheme.radiusLg
                                        color: selected ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.12)
                                                        : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.034)
                                        border.color: selected ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.5)
                                                               : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                        border.width: selected ? 1.5 : 1
                                        implicitHeight: mCardCol.implicitHeight + SentinelTheme.spaceLg * 2
                                        Behavior on color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }
                                        Behavior on border.color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }

                                        ColumnLayout {
                                            id: mCardCol
                                            anchors.fill: parent
                                            anchors.margins: SentinelTheme.spaceLg
                                            spacing: SentinelTheme.spaceSm

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: SentinelTheme.spaceMd
                                                ColumnLayout {
                                                    spacing: 2
                                                    Layout.fillWidth: true
                                                    RowLayout {
                                                        spacing: SentinelTheme.spaceSm
                                                        Label {
                                                            text: modelData.name
                                                            color: SentinelTheme.textPrimary
                                                            font.pixelSize: SentinelTheme.fontCard
                                                            font.bold: true
                                                        }
                                                        Rectangle {
                                                            radius: 4
                                                            color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.1)
                                                            border.color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.2)
                                                            border.width: 1
                                                            implicitWidth: tagLabel.implicitWidth + 8
                                                            implicitHeight: tagLabel.implicitHeight + 4
                                                            Label {
                                                                id: tagLabel
                                                                anchors.centerIn: parent
                                                                text: modelData.provider
                                                                color: onboarding.brandAccent
                                                                font.pixelSize: SentinelTheme.fontTiny
                                                                font.bold: true
                                                            }
                                                        }
                                                        Label {
                                                            text: modelData.size
                                                            color: SentinelTheme.textMuted
                                                            font.pixelSize: SentinelTheme.fontSmall
                                                        }
                                                    }
                                                    Label {
                                                        Layout.fillWidth: true
                                                        text: modelData.description
                                                        color: SentinelTheme.textMuted
                                                        font.pixelSize: SentinelTheme.fontBody
                                                        wrapMode: Text.WordWrap
                                                    }
                                                }

                                                // Action / Status indicators
                                                RowLayout {
                                                    spacing: SentinelTheme.spaceSm
                                                    Layout.alignment: Qt.AlignVCenter

                                                    // Download button
                                                    SentinelButton {
                                                        text: qsTr("Install")
                                                        visible: !installed && !activePull
                                                        onClicked: {
                                                            ollamaPuller.pull(modelData.ollamaId)
                                                        }
                                                    }

                                                    // Cancel button
                                                    SentinelButton {
                                                        text: qsTr("Cancel")
                                                        visible: activePull
                                                        onClicked: {
                                                            ollamaPuller.cancel()
                                                        }
                                                    }

                                                    // Select button
                                                    SentinelButton {
                                                        text: selected ? qsTr("Selected") : qsTr("Select")
                                                        enabled: installed
                                                        highlighted: selected
                                                        visible: installed && !activePull
                                                        onClicked: {
                                                            viewModel.selectedLocalModel = modelData.ollamaId
                                                        }
                                                    }
                                                }
                                            }

                                            // Progress bar for active pull
                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                visible: activePull
                                                spacing: 4
                                                RowLayout {
                                                    Layout.fillWidth: true
                                                    Label {
                                                        text: ollamaPuller.statusText
                                                        color: SentinelTheme.textMuted
                                                        font.pixelSize: SentinelTheme.fontTiny
                                                        Layout.fillWidth: true
                                                    }
                                                    Label {
                                                        text: Math.round(ollamaPuller.progress * 100) + "%"
                                                        color: onboarding.brandAccent
                                                        font.pixelSize: SentinelTheme.fontTiny
                                                        font.bold: true
                                                    }
                                                }
                                                // Progress track
                                                Rectangle {
                                                    Layout.fillWidth: true
                                                    height: 4
                                                    radius: 2
                                                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                                    Rectangle {
                                                        width: parent.width * ollamaPuller.progress
                                                        height: parent.height
                                                        radius: parent.radius
                                                        color: onboarding.brandAccent
                                                    }
                                                }
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            enabled: installed && !activePull && !selected
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                viewModel.selectedLocalModel = modelData.ollamaId
                                            }
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                Layout.topMargin: SentinelTheme.spaceMd
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceMd
                                Label {
                                    text: qsTr("Or select an already installed model:")
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    font.bold: true
                                    Layout.fillWidth: true
                                }
                                ComboBox {
                                    id: modelSelectCombo
                                    Layout.preferredWidth: 280
                                    model: viewModel.installedOllamaModelNames.length > 0 ? viewModel.installedOllamaModelNames : [qsTr("No models found")]
                                    currentIndex: viewModel.installedOllamaModelNames.indexOf(viewModel.selectedLocalModel)
                                    enabled: viewModel.installedOllamaModelNames.length > 0
                                    onActivated: (index) => {
                                        if (currentText !== qsTr("No models found")) {
                                            viewModel.selectedLocalModel = currentText
                                        }
                                    }
                                }
                            }
                        }

                        // LM Studio Specific Instructions
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: viewModel.selectedRuntimeProvider === "lm-studio"
                            spacing: SentinelTheme.spaceMd
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Configure LM Studio:")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontCard
                                font.bold: true
                            }
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("1. Open LM Studio on your computer.\n2. Search for and download your preferred model (e.g. Llama 3.2 3B).\n3. Go to the local server tab (double-headed arrow icon) and load the model.\n4. Start the server (usually on port 1234).\n5. Select the loaded model from the dropdown below.")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontBody
                                wrapMode: Text.WordWrap
                            }
                            RowLayout {
                                Layout.topMargin: SentinelTheme.spaceMd
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceMd
                                Label {
                                    text: qsTr("Active LM Studio Model:")
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    font.bold: true
                                    Layout.fillWidth: true
                                }
                                ComboBox {
                                    id: lmStudioModelCombo
                                    Layout.preferredWidth: 280
                                    model: viewModel.loadedLMStudioModelNames.length > 0 ? viewModel.loadedLMStudioModelNames : [qsTr("No models loaded")]
                                    currentIndex: viewModel.loadedLMStudioModelNames.indexOf(viewModel.selectedLocalModel)
                                    enabled: viewModel.loadedLMStudioModelNames.length > 0
                                    onActivated: (index) => {
                                        if (currentText !== qsTr("No models loaded")) {
                                            viewModel.selectedLocalModel = currentText
                                        }
                                    }
                                }
                            }
                        }

                        // llama.cpp Server Specific Instructions
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: viewModel.selectedRuntimeProvider === "llama-cpp-server"
                            spacing: SentinelTheme.spaceMd
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("llama.cpp Server Configuration:")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontCard
                                font.bold: true
                            }
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Make sure your llama.cpp server is running locally (default endpoint http://127.0.0.1:8080). Write the identifier or model path below so Sentinel can target it.")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontBody
                                wrapMode: Text.WordWrap
                            }
                            RowLayout {
                                Layout.topMargin: SentinelTheme.spaceMd
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceMd
                                Label {
                                    text: qsTr("Model Identifier:")
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    font.bold: true
                                }
                                SentinelTextField {
                                    Layout.fillWidth: true
                                    text: viewModel.selectedLocalModel
                                    placeholderText: "e.g. llama-3.2-3b"
                                    onTextChanged: {
                                        viewModel.selectedLocalModel = text
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // Step 5 — AI & System Preferences
                ScrollView {
                    contentData: ColumnLayout {
                        spacing: SentinelTheme.spaceMd
                        width: stack.width
                        Label {
                            text: qsTr("AI & System Preferences")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontDisplay
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Configure options for local memory integration, generation parameters, and performance.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontBody
                            wrapMode: Text.WordWrap
                        }

                        // Local Context toggle
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: SentinelTheme.spaceMd
                            spacing: SentinelTheme.spaceMd
                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true
                                Label {
                                    text: qsTr("Use local memory/context in chat")
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontCard
                                    font.bold: true
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Automatically retrieves and appends relevant memory notes and recent chat history to the prompt context.")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontBody
                                    wrapMode: Text.WordWrap
                                }
                            }
                            Switch {
                                checked: viewModel.promptContextInjectionEnabled
                                onToggled: viewModel.promptContextInjectionEnabled = checked
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                        }

                        // Creativity (Temperature)
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            RowLayout {
                                Layout.fillWidth: true
                                Label {
                                    text: qsTr("Creativity (Temperature): ") + viewModel.localInferenceTemperature.toFixed(2)
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontCard
                                    font.bold: true
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Slider {
                                    Layout.fillWidth: true
                                    from: 0.0
                                    to: 2.0
                                    stepSize: 0.05
                                    value: viewModel.localInferenceTemperature
                                    onMoved: {
                                        viewModel.localInferenceTemperature = value
                                    }
                                }
                            }
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Lower values are focused and deterministic; higher values are creative.")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontTiny
                            }
                        }

                        // Max tokens
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            RowLayout {
                                Layout.fillWidth: true
                                Label {
                                    text: qsTr("Max Response Tokens: ") + viewModel.localInferenceMaxTokens
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontCard
                                    font.bold: true
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Slider {
                                    Layout.fillWidth: true
                                    from: 256
                                    to: 8192
                                    stepSize: 256
                                    value: viewModel.localInferenceMaxTokens
                                    onMoved: {
                                        viewModel.localInferenceMaxTokens = Math.round(value)
                                    }
                                }
                            }
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Controls response length limits. High limits require more hardware resources.")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontTiny
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                        }

                        // Reduced Motion
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd
                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true
                                Label {
                                    text: qsTr("Reduced Motion (Animations)")
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontCard
                                    font.bold: true
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Disables sliding pages, glass drifts, and high-motion transitions to save power.")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontBody
                                    wrapMode: Text.WordWrap
                                }
                            }
                            Switch {
                                checked: viewModel.reducedMotionEnabled
                                onToggled: viewModel.reducedMotionEnabled = checked
                            }
                        }

                        // Check updates
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd
                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true
                                Label {
                                    text: qsTr("Check for Updates")
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontCard
                                    font.bold: true
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Choose how often Sentinel checks for updates. Checked locally without sharing logs.")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontBody
                                    wrapMode: Text.WordWrap
                                }
                            }
                            ComboBox {
                                model: ["Never", "Ask Before Checking", "Weekly", "On Startup"]
                                currentIndex: model.indexOf(viewModel.updateCheckPolicy)
                                onActivated: (index) => {
                                    viewModel.updateCheckPolicy = currentText
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // Step 6 — Voice & Speech Setup
                ScrollView {
                    contentData: ColumnLayout {
                        spacing: SentinelTheme.spaceLg
                        width: stack.width

                        Label {
                            text: qsTr("Voice & Speech Setup")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontDisplay
                            font.bold: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Sentinel supports local Text-to-Speech (TTS) and Speech-to-Text (STT) for hands-free operations. Select your preferred engine and model files below.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontBody
                            wrapMode: Text.WordWrap
                        }

                        // ── TTS Section ──
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm

                            Label {
                                text: qsTr("Text-to-Speech (TTS) Engine")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontCard
                                font.bold: true
                            }

                            RowLayout {
                                spacing: SentinelTheme.spaceMd
                                Layout.fillWidth: true
                                Label {
                                    text: qsTr("Engine:")
                                    color: SentinelTheme.textMuted
                                    Layout.preferredWidth: 60
                                }
                                ComboBox {
                                    id: ttsEngineCombo
                                    model: ["Piper", "Kokoro"]
                                    currentIndex: model.indexOf(viewModel.selectedTtsEngine)
                                    onActivated: {
                                        viewModel.selectedTtsEngine = currentText
                                    }
                                }
                            }
                        }

                        // ── Piper Config ──
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd
                            visible: viewModel.selectedTtsEngine === "Piper"

                            Label {
                                text: qsTr("Piper Configuration (Local ONNX)")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                                font.bold: true
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Label {
                                    text: qsTr("Piper Binary Path")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    SentinelTextField {
                                        id: piperBinaryField
                                        Layout.fillWidth: true
                                        text: viewModel.piperBinaryPath
                                        placeholderText: qsTr("Enter path or browse (e.g. /usr/local/bin/piper)")
                                        onEditingFinished: viewModel.piperBinaryPath = text
                                    }
                                    SentinelButton {
                                        text: "📁"
                                        Layout.preferredWidth: 40
                                        onClicked: voiceFileDialog.openWithField(piperBinaryField, qsTr("Select Piper Binary"))
                                    }
                                }
                                Label {
                                    text: piperBinaryField.text !== "" ? qsTr("✓ Detected on system") : qsTr("✗ Not detected. Enter path manually.")
                                    color: piperBinaryField.text !== "" ? "#4caf50" : "#ff9800"
                                    font.pixelSize: SentinelTheme.fontSmall - 1
                                    font.bold: true
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Label {
                                    text: qsTr("Piper Model File (.onnx)")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    SentinelTextField {
                                        id: piperModelField
                                        Layout.fillWidth: true
                                        text: viewModel.piperModelPath
                                        placeholderText: qsTr("Select .onnx voice model file path")
                                        onEditingFinished: viewModel.piperModelPath = text
                                    }
                                    SentinelButton {
                                        text: "📁"
                                        Layout.preferredWidth: 40
                                        onClicked: voiceFileDialog.openWithField(piperModelField, qsTr("Select Piper Model (.onnx)"))
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Recommended voice: en_US-lessac-medium.onnx")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall - 1
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }

                        // ── Kokoro Config ──
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd
                            visible: viewModel.selectedTtsEngine === "Kokoro"

                            Label {
                                text: qsTr("Kokoro Configuration (Ultra-Realistic)")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontBody
                                font.bold: true
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Label {
                                    text: qsTr("Kokoro Model File")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    SentinelTextField {
                                        id: kokoroModelField
                                        Layout.fillWidth: true
                                        text: viewModel.kokoroModelPath
                                        placeholderText: qsTr("Select Kokoro model file path (e.g. kokoro.onnx)")
                                        onEditingFinished: viewModel.kokoroModelPath = text
                                    }
                                    SentinelButton {
                                        text: "📁"
                                        Layout.preferredWidth: 40
                                        onClicked: voiceFileDialog.openWithField(kokoroModelField, qsTr("Select Kokoro Model"))
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Recommended: kokoro.onnx (ONNX weights with Python runner)")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall - 1
                                    wrapMode: Text.WordWrap
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Label {
                                    text: qsTr("Kokoro Voice Name")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }
                                SentinelTextField {
                                    Layout.fillWidth: true
                                    text: viewModel.kokoroVoice
                                    placeholderText: qsTr("Enter voice name (e.g. af_sky)")
                                    onEditingFinished: viewModel.kokoroVoice = text
                                }
                            }
                        }

                        // ── Whisper STT Config ──
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd

                            Label {
                                text: qsTr("Speech-to-Text (STT) Configuration (Whisper)")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontCard
                                font.bold: true
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Label {
                                    text: qsTr("Whisper Binary Path")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    SentinelTextField {
                                        id: whisperBinaryField
                                        Layout.fillWidth: true
                                        text: viewModel.whisperBinaryPath
                                        placeholderText: qsTr("Enter path or browse (e.g. /usr/local/bin/whisper-cpp)")
                                        onEditingFinished: viewModel.whisperBinaryPath = text
                                    }
                                    SentinelButton {
                                        text: "📁"
                                        Layout.preferredWidth: 40
                                        onClicked: voiceFileDialog.openWithField(whisperBinaryField, qsTr("Select Whisper Binary"))
                                    }
                                }
                                Label {
                                    text: whisperBinaryField.text !== "" ? qsTr("✓ Detected on system") : qsTr("✗ Not detected. Enter path manually.")
                                    color: whisperBinaryField.text !== "" ? "#4caf50" : "#ff9800"
                                    font.pixelSize: SentinelTheme.fontSmall - 1
                                    font.bold: true
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Label {
                                    text: qsTr("Whisper Model File (.bin)")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    SentinelTextField {
                                        id: whisperModelField
                                        Layout.fillWidth: true
                                        text: viewModel.whisperModelPath
                                        placeholderText: qsTr("Select ggml-base.bin model file path")
                                        onEditingFinished: viewModel.whisperModelPath = text
                                    }
                                    SentinelButton {
                                        text: "📁"
                                        Layout.preferredWidth: 40
                                        onClicked: voiceFileDialog.openWithField(whisperModelField, qsTr("Select Whisper Model"))
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Recommended: ggml-base.bin (~140 MB for fast transcription)")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall - 1
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // Step 7 — Capabilities
                ScrollView {
                    contentData: ColumnLayout {
                        spacing: SentinelTheme.spaceMd
                        width: stack.width
                        Label {
                            text: qsTr("What Sentinel can do")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontDisplay
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("A quick tour of the spaces you'll use every day.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontBody
                            wrapMode: Text.WordWrap
                        }
                        ColumnLayout {
                            Layout.topMargin: SentinelTheme.spaceLg
                            spacing: SentinelTheme.spaceMd
                            Repeater {
                                model: onboarding.capabilityPoints
                                Rectangle {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.034)
                                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                    border.width: 1
                                    implicitHeight: capRow.implicitHeight + SentinelTheme.spaceLg * 2
                                    RowLayout {
                                        id: capRow
                                        anchors.fill: parent
                                        anchors.margins: SentinelTheme.spaceLg
                                        spacing: SentinelTheme.spaceMd
                                        Rectangle {
                                            Layout.preferredWidth: 36; Layout.preferredHeight: 36; radius: 11
                                            color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.14)
                                            Label {
                                                anchors.centerIn: parent
                                                text: "◆"
                                                color: onboarding.brandAccent
                                                font.pixelSize: SentinelTheme.fontCard
                                            }
                                        }
                                        ColumnLayout {
                                            spacing: SentinelTheme.spaceXs
                                            Label {
                                                text: modelData.t
                                                color: SentinelTheme.textPrimary
                                                font.pixelSize: SentinelTheme.fontCard
                                                font.bold: true
                                            }
                                            Label {
                                                Layout.fillWidth: true
                                                text: modelData.d
                                                color: SentinelTheme.textMuted
                                                font.pixelSize: SentinelTheme.fontBody
                                                wrapMode: Text.WordWrap
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }

                // Step 5 — Finish
                ScrollView {
                    contentData: ColumnLayout {
                        spacing: SentinelTheme.spaceMd
                        width: stack.width
                        Rectangle {
                            Layout.preferredWidth: 64; Layout.preferredHeight: 64; radius: 20
                            color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.16)
                            border.color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.5)
                            border.width: 1
                            StatusPulse {
                                anchors.centerIn: parent
                                size: 12
                                accent: onboarding.brandAccent
                                active: true
                            }
                        }
                        Label {
                            Layout.topMargin: SentinelTheme.spaceMd
                            text: qsTr("You're all set")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontDisplay
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Here's your setup. You can revisit everything later from Settings.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontBody
                            wrapMode: Text.WordWrap
                        }
                        ColumnLayout {
                            Layout.topMargin: SentinelTheme.spaceLg
                            spacing: SentinelTheme.spaceSm
                            InfoRow { label: qsTr("Use Case"); value: viewModel.onboardingUseCase; Layout.fillWidth: true }
                            InfoRow { label: qsTr("Theme"); value: viewModel.themeName; Layout.fillWidth: true }
                            InfoRow { label: qsTr("AI Provider"); value: viewModel.onboardingAiProvider; Layout.fillWidth: true }
                            InfoRow { label: qsTr("Selected Model"); value: viewModel.selectedLocalModel ? viewModel.selectedLocalModel : qsTr("None selected"); Layout.fillWidth: true }
                            InfoRow { label: qsTr("Memory Context"); value: viewModel.promptContextInjectionEnabled ? qsTr("Enabled") : qsTr("Disabled"); Layout.fillWidth: true }
                            InfoRow { label: qsTr("TTS Engine"); value: viewModel.selectedTtsEngine; Layout.fillWidth: true }
                            InfoRow { label: qsTr("System Updates"); value: viewModel.updateCheckPolicy; Layout.fillWidth: true }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }
            }

            // ── Footer ──────────────────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                radius: SentinelTheme.radiusLg
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.030)
                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                border.width: 1
                implicitHeight: footerRow.implicitHeight + SentinelTheme.spaceMd * 2
                RowLayout {
                    id: footerRow
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.spaceMd
                    spacing: SentinelTheme.spaceMd
                    SentinelButton {
                        text: qsTr("Back")
                        enabled: onboarding.step > 0
                        onClicked: onboarding.step = Math.max(0, onboarding.step - 1)
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: (onboarding.step + 1) + " / " + onboarding.totalSteps
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                    }
                    Rectangle {
                        width: 1; height: SentinelTheme.controlHeight * 0.5
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                    }
                    SentinelButton {
                        text: onboarding.step < onboarding.totalSteps - 1 ? qsTr("Continue") : qsTr("Start using Sentinel")
                        highlighted: true
                        onClicked: {
                            if (onboarding.step === 6) {
                                viewModel.applyVoiceConfigurationPaths(
                                    piperBinaryField.text,
                                    piperModelField.text,
                                    whisperBinaryField.text,
                                    whisperModelField.text
                                );
                            }
                            if (onboarding.step < onboarding.totalSteps - 1) {
                                onboarding.step++
                            } else {
                                onboarding.finished()
                            }
                        }
                    }
                }
            }
        }
    }
}
