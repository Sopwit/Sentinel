// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/app/ApplicationController.h"
#include "sentinel/desktop/DesktopRuntimeClient.h"
#include <type_traits>
#include <utility>

namespace sentinel::desktop {
// Explicit compatibility seam: local source is used by legacy unit fixtures only.
// Production supplies a DesktopRuntimeClient and owns no ApplicationController.
class DesktopControllerBridge final : public QObject {
    Q_OBJECT
public:
    struct Source {
        core::ApplicationController* local = nullptr;
        DesktopRuntimeClient* remote = nullptr;
    };
    explicit DesktopControllerBridge(Source source, QObject* parent = nullptr);
    bool isRemote() const {
        return m_source.remote != nullptr;
    }
    core::ApplicationController* local() const { return m_source.local; }
    DesktopRuntimeClient* remote() const {
        return m_source.remote;
    }
    const QList<core::ChatMessage>& chatHistory() const {
        return m_source.local ? m_source.local->chatHistory() : m_source.remote->messages();
    }
#define SENTINEL_DESKTOP_FORWARD(name)                                                             \
    template <class... Args>                                                                       \
    auto name(Args&&... args) const -> decltype(std::declval<core::ApplicationController&>().name( \
        std::forward<Args>(args)...)) {                                                            \
        using Result = decltype(std::declval<core::ApplicationController&>().name(                 \
            std::forward<Args>(args)...));                                                         \
        if (m_source.local)                                                                        \
            return m_source.local->name(std::forward<Args>(args)...);                              \
        return project<Result>(QStringLiteral(#name), std::forward<Args>(args)...);                \
    }
#include "sentinel/desktop/DesktopBridge.generated.inc"
#undef SENTINEL_DESKTOP_FORWARD
signals:

    void modelLibraryChanged();
    void ollamaStatusChanged();
    void chatMessagesChanged();
    void memoryEntriesChanged();
    void maintenanceStatusChanged();
    void agentStatusChanged();
    void agentResponseChanged();
    void toolPlanChanged();
    void approvalChanged();
    void sandboxChanged();
    void toolExecutionChanged();
    void agentPipelineChanged();
    void runtimeContextChanged();
    void conversationSessionChanged();
    void conversationStateChanged();
    void conversationRuntimeChanged();
    void conversationSearchChanged();
    void conversationExportChanged();
    void conversationDuplicateChanged();
    void conversationDeleteChanged();
    void memoryCandidatesChanged();
    void memoryRecallChanged();
    void contextAssemblyChanged();
    void agentActivityChanged();
    void agentLoopStateChanged();
    void modelRoutingChanged();
    void taskPlanChanged();
    void orchestrationSnapshotChanged();
    void runtimeProviderRegistryChanged();
    void localModelSelectionChanged();
    void modelCapabilitiesChanged();
    void localChatInferenceRoutingChanged();
    void localInferenceChanged();
    void voiceConfigurationChanged();
    void promptContextInjectionChanged();

private:
    template <class T> static QVariant argument(T&& value) {
        if constexpr (std::is_base_of_v<QObject, std::remove_cvref_t<T>>)
            return {};
        else
            return QVariant::fromValue(std::forward<T>(value));
    }
    template <class R, class... Args> R project(const QString& name, Args&&... args) const {
        if constexpr (std::is_pointer_v<R>)
            return nullptr;
        else if constexpr (std::is_void_v<R>) {
            m_source.remote->dispatch(name, {argument(std::forward<Args>(args))...});
            return;
        } else if constexpr (std::is_same_v<R, QString> || std::is_same_v<R, QStringList> ||
                             std::is_same_v<R, QVariantList> || std::is_same_v<R, QVariantMap> ||
                             std::is_arithmetic_v<R>) {
            const auto value =
                m_source.remote->dispatch(name, {argument(std::forward<Args>(args))...});
            return value.template value<R>();
        } else
            return R{};
    }
    Source m_source;
};
} // namespace sentinel::desktop
