#pragma once

#include <QObject>

#include <memory>

class QAction;
class QMenu;
class QSystemTrayIcon;
class QWindow;

namespace sentinel::core {
class AppSettings;
} // namespace sentinel::core

namespace sentinel::desktop {

class DesktopShellViewModel;

class NativeCompanionAdapter final : public QObject {
    Q_OBJECT

public:
    NativeCompanionAdapter(DesktopShellViewModel& viewModel, sentinel::core::AppSettings& settings,
                           QObject* rootWindow, QObject* parent = nullptr);
    ~NativeCompanionAdapter() override;

private:
    void initialize();
    void refreshVisibility();
    void refreshActions();
    void activateMainWindow();
    void openSentinel();
    void newConversation();
    void openSettings();
    void togglePaused();

    DesktopShellViewModel& viewModel_;
    sentinel::core::AppSettings& settings_;
    QWindow* window_ = nullptr;
    std::unique_ptr<QSystemTrayIcon> trayIcon_;
    std::unique_ptr<QMenu> menu_;
    QAction* openAction_ = nullptr;
    QAction* newConversationAction_ = nullptr;
    QAction* quickNoteAction_ = nullptr;
    QAction* pauseAction_ = nullptr;
    QAction* settingsAction_ = nullptr;
    QAction* quitAction_ = nullptr;
};

} // namespace sentinel::desktop
