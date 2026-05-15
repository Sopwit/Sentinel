#pragma once

#include "sentinel/core/ISettingsStore.h"

#include <QObject>
#include <QString>
#include <memory>

namespace sentinel::core {

class AppSettings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY themeNameChanged)
    Q_PROPERTY(QString configurationProfile READ configurationProfile WRITE setConfigurationProfile
                   NOTIFY configurationProfileChanged)

public:
    explicit AppSettings(std::unique_ptr<ISettingsStore> store, QObject* parent = nullptr);

    QString themeName() const;
    void setThemeName(const QString& themeName);

    QString configurationProfile() const;
    void setConfigurationProfile(const QString& configurationProfile);

signals:
    void themeNameChanged();
    void configurationProfileChanged();

private:
    static constexpr auto themeNameKey = "themeName";
    static constexpr auto configurationProfileKey = "configurationProfile";
    static constexpr auto defaultThemeName = "Sentinel Dark";
    static constexpr auto defaultConfigurationProfile = "Desktop Alpha";

    std::unique_ptr<ISettingsStore> store_;
};

} // namespace sentinel::core
