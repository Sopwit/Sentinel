#include "sentinel/desktop/DesktopShellViewModel.h"

#include "sentinel/core/AppSettings.h"
#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/ModeManager.h"

namespace sentinel::desktop {

DesktopShellViewModel::DesktopShellViewModel(core::ApplicationController& controller,
                                             core::ModeManager& modeManager,
                                             core::AppSettings& settings, QObject* parent)
    : QObject(parent), controller_(controller), modeManager_(modeManager), settings_(settings) {
    connect(&controller_, &core::ApplicationController::chatMessagesChanged, this,
            &DesktopShellViewModel::chatMessagesChanged);
    connect(&controller_, &core::ApplicationController::memoryEntriesChanged, this,
            &DesktopShellViewModel::memoryEntriesChanged);
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

QString DesktopShellViewModel::currentModeName() const {
    return modeManager_.currentModeName();
}

QStringList DesktopShellViewModel::availableModes() const {
    return modeManager_.availableModes();
}

QStringList DesktopShellViewModel::chatMessages() const {
    return controller_.chatMessages();
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

void DesktopShellViewModel::sendMessage(const QString& message) {
    controller_.sendMessage(message);
}

void DesktopShellViewModel::setModeByName(const QString& modeName) {
    modeManager_.setModeByName(modeName);
}

void DesktopShellViewModel::remember(const QString& key, const QString& value) {
    controller_.remember(key, value);
}

} // namespace sentinel::desktop
