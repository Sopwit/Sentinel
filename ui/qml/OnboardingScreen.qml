import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: onboarding
    required property var viewModel

    property int step: 0
    property bool active: false
    readonly property int totalSteps: 6
    readonly property color brandAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property bool reducedMotion: viewModel.reducedMotionEnabled

    signal finished()

    opacity: active ? 1.0 : 0.0
    visible: opacity > 0.01
    enabled: active
    z: 300

    readonly property var stepMeta: [
        { key: "welcome",      title: qsTr("Welcome"),     caption: qsTr("A calm assistant built around you.") },
        { key: "privacy",      title: qsTr("Privacy"),     caption: qsTr("Your data stays on your device.") },
        { key: "appearance",   title: qsTr("Appearance"),  caption: qsTr("Make Sentinel feel like home.") },
        { key: "provider",     title: qsTr("AI Setup"),    caption: qsTr("Connect to models, your way.") },
        { key: "capabilities", title: qsTr("Capabilities"),caption: qsTr("Everything Sentinel can do.") },
        { key: "finish",       title: qsTr("Finish"),      caption: qsTr("You're ready to begin.") }
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
        { id: "llama.cpp server",         note: qsTr("Lightweight server for advanced users.") },
        { id: "OpenAI-compatible local endpoint", note: qsTr("Bring your own local endpoint.") }
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
                                        onClicked: viewModel.onboardingAiProvider = modelData.id
                                    }
                                }
                            }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }

                // Step 4 — Capabilities
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
                            InfoRow { label: qsTr("AI"); value: viewModel.onboardingAiProvider + qsTr(" · nothing downloaded yet"); Layout.fillWidth: true; valueMaximumLineCount: 2 }
                            InfoRow { label: qsTr("Privacy"); value: qsTr("No tracking, no hidden uploads, no silent updates."); Layout.fillWidth: true; valueMaximumLineCount: 3 }
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
