#pragma once

#include <QString>
#include <QtGlobal>

namespace sentinel::core {

class ContextEpoch final {
public:
    explicit ContextEpoch(QString sessionId = {}) : sessionId_(std::move(sessionId)) {}
    qint64 value() const {
        return epoch_;
    }
    QString sessionId() const {
        return sessionId_;
    }
    qint64 advance() {
        return ++epoch_;
    }
    void replace(qint64 epoch) {
        epoch_ = qMax<qint64>(0, epoch);
    }

private:
    QString sessionId_;
    qint64 epoch_ = 0;
};

} // namespace sentinel::core
