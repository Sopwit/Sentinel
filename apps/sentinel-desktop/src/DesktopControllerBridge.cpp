// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/desktop/DesktopControllerBridge.h"
namespace sentinel::desktop {
DesktopControllerBridge::DesktopControllerBridge(Source source, QObject* parent)
    : QObject(parent), m_source(source) {
    Q_ASSERT((source.local != nullptr) != (source.remote != nullptr));
    if (source.local)
        connect(source.local, &core::ApplicationController::modelLibraryChanged, this,
                &DesktopControllerBridge::modelLibraryChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::ollamaStatusChanged, this,
                &DesktopControllerBridge::ollamaStatusChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::chatMessagesChanged, this,
                &DesktopControllerBridge::chatMessagesChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::memoryEntriesChanged, this,
                &DesktopControllerBridge::memoryEntriesChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::maintenanceStatusChanged, this,
                &DesktopControllerBridge::maintenanceStatusChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::agentStatusChanged, this,
                &DesktopControllerBridge::agentStatusChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::agentResponseChanged, this,
                &DesktopControllerBridge::agentResponseChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::toolPlanChanged, this,
                &DesktopControllerBridge::toolPlanChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::approvalChanged, this,
                &DesktopControllerBridge::approvalChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::sandboxChanged, this,
                &DesktopControllerBridge::sandboxChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::toolExecutionChanged, this,
                &DesktopControllerBridge::toolExecutionChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::agentPipelineChanged, this,
                &DesktopControllerBridge::agentPipelineChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::runtimeContextChanged, this,
                &DesktopControllerBridge::runtimeContextChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::conversationSessionChanged, this,
                &DesktopControllerBridge::conversationSessionChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::conversationStateChanged, this,
                &DesktopControllerBridge::conversationStateChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::conversationRuntimeChanged, this,
                &DesktopControllerBridge::conversationRuntimeChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::conversationSearchChanged, this,
                &DesktopControllerBridge::conversationSearchChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::conversationExportChanged, this,
                &DesktopControllerBridge::conversationExportChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::conversationDuplicateChanged, this,
                &DesktopControllerBridge::conversationDuplicateChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::conversationDeleteChanged, this,
                &DesktopControllerBridge::conversationDeleteChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::memoryCandidatesChanged, this,
                &DesktopControllerBridge::memoryCandidatesChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::memoryRecallChanged, this,
                &DesktopControllerBridge::memoryRecallChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::contextAssemblyChanged, this,
                &DesktopControllerBridge::contextAssemblyChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::agentActivityChanged, this,
                &DesktopControllerBridge::agentActivityChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::agentLoopStateChanged, this,
                &DesktopControllerBridge::agentLoopStateChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::modelRoutingChanged, this,
                &DesktopControllerBridge::modelRoutingChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::taskPlanChanged, this,
                &DesktopControllerBridge::taskPlanChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::orchestrationSnapshotChanged, this,
                &DesktopControllerBridge::orchestrationSnapshotChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::runtimeProviderRegistryChanged, this,
                &DesktopControllerBridge::runtimeProviderRegistryChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::localModelSelectionChanged, this,
                &DesktopControllerBridge::localModelSelectionChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::modelCapabilitiesChanged, this,
                &DesktopControllerBridge::modelCapabilitiesChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::localChatInferenceRoutingChanged, this,
                &DesktopControllerBridge::localChatInferenceRoutingChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::localInferenceChanged, this,
                &DesktopControllerBridge::localInferenceChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::voiceConfigurationChanged, this,
                &DesktopControllerBridge::voiceConfigurationChanged);
    if (source.local)
        connect(source.local, &core::ApplicationController::promptContextInjectionChanged, this,
                &DesktopControllerBridge::promptContextInjectionChanged);
    if (source.remote)
        connect(source.remote, &DesktopRuntimeClient::changed, this, [this] {
            emit modelLibraryChanged();
            emit ollamaStatusChanged();
            emit chatMessagesChanged();
            emit memoryEntriesChanged();
            emit maintenanceStatusChanged();
            emit agentStatusChanged();
            emit agentResponseChanged();
            emit toolPlanChanged();
            emit approvalChanged();
            emit sandboxChanged();
            emit toolExecutionChanged();
            emit agentPipelineChanged();
            emit runtimeContextChanged();
            emit conversationSessionChanged();
            emit conversationStateChanged();
            emit conversationRuntimeChanged();
            emit conversationSearchChanged();
            emit conversationExportChanged();
            emit conversationDuplicateChanged();
            emit conversationDeleteChanged();
            emit memoryCandidatesChanged();
            emit memoryRecallChanged();
            emit contextAssemblyChanged();
            emit agentActivityChanged();
            emit agentLoopStateChanged();
            emit modelRoutingChanged();
            emit taskPlanChanged();
            emit orchestrationSnapshotChanged();
            emit runtimeProviderRegistryChanged();
            emit localModelSelectionChanged();
            emit modelCapabilitiesChanged();
            emit localChatInferenceRoutingChanged();
            emit localInferenceChanged();
            emit voiceConfigurationChanged();
            emit promptContextInjectionChanged();
        });
}
} // namespace sentinel::desktop
