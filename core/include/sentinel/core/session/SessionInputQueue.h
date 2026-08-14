#pragma once

#include <QQueue>
#include <QString>

namespace sentinel::core {

struct SessionInput {
    QString id;
    QString text;
    bool promoted = false;
    bool admitted = false;
};

class SessionInputQueue final {
public:
    void promote(SessionInput input) { input.promoted = true; queue_.enqueue(std::move(input)); }
    bool admitNext(SessionInput* admitted) {
        if (!admitted || queue_.isEmpty()) return false;
        SessionInput input = queue_.dequeue();
        input.admitted = true;
        *admitted = std::move(input);
        return true;
    }
    int size() const { return queue_.size(); }
    void clear() { queue_.clear(); }

private:
    QQueue<SessionInput> queue_;
};

} // namespace sentinel::core
