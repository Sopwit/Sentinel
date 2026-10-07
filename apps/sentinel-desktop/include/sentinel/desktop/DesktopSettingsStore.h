// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/interfaces/ISettingsStore.h"
#include "sentinel/core/memory/JsonSettingsStore.h"
#include "sentinel/desktop/DesktopRuntimeClient.h"
#include <QSet>

namespace sentinel::desktop {
// UI preferences persist locally; runtime configuration is a daemon projection.
class DesktopSettingsStore final : public core::ISettingsStore {
public:
    DesktopSettingsStore(DesktopRuntimeClient& runtime, const QString& path)
        : m_runtime(runtime), m_preferences(path) {}
    QString value(const QString& key, const QString& fallback = {}) const override {
        return presentationKey(key) ? m_preferences.value(key, fallback)
                                    : m_runtime.settingsValue(key, fallback);
    }
    void setValue(QString key, QString value) override {
        if (presentationKey(key))
            m_preferences.setValue(std::move(key), std::move(value));
        else
            m_runtime.setSetting(key, value);
    }
    void remove(const QString& key) override {
        if (presentationKey(key))
            m_preferences.remove(key);
    }
    QString errorCode() const override {
        return m_preferences.errorCode();
    }
    static bool presentationKey(const QString& key) {
        static const QSet<QString> keys = {"themeName",
                                           "appLanguage",
                                           "activeConversationId",
                                           "companionEnabled",
                                           "quickPanelShortcut",
                                           "quickPanelStartAtLogin",
                                           "developerModeEnabled",
                                           "contextExplainabilityVisible",
                                           "retrievalExplainabilityEnabled",
                                           "onboardingComplete",
                                           "onboardingFlowJson",
                                           "onboardingUseCase",
                                           "onboardingAiProvider",
                                           "selectedSystemMode",
                                           "recoveryDraftText",
                                           "reducedMotionEnabled",
                                           "highContrastEnabled",
                                           "uiDensity",
                                           "notificationCenterJson",
                                           "notificationPolicy",
                                           "notifyModelDownloads",
                                           "notifyModelRemovals",
                                           "notifyAgentResponses",
                                           "notifySystemUpdates",
                                           "soundEffectsEnabled",
                                           "updateCheckPolicy",
                                           "updateCheckUrl",
                                           "updateWorkflowState"};
        return keys.contains(key);
    }

private:
    DesktopRuntimeClient& m_runtime;
    core::JsonSettingsStore m_preferences;
};
} // namespace sentinel::desktop
