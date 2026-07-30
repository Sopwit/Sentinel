#ifndef SENTINEL_DESKTOP_VIEWMODELS_AGENTVIEWMODEL_H
#define SENTINEL_DESKTOP_VIEWMODELS_AGENTVIEWMODEL_H

#include <QObject>
#include <QString>

namespace sentinel::desktop::viewmodels {

class AgentViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isAgentRunning READ isAgentRunning NOTIFY isAgentRunningChanged)
    Q_PROPERTY(QString currentTaskName READ currentTaskName NOTIFY currentTaskNameChanged)

public:
    explicit AgentViewModel(QObject* parent = nullptr);
    ~AgentViewModel() override = default;

    [[nodiscard]] bool isAgentRunning() const { return m_isAgentRunning; }
    void setIsAgentRunning(bool running);

    [[nodiscard]] QString currentTaskName() const { return m_currentTaskName; }
    void setCurrentTaskName(const QString& name);

public Q_SLOTS:
    void cancelCurrentTask();

Q_SIGNALS:
    void isAgentRunningChanged();
    void currentTaskNameChanged();
    void taskCompleted(const QString& taskName, bool success);

private:
    bool m_isAgentRunning{false};
    QString m_currentTaskName;
};

} // namespace sentinel::desktop::viewmodels

#endif // SENTINEL_DESKTOP_VIEWMODELS_AGENTVIEWMODEL_H
