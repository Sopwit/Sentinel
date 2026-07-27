import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Layouts
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
        { id: "General Assistant", tag: qsTr("Daily companion"),  desc: qsTr("A helpful partner for whatever life or work brings.") },
        { id: "Research",          tag: qsTr("Discover & analyze"), desc: qsTr("Deep research, cross-reference sources, and synthesize findings.") },
        { id: "Creative",          tag: qsTr("Imagine & design"),   desc: qsTr("Brainstorm ideas, generate art concepts, and explore creative projects.") },
        { id: "Business",          tag: qsTr("Manage & decide"),    desc: qsTr("Draft emails, prepare reports, and organize workflows.") },
        { id: "Personal Assistant",tag: qsTr("Plan & organize"),    desc: qsTr("Manage schedules, set reminders, and keep track of tasks.") }
    ]

    readonly property var themes: ["Liquid Glass Light", "Liquid Glass Dark", "Sentinel Classic", "Midnight Blue", "Aurora Teal", "Graphite Grey", "Solarized Light", "Nord Frost", "Dracula", "Tokyo Night"]

    readonly property var providers: [
        { id: "Ollama",                   note: qsTr("Popular local runtime, easy to start.") },
        { id: "LM Studio",                note: qsTr("Friendly desktop app for local models.") },
        { id: "Cloud API",                note: qsTr("Direct Cloud APIs: OpenAI ChatGPT, Anthropic Claude & Google Gemini.") },
        { id: "llama.cpp server",         note: qsTr("Lightweight server for advanced users.") }
    ]

    readonly property var privacyPoints: [
        { t: qsTr("Local-first architecture"), d: qsTr("Your settings, memory, chats, notes, and history stay on your own device. No cloud dependency required."), detail: qsTr("Sentinel stores everything locally on your machine using SQLite databases for memory and chat history, and JSON files for settings. There are no cloud servers involved. Your data never leaves your device unless you explicitly choose to share it. This means you can use Sentinel fully offline, and your private conversations remain truly private.") },
        { t: qsTr("No telemetry or tracking"), d: qsTr("Nothing is sent to us. No analytics, no ads, no hidden uploads, no fingerprinting."), detail: qsTr("Sentinel does not include any analytics SDK, telemetry framework, or tracking code. We do not collect usage statistics, crash reports, or any personally identifiable information. There are no 'phone home' mechanisms. The application checks for updates by querying a static manifest — no unique identifiers are transmitted.") },
        { t: qsTr("You stay in control"), d: qsTr("Updates, exports, tool execution, and sensitive actions always ask for your permission first."), detail: qsTr("Every sensitive operation in Sentinel requires your explicit consent. Tool execution goes through a permission gateway where you approve or deny each action. Updates are opt-in and you control the policy. Memory operations are transparent and you can view, edit, or delete anything. The permission policy service ensures no action happens without your awareness.") },
        { t: qsTr("Open and auditable"), d: qsTr("Built with open-source components. You can inspect, modify, and verify everything."), detail: qsTr("Sentinel is built on open standards and open-source components including Qt 6, CMake, and SQLite. The application architecture is modular and transparent. You can inspect the source code, build from source, audit dependencies, and verify there are no hidden behaviors. The AGENTS.md file documents all architecture decisions.") },
        { t: qsTr("Encrypted where it counts"), d: qsTr("API keys and credentials are stored encrypted. Your data belongs to you."), detail: qsTr("Sensitive data such as API keys for cloud providers (OpenAI, Claude, Gemini, etc.) and credentials are stored using encrypted storage via the CredentialStore interface. The credential store uses platform-native secure storage where available. Local SQLite databases for memory and chat history are stored with file-level permissions restricted to your user account.") }
    ]

    readonly property var capabilityDetails: ({
        "Brain & Memory": qsTr("The Brain is Sentinel's long-term memory system. It automatically extracts key information from your conversations, creates summaries, and indexes them for fast retrieval. When you ask a question, Sentinel searches its memory for relevant context and appends it to the prompt, so you don't have to repeat yourself. Memory can be viewed, searched, edited, or deleted at any time through the Memory panel."),
        "Workspaces": qsTr("Workspaces let you create isolated environments for different areas of your life — Home, Work, Study, Creative Projects, and more. Each workspace has its own memory store, chat history, and settings. Switch between them with a single click. This keeps your work conversations separate from personal ones, and your study notes don't mix with your business emails."),
        "Task Planning": qsTr("The task planning system can break down complex requests into step-by-step plans. Before executing any step that affects your system (like creating files, running commands, or accessing data), Sentinel asks for your permission. You can approve, modify, or reject each step. This controlled execution model ensures you remain in charge of what happens on your computer."),
        "Notifications": qsTr("The in-app notification center keeps you informed about what Sentinel is doing. It shows real-time activity, completed tasks, errors, and system updates. Notifications are grouped by type and can be dismissed individually or all at once. You can configure which notifications you want to see through the Settings panel."),
        "Voice & Speech": qsTr("Sentinel supports local Text-to-Speech and Speech-to-Text without cloud dependencies. TTS options include Piper (fast, lightweight ONNX-based) and Kokoro (ultra-realistic). For STT, Whisper (via whisper-cpp) provides accurate transcription. All audio processing stays on your device. Configure voice paths and select your preferred engine in the Voice Setup section."),
        "Multi-Provider": qsTr("Sentinel is provider-agnostic. You can use Ollama for fully local inference, connect to LM Studio or llama.cpp server for other local runtimes, or use cloud APIs from OpenAI, Anthropic Claude, Google Gemini, DeepSeek, Groq, and Mistral. Switch between providers at any time. Each provider has its own endpoint configuration and model selection."),
        "Tool Integration": qsTr("Extend Sentinel's capabilities through a permission-based tool system. Tools are registered in the Tool Registry and executed through the Tool Execution Gateway, which enforces your permission policies. Skills provide specialized workflows. Agents combine tools and skills for autonomous task completion — all behind a safety boundary you control.")
    })

    readonly property var capabilityPoints: [
        { t: qsTr("Brain & Memory"), d: qsTr("Personal memory system that recalls context, summaries, and insights from your past conversations.") },
        { t: qsTr("Workspaces"), d: qsTr("Separate spaces for home, work, study, writing, and more. Each with its own memory and context.") },
        { t: qsTr("Task Planning"), d: qsTr("Step-by-step plans that check with you before acting. Controlled, transparent, and safe.") },
        { t: qsTr("Notifications"), d: qsTr("A tidy in-app center for reminders, updates, and system alerts.") },
        { t: qsTr("Voice & Speech"), d: qsTr("Local text-to-speech and speech-to-text. Piper, Kokoro, and Whisper support.") },
        { t: qsTr("Multi-Provider"), d: qsTr("Switch between Ollama, LM Studio, llama.cpp, Cloud APIs, and more.") },
        { t: qsTr("Tool Integration"), d: qsTr("Extend Sentinel with tools, skills, and custom agents through a permission-based gateway.") }
    ]

    readonly property var themePalette: ({
        "Liquid Glass Light": { bg: "#f4f6f9", raised: "#ffffff", accent: "#4f8ef7", text: "#0f1724", muted: "#5a6b82" },
        "Liquid Glass Dark":  { bg: "#0d1117", raised: "#161b27", accent: "#7eb8ff", text: "#e8f0ff", muted: "#8899bb" },
        "Sentinel Classic":   { bg: "#0d1117", raised: "#18242d", accent: "#7eb8ff", text: "#eef8ff", muted: "#94abb8" },
        "Midnight Blue":      { bg: "#0a0f1e", raised: "#111a31", accent: "#8fb4ff", text: "#f0f5ff", muted: "#98a9c8" },
        "Aurora Teal":        { bg: "#0f1a1c", raised: "#1b2a2d", accent: "#7de0b9", text: "#effbf7", muted: "#9fb8b4" },
        "Graphite Grey":      { bg: "#121416", raised: "#202326", accent: "#d0d7dc", text: "#f2f4f4", muted: "#a7adaf" },
        "Solarized Light":    { bg: "#fdf6e3", raised: "#eee8d5", accent: "#268bd2", text: "#073642", muted: "#839496" },
        "Nord Frost":         { bg: "#2e3440", raised: "#3b4252", accent: "#88c0d0", text: "#e5e9f0", muted: "#81a1c1" },
        "Dracula":            { bg: "#1e1f2e", raised: "#282a3a", accent: "#bd93f9", text: "#f8f8f2", muted: "#6272a4" },
        "Tokyo Night":        { bg: "#0f1419", raised: "#1a1f2b", accent: "#7aa2f7", text: "#c0caf5", muted: "#565f89" }
    })

    function paletteFor(name) { return onboarding.themePalette[name] || onboarding.themePalette["Liquid Glass Light"] }

    // ── Detail Popup for Privacy & Capabilities ──
    Popup {
        id: detailPopup
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: Overlay.overlay
        width: Math.min(stack.width * 0.7, 420)
        height: popupContent.implicitHeight + SentinelTheme.space3Xl * 2
        padding: SentinelTheme.space3Xl

        background: Rectangle {
            radius: SentinelTheme.radiusXl
            color: SentinelTheme.backgroundRaised
            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
            border.width: 1
        }

        property string popupTitle: ""
        property string popupDetail: ""

        ColumnLayout {
            id: popupContent
            spacing: SentinelTheme.spaceLg
            width: parent.width

            Label {
                text: detailPopup.popupTitle
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontCard
                font.bold: true
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                text: detailPopup.popupDetail
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontBody
                wrapMode: Text.WordWrap
                lineHeight: 1.5
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: SentinelTheme.spaceSm
                Item { Layout.fillWidth: true }
                SentinelButton {
                    text: qsTr("Got it")
                    highlighted: true
                    onClicked: detailPopup.close()
                }
            }
        }
    }

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

    // ── Full-screen event blocker ──
    MouseArea {
        anchors.fill: parent
        enabled: active
        hoverEnabled: false
        acceptedButtons: Qt.AllButtons
        scrollGestureEnabled: false
        onWheel: (wheel) => wheel.accepted = true
    }

    // ── Background ──────────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: SentinelTheme.backgroundBase
        Behavior on color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }
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

            Label {
                text: qsTr("Sentinel")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBrand
                font.bold: true
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
                spacing: 0
                Repeater {
                    model: onboarding.stepMeta
                    ColumnLayout {
                        required property int index
                        required property var modelData
                        spacing: SentinelTheme.spaceSm
                        Layout.fillWidth: true

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd
                            Layout.leftMargin: 4

                            Rectangle {
                                id: stepIndicator
                                Layout.preferredWidth: 28; Layout.preferredHeight: 28; radius: 14
                                color: index < onboarding.step ? onboarding.brandAccent
                                     : index === onboarding.step ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.20)
                                     : "transparent"
                                border.color: index <= onboarding.step ? onboarding.brandAccent
                                     : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.15)
                                border.width: index === onboarding.step ? 2 : 1
                                Behavior on color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }
                                Behavior on border.color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }
                                Behavior on border.width { NumberAnimation { duration: MotionTokens.fast } }

                                Label {
                                    anchors.centerIn: parent
                                    text: index < onboarding.step ? "✓" : (index + 1).toString()
                                    color: index < onboarding.step ? SentinelTheme.textOnAccent
                                         : index === onboarding.step ? onboarding.brandAccent
                                         : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.30)
                                    font.pixelSize: SentinelTheme.fontSmall
                                    font.bold: true
                                }
                            }
                            ColumnLayout {
                                spacing: 1
                                Label {
                                    text: modelData.title
                                    color: index <= onboarding.step ? SentinelTheme.textPrimary
                                         : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.40)
                                    font.pixelSize: SentinelTheme.fontControl
                                    font.bold: index <= onboarding.step
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.caption
                                    color: index === onboarding.step
                                           ? SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.85)
                                           : SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.35)
                                    font.pixelSize: SentinelTheme.fontTiny
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            Layout.leftMargin: 4
                            Layout.rightMargin: 4
                            visible: index < onboarding.stepMeta.length - 1
                            color: index < onboarding.step
                                   ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.25)
                                   : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
                            Behavior on color { ColorAnimation { duration: MotionTokens.normal; easing.type: MotionTokens.standard } }
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
                                    id: ucCard
                                    required property var modelData
                                    readonly property bool chosen: viewModel.onboardingUseCase === modelData.id
                                    Layout.fillWidth: true
                                    Layout.minimumHeight: 120
                                    Layout.maximumHeight: 200
                                    radius: SentinelTheme.radiusLg
                                    color: chosen ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.12)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.034)
                                    border.color: chosen ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.5)
                                                         : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                    border.width: chosen ? 1.5 : 1
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
                                    id: privCard
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
                                    HoverHandler { id: privHover }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            detailPopup.popupTitle = modelData.t
                                            detailPopup.popupDetail = modelData.detail

                                            detailPopup.open()
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
                                    id: themeCard
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
                                    id: provCard
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
                                    HoverHandler { id: provHover }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            viewModel.onboardingAiProvider = modelData.id
                                            if (modelData.id === "Ollama") {
                                                viewModel.selectedRuntimeProvider = "ollama"
                                                viewModel.selectedCloudProvider = ""
                                            } else if (modelData.id === "LM Studio") {
                                                viewModel.selectedRuntimeProvider = "lm-studio"
                                                viewModel.selectedCloudProvider = ""
                                            } else if (modelData.id === "Cloud API") {
                                                viewModel.selectedRuntimeProvider = "cloud-api"
                                                viewModel.selectedCloudProvider = ""
                                            } else if (modelData.id === "llama.cpp server") {
                                                viewModel.selectedRuntimeProvider = "llama-cpp-server"
                                                viewModel.selectedCloudProvider = ""
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        // ── Provider endpoint configuration ──
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: SentinelTheme.spaceLg
                            spacing: SentinelTheme.spaceSm

                            Label {
                                text: qsTr("Endpoint Configuration")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontCard
                                font.bold: true
                            }

                            // Ollama endpoint
                            ColumnLayout {
                                Layout.fillWidth: true
                                visible: viewModel.selectedRuntimeProvider === "ollama"
                                spacing: SentinelTheme.spaceXs
                                Label {
                                    text: qsTr("Ollama Server URL")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: SentinelTheme.spaceSm
                                    SentinelTextField {
                                        Layout.fillWidth: true
                                        text: viewModel.ollamaEndpoint
                                        placeholderText: "http://127.0.0.1:11434"
                                        onEditingFinished: viewModel.ollamaEndpoint = text
                                    }
                                }
                            }

                            // LM Studio endpoint
                            ColumnLayout {
                                Layout.fillWidth: true
                                visible: viewModel.selectedRuntimeProvider === "lm-studio"
                                spacing: SentinelTheme.spaceXs
                                Label {
                                    text: qsTr("LM Studio Server URL")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: SentinelTheme.spaceSm
                                    SentinelTextField {
                                        Layout.fillWidth: true
                                        text: viewModel.lmStudioEndpoint || "http://127.0.0.1:1234"
                                        placeholderText: "http://127.0.0.1:1234"
                                        onEditingFinished: viewModel.lmStudioEndpoint = text
                                    }
                                }
                            }

                            // llama.cpp endpoint
                            ColumnLayout {
                                Layout.fillWidth: true
                                visible: viewModel.selectedRuntimeProvider === "llama-cpp-server"
                                spacing: SentinelTheme.spaceXs
                                Label {
                                    text: qsTr("llama.cpp Server URL")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: SentinelTheme.spaceSm
                                    SentinelTextField {
                                        Layout.fillWidth: true
                                        text: viewModel.llamaCppEndpoint || "http://127.0.0.1:8080"
                                        placeholderText: "http://127.0.0.1:8080"
                                        onEditingFinished: viewModel.llamaCppEndpoint = text
                                    }
                                }
                            }

                            // Cloud API - custom endpoint
                            ColumnLayout {
                                Layout.fillWidth: true
                                visible: viewModel.selectedRuntimeProvider === "cloud-api"
                                spacing: SentinelTheme.spaceXs
                                Label {
                                    text: qsTr("Custom API Base URL (optional)")
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: SentinelTheme.spaceSm
                                    SentinelTextField {
                                        Layout.fillWidth: true
                                        text: viewModel.cloudApiEndpoint || ""
                                        placeholderText: qsTr("Override default API endpoint (e.g. self-hosted proxy)")
                                        onEditingFinished: viewModel.cloudApiEndpoint = text
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.topMargin: SentinelTheme.spaceMd
                            height: 1
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
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
                            text: qsTr("Sentinel requires an AI model to generate responses. Below are the recommended models for your selected provider. These are real Ollama models that can be downloaded and run locally.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontBody
                            wrapMode: Text.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.topMargin: SentinelTheme.spaceSm
                            text: qsTr("Tip: For systems with 8 GB RAM or less, we recommend 7B-8B parameter models. For 16 GB+ RAM, 14B+ models will perform well.")
                            color: SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.7)
                            font.pixelSize: SentinelTheme.fontSmall
                            wrapMode: Text.WordWrap
                        }

                        // ── Health status indicator ──
                        Rectangle {
                            Layout.fillWidth: true
                            visible: viewModel.selectedRuntimeProvider === "ollama"
                            radius: SentinelTheme.radiusMd
                            color: viewModel.ollamaHealthStatus === "Healthy"
                                   ? SentinelTheme.withAlpha(SentinelTheme.success, 0.10)
                                   : SentinelTheme.withAlpha(SentinelTheme.warning, 0.10)
                            border.color: viewModel.ollamaHealthStatus === "Healthy"
                                          ? SentinelTheme.withAlpha(SentinelTheme.success, 0.25)
                                          : SentinelTheme.withAlpha(SentinelTheme.warning, 0.25)
                            border.width: 1
                            implicitHeight: statusRow.implicitHeight + SentinelTheme.spaceSm * 2
                            RowLayout {
                                id: statusRow
                                anchors.fill: parent
                                anchors.margins: SentinelTheme.spaceSm
                                spacing: SentinelTheme.spaceSm
                                Text {
                                    text: viewModel.ollamaHealthStatus === "Healthy" ? "●" : "○"
                                    color: viewModel.ollamaHealthStatus === "Healthy" ? SentinelTheme.success : SentinelTheme.warning
                                    font.pixelSize: SentinelTheme.fontBody
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: viewModel.ollamaHealthSummary
                                    color: SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontTiny
                                    wrapMode: Text.WordWrap
                                }
                            }
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
                                        id: modelCard
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

                                        HoverHandler { id: modelHover }
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

                            ColumnLayout {
                                Layout.topMargin: SentinelTheme.spaceMd
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceSm
                                Label {
                                    text: qsTr("Installed Models")
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    font.bold: true
                                }
                                ComboBox {
                                    id: modelSelectCombo
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 38
                                    hoverEnabled: true
                                    model: viewModel.installedOllamaModelNames.length > 0 ? viewModel.installedOllamaModelNames : [qsTr("No models found")]
                                    currentIndex: viewModel.installedOllamaModelNames.indexOf(viewModel.selectedLocalModel)
                                    enabled: viewModel.installedOllamaModelNames.length > 0
                                    onActivated: (index) => {
                                        if (currentText !== qsTr("No models found")) {
                                            viewModel.selectedLocalModel = currentText
                                        }
                                    }
                                    contentItem: Text {
                                        leftPadding: SentinelTheme.spaceMd
                                        rightPadding: SentinelTheme.space2Xl
                                        text: modelSelectCombo.currentIndex >= 0 ? modelSelectCombo.currentText : qsTr("Select a model...")
                                        color: modelSelectCombo.currentIndex >= 0 ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontBody
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                    background: Rectangle {
                                        implicitHeight: 38
                                        radius: SentinelTheme.radiusMd
                                        color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                        border.color: modelSelectCombo.activeFocus || modelSelectCombo.popup.visible
                                                      ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.46)
                                                      : modelSelectCombo.hovered
                                                        ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.24)
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
                                        id: modelSelectDelegate
                                        width: modelSelectCombo.width
                                        implicitHeight: 36
                                        highlighted: modelSelectCombo.highlightedIndex === index
                                        hoverEnabled: true
                                        contentItem: RowLayout {
                                            spacing: SentinelTheme.spaceSm
                                            anchors.fill: parent
                                            anchors.leftMargin: SentinelTheme.spaceMd
                                            anchors.rightMargin: SentinelTheme.spaceMd
                                            Text {
                                                Layout.fillWidth: true
                                                text: modelData === qsTr("No models found") ? modelData : modelData
                                                color: modelSelectDelegate.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                                font.pixelSize: SentinelTheme.fontBody
                                                font.bold: modelSelectDelegate.highlighted
                                                verticalAlignment: Text.AlignVCenter
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                visible: modelSelectCombo.currentIndex === index
                                                text: "\u2713"
                                                color: onboarding.brandAccent
                                                font.pixelSize: SentinelTheme.fontSmall
                                            }
                                        }
                                        background: Rectangle {
                                            color: modelSelectDelegate.highlighted
                                                   ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.12)
                                                   : modelSelectDelegate.hovered
                                                     ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                                     : "transparent"
                                            radius: SentinelTheme.radiusSm
                                            Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                        }
                                    }
                                    popup.background: Rectangle {
                                        radius: SentinelTheme.radiusLg
                                        color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                        border.color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.20)
                                        border.width: 1
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
                            ColumnLayout {
                                Layout.topMargin: SentinelTheme.spaceMd
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceSm
                                Label {
                                    text: qsTr("Active LM Studio Model")
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    font.bold: true
                                }
                                ComboBox {
                                    id: lmStudioModelCombo
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 38
                                    hoverEnabled: true
                                    model: viewModel.loadedLMStudioModelNames.length > 0 ? viewModel.loadedLMStudioModelNames : [qsTr("No models loaded")]
                                    currentIndex: viewModel.loadedLMStudioModelNames.indexOf(viewModel.selectedLocalModel)
                                    enabled: viewModel.loadedLMStudioModelNames.length > 0
                                    displayText: currentIndex >= 0 ? currentText : qsTr("Select a loaded model...")
                                    onActivated: (index) => {
                                        if (currentText !== qsTr("No models loaded")) {
                                            viewModel.selectedLocalModel = currentText
                                        }
                                    }
                                    contentItem: Text {
                                        leftPadding: SentinelTheme.spaceMd
                                        rightPadding: SentinelTheme.space2Xl
                                        text: lmStudioModelCombo.displayText
                                        color: lmStudioModelCombo.currentIndex >= 0 ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontBody
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                    background: Rectangle {
                                        implicitHeight: 38
                                        radius: SentinelTheme.radiusMd
                                        color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                        border.color: lmStudioModelCombo.activeFocus || lmStudioModelCombo.popup.visible
                                                      ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.46)
                                                      : lmStudioModelCombo.hovered
                                                        ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.24)
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
                                        id: lmStudioDelegate
                                        width: lmStudioModelCombo.width
                                        implicitHeight: 36
                                        highlighted: lmStudioModelCombo.highlightedIndex === index
                                        hoverEnabled: true
                                        contentItem: RowLayout {
                                            spacing: SentinelTheme.spaceSm
                                            anchors.fill: parent
                                            anchors.leftMargin: SentinelTheme.spaceMd
                                            anchors.rightMargin: SentinelTheme.spaceMd
                                            Text {
                                                Layout.fillWidth: true
                                                text: modelData === qsTr("No models loaded") ? modelData : modelData
                                                color: lmStudioDelegate.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                                font.pixelSize: SentinelTheme.fontBody
                                                font.bold: lmStudioDelegate.highlighted
                                                verticalAlignment: Text.AlignVCenter
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                visible: lmStudioModelCombo.currentIndex === index
                                                text: "\u2713"
                                                color: onboarding.brandAccent
                                                font.pixelSize: SentinelTheme.fontSmall
                                            }
                                        }
                                        background: Rectangle {
                                            color: lmStudioDelegate.highlighted
                                                   ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.12)
                                                   : lmStudioDelegate.hovered
                                                     ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                                     : "transparent"
                                            radius: SentinelTheme.radiusSm
                                            Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                        }
                                    }
                                    popup.background: Rectangle {
                                        radius: SentinelTheme.radiusLg
                                        color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                        border.color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.20)
                                        border.width: 1
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

                        // Cloud API Specific Setup (OpenAI, Claude, Gemini, DeepSeek, Groq, Mistral)
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: viewModel.selectedRuntimeProvider === "cloud-api" ||
                                     viewModel.selectedRuntimeProvider === "openai" ||
                                     viewModel.selectedRuntimeProvider === "claude" ||
                                     viewModel.selectedRuntimeProvider === "gemini" ||
                                     viewModel.selectedRuntimeProvider === "deepseek" ||
                                     viewModel.selectedRuntimeProvider === "groq" ||
                                     viewModel.selectedRuntimeProvider === "mistral"
                            spacing: SentinelTheme.spaceMd

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Cloud API Configuration:")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: SentinelTheme.fontCard
                                font.bold: true
                            }

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Select your cloud AI provider distribution and enter your API key to connect natively.")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontBody
                                wrapMode: Text.WordWrap
                            }

                            // Sub-Provider Selection Combo
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceXs

                                Label {
                                    text: qsTr("Cloud Provider")
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    font.bold: true
                                }

                                ComboBox {
                                    id: cloudProvCombo
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 38
                                    hoverEnabled: true
                                    model: [
                                        { text: "OpenAI (ChatGPT)",  id: "openai" },
                                        { text: "Anthropic Claude",  id: "claude" },
                                        { text: "Google Gemini",     id: "gemini" },
                                        { text: "DeepSeek API",      id: "deepseek" },
                                        { text: "Groq Cloud",        id: "groq" },
                                        { text: "Mistral AI",        id: "mistral" }
                                    ]
                                    textRole: "text"
                                    valueRole: "id"
                                    currentIndex: {
                                        var idx = 0;
                                        for (var i = 0; i < cloudProvCombo.model.length; i++) {
                                            if (cloudProvCombo.model[i].id === viewModel.selectedCloudProvider) {
                                                idx = i;
                                                break;
                                            }
                                        }
                                        return idx;
                                    }
                                    displayText: currentIndex >= 0 ? currentText : qsTr("Select a cloud provider...")
                                    onActivated: (idx) => {
                                        var prov = model[idx]
                                        viewModel.selectedCloudProvider = prov.id
                                        var defaults = {
                                            openai: "gpt-4o",
                                            claude: "claude-3-5-sonnet-20241022",
                                            gemini: "gemini-2.0-flash",
                                            deepseek: "deepseek-chat",
                                            groq: "llama-3.3-70b-versatile",
                                            mistral: "mistral-large-latest"
                                        }
                                        if (!viewModel.selectedLocalModel ||
                                            !viewModel.selectedLocalModel.startsWith(prov.id === "openai" ? "gpt" : prov.id) &&
                                            !viewModel.selectedLocalModel.startsWith("o1") &&
                                            !viewModel.selectedLocalModel.startsWith("o3")) {
                                            viewModel.selectedLocalModel = defaults[prov.id] || prov.id
                                        }
                                    }
                                    contentItem: Text {
                                        leftPadding: SentinelTheme.spaceMd
                                        rightPadding: SentinelTheme.space2Xl
                                        text: cloudProvCombo.displayText
                                        color: cloudProvCombo.currentIndex >= 0 ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontBody
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                    background: Rectangle {
                                        implicitHeight: 38
                                        radius: SentinelTheme.radiusMd
                                        color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                        border.color: cloudProvCombo.activeFocus || cloudProvCombo.popup.visible
                                                      ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.46)
                                                      : cloudProvCombo.hovered
                                                        ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.24)
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
                                        id: cloudProvDelegate
                                        width: cloudProvCombo.width
                                        implicitHeight: 36
                                        highlighted: cloudProvCombo.highlightedIndex === index
                                        hoverEnabled: true
                                        contentItem: RowLayout {
                                            spacing: SentinelTheme.spaceSm
                                            anchors.fill: parent
                                            anchors.leftMargin: SentinelTheme.spaceMd
                                            anchors.rightMargin: SentinelTheme.spaceMd
                                            Text {
                                                Layout.fillWidth: true
                                                text: modelData.text
                                                color: cloudProvDelegate.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                                font.pixelSize: SentinelTheme.fontBody
                                                font.bold: cloudProvDelegate.highlighted
                                                verticalAlignment: Text.AlignVCenter
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                visible: cloudProvCombo.currentIndex === index
                                                text: "\u2713"
                                                color: onboarding.brandAccent
                                                font.pixelSize: SentinelTheme.fontSmall
                                            }
                                        }
                                        background: Rectangle {
                                            color: cloudProvDelegate.highlighted
                                                   ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.12)
                                                   : cloudProvDelegate.hovered
                                                     ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                                     : "transparent"
                                            radius: SentinelTheme.radiusSm
                                            Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                        }
                                    }
                                    popup.background: Rectangle {
                                        radius: SentinelTheme.radiusLg
                                        color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                        border.color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.20)
                                        border.width: 1
                                    }
                                }
                            }

                            // API Key Field
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceXs

                                Label {
                                    text: viewModel.selectedCloudProvider === "claude" ? qsTr("Anthropic Claude API Key:") :
                                          viewModel.selectedCloudProvider === "gemini" ? qsTr("Google Gemini API Key:") :
                                          viewModel.selectedCloudProvider === "deepseek" ? qsTr("DeepSeek API Key:") :
                                          viewModel.selectedCloudProvider === "groq" ? qsTr("Groq Cloud API Key:") :
                                          viewModel.selectedCloudProvider === "mistral" ? qsTr("Mistral AI API Key:") :
                                          qsTr("OpenAI (ChatGPT) API Key:")
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    font.bold: true
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: 38
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.12)
                                    property bool showKey: false

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: SentinelTheme.spaceMd
                                        anchors.rightMargin: SentinelTheme.spaceSm
                                        spacing: SentinelTheme.spaceSm

                                        TextInput {
                                            Layout.fillWidth: true
                                            text: viewModel.selectedCloudProvider === "claude" ? viewModel.claudeApiKey :
                                                  viewModel.selectedCloudProvider === "gemini" ? viewModel.geminiApiKey :
                                                  viewModel.selectedCloudProvider === "deepseek" ? viewModel.deepseekApiKey :
                                                  viewModel.selectedCloudProvider === "groq" ? viewModel.groqApiKey :
                                                  viewModel.selectedCloudProvider === "mistral" ? viewModel.mistralApiKey :
                                                  viewModel.openAiApiKey
                                            echoMode: parent.parent.showKey ? TextInput.Normal : TextInput.Password
                                            color: SentinelTheme.textPrimary
                                            verticalAlignment: Text.AlignVCenter
                                            font.pixelSize: SentinelTheme.fontBody
                                            selectByMouse: true
                                            clip: true
                                            onEditingFinished: {
                                                if (viewModel.selectedCloudProvider === "claude") viewModel.claudeApiKey = text
                                                else if (viewModel.selectedCloudProvider === "gemini") viewModel.geminiApiKey = text
                                                else if (viewModel.selectedCloudProvider === "deepseek") viewModel.deepseekApiKey = text
                                                else if (viewModel.selectedCloudProvider === "groq") viewModel.groqApiKey = text
                                                else if (viewModel.selectedCloudProvider === "mistral") viewModel.mistralApiKey = text
                                                else viewModel.openAiApiKey = text
                                            }
                                        }

                                        SentinelButton {
                                            text: parent.parent.showKey ? "🔒" : "👁️"
                                            onClicked: parent.parent.showKey = !parent.parent.showKey
                                        }
                                    }
                                }
                            }

                            // Cloud Model Selection
                            ColumnLayout {
                                Layout.topMargin: SentinelTheme.spaceSm
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceXs

                                Label {
                                    text: qsTr("Cloud Model")
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    font.bold: true
                                }

                                ComboBox {
                                    id: cloudModelCombo
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 38
                                    hoverEnabled: true
                                    model: viewModel.selectedCloudProvider === "claude" ?
                                        ["claude-3-5-sonnet-20241022", "claude-3-5-haiku-20241022", "claude-3-opus-20240229", "claude-3-sonnet-20240229", "claude-3-haiku-20240307"] :
                                        viewModel.selectedCloudProvider === "gemini" ?
                                         ["gemini-2.0-flash", "gemini-2.5-pro", "gemini-2.5-flash", "gemini-2.0-flash-lite", "gemini-1.5-pro", "gemini-1.5-flash", "gemini-1.5-flash-8b"] :
                                        viewModel.selectedCloudProvider === "deepseek" ?
                                        ["deepseek-chat", "deepseek-reasoner"] :
                                        viewModel.selectedCloudProvider === "groq" ?
                                        ["llama-3.3-70b-versatile", "llama-3.1-8b-instant", "mixtral-8x7b-32768", "deepseek-r1-distill-llama-70b"] :
                                        viewModel.selectedCloudProvider === "mistral" ?
                                        ["mistral-large-latest", "pixtral-large-latest", "codestral-latest", "mistral-small-latest"] :
                                        viewModel.selectedCloudProvider === "openai" ?
                                        ["gpt-4o", "gpt-4o-mini", "o1", "o1-preview", "o1-mini", "o3-mini", "gpt-4-turbo", "gpt-4", "gpt-3.5-turbo"] :
                                        [qsTr("Select a cloud provider first")]
                                    currentIndex: Math.max(0, model.indexOf(viewModel.selectedLocalModel))
                                    displayText: currentIndex >= 0 ? currentText : qsTr("Select a model...")
                                    onActivated: (idx) => {
                                        viewModel.selectedLocalModel = model[idx]
                                    }
                                    contentItem: Text {
                                        leftPadding: SentinelTheme.spaceMd
                                        rightPadding: SentinelTheme.space2Xl
                                        text: cloudModelCombo.displayText
                                        color: cloudModelCombo.currentIndex >= 0 ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                        font.pixelSize: SentinelTheme.fontBody
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                    background: Rectangle {
                                        implicitHeight: 38
                                        radius: SentinelTheme.radiusMd
                                        color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                        border.color: cloudModelCombo.activeFocus || cloudModelCombo.popup.visible
                                                      ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.46)
                                                      : cloudModelCombo.hovered
                                                        ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.24)
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
                                        id: cloudModelDelegate
                                        width: cloudModelCombo.width
                                        implicitHeight: 36
                                        highlighted: cloudModelCombo.highlightedIndex === index
                                        hoverEnabled: true
                                        contentItem: RowLayout {
                                            spacing: SentinelTheme.spaceSm
                                            anchors.fill: parent
                                            anchors.leftMargin: SentinelTheme.spaceMd
                                            anchors.rightMargin: SentinelTheme.spaceMd
                                            Text {
                                                Layout.fillWidth: true
                                                text: modelData
                                                color: cloudModelDelegate.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                                font.pixelSize: SentinelTheme.fontBody
                                                font.bold: cloudModelDelegate.highlighted
                                                verticalAlignment: Text.AlignVCenter
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                visible: cloudModelCombo.currentIndex === index
                                                text: "\u2713"
                                                color: onboarding.brandAccent
                                                font.pixelSize: SentinelTheme.fontSmall
                                            }
                                        }
                                        background: Rectangle {
                                            color: cloudModelDelegate.highlighted
                                                   ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.12)
                                                   : cloudModelDelegate.hovered
                                                     ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                                     : "transparent"
                                            radius: SentinelTheme.radiusSm
                                            Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                        }
                                    }
                                    popup.background: Rectangle {
                                        radius: SentinelTheme.radiusLg
                                        color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                        border.color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.20)
                                        border.width: 1
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
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: SentinelTheme.spaceMd
                            spacing: SentinelTheme.spaceSm
                            RowLayout {
                                Layout.fillWidth: true
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
                                    id: contextSwitch
                                    checked: viewModel.promptContextInjectionEnabled
                                    onToggled: viewModel.promptContextInjectionEnabled = checked
                                    indicator: Rectangle {
                                        implicitWidth: 46
                                        implicitHeight: 24
                                        x: contextSwitch.leftPadding
                                        y: parent.height / 2 - height / 2
                                        radius: height / 2
                                        color: contextSwitch.checked
                                               ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.18)
                                               : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                        border.color: contextSwitch.checked
                                                      ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.44)
                                                      : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                        Rectangle {
                                            x: contextSwitch.checked ? parent.width - width - 2 : 2
                                            y: 2
                                            width: 20
                                            height: 20
                                            radius: 10
                                            color: contextSwitch.checked ? onboarding.brandAccent : SentinelTheme.textPrimary
                                            opacity: contextSwitch.hovered ? 0.90 : 0.74
                                            Behavior on x { NumberAnimation { duration: MotionTokens.fast } }
                                        }
                                    }
                                }
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
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm
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
                                    id: reducedMotionSwitch
                                    checked: viewModel.reducedMotionEnabled
                                    onToggled: viewModel.reducedMotionEnabled = checked
                                    indicator: Rectangle {
                                        implicitWidth: 46
                                        implicitHeight: 24
                                        x: reducedMotionSwitch.leftPadding
                                        y: parent.height / 2 - height / 2
                                        radius: height / 2
                                        color: reducedMotionSwitch.checked
                                               ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.18)
                                               : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                        border.color: reducedMotionSwitch.checked
                                                      ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.44)
                                                      : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                        Rectangle {
                                            x: reducedMotionSwitch.checked ? parent.width - width - 2 : 2
                                            y: 2
                                            width: 20
                                            height: 20
                                            radius: 10
                                            color: reducedMotionSwitch.checked ? onboarding.brandAccent : SentinelTheme.textPrimary
                                            opacity: reducedMotionSwitch.hovered ? 0.90 : 0.74
                                            Behavior on x { NumberAnimation { duration: MotionTokens.fast } }
                                        }
                                    }
                                }
                            }
                        }

                        // Check updates
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceSm
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
                            }
                            ComboBox {
                                id: updatePolicyCombo
                                Layout.preferredWidth: 280
                                Layout.alignment: Qt.AlignLeft
                                Layout.preferredHeight: 38
                                hoverEnabled: true
                                model: ["Never", "Ask Before Checking", "Weekly", "On Startup"]
                                currentIndex: Math.max(0, model.indexOf(viewModel.updateCheckPolicy))
                                displayText: currentIndex >= 0 ? currentText : qsTr("Select policy...")
                                onActivated: (index) => {
                                    viewModel.updateCheckPolicy = currentText
                                }
                                contentItem: Text {
                                    leftPadding: SentinelTheme.spaceMd
                                    rightPadding: SentinelTheme.space2Xl
                                    text: updatePolicyCombo.displayText
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }
                                background: Rectangle {
                                    implicitHeight: 38
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: updatePolicyCombo.activeFocus || updatePolicyCombo.popup.visible
                                                  ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.46)
                                                  : updatePolicyCombo.hovered
                                                    ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.24)
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
                                    id: updatePolicyDelegate
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
                                            text: modelData
                                            color: updatePolicyDelegate.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontBody
                                            font.bold: updatePolicyDelegate.highlighted
                                            verticalAlignment: Text.AlignVCenter
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            visible: updatePolicyCombo.currentIndex === index
                                            text: "\u2713"
                                            color: onboarding.brandAccent
                                            font.pixelSize: SentinelTheme.fontSmall
                                        }
                                    }
                                    background: Rectangle {
                                        color: updatePolicyDelegate.highlighted
                                               ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.12)
                                               : updatePolicyDelegate.hovered
                                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                                 : "transparent"
                                        radius: SentinelTheme.radiusSm
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                    }
                                }
                                popup.background: Rectangle {
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                    border.color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.20)
                                    border.width: 1
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

                            ComboBox {
                                id: ttsEngineCombo
                                Layout.preferredWidth: 280
                                Layout.preferredHeight: 38
                                hoverEnabled: true
                                model: ["Piper", "Kokoro"]
                                currentIndex: Math.max(0, model.indexOf(viewModel.selectedTtsEngine))
                                displayText: currentIndex >= 0 ? currentText : qsTr("Select engine...")
                                onActivated: {
                                    viewModel.selectedTtsEngine = currentText
                                }
                                contentItem: Text {
                                    leftPadding: SentinelTheme.spaceMd
                                    rightPadding: SentinelTheme.space2Xl
                                    text: ttsEngineCombo.displayText
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontBody
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }
                                background: Rectangle {
                                    implicitHeight: 38
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                                    border.color: ttsEngineCombo.activeFocus || ttsEngineCombo.popup.visible
                                                  ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.46)
                                                  : ttsEngineCombo.hovered
                                                    ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.24)
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
                                    id: ttsDelegate
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
                                            text: modelData
                                            color: ttsDelegate.highlighted ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontBody
                                            font.bold: ttsDelegate.highlighted
                                            verticalAlignment: Text.AlignVCenter
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            visible: ttsEngineCombo.currentIndex === index
                                            text: "\u2713"
                                            color: onboarding.brandAccent
                                            font.pixelSize: SentinelTheme.fontSmall
                                        }
                                    }
                                    background: Rectangle {
                                        color: ttsDelegate.highlighted
                                               ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.12)
                                               : ttsDelegate.hovered
                                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                                 : "transparent"
                                        radius: SentinelTheme.radiusSm
                                        Behavior on color { ColorAnimation { duration: MotionTokens.fast } }
                                    }
                                }
                                popup.background: Rectangle {
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                    border.color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.20)
                                    border.width: 1
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
                                    color: piperBinaryField.text !== "" ? SentinelTheme.success : SentinelTheme.warning
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
                                    id: kokoroDetectLabel
                                    Layout.fillWidth: true
                                    text: kokoroModelField.text !== ""
                                          ? qsTr("✓ Path set") : qsTr("✗ Not yet configured")
                                    color: kokoroModelField.text !== "" ? SentinelTheme.success : SentinelTheme.warning
                                    font.pixelSize: SentinelTheme.fontTiny
                                    font.bold: true
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
                                Label {
                                    Layout.fillWidth: true
                                    text: viewModel.kokoroVoice !== ""
                                          ? qsTr("✓ Voice selected") : qsTr("✗ No voice selected")
                                    color: viewModel.kokoroVoice !== "" ? SentinelTheme.success : SentinelTheme.warning
                                    font.pixelSize: SentinelTheme.fontTiny
                                    font.bold: true
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
                                    color: whisperBinaryField.text !== "" ? SentinelTheme.success : SentinelTheme.warning
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
                                    id: capCard
                                    required property var modelData
                                    Layout.fillWidth: true
                                    radius: SentinelTheme.radiusLg
                                    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.034)
                                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                                    border.width: 1
                                    implicitHeight: capRow.implicitHeight + SentinelTheme.spaceLg * 2
                                    scale: capHover.hovered ? 1.008 : 1.0
                                    Behavior on scale { NumberAnimation { duration: MotionTokens.fast; easing.type: MotionTokens.enter } }
                                    RowLayout {
                                        id: capRow
                                        anchors.fill: parent
                                        anchors.margins: SentinelTheme.spaceLg
                                        spacing: SentinelTheme.spaceMd
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
                                    HoverHandler { id: capHover }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            detailPopup.popupTitle = modelData.t
                                            detailPopup.popupDetail = onboarding.capabilityDetails[modelData.t] || modelData.d

                                            detailPopup.open()
                                        }
                                    }
                                }
                            }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }

                // Step 8 — Finish
                ScrollView {
                    contentData: ColumnLayout {
                        spacing: SentinelTheme.spaceMd
                        width: stack.width

                        Item { Layout.preferredHeight: Math.max(0, (stack.height - contentImplicit.implicitHeight) / 3) }

                        ColumnLayout {
                            id: contentImplicit
                            Layout.fillWidth: true
                            spacing: SentinelTheme.spaceMd
                            Layout.maximumWidth: Math.min(stack.width * 0.88, 580)
                            Layout.alignment: Qt.AlignHCenter

                            Label {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("You're all set")
                                color: SentinelTheme.textPrimary
                                font.pixelSize: Math.max(SentinelTheme.fontTitle, Math.min(SentinelTheme.fontHero, stack.width * 0.04))
                                font.bold: true
                            }

                            Label {
                                Layout.alignment: Qt.AlignHCenter
                                horizontalAlignment: Text.AlignHCenter
                                text: qsTr("Here's your setup. You can revisit everything later from Settings.")
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontBody
                                wrapMode: Text.WordWrap
                                Layout.maximumWidth: parent.width * 0.85
                            }

                            // ── Summary card ──
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.topMargin: SentinelTheme.spaceLg
                                Layout.alignment: Qt.AlignHCenter
                                radius: SentinelTheme.radiusXl

                                color: SentinelTheme.backgroundRaised
                                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
                                border.width: 1

                                implicitHeight: summaryRows.implicitHeight + SentinelTheme.spaceLg * 2

                                ColumnLayout {
                                    id: summaryRows
                                    anchors.fill: parent
                                    anchors.margins: SentinelTheme.spaceLg
                                    spacing: SentinelTheme.spaceSm
                                    InfoRow { label: qsTr("Use Case"); value: viewModel.onboardingUseCase; Layout.fillWidth: true }
                                    InfoRow { label: qsTr("Theme"); value: viewModel.themeName; Layout.fillWidth: true }
                                    InfoRow { label: qsTr("AI Provider"); value: viewModel.onboardingAiProvider; Layout.fillWidth: true }
                                    InfoRow { label: qsTr("Selected Model"); value: viewModel.selectedLocalModel ? viewModel.selectedLocalModel : qsTr("None selected"); Layout.fillWidth: true }
                                    InfoRow { label: qsTr("Memory Context"); value: viewModel.promptContextInjectionEnabled ? qsTr("Enabled") : qsTr("Disabled"); Layout.fillWidth: true }
                                    InfoRow { label: qsTr("TTS Engine"); value: viewModel.selectedTtsEngine; Layout.fillWidth: true }
                                    InfoRow { label: qsTr("System Updates"); value: viewModel.updateCheckPolicy; Layout.fillWidth: true }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            // ── Footer ──────────────────────────────────────────────────────
            Rectangle {
                id: footerRect
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
                        enabled: onboarding.step !== 0 || viewModel.onboardingUseCase !== ""
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
