#include "sentinel/desktop/NativeCompanionAdapter.h"

#include "sentinel/core/AppMetadata.h"
#include "sentinel/core/AppSettings.h"
#include "sentinel/desktop/DesktopShellViewModel.h"

#include <QAction>
#include <QApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QKeySequence>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QWindow>

namespace sentinel::desktop {

namespace {

bool headlessQtPlatform() {
    const QString platformName = QGuiApplication::platformName().toLower();
    return platformName == QStringLiteral("offscreen") || platformName == QStringLiteral("minimal");
}

} // namespace

NativeCompanionAdapter::NativeCompanionAdapter(DesktopShellViewModel& viewModel,
                                               sentinel::core::AppSettings& settings,
                                               QObject* rootWindow, QObject* parent)
    : QObject(parent), viewModel_(viewModel), settings_(settings),
      window_(qobject_cast<QWindow*>(rootWindow)) {
    initialize();
}

NativeCompanionAdapter::~NativeCompanionAdapter() = default;

void NativeCompanionAdapter::applyMenuStylesheet() {
    if (!menu_)
        return;

    const QString theme = settings_.themeName();
    const bool isLight = theme.contains(QStringLiteral("Light"), Qt::CaseInsensitive);

    if (isLight) {
        menu_->setStyleSheet(QStringLiteral(R"(
            QMenu {
                background-color: #ffffff;
                color: #1f2937;
                border: 1px solid rgba(79, 142, 247, 0.35);
                border-radius: 10px;
                padding: 6px;
            }
            QMenu::item {
                padding: 7px 28px 7px 12px;
                border-radius: 6px;
                font-size: 13px;
                font-weight: 500;
            }
            QMenu::item:disabled {
                color: #9ca3af;
                background-color: transparent;
                font-weight: 600;
            }
            QMenu::item:selected:enabled {
                background-color: rgba(79, 142, 247, 0.15);
                color: #2563eb;
            }
            QMenu::separator {
                height: 1px;
                background-color: rgba(0, 0, 0, 0.08);
                margin: 4px 8px;
            }
        )"));
    } else {
        menu_->setStyleSheet(QStringLiteral(R"(
            QMenu {
                background-color: #111827;
                color: #f3f4f6;
                border: 1px solid rgba(56, 189, 248, 0.30);
                border-radius: 10px;
                padding: 6px;
            }
            QMenu::item {
                padding: 7px 28px 7px 12px;
                border-radius: 6px;
                font-size: 13px;
                font-weight: 500;
            }
            QMenu::item:disabled {
                color: #9ca3af;
                background-color: transparent;
                font-weight: 600;
            }
            QMenu::item:selected:enabled {
                background-color: rgba(56, 189, 248, 0.20);
                color: #38bdf8;
            }
            QMenu::separator {
                height: 1px;
                background-color: rgba(255, 255, 255, 0.10);
                margin: 4px 8px;
            }
        )"));
    }
}

void NativeCompanionAdapter::initialize() {
    if (headlessQtPlatform()) {
        viewModel_.setCompanionNativeAvailable(false);
        return;
    }

    menu_ = std::make_unique<QMenu>();
    applyMenuStylesheet();

    // ── Header / Status Section ──────────────────────────────
    headerAction_ = menu_->addAction(QStringLiteral("Sentinel AI • Ready (Local Ollama)"));
    headerAction_->setEnabled(false);
    menu_->addSeparator();

    // ── Core Assistant Actions ───────────────────────────────
    quickChatAction_ = menu_->addAction(QStringLiteral("💬 Quick Prompt (Tray Chat)"));
    quickChatAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));

    newConversationAction_ = menu_->addAction(QStringLiteral("✨ New Conversation"));
    newConversationAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));

    openAction_ = menu_->addAction(QStringLiteral("🖥 Open Main Window"));
    openAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));

    menu_->addSeparator();

    // ── Utility & Capture Actions ─────────────────────────────
    quickNoteAction_ = menu_->addAction(QStringLiteral("📝 Quick Capture / Note"));
    quickNoteAction_->setEnabled(true);

    clearChatAction_ = menu_->addAction(QStringLiteral("🗑 Clear Companion Session"));

    pauseAction_ = menu_->addAction(QStringLiteral("⏸ Pause Companion"));

    menu_->addSeparator();

    // ── Settings & Updates ────────────────────────────────────
    settingsAction_ = menu_->addAction(QStringLiteral("⚙ Settings & Preferences..."));
    settingsAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma));

    updateAction_ = menu_->addAction(QStringLiteral("🔄 Check for Updates..."));

    menu_->addSeparator();

    // ── App Lifecycle ─────────────────────────────────────────
    quitAction_ = menu_->addAction(QStringLiteral("🚪 Quit Sentinel"));
    quitAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));

    QIcon trayIcon(QStringLiteral(":/icons/dev.sentinel.Sentinel.png"));
