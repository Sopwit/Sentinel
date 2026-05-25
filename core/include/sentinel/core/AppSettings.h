#pragma once

#include "sentinel/core/ISettingsStore.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

namespace sentinel::core {

class AppSettings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY themeNameChanged)
    Q_PROPERTY(QString configurationProfile READ configurationProfile WRITE setConfigurationProfile
                   NOTIFY configurationProfileChanged)
    Q_PROPERTY(QString appLanguage READ appLanguage WRITE setAppLanguage NOTIFY appLanguageChanged)
    Q_PROPERTY(QStringList availableLanguages READ availableLanguages CONSTANT)
    Q_PROPERTY(QString routingModeName READ routingModeName WRITE setRoutingModeName NOTIFY
                   routingModeNameChanged)
    Q_PROPERTY(QString ollamaEndpoint READ ollamaEndpoint WRITE setOllamaEndpoint NOTIFY
                   ollamaEndpointChanged)
    Q_PROPERTY(QString selectedLocalModel READ selectedLocalModel WRITE setSelectedLocalModel NOTIFY
                   selectedLocalModelChanged)
    Q_PROPERTY(bool localChatInferenceEnabled READ localChatInferenceEnabled WRITE
                   setLocalChatInferenceEnabled NOTIFY localChatInferenceEnabledChanged)
    Q_PROPERTY(bool localInferenceStreamingEnabled READ localInferenceStreamingEnabled WRITE
                   setLocalInferenceStreamingEnabled NOTIFY localInferenceStreamingEnabledChanged)
    Q_PROPERTY(bool promptContextInjectionEnabled READ promptContextInjectionEnabled WRITE
                   setPromptContextInjectionEnabled NOTIFY promptContextInjectionEnabledChanged)
    Q_PROPERTY(bool semanticPromptInclusionEnabled READ semanticPromptInclusionEnabled WRITE
                   setSemanticPromptInclusionEnabled NOTIFY semanticPromptInclusionEnabledChanged)
    Q_PROPERTY(bool contextExplainabilityVisible READ contextExplainabilityVisible WRITE
                   setContextExplainabilityVisible NOTIFY contextExplainabilityVisibleChanged)
    Q_PROPERTY(bool developerModeEnabled READ developerModeEnabled WRITE setDeveloperModeEnabled
                   NOTIFY developerModeEnabledChanged)
    Q_PROPERTY(QString piperBinaryPath READ piperBinaryPath WRITE setPiperBinaryPath NOTIFY
                   piperBinaryPathChanged)
    Q_PROPERTY(QString piperModelPath READ piperModelPath WRITE setPiperModelPath NOTIFY
                   piperModelPathChanged)
    Q_PROPERTY(QString whisperBinaryPath READ whisperBinaryPath WRITE setWhisperBinaryPath NOTIFY
                   whisperBinaryPathChanged)
    Q_PROPERTY(QString whisperModelPath READ whisperModelPath WRITE setWhisperModelPath NOTIFY
                   whisperModelPathChanged)
    Q_PROPERTY(bool piperFileOutputExecutionEnabled READ piperFileOutputExecutionEnabled WRITE
                   setPiperFileOutputExecutionEnabled NOTIFY piperFileOutputExecutionEnabledChanged)
    Q_PROPERTY(QString activeConversationId READ activeConversationId WRITE setActiveConversationId
                   NOTIFY activeConversationIdChanged)

