// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/notification/AttentionService.h"
#include <QDebug>
#include <algorithm>

namespace sentinel::core {

AttentionService::AttentionService(QObject* parent)
    : QObject(parent), m_soundEffect(std::make_unique<QSoundEffect>()) {
    m_soundEffect->setSource(QUrl(QStringLiteral("qrc:/sounds/notification.wav")));
    m_soundEffect->setVolume(0.35f);
}
AttentionService::~AttentionService() = default;

void AttentionService::configure(const NotificationConfig& config) {
    m_config = config;
    if (m_soundEffect) {
        m_soundEffect->setVolume(std::clamp(config.soundVolume, 0.0, 1.0));
    }
}

NotificationConfig AttentionService::config() const {
    return m_config;
}

void AttentionService::notify(NotificationEvent event, const QString& title,
                              const QString& message) {
    if (!m_config.enabled)
        return;

    Q_UNUSED(event)
    Q_UNUSED(title)

    if (m_config.desktopNotifications) {
        qDebug() << "Notification:" << message;
    }

    if (m_config.soundEnabled) {
        playSound(event);
    }
}

void AttentionService::playSound(NotificationEvent event) {
    if (!m_config.soundEnabled)
        return;
    const auto source =
        event == NotificationEvent::Error  ? QUrl(QStringLiteral("qrc:/sounds/error.wav"))
        : event == NotificationEvent::Done ? QUrl(QStringLiteral("qrc:/sounds/success.wav"))
                                           : QUrl(QStringLiteral("qrc:/sounds/notification.wav"));
    m_soundEffect->setSource(source);
    m_soundEffect->play();
}

void AttentionService::setFocusAware(bool aware) {
    m_config.focusAware = aware;
}

} // namespace sentinel::core
