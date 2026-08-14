// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QList>

namespace sentinel::core {

struct ShareRecord {
    QString id;
    QString sessionId;
    QString url;
    QString secret;
    QDateTime createdAt;
    QDateTime lastSyncedAt;
    bool isActive{true};
};

struct ShareConfig {
    bool enabled{false};
    QString apiUrl;
    QString authToken;
    bool autoSync{true};
    int syncIntervalMs{1000};
};

class ISharingService {
public:
    virtual ~ISharingService() = default;

    // Share management
    virtual ShareRecord createShare(const QString& sessionId) = 0;
    virtual bool deleteShare(const QString& shareId) = 0;
    virtual std::optional<ShareRecord> findShare(const QString& shareId) const = 0;
    virtual QList<ShareRecord> shares() const = 0;
    virtual std::optional<ShareRecord> shareBySession(const QString& sessionId) const = 0;

    // Sync operations
    virtual bool syncShare(const QString& shareId) = 0;
    virtual void enableAutoSync(bool enabled) = 0;
    virtual bool isAutoSyncEnabled() const = 0;

    // Configuration
    virtual void configure(const ShareConfig& config) = 0;
    virtual ShareConfig config() const = 0;
};

} // namespace sentinel::core
