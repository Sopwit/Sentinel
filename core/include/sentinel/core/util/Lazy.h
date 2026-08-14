#pragma once

#include <QMutex>
#include <optional>
#include <functional>
#include <memory>

namespace Sentinel {

template <typename T>
class Lazy {
public:
    explicit Lazy(std::function<T()> factory)
        : m_factory(std::move(factory)) {}

    T &get() {
        QMutexLocker locker(&m_mutex);
        if (!m_value) {
            m_value = m_factory();
        }
        return *m_value;
    }

    const T &get() const {
        QMutexLocker locker(&m_mutex);
        if (!m_value) {
            m_value = m_factory();
        }
        return *m_value;
    }

    bool isLoaded() const {
        QMutexLocker locker(&m_mutex);
        return m_value.has_value();
    }

    void reset() {
        QMutexLocker locker(&m_mutex);
        m_value.reset();
    }

    void invalidate() {
        reset();
    }

private:
    std::function<T()> m_factory;
    mutable std::optional<T> m_value;
    mutable QMutex m_mutex;
};

template <typename T>
class LazyPtr {
public:
    explicit LazyPtr(std::function<std::shared_ptr<T>()> factory)
        : m_factory(std::move(factory)) {}

    std::shared_ptr<T> get() {
        QMutexLocker locker(&m_mutex);
        if (!m_value) {
            m_value = m_factory();
        }
        return m_value;
    }

    bool isLoaded() const {
        QMutexLocker locker(&m_mutex);
        return m_value != nullptr;
    }

    void reset() {
        QMutexLocker locker(&m_mutex);
        m_value.reset();
    }

private:
    std::function<std::shared_ptr<T>()> m_factory;
    std::shared_ptr<T> m_value;
    mutable QMutex m_mutex;
};

} // namespace Sentinel
