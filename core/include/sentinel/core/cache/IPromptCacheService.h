// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace sentinel::core {

struct CacheEntry {
    QString promptHash;
    QString cachedResponse;
    int hitCount{0};
    qint64 createdAt{0};
};

class IPromptCacheService {
public:
    virtual ~IPromptCacheService() = default;

    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual std::optional<QString> lookup(const QString& promptHash) const = 0;
    virtual void store(const QString& promptHash, const QString& response) = 0;
    virtual void invalidate(const QString& promptHash) = 0;
    virtual void clear() = 0;
    virtual int size() const = 0;
    virtual double hitRate() const = 0;
    virtual void setMaxEntries(int max) = 0;
    virtual void setTtlSeconds(int seconds) = 0;
};

} // namespace sentinel::core
