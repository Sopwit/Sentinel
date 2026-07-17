#include "sentinel/desktop/NativeCompanionAdapter.h"

#include "sentinel/core/AppMetadata.h"
#include "sentinel/core/AppSettings.h"
#include "sentinel/desktop/DesktopShellViewModel.h"

#include <QAction>
#include <QApplication>
#include <QGuiApplication>
#include <QIcon>
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

void NativeCompanionAdapter::initialize() {
    if (headlessQtPlatform()) {
        viewModel_.setCompanionNativeAvailable(false);
        return;
    }

    menu_ = std::make_unique<QMenu>();
    openAction_ = menu_->addAction(QStringLiteral("Open Sentinel"));
    newConversationAction_ = menu_->addAction(QStringLiteral("New Conversation"));
    quickNoteAction_ = menu_->addAction(QStringLiteral("Quick Note / Capture"));
    quickNoteAction_->setEnabled(false);
    menu_->addSeparator();
    pauseAction_ = menu_->addAction(QStringLiteral("Pause Companion"));
    settingsAction_ = menu_->addAction(QStringLiteral("Settings"));
    menu_->addSeparator();
    quitAction_ = menu_->addAction(QStringLiteral("Quit"));

    QIcon trayIcon(QStringLiteral(":/icons/dev.sentinel.Sentinel.png"));
#if defined(Q_OS_MACOS)
    trayIcon.setIsMask(true);
#endif
    trayIcon_ = std::make_unique<QSystemTrayIcon>(trayIcon);
    trayIcon_->setContextMenu(menu_.get());
    trayIcon_->setToolTip(sentinel::core::AppMetadata::displayName());

    connect(openAction_, &QAction::triggered, this, &NativeCompanionAdapter::openSentinel);
    connect(newConversationAction_, &QAction::triggered, this,
            &NativeCompanionAdapter::newConversation);
    connect(pauseAction_, &QAction::triggered, this, &NativeCompanionAdapter::togglePaused);
    connect(settingsAction_, &QAction::triggered, this, &NativeCompanionAdapter::openSettings);
    connect(quitAction_, &QAction::triggered, qApp, &QApplication::quit);
    connect(trayIcon_.get(), &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                    openSentinel();
                }
            });
    connect(&settings_, &sentinel::core::AppSettings::companionEnabledChanged, this,
            &NativeCompanionAdapter::refreshVisibility);
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

    if (openAction_) {
        openAction_->setEnabled(enabled);
    }
    if (newConversationAction_) {
        newConversationAction_->setEnabled(enabled);
    }
    if (pauseAction_) {
        pauseAction_->setEnabled(enabled);
        pauseAction_->setText(paused ? QStringLiteral("Resume Companion")
                                     : QStringLiteral("Pause Companion"));
    }
    if (settingsAction_) {
        settingsAction_->setEnabled(enabled);
    }
    if (quitAction_) {
        quitAction_->setEnabled(enabled);
    }
    if (trayIcon_) {
        trayIcon_->setToolTip(paused ? QStringLiteral("Sentinel Companion paused")
                                     : QStringLiteral("Sentinel Companion ready"));
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

} // namespace sentinel::desktop
