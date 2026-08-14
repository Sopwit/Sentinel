// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QMap>

namespace sentinel::core {

enum class NotificationEvent : std::uint8_t {
    Question,
    Permission,
    Error,
    Done,
    SubagentDone
};

struct NotificationConfig {
    bool enabled{true};
    bool soundEnabled{true};
    double soundVolume{0.8};
    bool desktopNotifications{true};
    bool focusAware{true};
};

class IAttentionService {
public:
    virtual ~IAttentionService() = default;

    virtual void configure(const NotificationConfig& config) = 0;
    virtual NotificationConfig config() const = 0;
    virtual void notify(NotificationEvent event, const QString& title, const QString& message) = 0;
    virtual void playSound(NotificationEvent event) = 0;
    virtual void setFocusAware(bool aware) = 0;
};

} // namespace sentinel::core
