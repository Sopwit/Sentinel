#include <sentinel/desktop/viewmodels/NotificationViewModel.h>

namespace sentinel::desktop::viewmodels {

NotificationViewModel::NotificationViewModel(QObject* parent)
    : QObject(parent) {
}

void NotificationViewModel::setUnreadCount(int count) {
    if (m_unreadCount != count) {
        m_unreadCount = count;
        Q_EMIT unreadCountChanged();
    }
}

void NotificationViewModel::setLastNotificationText(const QString& text) {
    if (m_lastNotificationText != text) {
        m_lastNotificationText = text;
        Q_EMIT lastNotificationTextChanged();
    }
}

void NotificationViewModel::clearNotifications() {
    setUnreadCount(0);
    setLastNotificationText(QString());
}

void NotificationViewModel::postNotification(const QString& title, const QString& message) {
    setLastNotificationText(title + QStringLiteral(": ") + message);
    setUnreadCount(m_unreadCount + 1);
    Q_EMIT notificationTriggered(title, message);
}

} // namespace sentinel::desktop::viewmodels
