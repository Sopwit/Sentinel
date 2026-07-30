#ifndef SENTINEL_DESKTOP_VIEWMODELS_NOTIFICATIONVIEWMODEL_H
#define SENTINEL_DESKTOP_VIEWMODELS_NOTIFICATIONVIEWMODEL_H

#include <QObject>
#include <QString>

namespace sentinel::desktop::viewmodels {

class NotificationViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY unreadCountChanged)
    Q_PROPERTY(QString lastNotificationText READ lastNotificationText NOTIFY lastNotificationTextChanged)

public:
    explicit NotificationViewModel(QObject* parent = nullptr);
    ~NotificationViewModel() override = default;

    [[nodiscard]] int unreadCount() const { return m_unreadCount; }
    void setUnreadCount(int count);

    [[nodiscard]] QString lastNotificationText() const { return m_lastNotificationText; }
    void setLastNotificationText(const QString& text);

public Q_SLOTS:
    void clearNotifications();
    void postNotification(const QString& title, const QString& message);

Q_SIGNALS:
    void unreadCountChanged();
    void lastNotificationTextChanged();
    void notificationTriggered(const QString& title, const QString& message);

private:
    int m_unreadCount{0};
    QString m_lastNotificationText;
};

} // namespace sentinel::desktop::viewmodels

#endif // SENTINEL_DESKTOP_VIEWMODELS_NOTIFICATIONVIEWMODEL_H
