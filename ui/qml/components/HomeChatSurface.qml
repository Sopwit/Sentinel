import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Sentinel.Desktop

ShellPanel {
    id: homeChat
    required property var viewModel
    property bool compact: width < 760
    property bool forceChatView: false
    readonly property bool inChatMode: (viewModel.conversationHistoryMessageCount > 0)
                                       || sendBusy
                                       || forceChatView
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property bool chatReady: viewModel.localChatSendAvailable
    readonly property bool canSend: viewModel.localChatSendAvailable
    readonly property string sendState: viewModel.chatSendLifecycleState
    readonly property bool sendBusy: sendState === "validating" || sendState === "sending"
                                     || sendState === "streaming"
    readonly property bool streamingActive: sendState === "streaming"
                                            || viewModel.localInferenceRuntimeState === "Streaming"
    readonly property string uiSelfCheck: "chat-scroll-safe-area composer-visible no-bridge-duplication"
    readonly property string disabledReason: viewModel.activeConversationArchived
                                             ? viewModel.activeConversationStateSummary
                                             : viewModel.localChatSendAvailabilitySummary
    readonly property bool sidebarEffectiveOpen: conversationSidebarOpen && !compact
    readonly property bool isLMStudio: homeChat.viewModel.selectedRuntimeProvider === "lm-studio"
    readonly property string localProviderLabel: isLMStudio ? "LM Studio" : "Ollama"
    property bool conversationSidebarOpen: true
    property string conversationFilter: ""
    property string sidebarView: "recent"
    property string pendingDeleteConversationId: ""
    property string pendingDeleteConversationTitle: ""   // "recent" | "pinned" | "archived"
    readonly property real resolutionScale: Math.max(0.7, Math.min(1.4, homeChat.height / 860.0))
    property bool showSuggestions: true
    property var activeGreeting: ({ title: qsTr("Merhaba, Operator"), subtitle: qsTr("Sentinel tüm yerel yapay zeka gücüyle hizmetinizde. Nasıl yardımcı olabilirim?") })

    radius: SentinelTheme.radiusPanel
    color: "transparent"

    Connections {
        target: homeChat.viewModel
        function onVoiceTranscriptionCompleted(transcript) {
            if (homeChat.inChatMode) {
                promptInput.text = transcript
                promptInput.forceActiveFocus()
            } else {
                homePromptInput.text = transcript
                homePromptInput.forceActiveFocus()
            }
        }
        function onChatMessagesChanged() {
            if (homeChat.viewModel.conversationHistoryMessageCount === 0) {
                homeChat.forceChatView = false;
            }
        }
    }
    border.color: "transparent"

    function selectRandomGreeting() {
        var hour = new Date().getHours();
        var morningGreetings = [
            { title: qsTr("Günaydın, Operator"), subtitle: qsTr("Bugün yeni projeler üretmek ve hedeflerine ulaşmak için harika bir gün. Nasıl yardımcı olabilirim?") },
            { title: qsTr("Güzel Bir Sabah"), subtitle: qsTr("Zihnini taze tut, üretkenlik senin elinde. Bugün birlikte ne inşa edelim?") },
            { title: qsTr("Günaydın, Sentinel Hazır"), subtitle: qsTr("Yeni bir güne başlarken tüm yerel arama, analiz ve yapay zeka araçlarınla yanındayım.") },
            { title: qsTr("Sabah Odaklanması Başladı"), subtitle: qsTr("Taze bir zihinle günlük planları yapmaya, işleri organize etmeye veya yeni araştırmalara başlamaya hazırız.") },
            { title: qsTr("Günaydın! Yeni Hedefler"), subtitle: qsTr("Günün ilk saatlerinde zihnindeki o parlak fikri birlikte hayata geçirelim mi?") },
            { title: qsTr("Harika Bir Sabah Başlangıcı"), subtitle: qsTr("Günün üretkenliğini tetikleyecek en iyi çözümleri, yerel zekamızla harmanlayalım.") }
        ];
        var afternoonGreetings = [
            { title: qsTr("İyi Günler, Operator"), subtitle: qsTr("İş akışını hızlandırmak ve operasyonlarını yönetmek için hazır mıyım? Nasıl destek olabilirim?") },
            { title: qsTr("Tünaydın! Üretkenliğe Devam"), subtitle: qsTr("Günün en verimli saatindeyiz. Projelerindeki tıkanıklıkları birlikte çözelim mi?") },
            { title: qsTr("Verimli Bir Gün"), subtitle: qsTr("Kişisel ve profesyonel hedeflerini gerçekleştirelim. Yazılarında, planlarında veya analizlerinde yardım etmeye hazırım.") },
            { title: qsTr("Tünaydın! Odaklanma Zamanı"), subtitle: qsTr("Hız kesmeden projelere devam ediyoruz. Metin yazma, araştırma yapma veya analiz süreçlerinde yanındayım.") },
            { title: qsTr("Tünaydın! Sentinel Göreve Hazır"), subtitle: qsTr("Günün temposunu yakalamışken, yerel yapay zeka gücüyle iş akışını bir üst seviyeye taşıyalım.") },
            { title: qsTr("Öğleden Sonra Sinerjisi"), subtitle: qsTr("Yaratıcı fikirleri, raporları ve doküman analizlerini birlikte çözmenin tam vakti.") }
        ];
        var eveningGreetings = [
            { title: qsTr("İyi Akşamlar, Operator"), subtitle: qsTr("Günün geri kalanını başarılı ve verimli bir şekilde kapatmana nasıl yardımcı olabilirim?") },
            { title: qsTr("Keyifli Bir Akşam Seansı"), subtitle: qsTr("Günün yorgunluğunu hafifletelim. Sentinel tüm yerel zekasıyla seni destekliyor.") },
            { title: qsTr("İyi Akşamlar! Fikirler Canlansın"), subtitle: qsTr("Günün hedeflerini tamamlayalım. Akşam sessizliğinde üretkenliği zirveye çıkarabiliriz.") },
            { title: qsTr("Akşam Saatleri ve Odaklanma"), subtitle: qsTr("Günün raporlarını gözden geçirelim, yarım kalan işleri toparlayalım veya yeni fikirleri hayata geçirelim.") },
            { title: qsTr("İyi Akşamlar! Sentinel Yanında"), subtitle: qsTr("Sakin bir akşam seansında, en karmaşık problemleri adım adım analiz etmek için buradayım.") },
            { title: qsTr("Akşam Üretkenliği"), subtitle: qsTr("Günün son adımlarını atarken yerel arama gücüyle en kritik belgelere ve bilgilere ulaş.") }
        ];
        var nightGreetings = [
            { title: qsTr("İyi Geceler, Operator"), subtitle: qsTr("Son detayları gözden geçirmek, planlarını hazırlamak veya yaratıcı yazılar yazmak için burayım.") },
            { title: qsTr("Gece Sessizliği ve Odaklanma"), subtitle: qsTr("Sessiz ve odaklanmış bir çalışma seansı için hazırım. Zihnindeki fikirleri hayata geçirelim mi?") },
            { title: qsTr("İyi Geceler! Sentinel Aktif"), subtitle: qsTr("Günü kapatmadan önce yarım kalan işleri veya yarına dair planları organize edebiliriz.") },
            { title: qsTr("Gece Boyu Üretkenlik"), subtitle: qsTr("Gece sessizliğinin getirdiği o benzersiz odakla, en karmaşık soruları ve projeleri birlikte analiz edelim.") },
            { title: qsTr("İyi Geceler! Gece Seansı Başladı"), subtitle: qsTr("Uykudan önce son bir okuma mı? Yoksa yarına dair hedeflerin yapılandırılması mı?") },
            { title: qsTr("Gece Yarısı Yaratıcılığı"), subtitle: qsTr("Herkes uyurken yeni projelerin temellerini yerel yapay zeka asistanınla güvenle at.") }
        ];

        var list;
        if (hour >= 6 && hour < 12) {
            list = morningGreetings;
        } else if (hour >= 12 && hour < 18) {
            list = afternoonGreetings;
        } else if (hour >= 18 && hour < 22) {
            list = eveningGreetings;
        } else {
            list = nightGreetings;
        }

        var index = Math.floor(Math.random() * list.length);
        activeGreeting = list[index];
    }

    function formatAttachmentSummary(summary) {
        if (!summary) return "";
        var parts = summary.split(" / ");
        if (parts.length < 3) return summary;
        
        var fileName = parts[0];
        var fileType = parts[1];
        var sizeBytes = parseInt(parts[2]);
        
        var sizeStr = "";
        if (sizeBytes < 1024) {
            sizeStr = sizeBytes + " B";
        } else if (sizeBytes < 1024 * 1024) {
            sizeStr = (sizeBytes / 1024).toFixed(1) + " KB";
        } else {
            sizeStr = (sizeBytes / (1024 * 1024)).toFixed(1) + " MB";
        }
        
        return fileName + " (" + sizeStr + ")";
    }

    function getAttachmentIcon(summary) {
        if (!summary) return "📎";
        var parts = summary.split(" / ");
        if (parts.length < 2) return "📎";
        var fileType = parts[1].toLowerCase();
        
        if (fileType === "image") {
            return "🖼️";
        } else if (fileType === "pdf" || fileType === "docx") {
            return "📄";
        } else if (fileType === "source code") {
            return "💻";
        } else {
            return "📎";
        }
    }

    function scrollToLatest(force) {
        if (force || recentMessages.followNewMessages)
            Qt.callLater(recentMessages.positionViewAtEnd)
    }

    function focusComposer() {
        promptInput.forceActiveFocus()
    }

    function restoreDraft(text) {
        promptInput.text = text
        promptInput.forceActiveFocus()
    }

    function sendComposerText() {
        var prompt = promptInput.text.trim()
        if (prompt.length === 0 || !homeChat.canSend || homeChat.sendBusy)
            return
        var accepted = homeChat.viewModel.sendMessage(promptInput.text)
        var lifecycle = homeChat.viewModel.chatSendLifecycleState
        if (accepted || lifecycle === "sending" || lifecycle === "streaming"
                || lifecycle === "completed" || lifecycle === "failed") {
            promptInput.clear()
            recentMessages.followNewMessages = true
            homeChat.scrollToLatest(true)
        }
    }

    // Returns true if the conversation at the given conversationIds index is pinned
    function isPinned(index) {
        return viewModel.conversationPinnedSummaries[index] === "Pinned"
    }

    // Returns true if the conversation at the given conversationIds index is archived
    function isArchived(index) {
        return viewModel.conversationArchivedSummaries[index] === "Archived"
    }

    // Returns indexes of conversations matching the current view/filter
    // view: "recent" | "pinned" | "archived"
    function filteredConversationIndexes(view) {
        var normalized = conversationFilter.trim().toLowerCase()
        var result = []
        for (var i = 0; i < viewModel.conversationIds.length; ++i) {
            var archived = isArchived(i)
            var pinned = isPinned(i)
            if (view === "pinned" && !pinned) continue
            if (view === "archived" && !archived) continue
            if (view === "recent" && (pinned || archived)) continue
            var title = viewModel.conversationTitles[i]
            var id = viewModel.conversationIds[i]
            if (normalized.length === 0
                    || title.toLowerCase().indexOf(normalized) >= 0
                    || id.toLowerCase().indexOf(normalized) >= 0) {
                result.push(i)
            }
        }
        return result
    }

    onStreamingActiveChanged: {
        if (!streamingActive)
            scrollToLatest(true)
    }

    onSidebarViewChanged: {
        conversationList.model = filteredConversationIndexes(sidebarView)
    }

    onConversationFilterChanged: {
        conversationList.model = filteredConversationIndexes(sidebarView)
    }

    onInChatModeChanged: {
        if (!inChatMode) {
            conversationSidebarOpen = false
            selectRandomGreeting();
        }
    }

    Component.onCompleted: {
        if (!inChatMode) {
            conversationSidebarOpen = false
        }
        selectRandomGreeting();
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: homeChat.compact ? SentinelTheme.spaceMd : SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceMd

        ShellPanel {
            id: conversationRail
            Layout.preferredWidth: homeChat.sidebarEffectiveOpen ? Math.min(300, Math.max(238, homeChat.width * 0.22)) : 44
            visible: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            color: SentinelTheme.lightTheme
                 ? "#ffffff"
                 : SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.70)
            border.color: SentinelTheme.lightTheme
                        ? SentinelTheme.withAlpha("#e2e8f4", 0.90)
                        : SentinelTheme.withAlpha(homeChat.modeAccent, 0.20)
            showBrackets: false
            clip: true

            Behavior on Layout.preferredWidth {
                NumberAnimation {
                    duration: MotionTokens.duration(MotionTokens.normal, homeChat.viewModel.currentModeName)
                    easing.type: MotionTokens.enter
                }
            }

            // Collapsed strip: New Chat + expand toggle
            ColumnLayout {
                visible: !homeChat.sidebarEffectiveOpen
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceXs
                width: 32

                Button {
                    id: collapsedNewChatBtn
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("New Chat")
                    onClicked: {
                        homeChat.viewModel.createConversation(qsTr("New Chat"))
                        homeChat.inChatMode = true
                        promptInput.clear()
                        homePromptInput.clear()
                        if (homeChat.compact)
                            homeChat.conversationSidebarOpen = false
                    }
                    contentItem: Text {
                        text: "+"
                        color: SentinelTheme.textPrimary
                        font.pixelSize: 20
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: SentinelTheme.radiusMd
                        color: InteractionTokens.surfaceColor(collapsedNewChatBtn.hovered, collapsedNewChatBtn.down, false, homeChat.modeAccent)
                        border.color: InteractionTokens.borderColor(collapsedNewChatBtn.activeFocus, collapsedNewChatBtn.hovered, false, homeChat.modeAccent)
                    }
                }

                Button {
                    id: sidebarToggleCollapsed
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Show sidebar")
                    onClicked: homeChat.conversationSidebarOpen = true
                    contentItem: Text {
                        text: "\u2630"
                        color: SentinelTheme.textPrimary
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: SentinelTheme.radiusMd
                        color: InteractionTokens.surfaceColor(sidebarToggleCollapsed.hovered, sidebarToggleCollapsed.down, false, homeChat.modeAccent)
                        border.color: InteractionTokens.borderColor(sidebarToggleCollapsed.activeFocus, sidebarToggleCollapsed.hovered, false, homeChat.modeAccent)
                    }
                }
            }

            // Expanded panel
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceSm
                visible: homeChat.sidebarEffectiveOpen

                // Top bar: New Chat icon + Search field (with search icon inside) + Hide icon
                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceXs

                    // New Chat icon button
                    Button {
                        id: newChatIconBtn
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 30
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("New Chat")
                        onClicked: {
                            homeChat.viewModel.createConversation(qsTr("New Chat"))
                            homeChat.inChatMode = true
                            homeChat.conversationSidebarOpen = !homeChat.compact
                            promptInput.clear()
                            homePromptInput.clear()
                        }
                        contentItem: Text {
                            text: "+"
                            color: SentinelTheme.textPrimary
                            font.pixelSize: 18
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: SentinelTheme.radiusMd
                            color: InteractionTokens.surfaceColor(newChatIconBtn.hovered, newChatIconBtn.down, false, homeChat.modeAccent)
                            border.color: InteractionTokens.borderColor(newChatIconBtn.activeFocus, newChatIconBtn.hovered, false, homeChat.modeAccent)
                        }
                    }

                    // Search field with embedded search icon on the right
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        radius: SentinelTheme.radiusMd
                        color: conversationSearch.activeFocus
                               ? SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
                               : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.48)
                        border.color: conversationSearch.activeFocus
                                      ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.55)
                                      : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)

                        TextInput {
                            id: conversationSearch
                            anchors.left: parent.left
                            anchors.right: searchIconBtn.left
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: SentinelTheme.spaceSm
                            anchors.rightMargin: SentinelTheme.spaceXs
                            text: homeChat.conversationFilter
                            onTextChanged: homeChat.conversationFilter = text
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontSmall
                            selectionColor: SentinelTheme.withAlpha(homeChat.modeAccent, 0.34)
                            selectedTextColor: SentinelTheme.textPrimary
                            clip: true

                            Text {
                                anchors.fill: parent
                                text: qsTr("Search")
                                color: SentinelTheme.textPlaceholder
                                font.pixelSize: SentinelTheme.fontSmall
                                verticalAlignment: Text.AlignVCenter
                                visible: conversationSearch.text.length === 0 && !conversationSearch.activeFocus
                            }
                        }

                        // Search icon button (right side of field)
                        Button {
                            id: searchIconBtn
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.rightMargin: 3
                            width: 24
                            height: 24
                            hoverEnabled: true
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Search")
                            enabled: conversationSearch.text.trim().length > 0
                            onClicked: homeChat.viewModel.searchConversation(conversationSearch.text)
                            contentItem: Text {
                                text: "\uD83D\uDD0D"
                                font.pixelSize: 11
                                color: searchIconBtn.enabled ? SentinelTheme.textMuted : SentinelTheme.textPlaceholder
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                radius: SentinelTheme.radiusSm
                                color: searchIconBtn.hovered ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.12) : "transparent"
                            }
                        }
                    }

                    // Hide icon button (compact, right side of sidebar)
                    Button {
                        id: sidebarToggleOpen
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 30
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Hide sidebar")
                        onClicked: homeChat.conversationSidebarOpen = false
                        contentItem: Text {
                            text: "\u2715"
                            color: SentinelTheme.textMuted
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: SentinelTheme.radiusMd
                            color: InteractionTokens.surfaceColor(sidebarToggleOpen.hovered, sidebarToggleOpen.down, false, homeChat.modeAccent)
                            border.color: InteractionTokens.borderColor(sidebarToggleOpen.activeFocus, sidebarToggleOpen.hovered, false, homeChat.modeAccent)
                        }
                    }
                }

                // Section tabs: Recent | Pinned | Archived
                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceXs

                    Button {
                        id: recentTabBtn
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Recent chats")
                        onClicked: homeChat.sidebarView = "recent"
                        contentItem: Text {
                            text: qsTr("Recent")
                            color: homeChat.sidebarView === "recent" ? homeChat.modeAccent : SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontTiny
                            font.bold: homeChat.sidebarView === "recent"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: SentinelTheme.radiusSm
                            color: homeChat.sidebarView === "recent"
                                   ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.12)
                                   : (recentTabBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06) : "transparent")
                            border.color: homeChat.sidebarView === "recent"
                                          ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.28)
                                          : "transparent"
                        }
                    }

                    Button {
                        id: pinnedTabBtn
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 26
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Pinned chats")
                        onClicked: homeChat.sidebarView = (homeChat.sidebarView === "pinned" ? "recent" : "pinned")
                        contentItem: Text {
                            text: "\uD83D\uDCCC"
                            font.pixelSize: 12
                            color: homeChat.sidebarView === "pinned" ? homeChat.modeAccent : SentinelTheme.textMuted
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: SentinelTheme.radiusSm
                            color: homeChat.sidebarView === "pinned"
                                   ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.12)
                                   : (pinnedTabBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06) : "transparent")
                            border.color: homeChat.sidebarView === "pinned"
                                          ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.28)
                                          : "transparent"
                        }
                    }

                    Button {
                        id: archivedTabBtn
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 26
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Archived chats")
                        onClicked: homeChat.sidebarView = (homeChat.sidebarView === "archived" ? "recent" : "archived")
                        contentItem: Text {
                            text: "\uD83D\uDDC4"
                            font.pixelSize: 12
                            color: homeChat.sidebarView === "archived" ? homeChat.modeAccent : SentinelTheme.textMuted
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: SentinelTheme.radiusSm
                            color: homeChat.sidebarView === "archived"
                                   ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.12)
                                   : (archivedTabBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06) : "transparent")
                            border.color: homeChat.sidebarView === "archived"
                                          ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.28)
                                          : "transparent"
                        }
                    }
                }

                // Unified conversation list (Recent / Pinned / Archived views)
                ListView {
                    id: conversationList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 100
                    clip: true
                    spacing: SentinelTheme.spaceXs
                    // Re-evaluate model when view changes or conversation data changes
                    model: homeChat.filteredConversationIndexes(homeChat.sidebarView)

                    // Force refresh when sidebarView tab changes
                    onVisibleChanged: {
                        if (visible)
                            model = homeChat.filteredConversationIndexes(homeChat.sidebarView)
                    }

                    // Force refresh when underlying data changes
                    Connections {
                        target: homeChat.viewModel
                        function onConversationIdsChanged() { conversationList.model = homeChat.filteredConversationIndexes(homeChat.sidebarView) }
                        function onConversationTitlesChanged() { conversationList.model = homeChat.filteredConversationIndexes(homeChat.sidebarView) }
                        function onConversationPinnedSummariesChanged() { conversationList.model = homeChat.filteredConversationIndexes(homeChat.sidebarView) }
                        function onConversationArchivedSummariesChanged() { conversationList.model = homeChat.filteredConversationIndexes(homeChat.sidebarView) }
                    }

                    delegate: Item {
                        id: convItem
                        required property int modelData   // this is the sourceIndex into conversationIds
                        readonly property string convId: homeChat.viewModel.conversationIds[modelData]
                        readonly property string convTitle: homeChat.viewModel.conversationTitles[modelData]
                        readonly property bool active: convId === homeChat.viewModel.activeConversationId
                        readonly property bool pinned: homeChat.isPinned(modelData)
                        readonly property bool archived: homeChat.isArchived(modelData)
                        width: ListView.view.width
                        height: 38

                        // Background button for selecting conversation (left side)
                        Button {
                            id: convItemBtn
                            anchors.left: parent.left
                            anchors.right: convItemMenuBtn.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.rightMargin: 2
                            hoverEnabled: true
                            onClicked: {
                                homeChat.viewModel.switchConversation(convItem.convId)
                                homeChat.inChatMode = true
                                if (homeChat.compact)
                                    homeChat.conversationSidebarOpen = false
                                homeChat.scrollToLatest(true)
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusMd
                                color: InteractionTokens.surfaceColor(convItemBtn.hovered, convItemBtn.down, convItem.active, homeChat.modeAccent)
                                border.color: InteractionTokens.borderColor(convItemBtn.activeFocus, convItemBtn.hovered, convItem.active, homeChat.modeAccent)
                            }

                            contentItem: RowLayout {
                                spacing: 4
                                anchors.fill: parent
                                anchors.leftMargin: SentinelTheme.spaceSm
                                anchors.rightMargin: SentinelTheme.spaceXs

                                Text {
                                    Layout.fillWidth: true
                                    text: convItem.convTitle
                                    color: convItem.active ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    font.bold: convItem.active
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                }

                                Text {
                                    visible: convItem.pinned
                                    text: "📌"
                                    font.pixelSize: 10
                                    color: homeChat.modeAccent
                                }
                            }
                        }

                        // Overflow 3-dots Menu button (right side, on top with z: 2)
                        Button {
                            id: convItemMenuBtn
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.rightMargin: 4
                            width: 28
                            height: 28
                            z: 2
                            hoverEnabled: true
                            opacity: (convItemBtn.hovered || convItemMenuBtn.hovered || convItemMenu.opened || convItem.active) ? 1.0 : 0.45
                            onClicked: convItemMenu.popup()

                            contentItem: Text {
                                text: "⋮"
                                color: convItemMenuBtn.hovered ? homeChat.modeAccent : SentinelTheme.textPrimary
                                font.pixelSize: 14
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                radius: SentinelTheme.radiusSm
                                color: convItemMenuBtn.hovered ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.20) : "transparent"
                                border.color: convItemMenuBtn.hovered ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.3) : "transparent"
                                border.width: 1
                            }
                        }

                        Menu {
                            id: convItemMenu
                            width: 180

                            background: Rectangle {
                                radius: SentinelTheme.radiusMd
                                color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.94)
                                border.color: SentinelTheme.withAlpha(homeChat.modeAccent, 0.25)
                                border.width: 1
                            }

                            // Pin / Unpin
                            MenuItem {
                                text: convItem.pinned ? "📍  " + qsTr("Unpin") : "📌  " + qsTr("Pin")
                                onTriggered: {
                                    if (convItem.pinned)
                                        homeChat.viewModel.unpinConversation(convItem.convId)
                                    else
                                        homeChat.viewModel.pinConversation(convItem.convId)
                                }
                            }

                            // Archive / Unarchive
                            MenuItem {
                                text: convItem.archived ? "📥  " + qsTr("Unarchive") : "📦  " + qsTr("Archive")
                                onTriggered: {
                                    if (convItem.archived)
                                        homeChat.viewModel.unarchiveConversation(convItem.convId)
                                    else
                                        homeChat.viewModel.archiveConversation(convItem.convId)
                                }
                            }

                            // Delete — opens confirmation dialog
                            MenuSeparator {}

                            MenuItem {
                                text: "🗑️  " + qsTr("Delete")
                                onTriggered: {
                                    homeChat.pendingDeleteConversationId = convItem.convId
                                    homeChat.pendingDeleteConversationTitle = convItem.convTitle
                                    deleteConfirmDialog.open()
                                }
                            }
                        }
                    }

                    // Empty state
                    Label {
                        anchors.centerIn: parent
                        visible: conversationList.count === 0
                        text: homeChat.sidebarView === "pinned" ? qsTr("No pinned chats")
                              : homeChat.sidebarView === "archived" ? qsTr("No archived chats")
                              : qsTr("No chats yet")
                        color: SentinelTheme.textPlaceholder
                        font.pixelSize: SentinelTheme.fontSmall
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            spacing: homeChat.inChatMode ? SentinelTheme.spaceMd : SentinelTheme.spaceMd * homeChat.resolutionScale

            Item {
                id: homeCenterWrapper
                Layout.fillWidth: true
                Layout.fillHeight: !homeChat.inChatMode
                visible: !homeChat.inChatMode

                ColumnLayout {
                    anchors.centerIn: parent
                    width: Math.min(parent.width, 720 * homeChat.resolutionScale)
                    spacing: SentinelTheme.spaceMd * 1.5 * homeChat.resolutionScale

                    Item {
                        id: greetingAreaWrapper
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter
                        implicitHeight: greetingArea.implicitHeight
                        
                        scale: greetingMouse.containsMouse ? 1.012 : 1.0
                        Behavior on scale {
                            NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
                        }

                        MouseArea {
                            id: greetingMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                homeChat.showSuggestions = !homeChat.showSuggestions;
                                if (homeChat.showSuggestions) {
                                    suggestionGrid.shuffleSuggestions();
                                }
                            }
                        }

                        ColumnLayout {
                            id: greetingArea
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            spacing: SentinelTheme.spaceMd * homeChat.resolutionScale

                            Label {
                                id: greetingLabel
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignHCenter
                                horizontalAlignment: Text.AlignHCenter
                                text: homeChat.activeGreeting.title
                                color: SentinelTheme.textPrimary
                                font.pixelSize: (homeChat.compact ? SentinelTheme.fontDisplay : SentinelTheme.fontHero) * homeChat.resolutionScale
                                font.bold: true
                                font.family: "Outfit"
                            }

                            Label {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignHCenter
                                horizontalAlignment: Text.AlignHCenter
                                text: homeChat.activeGreeting.subtitle
                                color: SentinelTheme.textMuted
                                font.pixelSize: (homeChat.compact ? SentinelTheme.fontBody : SentinelTheme.fontCard) * homeChat.resolutionScale
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    Item {
                        id: suggestionsWrapper
                        Layout.fillWidth: true
                        implicitHeight: homeChat.showSuggestions ? suggestionGrid.implicitHeight : 0
                        visible: opacity > 0.0
                        opacity: homeChat.showSuggestions ? 1.0 : 0.0
                        clip: true
                        Layout.bottomMargin: homeChat.showSuggestions ? (SentinelTheme.spaceSm * homeChat.resolutionScale) : 0

                        Behavior on implicitHeight {
                            NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
                        }
                        Behavior on opacity {
                            NumberAnimation { duration: 250; easing.type: Easing.InOutQuad }
                        }
                        Behavior on Layout.bottomMargin {
                            NumberAnimation { duration: 300 }
                        }

                        GridLayout {
                            id: suggestionGrid
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            columns: homeChat.compact ? 1 : 2
                            rowSpacing: SentinelTheme.spaceSm * homeChat.resolutionScale
                            columnSpacing: SentinelTheme.spaceSm * homeChat.resolutionScale

                            property var suggestions: []

                            readonly property var allSuggestions: [
                                { icon: "📚", title: qsTr("Özet Çıkarma"), prompt: qsTr("Eklediğim PDF belgesinin yönetici özetini ve ana hatlarını çıkar") },
                                { icon: "📊", title: qsTr("Excel Formül Asistanı"), prompt: qsTr("İki farklı sütundaki verileri karşılaştırıp eşleşmeyenleri bulan bir Excel formülü yaz") },
                                { icon: "📝", title: qsTr("İş E-postası"), prompt: qsTr("Müşteriye veya yöneticiye durumu açıklayan kibar ve profesyonel bir e-posta taslağı yaz") },
                                { icon: "💡", title: qsTr("İçerik Fikirleri"), prompt: qsTr("Sosyal medya veya blog için ilgi çekici 5 içerik fikri ve başlığı bul") },
                                { icon: "🧠", title: qsTr("Basitçe Açıkla"), prompt: qsTr("Karmaşık bir kavramı veya teoriyi 10 yaşındaki birine anlatır gibi sade ve anlaşılır biçimde açıkla") },
                                { icon: "🌐", title: qsTr("Dil Öğrenimi"), prompt: qsTr("Verilen bir metni İngilizceye doğal ve akıcı bir şekilde çevir ve kullanılan önemli kalıpları göster") },
                                { icon: "📅", title: qsTr("Haftalık Yemek Planı"), prompt: qsTr("Sağlıklı, pratik ve bütçe dostu 5 günlük akşam yemeği menüsü ve alışveriş listesi oluştur") },
                                { icon: "✈️", title: qsTr("Seyahat Rotası"), prompt: qsTr("3 günlük bir şehir gezisi (örneğin Kapadokya veya Roma) için ayrıntılı ve optimize edilmiş bir rota planla") },
                                { icon: "🎁", title: qsTr("Hediye Önerileri"), prompt: qsTr("Belirli bir bütçeye ve ilgi alanlarına göre arkadaşım/ailem için 5 özgün hediye önerisi sun") },
                                { icon: "⏱️", title: qsTr("Zaman Yönetimi"), prompt: qsTr("Günün yoğun temposunu yönetmek ve ertelemeyi önlemek için Pomodoro tabanlı bir günlük plan hazırlamamda yardımcı ol") },
                                { icon: "📋", title: qsTr("Toplantı Tutanakları"), prompt: qsTr("Toplantıda karışık olarak aldığım notları maddeler halinde düzenli bir toplantı özetine dönüştür") },
                                { icon: "🛠️", title: qsTr("Temel Otomasyon"), prompt: qsTr("Bilgisayardaki dosya adlarını topluca değiştiren veya düzenleyen basit bir betik veya komut satırı kodu yaz") },
                                { icon: "🔍", title: qsTr("Metin Düzenleme"), prompt: qsTr("Yazdığım bu metni dil bilgisi, imla ve anlatım bozuklukları açısından inceleyip daha profesyonel hale getir") },
                                { icon: "🎯", title: qsTr("Hedef Planlama"), prompt: qsTr("Kişisel hedeflerimi veya bir projenin çeyreklik hedeflerini (OKR) belirlemek için bir çerçeve öner") },
                                { icon: "📈", title: qsTr("Veri Analizi"), prompt: qsTr("Bir tablo veya veri setindeki önemli eğilimleri, desenleri ve anomalileri özetleyen bir rapor taslağı hazırla") },
                                { icon: "🤝", title: qsTr("Diplomatik Yanıt"), prompt: qsTr("Müşteri veya iş ortağından gelen beklenmedik bir talebe ya da gecikmeye karşı profesyonel ve yapıcı bir yanıt yaz") },
                                { icon: "📓", title: qsTr("Yaratıcı Yazarlık"), prompt: qsTr("Belirli bir tema etrafında ilgi çekici bir kısa hikaye başlangıcı veya yaratıcı yazı taslağı oluştur") },
                                { icon: "📣", title: qsTr("Bülten Hazırlama"), prompt: qsTr("Aylık güncellemelerimizi veya ürün lansmanımızı duyuran ilgi çekici bir e-bülten taslağı yaz") },
                                { icon: "💻", title: qsTr("Python CSV İşleme"), prompt: qsTr("İki CSV dosyasını birleştiren ve filtreleyen basit bir Python betiği yaz") },
                                { icon: "📐", title: qsTr("Sunum Taslağı"), prompt: qsTr("Belirli bir konuda etkileyici ve akıcı bir sunum slayt yapısı ve konuşma notları tasarla") },
                                { icon: "⚡", title: qsTr("Klavye Kısayolları"), prompt: qsTr("İşletim sisteminde veya sık kullanılan bir uygulamada iş akışını hızlandıracak en pratik kısayolları listele") },
                                { icon: "🏫", title: qsTr("Öğrenme Planı"), prompt: qsTr("Yeni bir konuyu sıfırdan öğrenmek için (örneğin temel finans veya temel fotoğrafçılık) 4 haftalık adım adım çalışma planı hazırla") },
                                { icon: "🩺", title: qsTr("Sağlıklı Yaşam"), prompt: qsTr("Masa başında çalışanlar için gün içinde yapılabilecek esneme egzersizleri ve duruş düzeltme önerileri listele") },
                                { icon: "✍️", title: qsTr("Blog Yazısı Taslağı"), prompt: qsTr("Belirli bir konuda SEO uyumlu, alt başlıkları ve giriş paragrafı hazır olan detaylı bir blog yazısı şablonu oluştur") }
                            ]

                            function shuffleSuggestions() {
                                var temp = allSuggestions.slice();
                                var result = [];
                                for (var i = 0; i < 4; i++) {
                                    if (temp.length === 0) break;
                                    var randIdx = Math.floor(Math.random() * temp.length);
                                    result.push(temp[randIdx]);
                                    temp.splice(randIdx, 1);
                                }
                                suggestions = result;
                            }

                            Component.onCompleted: {
                                shuffleSuggestions();
                            }

                            Repeater {
                                model: suggestionGrid.suggestions
                                delegate: Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 62 * homeChat.resolutionScale
                                    radius: SentinelTheme.radiusMd
                                    color: sugMouse.containsMouse
                                           ? SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.88)
                                           : SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.48)
                                    border.color: sugMouse.containsMouse
                                                  ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.28)
                                                  : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.06)
                                    border.width: 1

                                    Behavior on color { ColorAnimation { duration: 120 } }
                                    Behavior on border.color { ColorAnimation { duration: 120 } }

                                    MouseArea {
                                        id: sugMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            homePromptInput.text = modelData.prompt
                                            homePromptInput.forceActiveFocus()
                                        }
                                    }

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: SentinelTheme.spaceSm
                                        spacing: SentinelTheme.spaceSm

                                        Text {
                                            text: modelData.icon
                                            font.pixelSize: 22 * homeChat.resolutionScale
                                            Layout.alignment: Qt.AlignVCenter
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2
                                            Layout.alignment: Qt.AlignVCenter

                                            Label {
                                                text: modelData.title
                                                font.pixelSize: SentinelTheme.fontSmall
                                                font.bold: true
                                                color: SentinelTheme.textPrimary
                                            }

                                            Label {
                                                text: modelData.prompt
                                                font.pixelSize: SentinelTheme.fontTiny
                                                color: SentinelTheme.textMuted
                                                elide: Text.ElideRight
                                                maximumLineCount: 1
                                                Layout.fillWidth: true
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        id: homeComposer
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter
                        radius: SentinelTheme.radiusMd * homeChat.resolutionScale
                        color: homePromptInput.activeFocus
                               ? SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.82)
                               : SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.68)
                        border.color: InteractionTokens.borderColor(homePromptInput.activeFocus, homeComposerMouse.containsMouse,
                                                                     false, homeChat.modeAccent)
                        implicitHeight: Math.max(76 * homeChat.resolutionScale, homeComposerMainLayout.implicitHeight + SentinelTheme.spaceSm * homeChat.resolutionScale)

                        MouseArea {
                            id: homeComposerMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }

                        DropArea {
                            anchors.fill: parent
                            onDropped: function(drop) {
                                if (drop.hasUrls && drop.urls.length > 0)
                                    homeChat.viewModel.attachFileToChat(drop.urls[0].toString().replace("file://", ""))
                            }
                        }

                        ColumnLayout {
                            id: homeComposerMainLayout
                            anchors.fill: parent
                            anchors.margins: SentinelTheme.spaceSm * homeChat.resolutionScale
                            spacing: SentinelTheme.spaceXs * homeChat.resolutionScale

                            // Attachment Preview Bar
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28 * homeChat.resolutionScale
                                radius: SentinelTheme.radiusSm
                                color: SentinelTheme.withAlpha(homeChat.modeAccent, 0.1)
                                border.color: SentinelTheme.withAlpha(homeChat.modeAccent, 0.2)
                                border.width: 1
                                visible: homeChat.viewModel.attachmentSummaries.length > 0

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: SentinelTheme.spaceSm
                                    anchors.rightMargin: SentinelTheme.spaceSm
                                    spacing: SentinelTheme.spaceSm

                                    Text {
                                        text: (homeChat.viewModel.attachmentSummaries.length > 0)
                                              ? (homeChat.getAttachmentIcon(homeChat.viewModel.attachmentSummaries[0]) + " " + homeChat.formatAttachmentSummary(homeChat.viewModel.attachmentSummaries[0]))
                                              : ""
                                        color: SentinelTheme.textPrimary
                                        font.pixelSize: SentinelTheme.fontTiny * homeChat.resolutionScale
                                        elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }

                                    Button {
                                        id: homeRemoveAttachmentBtn
                                        Layout.preferredWidth: 20 * homeChat.resolutionScale
                                        Layout.preferredHeight: 20 * homeChat.resolutionScale
                                        flat: true
                                        hoverEnabled: true
                                        onClicked: homeChat.viewModel.clearAttachments()
                                        background: Rectangle { color: "transparent" }
                                        contentItem: Text {
                                            text: "×"
                                            color: homeRemoveAttachmentBtn.hovered ? SentinelTheme.error : SentinelTheme.textMuted
                                            font.pixelSize: SentinelTheme.fontBody
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                id: homeComposerLayout
                                Layout.fillWidth: true
                                spacing: SentinelTheme.spaceSm * homeChat.resolutionScale

                            Button {
                                id: homeAttachButton
                                Layout.preferredWidth: 34 * homeChat.resolutionScale
                                Layout.preferredHeight: 34 * homeChat.resolutionScale
                                text: "+"
                                hoverEnabled: true
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Add or Actions")
                                onClicked: homeAttachMenu.open()

                                contentItem: Text {
                                    text: homeAttachButton.text
                                    color: SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontControl * homeChat.resolutionScale
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                background: Rectangle {
                                    radius: 17 * homeChat.resolutionScale
                                    color: InteractionTokens.surfaceColor(homeAttachButton.hovered, homeAttachButton.down,
                                                                           homeAttachMenu.opened,
                                                                           homeChat.modeAccent)
                                    border.color: InteractionTokens.borderColor(homeAttachButton.activeFocus,
                                                                                 homeAttachButton.hovered,
                                                                                 homeAttachMenu.opened,
                                                                                 homeChat.modeAccent)
                                }

                                Popup {
                                    id: homeAttachMenu
                                    y: -height - 8
                                    x: 0
                                    width: 140 * homeChat.resolutionScale
                                    height: 70 * homeChat.resolutionScale
                                    padding: 6 * homeChat.resolutionScale

                                    background: Rectangle {
                                        radius: SentinelTheme.radiusMd
                                        color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                                        border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                        border.width: 1
                                    }

                                    ColumnLayout {
                                        anchors.fill: parent
                                        spacing: 4 * homeChat.resolutionScale

                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 28 * homeChat.resolutionScale
                                            color: homeImgMouse.containsMouse ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.08) : "transparent"
                                            radius: SentinelTheme.radiusSm

                                            MouseArea {
                                                id: homeImgMouse
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                onClicked: {
                                                    homeAttachMenu.close()
                                                    imageFileDialog.open()
                                                }
                                            }

                                            Text {
                                                anchors.fill: parent
                                                anchors.leftMargin: SentinelTheme.spaceSm
                                                text: "🖼️  " + qsTr("Upload Image")
                                                color: homeImgMouse.containsMouse ? homeChat.modeAccent : SentinelTheme.textPrimary
                                                font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 28 * homeChat.resolutionScale
                                            color: homeDocMouse.containsMouse ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.08) : "transparent"
                                            radius: SentinelTheme.radiusSm

                                            MouseArea {
                                                id: homeDocMouse
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                onClicked: {
                                                    homeAttachMenu.close()
                                                    documentFileDialog.open()
                                                }
                                            }

                                            Text {
                                                anchors.fill: parent
                                                anchors.leftMargin: SentinelTheme.spaceSm
                                                text: "📄  " + qsTr("Upload File")
                                                color: homeDocMouse.containsMouse ? homeChat.modeAccent : SentinelTheme.textPrimary
                                                font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                        }
                                    }
                                }
                            }

                            TextArea {
                                id: homePromptInput
                                Layout.fillWidth: true
                                Layout.minimumHeight: 48 * homeChat.resolutionScale
                                Layout.maximumHeight: 126 * homeChat.resolutionScale
                                placeholderText: homeChat.chatReady ? (homeChat.sendBusy ? qsTr("Sentinel is responding") : qsTr("Ask Sentinel"))
                                                                    : homeChat.viewModel.localChatSendAvailabilitySummary
                                enabled: !homeChat.viewModel.activeConversationArchived && !homeChat.sendBusy
                                color: SentinelTheme.textPrimary
                                placeholderTextColor: SentinelTheme.textPlaceholder
                                wrapMode: TextEdit.WordWrap
                                selectByMouse: true
                                selectionColor: SentinelTheme.withAlpha(homeChat.modeAccent, 0.34)
                                selectedTextColor: SentinelTheme.textPrimary
                                font.pixelSize: (homeChat.compact ? SentinelTheme.fontBody : SentinelTheme.fontControl) * homeChat.resolutionScale
                                onTextChanged: {
                                    homeChat.viewModel.recoveryDraftText = text
                                }
                                background: Rectangle {
                                    color: "transparent"
                                }
                                Keys.onPressed: function(event) {
                                    if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                                            && !(event.modifiers & Qt.ShiftModifier)) {
                                        promptInput.text = homePromptInput.text
                                        homePromptInput.clear()
                                        homeChat.sendComposerText()
                                        event.accepted = true
                                    } else if (event.key === Qt.Key_Escape) {
                                        focus = false
                                        event.accepted = true
                                    }
                                }
                            }

                            Button {
                                id: homeMicButton
                                Layout.preferredWidth: 34 * homeChat.resolutionScale
                                Layout.preferredHeight: 34 * homeChat.resolutionScale
                                Layout.alignment: Qt.AlignBottom
                                hoverEnabled: true
                                ToolTip.visible: hovered
                                ToolTip.text: recordingActive ? qsTr("Stop Recording") : qsTr("Voice Input")

                                property bool recordingActive: homeChat.viewModel.voiceRecordingActive

                                onClicked: {
                                    if (recordingActive) {
                                        homeChat.viewModel.stopVoiceCapture()
                                    } else {
                                        homeChat.viewModel.startVoiceCapture()
                                    }
                                }

                                contentItem: Text {
                                    text: homeMicButton.recordingActive ? "🔴" : "🎤"
                                    font.pixelSize: 16 * homeChat.resolutionScale
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                background: Rectangle {
                                    radius: 17 * homeChat.resolutionScale
                                    color: homeMicButton.recordingActive
                                           ? SentinelTheme.withAlpha(SentinelTheme.warning, 0.15)
                                           : InteractionTokens.surfaceColor(homeMicButton.hovered, homeMicButton.down,
                                                                                homeMicButton.activeFocus,
                                                                                homeChat.modeAccent)
                                    border.color: homeMicButton.recordingActive
                                                  ? SentinelTheme.warning
                                                  : InteractionTokens.borderColor(homeMicButton.activeFocus,
                                                                                     homeMicButton.hovered,
                                                                                     false,
                                                                                     homeChat.modeAccent)

                                    SequentialAnimation on opacity {
                                        running: homeMicButton.recordingActive
                                        loops: Animation.Infinite
                                        NumberAnimation { from: 1.0; to: 0.5; duration: 800; easing.type: Easing.InOutQuad }
                                        NumberAnimation { from: 0.5; to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                                    }
                                }
                            }

                            SentinelButton {
                                id: homeSendButton
                                text: homeChat.sendBusy ? qsTr("Stop") : qsTr("Send")
                                Layout.preferredWidth: 82 * homeChat.resolutionScale
                                Layout.alignment: Qt.AlignBottom
                                font.pixelSize: SentinelTheme.fontControl * homeChat.resolutionScale
                                enabled: homeChat.sendBusy || (homePromptInput.text.trim().length > 0
                                                               && homeChat.canSend)
                                opacity: enabled ? 1.0 : 0.58
                                onClicked: {
                                    if (homeChat.sendBusy) {
                                        homeChat.viewModel.cancelLocalInference()
                                    } else {
                                        promptInput.text = homePromptInput.text
                                        homePromptInput.clear()
                                        homeChat.sendComposerText()
                                    }
                                }
                            }
                        }
                    }
                }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter
                        spacing: SentinelTheme.spaceSm * homeChat.resolutionScale

                        Label {
                            text: qsTr("Mode")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                        }

                        ComboBox {
                            id: homeModeSelector
                            Layout.preferredWidth: Math.min(140, 110 * homeChat.resolutionScale)
                            Layout.preferredHeight: 32 * homeChat.resolutionScale
                            model: homeChat.viewModel.availableModes
                            currentIndex: homeChat.viewModel.availableModes.indexOf(homeChat.viewModel.currentModeName)
                            onActivated: function(index) {
                                if (index >= 0 && index < homeChat.viewModel.availableModes.length)
                                    homeChat.viewModel.currentModeName = homeChat.viewModel.availableModes[index]
                            }
                            displayText: currentIndex >= 0 ? homeChat.viewModel.availableModes[currentIndex] : qsTr("Mode")

                            contentItem: Text {
                                text: homeModeSelector.displayText
                                color: homeModeSelector.currentIndex >= 0 ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: SentinelTheme.spaceSm
                                rightPadding: SentinelTheme.spaceSm
                                elide: Text.ElideRight
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusMd * homeChat.resolutionScale
                                color: homeModeSelector.hovered
                                       ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.10)
                                       : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.48)
                                border.color: homeModeSelector.activeFocus
                                              ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.62)
                                              : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                            }

                            popup: Popup {
                                id: homeModePopup
                                y: homeModeSelector.height + 4
                                width: homeModeSelector.width
                                implicitHeight: Math.min(contentItem.implicitHeight + 2 * padding, 280)
                                padding: SentinelTheme.spaceXs

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd * homeChat.resolutionScale
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.96)
                                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                }

                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: contentHeight
                                    model: homeModeSelector.popup.visible ? homeModeSelector.delegateModel : null
                                    currentIndex: homeModeSelector.highlightedIndex
                                    ScrollBar.vertical: ScrollBar {
                                        policy: ScrollBar.AsNeeded
                                    }
                                }
                            }

                            delegate: ItemDelegate {
                                width: homeModeSelector.width - SentinelTheme.spaceXs * 2
                                height: 32 * homeChat.resolutionScale
                                highlighted: homeModeSelector.highlightedIndex === index
                                hoverEnabled: true

                                contentItem: Text {
                                    text: modelData
                                    color: highlighted ? homeChat.modeAccent : SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                    font.bold: homeChat.viewModel.availableModes[index] === homeChat.viewModel.currentModeName
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                    leftPadding: SentinelTheme.spaceSm
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusSm
                                    color: highlighted || hovered
                                           ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.10)
                                           : "transparent"
                                }
                            }
                        }

                        Label {
                            text: qsTr("Provider")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                        }

                        ComboBox {
                            id: homeProviderSelector
                            Layout.preferredWidth: Math.min(200, 160 * homeChat.resolutionScale)
                            Layout.preferredHeight: 32 * homeChat.resolutionScale
                            model: homeChat.viewModel.selectableRuntimeProviderLabels
                            currentIndex: homeChat.viewModel.selectableRuntimeProviderIds.indexOf(homeChat.viewModel.selectedRuntimeProvider)
                            onActivated: function(index) {
                                if (index >= 0 && index < homeChat.viewModel.selectableRuntimeProviderIds.length)
                                    homeChat.viewModel.selectedRuntimeProvider = homeChat.viewModel.selectableRuntimeProviderIds[index]
                            }
                            displayText: currentIndex >= 0 ? homeChat.viewModel.selectableRuntimeProviderLabels[currentIndex] : homeChat.viewModel.activeRuntimeProviderLabel

                            contentItem: Text {
                                text: homeProviderSelector.displayText
                                color: homeProviderSelector.currentIndex >= 0 ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: SentinelTheme.spaceSm
                                rightPadding: SentinelTheme.spaceSm
                                elide: Text.ElideRight
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusMd * homeChat.resolutionScale
                                color: homeProviderSelector.hovered
                                       ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.10)
                                       : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.48)
                                border.color: homeProviderSelector.activeFocus
                                              ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.62)
                                              : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                            }

                            popup: Popup {
                                y: homeProviderSelector.height + 4
                                width: homeProviderSelector.width
                                implicitHeight: Math.min(contentItem.implicitHeight + 2 * padding, 280)
                                padding: SentinelTheme.spaceXs

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.96)
                                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                }

                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: contentHeight
                                    model: homeProviderSelector.delegateModel
                                    currentIndex: homeProviderSelector.highlightedIndex
                                    ScrollBar.vertical: ScrollBar {
                                        policy: ScrollBar.AsNeeded
                                    }
                                }
                            }

                            delegate: ItemDelegate {
                                width: homeProviderSelector.width - SentinelTheme.spaceXs * 2
                                height: 32 * homeChat.resolutionScale
                                highlighted: homeProviderSelector.highlightedIndex === index
                                hoverEnabled: true

                                contentItem: Text {
                                    text: modelData
                                    color: highlighted ? homeChat.modeAccent : SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                    font.bold: homeChat.viewModel.selectableRuntimeProviderIds[index] === homeChat.viewModel.selectedRuntimeProvider
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                    leftPadding: SentinelTheme.spaceSm
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusSm
                                    color: highlighted || hovered
                                           ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.10)
                                           : "transparent"
                                }
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 1
                            Layout.preferredHeight: 20 * homeChat.resolutionScale
                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                        }

                        Label {
                            text: qsTr("Model")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                        }

                        ComboBox {
                            id: homeModelSelector
                            Layout.preferredWidth: Math.min(280, 220 * homeChat.resolutionScale)
                            Layout.preferredHeight: 32 * homeChat.resolutionScale
                            model: homeChat.viewModel.ollamaModelNames
                            currentIndex: {
                                var names = homeChat.viewModel.ollamaModelNames
                                var selected = homeChat.viewModel.selectedLocalModel
                                for (var i = 0; i < names.length; ++i) {
                                    if (names[i] === selected) return i
                                }
                                return -1
                            }
                            onActivated: function(index) {
                                if (index >= 0 && index < homeChat.viewModel.ollamaModelNames.length)
                                    homeChat.viewModel.selectedLocalModel = homeChat.viewModel.ollamaModelNames[index]
                            }
                            displayText: currentIndex >= 0 ? homeChat.viewModel.ollamaModelNames[currentIndex] : qsTr("No model")

                            contentItem: Text {
                                text: homeModelSelector.displayText
                                color: homeModelSelector.currentIndex >= 0 ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: SentinelTheme.spaceSm
                                rightPadding: SentinelTheme.spaceSm
                                elide: Text.ElideRight
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusMd * homeChat.resolutionScale
                                color: homeModelSelector.hovered
                                       ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.10)
                                       : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.48)
                                border.color: homeModelSelector.activeFocus
                                              ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.62)
                                              : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                            }

                            popup: Popup {
                                y: homeModelSelector.height + 4
                                width: homeModelSelector.width
                                implicitHeight: Math.min(contentItem.implicitHeight + 2 * padding, 280)
                                padding: SentinelTheme.spaceXs

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.96)
                                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                                }

                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: contentHeight
                                    model: homeModelSelector.delegateModel
                                    currentIndex: homeModelSelector.highlightedIndex
                                    ScrollBar.vertical: ScrollBar {
                                        policy: ScrollBar.AsNeeded
                                    }
                                }
                            }

                            delegate: ItemDelegate {
                                width: homeModelSelector.width - SentinelTheme.spaceXs * 2
                                height: 32 * homeChat.resolutionScale
                                highlighted: homeModelSelector.highlightedIndex === index
                                hoverEnabled: true

                                contentItem: Text {
                                    text: modelData + " (" + homeChat.localProviderLabel + ")"
                                    color: highlighted ? homeChat.modeAccent : SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                    font.bold: modelData === homeChat.viewModel.selectedLocalModel
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                    leftPadding: SentinelTheme.spaceSm
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusSm
                                    color: highlighted || hovered
                                           ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.10)
                                           : "transparent"
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: homeChat.inChatMode
                spacing: SentinelTheme.spaceSm

            ColumnLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceXs

                Label {
                    Layout.fillWidth: true
                    text: homeChat.viewModel.conversationListCurrentTitle
                    color: SentinelTheme.textPrimary
                    font.pixelSize: homeChat.compact ? SentinelTheme.fontTitle : SentinelTheme.fontTitle + 2
                    maximumLineCount: 1
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: homeChat.viewModel.conversationLastRestoredStatus
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    maximumLineCount: 1
                    elide: Text.ElideRight
                }
            }


        }

        ListView {
            id: recentMessages
            property bool followNewMessages: true
            function nearBottom() {
                return contentHeight <= height || (contentY + height >= contentHeight - 96)
            }
            Layout.fillWidth: true
            Layout.fillHeight: homeChat.inChatMode
            Layout.minimumHeight: homeChat.inChatMode ? 210 : 0
            visible: homeChat.inChatMode
            clip: true
            spacing: homeChat.compact ? SentinelTheme.spaceSm : SentinelTheme.spaceMd
            model: homeChat.viewModel.chatMessages
            boundsBehavior: Flickable.StopAtBounds
            boundsMovement: Flickable.StopAtBounds
            maximumFlickVelocity: 2200
            flickDeceleration: 5200
            bottomMargin: SentinelTheme.spaceXl + SentinelTheme.spaceMd
            ScrollBar.vertical: ScrollBar {
                id: recentMessagesScrollBar
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 4
                    radius: 2
                    color: SentinelTheme.withAlpha(homeChat.modeAccent, recentMessagesScrollBar.active ? 0.34 : 0.18)
                }
                background: Rectangle {
                    color: "transparent"
                }
            }
            onCountChanged: homeChat.scrollToLatest(false)
            Component.onCompleted: Qt.callLater(positionViewAtEnd)
            onMovementStarted: followNewMessages = nearBottom()
            onMovementEnded: followNewMessages = nearBottom()
            onFlickEnded: followNewMessages = nearBottom()

            add: Transition {
                NumberAnimation {
                    property: "opacity"
                    from: 0.0
                    to: 1.0
                    duration: MotionTokens.duration(MotionTokens.message, homeChat.viewModel.currentModeName)
                    easing.type: MotionTokens.enter
                }
                NumberAnimation {
                    property: "scale"
                    from: 0.985
                    to: 1.0
                    duration: MotionTokens.duration(MotionTokens.message, homeChat.viewModel.currentModeName)
                    easing.type: MotionTokens.enter
                }
            }

            delegate: Rectangle {
                id: recentMessage
                required property int index
                required property string messageRole
                required property string content
                readonly property bool displayable: messageRole !== "system"

                width: ListView.view.width
                height: displayable ? recentMessageColumn.implicitHeight + SentinelTheme.spaceMd : 0
                visible: displayable
                radius: SentinelTheme.radiusMd
                color: SentinelTheme.lightTheme
                       ? (messageRole === "user"
                          ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.16)
                          : SentinelTheme.withAlpha("#ffffff", 0.95))
                       : (messageRole === "user"
                          ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.105)
                          : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.026))
                border.color: SentinelTheme.withAlpha(messageRole === "user"
                                                      ? homeChat.modeAccent
                                                      : SentinelTheme.textPrimary,
                                                      messageRole === "user" ? 0.13 : 0.07)
                opacity: 1.0
                scale: 1.0

                Behavior on color {
                    ColorAnimation {
                        duration: MotionTokens.normal
                        easing.type: MotionTokens.standard
                    }
                }

                Rectangle {
                    width: 3
                    height: parent.height - SentinelTheme.spaceSm
                    radius: 2
                    anchors.left: parent.left
                    anchors.leftMargin: SentinelTheme.spaceXs
                    anchors.verticalCenter: parent.verticalCenter
                    visible: recentMessage.messageRole !== "user"
                    color: SentinelTheme.withAlpha(homeChat.modeAccent, 0.42)
                }

                ColumnLayout {
                    id: recentMessageColumn
                    x: SentinelTheme.spaceSm
                    y: SentinelTheme.spaceSm
                    width: parent.width - SentinelTheme.spaceSm * 2
                    spacing: SentinelTheme.spaceXs

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            Layout.fillWidth: true
                            text: recentMessage.messageRole === "user" ? qsTr("You") : qsTr("Sentinel")
                            color: recentMessage.messageRole === "user"
                                   ? homeChat.modeAccent
                                   : SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontTiny
                            elide: Text.ElideRight
                        }

                        Button {
                            id: copyButton
                            Layout.preferredWidth: 46
                            Layout.preferredHeight: 24
                            text: qsTr("Copy")
                            hoverEnabled: true
                            focusPolicy: Qt.StrongFocus
                            onClicked: {
                                messageBody.forceActiveFocus()
                                messageBody.selectAll()
                                messageBody.copy()
                                messageBody.deselect()
                            }

                            contentItem: Text {
                                text: copyButton.text
                                color: copyButton.enabled ? SentinelTheme.textMuted : SentinelTheme.textPlaceholder
                                font.pixelSize: SentinelTheme.fontTiny
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusSm
                                color: InteractionTokens.surfaceColor(copyButton.hovered, copyButton.down,
                                                                       copyButton.activeFocus,
                                                                       homeChat.modeAccent)
                                border.color: InteractionTokens.borderColor(copyButton.activeFocus,
                                                                             copyButton.hovered,
                                                                             false,
                                                                             homeChat.modeAccent)
                            }
                        }

                        Button {
                            id: messageMenuButton
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 24
                            text: "⋮"
                            hoverEnabled: true
                            focusPolicy: Qt.StrongFocus
                            onClicked: messageMenu.popup()

                            contentItem: Text {
                                text: messageMenuButton.text
                                color: SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontSmall
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: SentinelTheme.radiusSm
                                color: InteractionTokens.surfaceColor(messageMenuButton.hovered,
                                                                       messageMenuButton.down,
                                                                       messageMenuButton.activeFocus,
                                                                       homeChat.modeAccent)
                                border.color: InteractionTokens.borderColor(messageMenuButton.activeFocus,
                                                                             messageMenuButton.hovered,
                                                                             false,
                                                                             homeChat.modeAccent)
                            }

                            Menu {
                                id: messageMenu
                                width: 200

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.94)
                                    border.color: SentinelTheme.withAlpha(homeChat.modeAccent, 0.25)
                                    border.width: 1
                                }

                                MenuItem {
                                    text: qsTr("Edit")
                                    enabled: recentMessage.messageRole === "user"
                                    onTriggered: {
                                        promptInput.text = recentMessage.content
                                        promptInput.forceActiveFocus()
                                    }
                                }
                                MenuItem {
                                    text: qsTr("Regenerate")
                                    enabled: recentMessage.messageRole !== "user" && homeChat.canSend && !homeChat.sendBusy
                                    onTriggered: homeChat.viewModel.sendMessage(qsTr("Regenerate the previous response."))
                                }

                                MenuItem {
                                    text: qsTr("Copy Text")
                                    onTriggered: {
                                        messageBody.forceActiveFocus()
                                        messageBody.selectAll()
                                        messageBody.copy()
                                        messageBody.deselect()
                                    }
                                }

                                MenuSeparator {}

                                MenuItem {
                                    readonly property bool convPinned: {
                                        var idx = homeChat.viewModel.conversationIds.indexOf(homeChat.viewModel.activeConversationId)
                                        return idx >= 0 && homeChat.isPinned(idx)
                                    }
                                    text: convPinned ? qsTr("Unpin conversation") : qsTr("Pin conversation")
                                    onTriggered: {
                                        var idx = homeChat.viewModel.conversationIds.indexOf(homeChat.viewModel.activeConversationId)
                                        if (idx >= 0) {
                                            if (convPinned)
                                                homeChat.viewModel.unpinConversation(homeChat.viewModel.activeConversationId)
                                            else
                                                homeChat.viewModel.pinConversation(homeChat.viewModel.activeConversationId)
                                        }
                                    }
                                }
                                MenuItem {
                                    readonly property bool convArchived: {
                                        var idx = homeChat.viewModel.conversationIds.indexOf(homeChat.viewModel.activeConversationId)
                                        return idx >= 0 && homeChat.isArchived(idx)
                                    }
                                    text: convArchived ? qsTr("Unarchive conversation") : qsTr("Archive conversation")
                                    onTriggered: {
                                        if (convArchived)
                                            homeChat.viewModel.unarchiveConversation(homeChat.viewModel.activeConversationId)
                                        else
                                            homeChat.viewModel.archiveConversation(homeChat.viewModel.activeConversationId)
                                    }
                                }

                                MenuSeparator {}

                                MenuItem {
                                    text: qsTr("Export Markdown")
                                    onTriggered: homeChat.viewModel.exportTranscript("markdown")
                                }
                                MenuItem {
                                    text: qsTr("Export TXT")
                                    onTriggered: homeChat.viewModel.exportTranscript("txt")
                                }

                                MenuSeparator {}

                                MenuItem {
                                    text: qsTr("Delete conversation")
                                    onTriggered: {
                                        homeChat.pendingDeleteConversationId = homeChat.viewModel.activeConversationId
                                        var idx = homeChat.viewModel.conversationIds.indexOf(homeChat.viewModel.activeConversationId)
                                        homeChat.pendingDeleteConversationTitle = idx >= 0 ? homeChat.viewModel.conversationTitles[idx] : homeChat.viewModel.activeConversationId
                                        deleteConfirmDialog.open()
                                    }
                                }
                            }
                        }
                    }

                    TextEdit {
                        id: messageBody
                        Layout.fillWidth: true
                        text: recentMessage.content
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontSmall
                        wrapMode: Text.WordWrap
                        readOnly: true
                        selectByMouse: true
                        selectByKeyboard: true
                        textFormat: TextEdit.PlainText
                        selectionColor: SentinelTheme.withAlpha(homeChat.modeAccent, 0.34)
                        selectedTextColor: SentinelTheme.textPrimary
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            visible: homeChat.inChatMode
            Layout.maximumWidth: 9999
            Layout.alignment: Qt.AlignHCenter
            radius: SentinelTheme.radiusMd * homeChat.resolutionScale
            color: promptInput.activeFocus
                   ? SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.82)
                   : SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.68)
            border.color: InteractionTokens.borderColor(promptInput.activeFocus, composerMouse.containsMouse,
                                                         false, homeChat.modeAccent)
            implicitHeight: Math.max(76 * homeChat.resolutionScale, composerMainLayout.implicitHeight + SentinelTheme.spaceSm * homeChat.resolutionScale)

            MouseArea {
                id: composerMouse
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }

            DropArea {
                anchors.fill: parent
                onDropped: function(drop) {
                    if (drop.hasUrls && drop.urls.length > 0)
                        homeChat.viewModel.attachFileToChat(drop.urls[0].toString().replace("file://", ""))
                }
            }

            ColumnLayout {
                id: composerMainLayout
                anchors.fill: parent
                anchors.margins: SentinelTheme.spaceSm * homeChat.resolutionScale
                spacing: SentinelTheme.spaceXs * homeChat.resolutionScale

                // Attachment Preview Bar
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28 * homeChat.resolutionScale
                    radius: SentinelTheme.radiusSm
                    color: SentinelTheme.withAlpha(homeChat.modeAccent, 0.1)
                    border.color: SentinelTheme.withAlpha(homeChat.modeAccent, 0.2)
                    border.width: 1
                    visible: homeChat.viewModel.attachmentSummaries.length > 0

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: SentinelTheme.spaceSm
                        anchors.rightMargin: SentinelTheme.spaceSm
                        spacing: SentinelTheme.spaceSm

                        Text {
                            text: (homeChat.viewModel.attachmentSummaries.length > 0)
                                  ? (homeChat.getAttachmentIcon(homeChat.viewModel.attachmentSummaries[0]) + " " + homeChat.formatAttachmentSummary(homeChat.viewModel.attachmentSummaries[0]))
                                  : ""
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontTiny * homeChat.resolutionScale
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }

                        Button {
                            id: removeAttachmentBtn
                            Layout.preferredWidth: 20 * homeChat.resolutionScale
                            Layout.preferredHeight: 20 * homeChat.resolutionScale
                            flat: true
                            hoverEnabled: true
                            onClicked: homeChat.viewModel.clearAttachments()
                            background: Rectangle { color: "transparent" }
                            contentItem: Text {
                                text: "×"
                                color: removeAttachmentBtn.hovered ? SentinelTheme.error : SentinelTheme.textMuted
                                font.pixelSize: SentinelTheme.fontBody
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                RowLayout {
                    id: composerLayout
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceSm * homeChat.resolutionScale

                    Button {
                        id: attachButton
                    Layout.preferredWidth: 34 * homeChat.resolutionScale
                    Layout.preferredHeight: 34 * homeChat.resolutionScale
                    text: "+"
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Add or Actions")
                    onClicked: attachMenu.open()

                    contentItem: Text {
                        text: attachButton.text
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontControl * homeChat.resolutionScale
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 17 * homeChat.resolutionScale
                        color: InteractionTokens.surfaceColor(attachButton.hovered, attachButton.down,
                                                               attachMenu.opened,
                                                               homeChat.modeAccent)
                        border.color: InteractionTokens.borderColor(attachButton.activeFocus,
                                                                      attachButton.hovered,
                                                                      attachMenu.opened,
                                                                      homeChat.modeAccent)
                    }

                    Popup {
                        id: attachMenu
                        y: -height - 8
                        x: 0
                        width: 140 * homeChat.resolutionScale
                        height: 104 * homeChat.resolutionScale
                        padding: 6 * homeChat.resolutionScale

                        background: Rectangle {
                            radius: SentinelTheme.radiusMd
                            color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                            border.width: 1
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 4 * homeChat.resolutionScale

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28 * homeChat.resolutionScale
                                color: imgMouse.containsMouse ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.08) : "transparent"
                                radius: SentinelTheme.radiusSm

                                MouseArea {
                                    id: imgMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        attachMenu.close()
                                        imageFileDialog.open()
                                    }
                                }

                                Text {
                                    anchors.fill: parent
                                    anchors.leftMargin: SentinelTheme.spaceSm
                                    text: "🖼️  " + qsTr("Upload Image")
                                    color: imgMouse.containsMouse ? homeChat.modeAccent : SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28 * homeChat.resolutionScale
                                color: docMouse.containsMouse ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.08) : "transparent"
                                radius: SentinelTheme.radiusSm

                                MouseArea {
                                    id: docMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        attachMenu.close()
                                        documentFileDialog.open()
                                    }
                                }

                                Text {
                                    anchors.fill: parent
                                    anchors.leftMargin: SentinelTheme.spaceSm
                                    text: "📄  " + qsTr("Upload File")
                                    color: docMouse.containsMouse ? homeChat.modeAccent : SentinelTheme.textPrimary
                                    font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28 * homeChat.resolutionScale
                                visible: homeChat.inChatMode
                                color: sumMouse.containsMouse && sumMouse.enabled ? SentinelTheme.withAlpha(homeChat.modeAccent, 0.08) : "transparent"
                                radius: SentinelTheme.radiusSm
                                opacity: sumMouse.enabled ? 1.0 : 0.48

                                MouseArea {
                                    id: sumMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    enabled: homeChat.viewModel.localChatSendAvailable && !homeChat.sendBusy
                                             && homeChat.viewModel.conversationSummaryGenerationStatus !== "Planned"
                                    onClicked: {
                                        attachMenu.close()
                                        homeChat.viewModel.requestConversationSummaryGeneration()
                                    }
                                }

                                Text {
                                    anchors.fill: parent
                                    anchors.leftMargin: SentinelTheme.spaceSm
                                    text: "📝  " + qsTr("Özet Oluştur")
                                    color: !sumMouse.enabled ? SentinelTheme.textMuted
                                                             : (sumMouse.containsMouse ? homeChat.modeAccent : SentinelTheme.textPrimary)
                                    font.pixelSize: SentinelTheme.fontSmall * homeChat.resolutionScale
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }
                }

                TextArea {
                    id: promptInput
                    Layout.fillWidth: true
                    Layout.minimumHeight: 48 * homeChat.resolutionScale
                    Layout.maximumHeight: 126 * homeChat.resolutionScale
                    placeholderText: homeChat.chatReady ? (homeChat.sendBusy ? qsTr("Sentinel is responding") : qsTr("Ask Sentinel"))
                                                        : homeChat.viewModel.localChatSendAvailabilitySummary
                    enabled: !homeChat.viewModel.activeConversationArchived && !homeChat.sendBusy
                    color: SentinelTheme.textPrimary
                    placeholderTextColor: SentinelTheme.textPlaceholder
                    wrapMode: TextEdit.WordWrap
                    selectByMouse: true
                    selectionColor: SentinelTheme.withAlpha(homeChat.modeAccent, 0.34)
                    selectedTextColor: SentinelTheme.textPrimary
                    font.pixelSize: (homeChat.compact ? SentinelTheme.fontBody : SentinelTheme.fontControl) * homeChat.resolutionScale
                    onTextChanged: homeChat.viewModel.recoveryDraftText = text
                    background: Rectangle {
                        color: "transparent"
                    }
                    Keys.onPressed: function(event) {
                        if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                                && !(event.modifiers & Qt.ShiftModifier)) {
                            homeChat.sendComposerText()
                            event.accepted = true
                        } else if (event.key === Qt.Key_Escape) {
                            focus = false
                            event.accepted = true
                        }
                    }
                }

                Button {
                    id: micButton
                    Layout.preferredWidth: 34 * homeChat.resolutionScale
                    Layout.preferredHeight: 34 * homeChat.resolutionScale
                    Layout.alignment: Qt.AlignBottom
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: recordingActive ? qsTr("Stop Recording") : qsTr("Voice Input")

                    property bool recordingActive: homeChat.viewModel.voiceRecordingActive

                    onClicked: {
                        if (recordingActive) {
                            homeChat.viewModel.stopVoiceCapture()
                        } else {
                            homeChat.viewModel.startVoiceCapture()
                        }
                    }

                    contentItem: Text {
                        text: micButton.recordingActive ? "🔴" : "🎤"
                        font.pixelSize: 16 * homeChat.resolutionScale
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 17 * homeChat.resolutionScale
                        color: micButton.recordingActive
                               ? SentinelTheme.withAlpha(SentinelTheme.warning, 0.15)
                               : InteractionTokens.surfaceColor(micButton.hovered, micButton.down,
                                                                    micButton.activeFocus,
                                                                    homeChat.modeAccent)
                        border.color: micButton.recordingActive
                                      ? SentinelTheme.warning
                                      : InteractionTokens.borderColor(micButton.activeFocus,
                                                                         micButton.hovered,
                                                                         false,
                                                                         homeChat.modeAccent)

                        SequentialAnimation on opacity {
                            running: micButton.recordingActive
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 0.5; duration: 800; easing.type: Easing.InOutQuad }
                            NumberAnimation { from: 0.5; to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                        }
                    }
                }

                SentinelButton {
                    id: sendButton
                    visible: true
                    text: homeChat.sendBusy ? qsTr("Stop") : qsTr("Send")
                    Layout.preferredWidth: 82 * homeChat.resolutionScale
                    Layout.alignment: Qt.AlignBottom
                    font.pixelSize: SentinelTheme.fontControl * homeChat.resolutionScale
                    enabled: homeChat.sendBusy || (promptInput.text.trim().length > 0
                                                   && homeChat.canSend)
                    opacity: enabled ? 1.0 : 0.58
                    onClicked: {
                        if (homeChat.sendBusy)
                            homeChat.viewModel.cancelLocalInference()
                        else
                            homeChat.sendComposerText()
                    }
                }
            }
        }
    }

        Label {
            Layout.fillWidth: true
            visible: homeChat.inChatMode && !homeChat.chatReady
            text: homeChat.disabledReason + (homeChat.chatReady ? "" : qsTr(" Local %1 only. No cloud provider active.").arg(homeChat.localProviderLabel))
            color: !homeChat.chatReady ? SentinelTheme.textMuted : SentinelTheme.warning
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }
        }




    }

    FileDialog {
        id: imageFileDialog
        title: qsTr("Upload Image")
        fileMode: FileDialog.OpenFile
        nameFilters: [
            qsTr("Images (*.png *.jpg *.jpeg *.webp *.gif *.bmp)"),
            qsTr("All files (*)")
        ]
        onAccepted: homeChat.viewModel.attachFileToChat(selectedFile.toString().replace("file://", ""))
    }

    FileDialog {
        id: documentFileDialog
        title: qsTr("Upload File")
        fileMode: FileDialog.OpenFile
        nameFilters: [
            qsTr("Documents (*.pdf *.txt *.md *.markdown *.docx *.csv *.json)"),
            qsTr("Source code files (*.cpp *.h *.hpp *.qml *.js *.ts *.py *.java *.cs *.go *.rs *.swift)"),
            qsTr("All files (*)")
        ]
        onAccepted: homeChat.viewModel.attachFileToChat(selectedFile.toString().replace("file://", ""))
    }

    // Permanent delete confirmation dialog
    Dialog {
        id: deleteConfirmDialog
        anchors.centerIn: parent
        modal: true
        dim: true
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: SentinelTheme.radiusLg
            color: SentinelTheme.backgroundRaised
            border.color: SentinelTheme.withAlpha(SentinelTheme.warning, 0.35)
        }

        contentItem: ColumnLayout {
            spacing: 0
            width: 340

            // Header
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: "transparent"
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: SentinelTheme.spaceLg
                spacing: SentinelTheme.spaceMd

                // Warning icon + title
                RowLayout {
                    spacing: SentinelTheme.spaceSm

                    Text {
                        text: "\u26A0"
                        font.pixelSize: 20
                        color: SentinelTheme.warning
                    }

                    Label {
                        text: qsTr("Delete conversation")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontCard
                        font.bold: true
                        Layout.fillWidth: true
                    }
                }

                // Conversation name
                Rectangle {
                    Layout.fillWidth: true
                    radius: SentinelTheme.radiusMd
                    color: SentinelTheme.withAlpha(SentinelTheme.warning, 0.07)
                    border.color: SentinelTheme.withAlpha(SentinelTheme.warning, 0.18)
                    implicitHeight: deleteConvLabel.implicitHeight + SentinelTheme.spaceSm * 2

                    Label {
                        id: deleteConvLabel
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: SentinelTheme.spaceSm
                        text: homeChat.pendingDeleteConversationTitle
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontSmall
                        font.bold: true
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("This action cannot be undone. The conversation will be permanently removed.")
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                    wrapMode: Text.WordWrap
                }

                // Buttons
                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceSm

                    // Cancel
                    Button {
                        id: deleteCancelBtn
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        text: qsTr("Cancel")
                        hoverEnabled: true
                        onClicked: deleteConfirmDialog.close()

                        contentItem: Text {
                            text: deleteCancelBtn.text
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontSmall
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: SentinelTheme.radiusMd
                            color: InteractionTokens.surfaceColor(deleteCancelBtn.hovered, deleteCancelBtn.down, false, homeChat.modeAccent)
                            border.color: InteractionTokens.borderColor(deleteCancelBtn.activeFocus, deleteCancelBtn.hovered, false, homeChat.modeAccent)
                        }
                    }

                    // Confirm delete
                    Button {
                        id: deleteConfirmBtn
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        text: qsTr("Delete")
                        hoverEnabled: true
                        onClicked: {
                            deleteConfirmDialog.close()
                            homeChat.viewModel.requestPermanentDeleteConversation(
                                homeChat.pendingDeleteConversationId)
                            homeChat.pendingDeleteConversationId = ""
                            homeChat.pendingDeleteConversationTitle = ""
                        }

                        contentItem: Text {
                            text: deleteConfirmBtn.text
                            color: SentinelTheme.warning
                            font.pixelSize: SentinelTheme.fontSmall
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: SentinelTheme.radiusMd
                            color: deleteConfirmBtn.hovered
                                   ? SentinelTheme.withAlpha(SentinelTheme.warning, 0.14)
                                   : SentinelTheme.withAlpha(SentinelTheme.warning, 0.07)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.warning, deleteConfirmBtn.hovered ? 0.45 : 0.28)
                        }
                    }
                }
            }
        }
    }
}
