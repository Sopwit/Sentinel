import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Shapes
import Sentinel.Desktop

SentinelOverlayModal {
    id: root

    // ── Public API ────────────────────────────────────────────────────────────
    required property var modelInfo    // catalog entry JS object

    // ── Geometry ──────────────────────────────────────────────────────────────
    preferredWidth:  480
    preferredHeight: 640
    accent: "#3b82f6"                  // Blue accent color for macOS feel
    modeName: "Sentinel"

    // ── Helpers & State ───────────────────────────────────────────────────────
    readonly property bool isOllama: modelInfo ? modelInfo.id === "ollama" : true
    readonly property string sfProFont: (Qt.platform.os === "osx" || Qt.platform.os === "macos") ? "SF Pro Text" : ""
    readonly property bool isInstalled: modelInfo ? shellViewModel.checkRuntimeInstalled(modelInfo.id) : false

    // Dynamic configuration based on selected runtime
    readonly property string name: modelInfo ? modelInfo.name : "Ollama"
    readonly property string provider: modelInfo ? modelInfo.provider : "Ollama"
    readonly property string subtitleText: isOllama
        ? qsTr("Run local AI models through a lightweight background runtime with an OpenAI-compatible API.")
        : qsTr("Desktop app for local inference. GUI model browser, playground, and OpenAI-compatible local server.")

    readonly property string runtimeType: isOllama ? qsTr("Background Service (Daemon)") : qsTr("Desktop Application")
    readonly property string licenseType: isOllama ? "MIT" : "Proprietary (Free)"
    readonly property string isOpenSource: isOllama ? qsTr("Yes") : qsTr("No")
    readonly property string platforms: "macOS • Windows • Linux"
    readonly property string apiCompatible: "OpenAI Compatible"
    readonly property string defaultPort: isOllama ? "11434" : "1234"

    readonly property var featuresList: isOllama ? [
        qsTr("Run Local Models"),
        qsTr("OpenAI API"),
        qsTr("Streaming"),
        qsTr("Embeddings"),
        qsTr("Tool Calling"),
        qsTr("GPU Acceleration"),
        qsTr("Background Service"),
        qsTr("CLI"),
        qsTr("Automatic Downloads")
    ] : [
        qsTr("Run Local Models"),
        qsTr("OpenAI API"),
        qsTr("Streaming"),
        qsTr("Embeddings"),
        qsTr("Tool Calling"),
        qsTr("GPU Acceleration"),
        qsTr("GUI Model Browser"),
        qsTr("Local Playground"),
        qsTr("Multi-Model Server"),
        qsTr("Server Logs")
    ]

    readonly property var worksWithList: [
        "Llama",
        "Qwen",
        "Gemma",
        "Mistral",
        "DeepSeek",
        "Phi",
        "CodeLlama"
    ]

    readonly property string bestForText: isOllama
        ? qsTr("Running local models for coding assistants, AI agents, automation workflows, backend services and developer tools.")
        : qsTr("Visual model exploration, testing different quantization levels in a GUI playground, and zero-config local server setup.")

    readonly property string downloadUrl: isOllama ? "https://ollama.com/download" : "https://lmstudio.ai"
    readonly property string docsUrl: isOllama ? "https://github.com/ollama/ollama/tree/main/docs" : "https://lmstudio.ai/docs"
    readonly property string githubUrl: isOllama ? "https://github.com/ollama/ollama" : "https://github.com/lmstudio-ai"

    readonly property bool isRunning: isOllama
        ? (shellViewModel.ollamaHealthStatus === "Healthy")
        : (lmStudioLibraryFetcher.models && lmStudioLibraryFetcher.models.length > 0)

    contentItem: Item {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ── Scrollable Inspector Body ─────────────────────────────────────
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.interactive: true

                ColumnLayout {
                    width: parent.width - SentinelTheme.spaceMd * 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: SentinelTheme.spaceMd
                    Layout.topMargin: SentinelTheme.spaceMd
                    Layout.bottomMargin: SentinelTheme.spaceLg

                    // ── HEADER ────────────────────────────────────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm
                        Layout.topMargin: 4

                        // Verified Runtime Badge
                        RowLayout {
                            spacing: SentinelTheme.spaceXs

                            Rectangle {
                                implicitHeight: 20
                                implicitWidth: badgeRow.implicitWidth + 12
                                radius: 10
                                color: SentinelTheme.withAlpha("#3b82f6", 0.08)
                                border.color: SentinelTheme.withAlpha("#3b82f6", 0.24)
                                border.width: 1

                                RowLayout {
                                    id: badgeRow
                                    anchors.centerIn: parent
                                    spacing: 4

                                    // BadgeCheck Icon
                                    Item {
                                        implicitWidth: 12
                                        implicitHeight: 12
                                        Layout.alignment: Qt.AlignVCenter
                                        Shape {
                                            anchors.centerIn: parent
                                            width: 24
                                            height: 24
                                            scale: 12 / 24
                                            ShapePath {
                                                strokeColor: "#3b82f6"
                                                strokeWidth: 2
                                                fillColor: "transparent"
                                                capStyle: ShapePath.RoundCap
                                                joinStyle: ShapePath.RoundJoin
                                                PathSvg {
                                                    path: "M3.85 8.62a4 4 0 0 1 .78-4.06 4 4 0 0 1 4.1-1 4 4 0 0 1 3-3.83 4 4 0 0 1 3 3.83 4 4 0 0 1 4.09 1 4 4 0 0 1 .78 4.06 4 4 0 0 1 3.85 3 4 4 0 0 1-3.85 3.07 4 4 0 0 1-.78 4.06 4 4 0 0 1-4.1 1 4 4 0 0 1-3 3.83 4 4 0 0 1-3-3.83 4 4 0 0 1-4.09-1 4 4 0 0 1-.78-4.06 4 4 0 0 1-3.85-3 4 4 0 0 1 3.85-3.07z"
                                                }
                                            }
                                            ShapePath {
                                                strokeColor: "#3b82f6"
                                                strokeWidth: 2
                                                fillColor: "transparent"
                                                capStyle: ShapePath.RoundCap
                                                joinStyle: ShapePath.RoundJoin
                                                PathSvg {
                                                    path: "M9 12l2 2l4-4"
                                                }
                                            }
                                        }
                                    }

                                    Label {
                                        text: qsTr("Verified Runtime")
                                        font.family: root.sfProFont
                                        font.pixelSize: SentinelTheme.fontTiny
                                        font.weight: Font.Medium
                                        color: "#3b82f6"
                                    }
                                }
                            }
                        }

                        // App/Daemon Title
                        ColumnLayout {
                            spacing: 1
                            Label {
                                text: root.name
                                font.family: root.sfProFont
                                font.pixelSize: SentinelTheme.fontTitle
                                font.weight: Font.Bold
                                color: SentinelTheme.textPrimary
                            }
                            Label {
                                text: qsTr("by %1").arg(root.provider)
                                font.family: root.sfProFont
                                font.pixelSize: SentinelTheme.fontSmall
                                color: SentinelTheme.textMuted
                            }
                        }

                        // Subtitle
                        Label {
                            Layout.fillWidth: true
                            text: root.subtitleText
                            font.family: root.sfProFont
                            font.pixelSize: SentinelTheme.fontSmall
                            color: SentinelTheme.textMuted
                            wrapMode: Text.Wrap
                            lineHeight: 1.25
                        }
                    }

                    // Subtle separator
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: SentinelTheme.separator
                    }

                    // ── OVERVIEW ──────────────────────────────────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        Label {
                            text: qsTr("OVERVIEW")
                            font.family: root.sfProFont
                            font.pixelSize: SentinelTheme.fontTiny
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.1
                            color: SentinelTheme.textPlaceholder
                        }

                        GridLayout {
                            columns: 2
                            rowSpacing: 6
                            columnSpacing: SentinelTheme.spaceSm
                            Layout.fillWidth: true

                            Label { text: qsTr("Category"); font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPlaceholder; Layout.preferredWidth: 120 }
                            Label { text: qsTr("Runtime"); font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPrimary; font.weight: Font.Medium; Layout.fillWidth: true }

                            Label { text: qsTr("Type"); font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPlaceholder }
                            Label { text: root.runtimeType; font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPrimary; font.weight: Font.Medium }

                            Label { text: qsTr("License"); font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPlaceholder }
                            Label { text: root.licenseType; font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPrimary; font.weight: Font.Medium }

                            Label { text: qsTr("Open Source"); font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPlaceholder }
                            Label { text: root.isOpenSource; font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPrimary; font.weight: Font.Medium }

                            Label { text: qsTr("Platforms"); font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPlaceholder }
                            Label { text: root.platforms; font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPrimary; font.weight: Font.Medium }

                            Label { text: qsTr("API"); font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPlaceholder }
                            Label { text: root.apiCompatible; font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPrimary; font.weight: Font.Medium }

                            Label { text: qsTr("Default Port"); font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPlaceholder }
                            Label { text: root.defaultPort; font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPrimary; font.weight: Font.Medium }
                        }
                    }

                    // Subtle separator
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: SentinelTheme.separator
                    }

                    // ── WHAT CAN IT DO? ───────────────────────────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        Label {
                            text: qsTr("WHAT CAN IT DO?")
                            font.family: root.sfProFont
                            font.pixelSize: SentinelTheme.fontTiny
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.1
                            color: SentinelTheme.textPlaceholder
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 6

                            Repeater {
                                model: root.featuresList
                                Rectangle {
                                    implicitHeight: 22
                                    implicitWidth: featureLbl.implicitWidth + 16
                                    radius: 11
                                    color: SentinelTheme.surfaceSoft
                                    border.color: SentinelTheme.separator
                                    border.width: 1

                                    Label {
                                        id: featureLbl
                                        anchors.centerIn: parent
                                        text: modelData
                                        font.family: root.sfProFont
                                        font.pixelSize: SentinelTheme.fontTiny
                                        color: SentinelTheme.textPrimary
                                    }
                                }
                            }
                        }
                    }

                    // Subtle separator
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: SentinelTheme.separator
                    }

                    // ── WORKS WITH ────────────────────────────────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        Label {
                            text: qsTr("WORKS WITH")
                            font.family: root.sfProFont
                            font.pixelSize: SentinelTheme.fontTiny
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.1
                            color: SentinelTheme.textPlaceholder
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 6

                            Repeater {
                                model: root.worksWithList
                                Rectangle {
                                    implicitHeight: 22
                                    implicitWidth: modelChipLbl.implicitWidth + 16
                                    radius: 11
                                    color: SentinelTheme.withAlpha("#3b82f6", 0.04)
                                    border.color: SentinelTheme.withAlpha("#3b82f6", 0.12)
                                    border.width: 1

                                    Label {
                                        id: modelChipLbl
                                        anchors.centerIn: parent
                                        text: modelData
                                        font.family: root.sfProFont
                                        font.pixelSize: SentinelTheme.fontTiny
                                        color: SentinelTheme.textMuted
                                    }
                                }
                            }
                        }

                        Label {
                            text: qsTr("Supports most GGUF-based language models.")
                            font.family: root.sfProFont
                            font.pixelSize: SentinelTheme.fontTiny
                            color: SentinelTheme.textPlaceholder
                            Layout.topMargin: 2
                        }
                    }

                    // Subtle separator
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: SentinelTheme.separator
                    }

                    // ── BEST FOR ──────────────────────────────────────────────
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: bestForCol.implicitHeight + 24
                        radius: SentinelTheme.radiusMd
                        color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.05)
                        border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.15)
                        border.width: 1

                        ColumnLayout {
                            id: bestForCol
                            anchors { fill: parent; margins: 12 }
                            spacing: 6

                            RowLayout {
                                spacing: 6
                                // Info icon (Lucide)
                                Item {
                                    implicitWidth: 14
                                    implicitHeight: 14
                                    Shape {
                                        anchors.centerIn: parent
                                        width: 24
                                        height: 24
                                        scale: 14 / 24
                                        ShapePath {
                                            strokeColor: SentinelTheme.accent
                                            strokeWidth: 2
                                            fillColor: "transparent"
                                            capStyle: ShapePath.RoundCap
                                            joinStyle: ShapePath.RoundJoin
                                            PathSvg { path: "M12 22c5.523 0 10-4.477 10-10S17.523 2 12 2 2 6.477 2 12s4.477 10 10 10z" }
                                            PathSvg { path: "M12 16v-4" }
                                            PathSvg { path: "M12 8h.01" }
                                        }
                                    }
                                }
                                Label {
                                    text: qsTr("Best For")
                                    font.family: root.sfProFont
                                    font.pixelSize: SentinelTheme.fontSmall
                                    font.weight: Font.DemiBold
                                    color: SentinelTheme.accent
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.bestForText
                                font.family: root.sfProFont
                                font.pixelSize: SentinelTheme.fontSmall
                                color: SentinelTheme.textPrimary
                                wrapMode: Text.Wrap
                                lineHeight: 1.25
                            }
                        }
                    }

                    // Subtle separator
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: SentinelTheme.separator
                    }

                    // ── INSTALLATION ──────────────────────────────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceSm

                        Label {
                            text: qsTr("INSTALLATION")
                            font.family: root.sfProFont
                            font.pixelSize: SentinelTheme.fontTiny
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.1
                            color: SentinelTheme.textPlaceholder
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: instGrid.implicitHeight + 20
                            radius: SentinelTheme.radiusMd
                            color: SentinelTheme.surfaceSoft
                            border.color: SentinelTheme.separator
                            border.width: 1

                            GridLayout {
                                id: instGrid
                                columns: 2
                                anchors { fill: parent; margins: 10 }
                                rowSpacing: 6
                                columnSpacing: SentinelTheme.spaceSm

                                Label { text: qsTr("Status"); font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPlaceholder; Layout.preferredWidth: 120 }
                                RowLayout {
                                    spacing: 6
                                    Layout.fillWidth: true
                                    Rectangle {
                                        implicitWidth: 8
                                        implicitHeight: 8
                                        radius: 4
                                        color: root.isRunning ? SentinelTheme.success
                                                              : (root.isInstalled ? root.accent : SentinelTheme.textPlaceholder)
                                    }
                                    Label {
                                        text: root.isRunning ? qsTr("Running")
                                                             : (root.isInstalled ? qsTr("Installed (Stopped)") : qsTr("Not Installed"))
                                        font.family: root.sfProFont
                                        font.pixelSize: SentinelTheme.fontSmall
                                        color: root.isRunning ? SentinelTheme.success
                                                              : (root.isInstalled ? SentinelTheme.accent : SentinelTheme.textPrimary)
                                        font.weight: Font.Medium
                                    }
                                }

                                Label { text: qsTr("Version"); font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPlaceholder }
                                Label { text: "—"; font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPrimary; font.weight: Font.Medium }

                                Label { text: qsTr("Auto Update"); font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPlaceholder }
                                Label { text: qsTr("Supported"); font.family: root.sfProFont; font.pixelSize: SentinelTheme.fontSmall; color: SentinelTheme.textPrimary; font.weight: Font.Medium }
                            }
                        }
                    }
                }
            }

            // Divider above footer
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: SentinelTheme.separator
            }

            // ── FOOTER ────────────────────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 56
                color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                radius: SentinelTheme.radiusXl

                RowLayout {
                    anchors { fill: parent; leftMargin: SentinelTheme.spaceMd; rightMargin: SentinelTheme.spaceMd }
                    spacing: SentinelTheme.spaceSm

                    // Close Button
                    Button {
                        id: footerCloseBtn
                        implicitHeight: 30
                        implicitWidth: closeBtnLbl.implicitWidth + 20
                        hoverEnabled: true
                        onClicked: root.close()
                        scale: down ? 0.98 : (hovered ? 1.02 : 1.0)
                        Behavior on scale { NumberAnimation { duration: 80 } }

                        background: Rectangle {
                            radius: SentinelTheme.radiusSm
                            color: footerCloseBtn.down ? SentinelTheme.surfaceHover
                                                       : footerCloseBtn.hovered ? SentinelTheme.surfaceSoft
                                                                                : "transparent"
                            border.color: SentinelTheme.separator
                            border.width: 1
                        }
                        contentItem: Label {
                            id: closeBtnLbl
                            text: qsTr("Close")
                            font.family: root.sfProFont
                            font.pixelSize: SentinelTheme.fontSmall
                            color: SentinelTheme.textPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    // Documentation Link
                    Button {
                        id: docsBtn
                        implicitHeight: 30
                        implicitWidth: docsBtnLbl.implicitWidth + 20
                        hoverEnabled: true
                        onClicked: Qt.openUrlExternally(root.docsUrl)
                        scale: down ? 0.98 : (hovered ? 1.02 : 1.0)
                        Behavior on scale { NumberAnimation { duration: 80 } }

                        background: Rectangle {
                            radius: SentinelTheme.radiusSm
                            color: docsBtn.down ? SentinelTheme.surfaceHover
                                                : docsBtn.hovered ? SentinelTheme.surfaceSoft
                                                                  : "transparent"
                            border.color: SentinelTheme.separator
                            border.width: 1
                        }
                        contentItem: RowLayout {
                            id: docsBtnLbl
                            spacing: 4
                            anchors.centerIn: parent

                            Item {
                                implicitWidth: 12
                                implicitHeight: 12
                                Shape {
                                    anchors.centerIn: parent
                                    width: 24
                                    height: 24
                                    scale: 12 / 24
                                    ShapePath {
                                        strokeColor: SentinelTheme.textMuted
                                        strokeWidth: 2
                                        fillColor: "transparent"
                                        capStyle: ShapePath.RoundCap
                                        joinStyle: ShapePath.RoundJoin
                                        PathSvg { path: "M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z" }
                                        PathSvg { path: "M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z" }
                                    }
                                }
                            }
                            Label {
                                text: qsTr("Documentation")
                                font.family: root.sfProFont
                                font.pixelSize: SentinelTheme.fontSmall
                                color: SentinelTheme.textMuted
                            }
                        }
                    }

                    // GitHub / Link Button
                    Button {
                        id: gitBtn
                        implicitHeight: 30
                        implicitWidth: gitBtnLbl.implicitWidth + 20
                        hoverEnabled: true
                        onClicked: Qt.openUrlExternally(root.githubUrl)
                        scale: down ? 0.98 : (hovered ? 1.02 : 1.0)
                        Behavior on scale { NumberAnimation { duration: 80 } }

                        background: Rectangle {
                            radius: SentinelTheme.radiusSm
                            color: gitBtn.down ? SentinelTheme.surfaceHover
                                               : gitBtn.hovered ? SentinelTheme.surfaceSoft
                                                                : "transparent"
                            border.color: SentinelTheme.separator
                            border.width: 1
                        }
                        contentItem: RowLayout {
                            id: gitBtnLbl
                            spacing: 4
                            anchors.centerIn: parent

                            Item {
                                implicitWidth: 12
                                implicitHeight: 12
                                Shape {
                                    anchors.centerIn: parent
                                    width: 24
                                    height: 24
                                    scale: 12 / 24
                                    ShapePath {
                                        strokeColor: SentinelTheme.textMuted
                                        strokeWidth: 2
                                        fillColor: "transparent"
                                        capStyle: ShapePath.RoundCap
                                        joinStyle: ShapePath.RoundJoin
                                        PathSvg { path: "M15 22v-4a4.8 4.8 0 0 0-1-3.5c3 0 6-2 6-5.5.08-1.25-.27-2.48-1-3.5.28-1.15.28-2.35 0-3.5 0 0-1 0-3 1.5-2.64-.5-5.36-.5-8 0C6 2 5 2 5 2c-.3 1.15-.3 2.35 0 3.5A5.403 5.403 0 0 0 4 9c0 3.5 3 5.5 6 5.5-.39.49-.68 1.05-.85 1.65-.17.6-.22 1.23-.15 1.85v4" }
                                        PathSvg { path: "M9 18c-4.51 2-5-2-7-2" }
                                    }
                                }
                            }
                            Label {
                                text: isOllama ? "GitHub" : qsTr("Website")
                                font.family: root.sfProFont
                                font.pixelSize: SentinelTheme.fontSmall
                                color: SentinelTheme.textMuted
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Primary Action: Install Runtime
                    Button {
                        id: installBtn
                        implicitHeight: 30
                        implicitWidth: installLbl.implicitWidth + 20
                        hoverEnabled: true
                        onClicked: Qt.openUrlExternally(root.downloadUrl)
                        scale: down ? 0.98 : (hovered ? 1.02 : 1.0)
                        Behavior on scale { NumberAnimation { duration: 80 } }

                        background: Rectangle {
                            radius: SentinelTheme.radiusSm
                            color: installBtn.down ? root.accent
                                                   : installBtn.hovered ? SentinelTheme.withAlpha(root.accent, 0.9)
                                                                        : root.accent
                            border.color: SentinelTheme.withAlpha(root.accent, 0.2)
                            border.width: 1
                        }
                        contentItem: RowLayout {
                            id: installLbl
                            spacing: 4
                            anchors.centerIn: parent

                            Item {
                                implicitWidth: 12
                                implicitHeight: 12
                                Shape {
                                    anchors.centerIn: parent
                                    width: 24
                                    height: 24
                                    scale: 12 / 24
                                    ShapePath {
                                        strokeColor: SentinelTheme.textOnAccent
                                        strokeWidth: 2
                                        fillColor: "transparent"
                                        capStyle: ShapePath.RoundCap
                                        joinStyle: ShapePath.RoundJoin
                                        PathSvg { path: "M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" }
                                        PathSvg { path: "M7 10l5 5l5-5" }
                                        PathSvg { path: "M12 15V3" }
                                    }
                                }
                            }

                            Label {
                                text: qsTr("Install Runtime")
                                font.family: root.sfProFont
                                font.pixelSize: SentinelTheme.fontSmall
                                font.weight: Font.DemiBold
                                color: SentinelTheme.textOnAccent
                            }
                        }
                    }
                }
            }
        }
    }
}
