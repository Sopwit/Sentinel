// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/notification/IAttentionService.h"
#include <QObject>
#include <QSoundEffect>
#include <memory>

namespace sentinel::core {

class AttentionService : public QObject, public IAttentionService {
    Q_OBJECT
public:
    explicit AttentionService(QObject* parent = nullptr);
    ~AttentionService() override;

    void configure(const NotificationConfig& config) override;
    NotificationConfig config() const override;
    void notify(NotificationEvent event, const QString& title, const QString& message) override;
    void playSound(NotificationEvent event) override;
    void setFocusAware(bool aware) override;

private:
    NotificationConfig m_config;
    std::unique_ptr<QSoundEffect> m_soundEffect;
};

} // namespace sentinel::core
