// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/cache/IPromptCacheService.h"
#include <QObject>
#include <QMap>

namespace sentinel::core {

class PromptCacheService : public QObject, public IPromptCacheService {
    Q_OBJECT
public:
    explicit PromptCacheService(QObject* parent = nullptr);
    ~PromptCacheService() override;

    bool isEnabled() const override;
    void setEnabled(bool enabled) override;
    std::optional<QString> lookup(const QString& promptHash) const override;
    void store(const QString& promptHash, const QString& response) override;
    void invalidate(const QString& promptHash) override;
    void clear() override;
    int size() const override;
    double hitRate() const override;
    void setMaxEntries(int max) override;
    void setTtlSeconds(int seconds) override;

private:
    bool m_enabled{false};
    int m_maxEntries{1000};
    int m_ttlSeconds{3600};
    mutable QMap<QString, CacheEntry> m_cache;
    mutable int m_hits{0};
    mutable int m_misses{0};
};

} // namespace sentinel::core
