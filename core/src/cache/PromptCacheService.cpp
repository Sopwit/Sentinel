// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/cache/PromptCacheService.h"
#include <QDateTime>

namespace sentinel::core {

PromptCacheService::PromptCacheService(QObject* parent) : QObject(parent) {}
PromptCacheService::~PromptCacheService() = default;

bool PromptCacheService::isEnabled() const { return m_enabled; }
void PromptCacheService::setEnabled(bool enabled) { m_enabled = enabled; }

std::optional<QString> PromptCacheService::lookup(const QString& promptHash) const {
    if (!m_enabled) return std::nullopt;

    auto it = m_cache.find(promptHash);
    if (it == m_cache.end()) {
        m_misses++;
        return std::nullopt;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - it->createdAt > m_ttlSeconds * 1000LL) {
        m_cache.erase(it);
        m_misses++;
        return std::nullopt;
    }

    it->hitCount++;
    m_hits++;
    return it->cachedResponse;
}

void PromptCacheService::store(const QString& promptHash, const QString& response) {
    if (!m_enabled) return;

    if (m_cache.size() >= m_maxEntries) {
        auto oldest = m_cache.begin();
        for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
            if (it->createdAt < oldest->createdAt) oldest = it;
        }
        m_cache.erase(oldest);
    }

    CacheEntry entry;
    entry.promptHash = promptHash;
    entry.cachedResponse = response;
    entry.createdAt = QDateTime::currentMSecsSinceEpoch();
    m_cache[promptHash] = entry;
}

void PromptCacheService::invalidate(const QString& promptHash) { m_cache.remove(promptHash); }
void PromptCacheService::clear() { m_cache.clear(); m_hits = 0; m_misses = 0; }
int PromptCacheService::size() const { return m_cache.size(); }
double PromptCacheService::hitRate() const {
    int total = m_hits + m_misses;
    return total > 0 ? (static_cast<double>(m_hits) / total * 100.0) : 0.0;
}
void PromptCacheService::setMaxEntries(int max) { m_maxEntries = max; }
void PromptCacheService::setTtlSeconds(int seconds) { m_ttlSeconds = seconds; }

} // namespace sentinel::core
