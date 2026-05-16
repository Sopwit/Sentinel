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
    controller_.setRoutingModeByName(settings_.routingModeName());
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
    connect(&controller_, &core::ApplicationController::toolPlanChanged, this,
            &DesktopShellViewModel::toolPlanChanged);
    connect(&controller_, &core::ApplicationController::approvalChanged, this,
            &DesktopShellViewModel::approvalChanged);
    connect(&controller_, &core::ApplicationController::sandboxChanged, this,
            &DesktopShellViewModel::sandboxChanged);
    connect(&controller_, &core::ApplicationController::toolExecutionChanged, this,
            &DesktopShellViewModel::toolExecutionChanged);
    connect(&controller_, &core::ApplicationController::agentPipelineChanged, this,
            &DesktopShellViewModel::agentPipelineChanged);
    connect(&controller_, &core::ApplicationController::runtimeContextChanged, this,
            &DesktopShellViewModel::runtimeContextChanged);
    connect(&controller_, &core::ApplicationController::agentActivityChanged, this,
            &DesktopShellViewModel::agentActivityChanged);
    connect(&controller_, &core::ApplicationController::modelRoutingChanged, this,
            &DesktopShellViewModel::modelRoutingChanged);
    connect(&controller_, &core::ApplicationController::taskPlanChanged, this,
            &DesktopShellViewModel::taskPlanChanged);
    connect(&controller_, &core::ApplicationController::orchestrationSnapshotChanged, this,
            &DesktopShellViewModel::orchestrationSnapshotChanged);
    connect(&modeManager_, &core::ModeManager::currentModeChanged, this,
            &DesktopShellViewModel::currentModeChanged);
    connect(&settings_, &core::AppSettings::themeNameChanged, this,
            &DesktopShellViewModel::themeNameChanged);
    connect(&settings_, &core::AppSettings::configurationProfileChanged, this,
            &DesktopShellViewModel::configurationProfileChanged);
    connect(&settings_, &core::AppSettings::routingModeNameChanged, this,
            &DesktopShellViewModel::modelRoutingChanged);
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

QString DesktopShellViewModel::latestToolPlanStatus() const {
    return controller_.latestToolPlanStatus();
}

QString DesktopShellViewModel::latestToolPlanSummary() const {
    return controller_.latestToolPlanSummary();
}

QString DesktopShellViewModel::latestApprovalStatus() const {
    return controller_.latestApprovalStatus();
}

QString DesktopShellViewModel::latestApprovalSummary() const {
    return controller_.latestApprovalSummary();
}

QString DesktopShellViewModel::latestSandboxStatus() const {
    return controller_.latestSandboxStatus();
}

QString DesktopShellViewModel::latestSandboxSummary() const {
    return controller_.latestSandboxSummary();
}

QString DesktopShellViewModel::latestToolExecutionStatus() const {
    return controller_.latestToolExecutionStatus();
}

QString DesktopShellViewModel::latestToolExecutionSummary() const {
    return controller_.latestToolExecutionSummary();
}

QString DesktopShellViewModel::latestAgentPipelineStatus() const {
    return controller_.latestAgentPipelineStatus();
}

QString DesktopShellViewModel::latestAgentPipelineSummary() const {
    return controller_.latestAgentPipelineSummary();
}

QString DesktopShellViewModel::runtimeSessionId() const {
    return controller_.runtimeSessionId();
}

QString DesktopShellViewModel::runtimeContextStatus() const {
    return controller_.runtimeContextStatus();
}

QString DesktopShellViewModel::runtimeContextSummary() const {
    return controller_.runtimeContextSummary();
}

QStringList DesktopShellViewModel::runtimeContextActiveToolIds() const {
    return controller_.runtimeContextActiveToolIds();
}

int DesktopShellViewModel::agentActivityCount() const {
    return controller_.agentActivityCount();
}

QString DesktopShellViewModel::latestAgentActivitySummary() const {
    return controller_.latestAgentActivitySummary();
}

QString DesktopShellViewModel::currentRoutingMode() const {
    return controller_.currentRoutingMode();
}

void DesktopShellViewModel::setRoutingModeByName(const QString& routingModeName) {
    const auto before = controller_.currentRoutingMode();
    settings_.setRoutingModeName(routingModeName);
    controller_.setRoutingModeByName(settings_.routingModeName());
    if (before == controller_.currentRoutingMode()) {
        emit modelRoutingChanged();
        emit orchestrationSnapshotChanged();
    }
}

QStringList DesktopShellViewModel::availableRoutingModes() const {
    return settings_.availableRoutingModes();
}

QString DesktopShellViewModel::modelRoutingStatus() const {
    return controller_.modelRoutingStatus();
}

QString DesktopShellViewModel::selectedModelProviderSummary() const {
    return controller_.selectedModelProviderSummary();
}

QString DesktopShellViewModel::latestTaskPlanStatus() const {
    return controller_.latestTaskPlanStatus();
}

QString DesktopShellViewModel::latestTaskPlanSummary() const {
    return controller_.latestTaskPlanSummary();
}

int DesktopShellViewModel::plannedTaskStepCount() const {
    return controller_.plannedTaskStepCount();
}

int DesktopShellViewModel::registeredAgentCount() const {
    return controller_.registeredAgentCount();
}

QStringList DesktopShellViewModel::activeAgentSummaries() const {
    return controller_.activeAgentSummaries();
}

QString DesktopShellViewModel::currentAgentSummary() const {
    return controller_.currentAgentSummary();
}

QString DesktopShellViewModel::currentMemoryAffinitySummary() const {
    return controller_.currentMemoryAffinitySummary();
}

int DesktopShellViewModel::providerCatalogCount() const {
    return controller_.providerCatalogCount();
}

QStringList DesktopShellViewModel::providerCatalogSummaries() const {
    return controller_.providerCatalogSummaries();
}

int DesktopShellViewModel::memoryCatalogCount() const {
    return controller_.memoryCatalogCount();
}

QStringList DesktopShellViewModel::memoryCatalogSummaries() const {
    return controller_.memoryCatalogSummaries();
}

QString DesktopShellViewModel::orchestrationSnapshotStatus() const {
    return controller_.orchestrationSnapshotStatus();
}

QString DesktopShellViewModel::orchestrationSnapshotSummary() const {
    return controller_.orchestrationSnapshotSummary();
}

QStringList DesktopShellViewModel::orchestrationSignals() const {
    return controller_.orchestrationSignals();
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
        QStringLiteral("Agents"),
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
        QStringLiteral("Agents"),
        QStringLiteral("Settings"),
    };
    return pages.contains(trimmed) ? trimmed : QStringLiteral("Dashboard");
}

} // namespace sentinel::desktop
