#pragma once

#include <QHash>
#include <QMutex>
#include <memory>
#include <functional>

namespace Sentinel {

template <typename Key, typename Value>
class MemoMap {
public:
    using Factory = std::function<Value(const Key &)>;

    explicit MemoMap(Factory factory)
        : m_factory(std::move(factory)) {}

    Value get(const Key &key) {
        QMutexLocker locker(&m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            return it.value();
        }
        Value value = m_factory(key);
        m_cache.insert(key, value);
        return value;
    }

    bool has(const Key &key) const {
        QMutexLocker locker(&m_mutex);
        return m_cache.contains(key);
    }

    void invalidate(const Key &key) {
        QMutexLocker locker(&m_mutex);
        m_cache.remove(key);
    }

    void invalidateAll() {
        QMutexLocker locker(&m_mutex);
        m_cache.clear();
    }

    int size() const {
        QMutexLocker locker(&m_mutex);
        return m_cache.size();
    }

    QHash<Key, Value> snapshot() const {
        QMutexLocker locker(&m_mutex);
        return m_cache;
    }

private:
    Factory m_factory;
    QHash<Key, Value> m_cache;
    mutable QMutex m_mutex;
};

template <typename Key, typename Value>
class SharedMemoMap {
public:
    using Factory = std::function<Value(const Key &)>;

    explicit SharedMemoMap(Factory factory)
        : m_factory(std::move(factory)) {}

    std::shared_ptr<Value> get(const Key &key) {
        QMutexLocker locker(&m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            return it.value();
        }
        auto value = std::make_shared<Value>(m_factory(key));
        m_cache.insert(key, value);
        return value;
    }

    bool has(const Key &key) const {
        QMutexLocker locker(&m_mutex);
        return m_cache.contains(key);
    }

    void invalidate(const Key &key) {
        QMutexLocker locker(&m_mutex);
        m_cache.remove(key);
    }

    void invalidateAll() {
        QMutexLocker locker(&m_mutex);
        m_cache.clear();
    }

private:
    Factory m_factory;
    QHash<Key, std::shared_ptr<Value>> m_cache;
    mutable QMutex m_mutex;
};

} // namespace Sentinel
