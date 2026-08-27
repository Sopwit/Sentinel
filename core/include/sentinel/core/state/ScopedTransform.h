#pragma once

#include <QJsonObject>
#include <QMutex>
#include <QUuid>
#include <functional>
#include <vector>

namespace Sentinel {

class ScopedTransform {
public:
    struct Transform {
        QString id;
        std::function<QJsonObject(const QJsonObject&)> apply;
    };

    explicit ScopedTransform(QJsonObject initialState = {})
        : m_initialState(std::move(initialState)), m_currentState(m_initialState) {}

    QString addTransform(std::function<QJsonObject(const QJsonObject&)> fn) {
        QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QMutexLocker locker(&m_mutex);
        m_transforms.push_back({id, std::move(fn)});
        m_currentState = applyAll(m_currentState, {m_transforms.back()});
        return id;
    }

    void removeTransform(const QString& id) {
        QMutexLocker locker(&m_mutex);
        std::erase_if(m_transforms, [&id](const Transform& t) { return t.id == id; });
        rebuild();
    }

    QJsonObject materialize() const {
        QMutexLocker locker(&m_mutex);
        return m_currentState;
    }

    void reset() {
        QMutexLocker locker(&m_mutex);
        m_transforms.clear();
        m_currentState = m_initialState;
    }

    void reload(std::function<QJsonObject(const QJsonObject&)> baseLoader) {
        QMutexLocker locker(&m_mutex);
        m_initialState = baseLoader(m_initialState);
        rebuild();
    }

    size_t transformCount() const {
        QMutexLocker locker(&m_mutex);
        return m_transforms.size();
    }

private:
    QJsonObject applyAll(const QJsonObject& base, const std::vector<Transform>& transforms) const {
        QJsonObject state = base;
        for (const auto& t : transforms) {
            state = t.apply(state);
        }
        return state;
    }

    void rebuild() {
        m_currentState = m_initialState;
        m_currentState = applyAll(m_currentState, m_transforms);
    }

    QJsonObject m_initialState;
    QJsonObject m_currentState;
    std::vector<Transform> m_transforms;
    mutable QMutex m_mutex;
};

} // namespace Sentinel
