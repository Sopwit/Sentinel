#pragma once

#include <QHash>
#include <QMutex>
#include <QWaitCondition>
#include <functional>
#include <QAtomicInt>

namespace Sentinel {

class KeyedMutex {
public:
    explicit KeyedMutex() = default;

    void lock(const QString &key) {
        QMutex *mutex = getOrCreateMutex(key);
        mutex->lock();
    }

    bool tryLock(const QString &key, int timeoutMs = 0) {
        QMutex *mutex = getOrCreateMutex(key);
        if (timeoutMs <= 0)
            return mutex->tryLock();
        return mutex->tryLock(timeoutMs);
    }

    void unlock(const QString &key) {
        QMutex *mutex = findMutex(key);
        if (mutex) {
            mutex->unlock();
            cleanupIfUnused(key, mutex);
        }
    }

    class Locker {
    public:
        Locker(KeyedMutex &km, const QString &key)
            : m_keyedMutex(km), m_key(key) {
            m_keyedMutex.lock(m_key);
        }
        ~Locker() { m_keyedMutex.unlock(m_key); }
        Locker(const Locker &) = delete;
        Locker &operator=(const Locker &) = delete;

    private:
        KeyedMutex &m_keyedMutex;
        QString m_key;
    };

private:
    struct MutexEntry {
        QMutex *mutex = nullptr;
        QAtomicInt holders{0};
        QAtomicInt waiters{0};
    };

    QMutex *getOrCreateMutex(const QString &key) {
        QMutexLocker locker(&m_globalMutex);
        auto it = m_entries.find(key);
        if (it == m_entries.end()) {
            auto *entry = new MutexEntry();
            entry->mutex = new QMutex();
            m_entries.insert(key, entry);
            return entry->mutex;
        }
        return it.value()->mutex;
    }

    QMutex *findMutex(const QString &key) const {
        QMutexLocker locker(&m_globalMutex);
        auto it = m_entries.constFind(key);
        return (it != m_entries.constEnd()) ? it.value()->mutex : nullptr;
    }

    void cleanupIfUnused(const QString &key, QMutex *mutex) {
        QMutexLocker locker(&m_globalMutex);
        auto it = m_entries.find(key);
        if (it != m_entries.end()) {
            Q_UNUSED(mutex);
            delete it.value()->mutex;
            delete it.value();
            m_entries.erase(it);
        }
    }

    mutable QMutex m_globalMutex;
    QHash<QString, MutexEntry*> m_entries;
};

} // namespace Sentinel
