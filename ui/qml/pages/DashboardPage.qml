import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ScrollView {
    id: dashboardPage
    required property var viewModel
    readonly property bool wideLayout: false
    readonly property bool compact: width < 860
    readonly property int panelPadding: SentinelTheme.spaceLg
    readonly property string runtimeStatusText: dashboardPage.viewModel.ollamaHealthStatus
                                                + " / "
                                                + dashboardPage.viewModel.localInferenceRuntimeState
    readonly property bool runtimeReady: dashboardPage.viewModel.ollamaHealthStatus === "Available"
                                  || dashboardPage.viewModel.ollamaHealthStatus === "Ready"
                                  || dashboardPage.viewModel.selectedLocalModelStatus === "Available"
                                  || dashboardPage.viewModel.selectedLocalModelStatus === "Fallback"
    readonly property bool streamingActive: dashboardPage.viewModel.localInferenceStreamingText.length > 0
                                            || dashboardPage.viewModel.localInferenceRuntimeState === "Streaming"
    readonly property bool companionMode: dashboardPage.viewModel.currentModeName === "Companion Mode"
    readonly property bool focusMode: dashboardPage.viewModel.currentModeName === "Focus Mode"
    readonly property bool telemetryMode: dashboardPage.viewModel.currentModeName === "Mission Mode"
                                          || dashboardPage.viewModel.currentModeName === "System Mode"
                                          || dashboardPage.viewModel.currentModeName === "Tactical Mode"
    readonly property string uiSelfCheck: "home-uses-available-height scroll-bottom-padding"
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    function focusComposer() {
        homeChatSurface.focusComposer()
    }

    contentWidth: availableWidth

    Item {
        width: dashboardPage.availableWidth
        height: dashboardPage.availableHeight
        implicitHeight: height

        GridLayout {
            id: dashboardGrid
            width: parent.width
            height: parent.height
            columns: dashboardPage.wideLayout ? 12 : 1
            columnSpacing: SentinelTheme.spaceLg
            rowSpacing: SentinelTheme.spaceLg

            ColumnLayout {
                Layout.fillWidth: true
                Layout.columnSpan: 1
                spacing: dashboardPage.focusMode ? SentinelTheme.spaceMd : SentinelTheme.spaceLg

                HomeChatSurface {
                    id: homeChatSurface
                    viewModel: dashboardPage.viewModel
                    compact: dashboardPage.compact || dashboardPage.focusMode
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 0
                }
            }
        }
    }
}
