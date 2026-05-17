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

public:
    explicit AppSettings(std::unique_ptr<ISettingsStore> store, QObject* parent = nullptr);

    QString themeName() const;
    void setThemeName(const QString& themeName);

    QString configurationProfile() const;
    void setConfigurationProfile(const QString& configurationProfile);

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

signals:
    void themeNameChanged();
    void configurationProfileChanged();
    void routingModeNameChanged();
    void ollamaEndpointChanged();
    void selectedLocalModelChanged();
    void localChatInferenceEnabledChanged();
    void localInferenceStreamingEnabledChanged();

private:
    static constexpr auto themeNameKey = "themeName";
    static constexpr auto configurationProfileKey = "configurationProfile";
    static constexpr auto routingModeKey = "routingMode";
    static constexpr auto ollamaEndpointKey = "ollamaEndpoint";
    static constexpr auto selectedLocalModelKey = "selectedLocalModel";
    static constexpr auto localChatInferenceEnabledKey = "localChatInferenceEnabled";
    static constexpr auto localInferenceStreamingEnabledKey = "localInferenceStreamingEnabled";
    static constexpr auto defaultThemeName = "Sentinel Dark";
    static constexpr auto defaultConfigurationProfile = "Desktop Alpha";

    std::unique_ptr<ISettingsStore> store_;
};

} // namespace sentinel::core
