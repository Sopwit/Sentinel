#include "sentinel/desktop/DesktopShellViewModel.h"

#include "sentinel/core/AppSettings.h"
#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/ModeManager.h"

namespace sentinel::desktop {

DesktopShellViewModel::DesktopShellViewModel(core::ApplicationController& controller,
                                             core::ModeManager& modeManager,
                                             core::AppSettings& settings, QObject* parent)
    : QObject(parent), controller_(controller), modeManager_(modeManager), settings_(settings),
      chatMessages_(this) {
    chatMessages_.setMessages(controller_.chatHistory());
    connect(&controller_, &core::ApplicationController::chatMessagesChanged, this, [this]() {
        chatMessages_.setMessages(controller_.chatHistory());
        emit chatMessagesChanged();
    });
    connect(&controller_, &core::ApplicationController::memoryEntriesChanged, this,
            &DesktopShellViewModel::memoryEntriesChanged);
    connect(&controller_, &core::ApplicationController::maintenanceStatusChanged, this,
            &DesktopShellViewModel::maintenanceStatusChanged);
    connect(&controller_, &core::ApplicationController::agentStatusChanged, this,
            &DesktopShellViewModel::agentStatusChanged);
    connect(&controller_, &core::ApplicationController::agentResponseChanged, this,
            &DesktopShellViewModel::agentResponseChanged);
    connect(&modeManager_, &core::ModeManager::currentModeChanged, this,
            &DesktopShellViewModel::currentModeChanged);
    connect(&settings_, &core::AppSettings::themeNameChanged, this,
            &DesktopShellViewModel::themeNameChanged);
    connect(&settings_, &core::AppSettings::configurationProfileChanged, this,
            &DesktopShellViewModel::configurationProfileChanged);
}

QString DesktopShellViewModel::providerName() const {
    return controller_.providerName();
}

QString DesktopShellViewModel::providerStatus() const {
    return controller_.providerStatus();
}

QString DesktopShellViewModel::agentStatus() const {
    return controller_.agentStatus();
}

QString DesktopShellViewModel::lastAgentResponse() const {
    return controller_.lastAgentResponse();
}

int DesktopShellViewModel::availableToolCount() const {
    return controller_.availableToolCount();
}

QStringList DesktopShellViewModel::availableToolIds() const {
    return controller_.availableToolIds();
}

QString DesktopShellViewModel::memoryStatus() const {
    return controller_.memoryStatus();
}

QString DesktopShellViewModel::chatHistoryStatus() const {
    return controller_.chatHistoryStatus();
}

QString DesktopShellViewModel::memoryMaintenanceStatus() const {
    return controller_.memoryMaintenanceStatus();
}

QString DesktopShellViewModel::chatMaintenanceStatus() const {
    return controller_.chatMaintenanceStatus();
}

QString DesktopShellViewModel::currentModeName() const {
    return modeManager_.currentModeName();
}

QStringList DesktopShellViewModel::availableModes() const {
    return modeManager_.availableModes();
}

QString DesktopShellViewModel::currentPage() const {
    return currentPage_;
}

void DesktopShellViewModel::setCurrentPage(const QString& page) {
    const auto normalized = normalizedPageOrDefault(page);
    if (normalized == currentPage_) {
        return;
    }

    currentPage_ = normalized;
    emit currentPageChanged();
}

QStringList DesktopShellViewModel::availablePages() const {
    return {
        QStringLiteral("Dashboard"),
        QStringLiteral("Memory"),
        QStringLiteral("Settings"),
    };
}

ChatMessageListModel* DesktopShellViewModel::chatMessages() {
    return &chatMessages_;
}

QStringList DesktopShellViewModel::memoryEntries() const {
    return controller_.memoryEntries();
}

QString DesktopShellViewModel::themeName() const {
    return settings_.themeName();
}

void DesktopShellViewModel::setThemeName(const QString& themeName) {
    settings_.setThemeName(themeName);
}

QString DesktopShellViewModel::configurationProfile() const {
    return settings_.configurationProfile();
}

void DesktopShellViewModel::setConfigurationProfile(const QString& configurationProfile) {
    settings_.setConfigurationProfile(configurationProfile);
}

bool DesktopShellViewModel::sendMessage(const QString& message) {
    return controller_.sendMessage(message);
}

bool DesktopShellViewModel::runAgentRequest(const QString& request) {
    return controller_.runAgentRequest(request);
}

bool DesktopShellViewModel::clearMemory() {
    return controller_.clearMemory();
}

bool DesktopShellViewModel::clearChat() {
    return controller_.clearChat();
}

void DesktopShellViewModel::setModeByName(const QString& modeName) {
    modeManager_.setModeByName(modeName);
}

void DesktopShellViewModel::remember(const QString& key, const QString& value) {
    controller_.remember(key, value);
}

QString DesktopShellViewModel::normalizedPageOrDefault(const QString& page) {
    const auto trimmed = page.trimmed();
    const QStringList pages{
        QStringLiteral("Dashboard"),
        QStringLiteral("Memory"),
        QStringLiteral("Settings"),
    };
    return pages.contains(trimmed) ? trimmed : QStringLiteral("Dashboard");
}

} // namespace sentinel::desktop
