#ifndef SENTINEL_DESKTOP_VIEWMODELS_SETTINGSVIEWMODEL_H
#define SENTINEL_DESKTOP_VIEWMODELS_SETTINGSVIEWMODEL_H

#include <QObject>
#include <QString>

namespace sentinel::desktop::viewmodels {

class SettingsViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString ollamaEndpoint READ ollamaEndpoint WRITE setOllamaEndpoint NOTIFY ollamaEndpointChanged)
    Q_PROPERTY(QString workspacePath READ workspacePath WRITE setWorkspacePath NOTIFY workspacePathChanged)
    Q_PROPERTY(QString activeTheme READ activeTheme WRITE setActiveTheme NOTIFY activeThemeChanged)

public:
    explicit SettingsViewModel(QObject* parent = nullptr);
    ~SettingsViewModel() override = default;

    [[nodiscard]] QString ollamaEndpoint() const { return m_ollamaEndpoint; }
    void setOllamaEndpoint(const QString& endpoint);

    [[nodiscard]] QString workspacePath() const { return m_workspacePath; }
    void setWorkspacePath(const QString& path);

    [[nodiscard]] QString activeTheme() const { return m_activeTheme; }
    void setActiveTheme(const QString& theme);

Q_SIGNALS:
    void ollamaEndpointChanged();
    void workspacePathChanged();
    void activeThemeChanged();

private:
    QString m_ollamaEndpoint{QStringLiteral("http://localhost:11434")};
    QString m_workspacePath;
    QString m_activeTheme{QStringLiteral("dark")};
};

} // namespace sentinel::desktop::viewmodels

#endif // SENTINEL_DESKTOP_VIEWMODELS_SETTINGSVIEWMODEL_H
