#pragma once

#include "sentinel/desktop/ChatMessageListModel.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace sentinel::core {
class AppSettings;
class ApplicationController;
class ModeManager;
} // namespace sentinel::core

namespace sentinel::desktop {

class DesktopShellViewModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString providerName READ providerName CONSTANT)
    Q_PROPERTY(QString providerStatus READ providerStatus CONSTANT)
    Q_PROPERTY(QString memoryStatus READ memoryStatus CONSTANT)
    Q_PROPERTY(QString chatHistoryStatus READ chatHistoryStatus CONSTANT)
    Q_PROPERTY(QString currentModeName READ currentModeName NOTIFY currentModeChanged)
    Q_PROPERTY(QStringList availableModes READ availableModes CONSTANT)
    Q_PROPERTY(QString currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(QStringList availablePages READ availablePages CONSTANT)
    Q_PROPERTY(ChatMessageListModel* chatMessages READ chatMessages CONSTANT)
    Q_PROPERTY(QStringList memoryEntries READ memoryEntries NOTIFY memoryEntriesChanged)
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY themeNameChanged)
    Q_PROPERTY(QString configurationProfile READ configurationProfile WRITE setConfigurationProfile
                   NOTIFY configurationProfileChanged)

public:
    DesktopShellViewModel(core::ApplicationController& controller, core::ModeManager& modeManager,
                          core::AppSettings& settings, QObject* parent = nullptr);

    QString providerName() const;
    QString providerStatus() const;
    QString memoryStatus() const;
    QString chatHistoryStatus() const;
    QString currentModeName() const;
    QStringList availableModes() const;
    QString currentPage() const;
    void setCurrentPage(const QString& page);
    QStringList availablePages() const;
    ChatMessageListModel* chatMessages();
    QStringList memoryEntries() const;
    QString themeName() const;
    void setThemeName(const QString& themeName);
    QString configurationProfile() const;
    void setConfigurationProfile(const QString& configurationProfile);

    Q_INVOKABLE bool sendMessage(const QString& message);
    Q_INVOKABLE void clearChat();
    Q_INVOKABLE void setModeByName(const QString& modeName);
    Q_INVOKABLE void remember(const QString& key, const QString& value);

signals:
    void currentModeChanged();
    void chatMessagesChanged();
    void memoryEntriesChanged();
    void themeNameChanged();
    void configurationProfileChanged();
    void currentPageChanged();

private:
    static QString normalizedPageOrDefault(const QString& page);

    core::ApplicationController& controller_;
    core::ModeManager& modeManager_;
    core::AppSettings& settings_;
    ChatMessageListModel chatMessages_;
    QString currentPage_ = QStringLiteral("Dashboard");
};

} // namespace sentinel::desktop