#if defined(Q_OS_MACOS)
    trayIcon.setIsMask(true);
#endif
    trayIcon_ = std::make_unique<QSystemTrayIcon>(trayIcon);
    trayIcon_->setContextMenu(menu_.get());
    trayIcon_->setToolTip(sentinel::core::AppMetadata::displayName() +
                          QStringLiteral(" Companion"));

    connect(openAction_, &QAction::triggered, this, &NativeCompanionAdapter::openSentinel);
    connect(quickChatAction_, &QAction::triggered, this,
            [this]() { viewModel_.toggleCompanionChat(); });
    connect(newConversationAction_, &QAction::triggered, this,
            &NativeCompanionAdapter::newConversation);
    connect(quickNoteAction_, &QAction::triggered, this,
            [this]() { viewModel_.toggleCompanionChat(); });
    connect(clearChatAction_, &QAction::triggered, this, [this]() { viewModel_.clearChat(); });
    connect(pauseAction_, &QAction::triggered, this, &NativeCompanionAdapter::togglePaused);
    connect(settingsAction_, &QAction::triggered, this, &NativeCompanionAdapter::openSettings);
    connect(updateAction_, &QAction::triggered, this, &NativeCompanionAdapter::checkUpdates);
    connect(quitAction_, &QAction::triggered, qApp, &QApplication::quit);

    connect(trayIcon_.get(), &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::DoubleClick) {
                    openSentinel();
                }
            });
    connect(&settings_, &sentinel::core::AppSettings::companionEnabledChanged, this,
            &NativeCompanionAdapter::refreshVisibility);
    connect(&settings_, &sentinel::core::AppSettings::themeNameChanged, this,
            &NativeCompanionAdapter::applyMenuStylesheet);
    connect(&viewModel_, &DesktopShellViewModel::companionChanged, this,
            &NativeCompanionAdapter::refreshActions);

    refreshVisibility();
}

void NativeCompanionAdapter::refreshVisibility() {
    if (headlessQtPlatform()) {
        viewModel_.setCompanionNativeAvailable(false);
        return;
    }

    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    viewModel_.setCompanionNativeAvailable(trayAvailable);

    if (!trayIcon_) {
        return;
    }

    if (trayAvailable) {
        refreshActions();
        trayIcon_->show();
        return;
    }

    trayIcon_->hide();
}

void NativeCompanionAdapter::refreshActions() {
    const bool enabled = QSystemTrayIcon::isSystemTrayAvailable();
    const bool paused = viewModel_.companionPaused();

    if (headerAction_) {
        headerAction_->setText(paused ? QStringLiteral("Sentinel AI • Paused")
                                      : QStringLiteral("Sentinel AI • Active (Local Ollama)"));
    }
    if (openAction_) {
        openAction_->setEnabled(enabled);
    }
    if (quickChatAction_) {
        quickChatAction_->setEnabled(enabled && !paused);
    }
    if (newConversationAction_) {
        newConversationAction_->setEnabled(enabled);
    }
    if (quickNoteAction_) {
        quickNoteAction_->setEnabled(enabled && !paused);
    }
    if (clearChatAction_) {
        clearChatAction_->setEnabled(enabled && !paused);
    }
    if (pauseAction_) {
        pauseAction_->setEnabled(enabled);
        pauseAction_->setText(paused ? QStringLiteral("▶ Resume Companion")
                                     : QStringLiteral("⏸ Pause Companion"));
    }
    if (updateAction_) {
        updateAction_->setEnabled(enabled);
    }
    if (settingsAction_) {
        settingsAction_->setEnabled(enabled);
    }
    if (quitAction_) {
        quitAction_->setEnabled(enabled);
    }
    if (trayIcon_) {
        trayIcon_->setToolTip(paused
                                  ? QStringLiteral("Sentinel Companion (Paused)")
                                  : QStringLiteral("Sentinel Companion (Active • Local Ollama)"));
    }
}

void NativeCompanionAdapter::activateMainWindow() {
    if (!window_) {
        return;
    }

    window_->show();
    window_->raise();
    window_->requestActivate();
}

void NativeCompanionAdapter::openSentinel() {
    activateMainWindow();
}

void NativeCompanionAdapter::newConversation() {
    const QString conversationId =
        viewModel_.createConversation(QStringLiteral("New Conversation"));
    if (!conversationId.isEmpty()) {
        viewModel_.setCurrentPage(QStringLiteral("Dashboard"));
    }
    activateMainWindow();
}

void NativeCompanionAdapter::openSettings() {
    viewModel_.setCurrentPage(QStringLiteral("Settings"));
    activateMainWindow();
}

void NativeCompanionAdapter::togglePaused() {
    viewModel_.setCompanionPaused(!viewModel_.companionPaused());
    refreshActions();
}

void NativeCompanionAdapter::checkUpdates() {
    viewModel_.checkForUpdates();
    activateMainWindow();
}

} // namespace sentinel::desktop