public:
    explicit AppSettings(std::unique_ptr<ISettingsStore> store, QObject* parent = nullptr);

    QString themeName() const;
    void setThemeName(const QString& themeName);

    QString configurationProfile() const;
    void setConfigurationProfile(const QString& configurationProfile);

    QString appLanguage() const;
    void setAppLanguage(const QString& language);
    QStringList availableLanguages() const;
    QString languageDisplayName(const QString& language) const;

    QString routingModeName() const;
    void setRoutingModeName(const QString& routingModeName);
    QStringList availableRoutingModes() const;

    QString ollamaEndpoint() const;
    void setOllamaEndpoint(const QString& endpoint);

    QString selectedLocalModel() const;
    void setSelectedLocalModel(const QString& model);

    bool localChatInferenceEnabled() const;
    void setLocalChatInferenceEnabled(bool enabled);
    bool localInferenceStreamingEnabled() const;
    void setLocalInferenceStreamingEnabled(bool enabled);
    bool promptContextInjectionEnabled() const;
    void setPromptContextInjectionEnabled(bool enabled);
    bool semanticPromptInclusionEnabled() const;
    void setSemanticPromptInclusionEnabled(bool enabled);
    bool contextExplainabilityVisible() const;
    void setContextExplainabilityVisible(bool visible);
    bool developerModeEnabled() const;
    void setDeveloperModeEnabled(bool enabled);
    QString piperBinaryPath() const;
    void setPiperBinaryPath(const QString& path);
    QString piperModelPath() const;
    void setPiperModelPath(const QString& path);
    QString whisperBinaryPath() const;
    void setWhisperBinaryPath(const QString& path);
    QString whisperModelPath() const;
    void setWhisperModelPath(const QString& path);
    bool piperFileOutputExecutionEnabled() const;
    void setPiperFileOutputExecutionEnabled(bool enabled);
    QString activeConversationId() const;
    void setActiveConversationId(const QString& conversationId);

signals:
    void themeNameChanged();
    void configurationProfileChanged();
    void appLanguageChanged();
    void routingModeNameChanged();
    void ollamaEndpointChanged();
    void selectedLocalModelChanged();
    void localChatInferenceEnabledChanged();
    void localInferenceStreamingEnabledChanged();
    void promptContextInjectionEnabledChanged();
    void semanticPromptInclusionEnabledChanged();
    void contextExplainabilityVisibleChanged();
    void developerModeEnabledChanged();
    void piperBinaryPathChanged();
    void piperModelPathChanged();
    void whisperBinaryPathChanged();
    void whisperModelPathChanged();
    void piperFileOutputExecutionEnabledChanged();
    void activeConversationIdChanged();

private:
    static constexpr auto themeNameKey = "themeName";
    static constexpr auto configurationProfileKey = "configurationProfile";
    static constexpr auto appLanguageKey = "appLanguage";
    static constexpr auto routingModeKey = "routingMode";
    static constexpr auto ollamaEndpointKey = "ollamaEndpoint";
    static constexpr auto selectedLocalModelKey = "selectedLocalModel";
    static constexpr auto localChatInferenceEnabledKey = "localChatInferenceEnabled";
    static constexpr auto localInferenceStreamingEnabledKey = "localInferenceStreamingEnabled";
    static constexpr auto promptContextInjectionEnabledKey = "promptContextInjectionEnabled";
    static constexpr auto semanticPromptInclusionEnabledKey = "semanticPromptInclusionEnabled";
    static constexpr auto contextExplainabilityVisibleKey = "contextExplainabilityVisible";
    static constexpr auto developerModeEnabledKey = "developerModeEnabled";
    static constexpr auto piperBinaryPathKey = "piperBinaryPath";
    static constexpr auto piperModelPathKey = "piperModelPath";
    static constexpr auto whisperBinaryPathKey = "whisperBinaryPath";
    static constexpr auto whisperModelPathKey = "whisperModelPath";
    static constexpr auto piperFileOutputExecutionEnabledKey = "piperFileOutputExecutionEnabled";
    static constexpr auto activeConversationIdKey = "activeConversationId";
    static constexpr auto defaultThemeName = "Sentinel Dark";
    static constexpr auto defaultConfigurationProfile = "Desktop Alpha";
    static constexpr auto defaultAppLanguage = "system";

    std::unique_ptr<ISettingsStore> store_;
};

} // namespace sentinel::core
