#pragma once

#include <QDateTime>

namespace sentinel::core {

class IClock {
public:
    virtual ~IClock() = default;

    virtual QDateTime nowUtc() const = 0;
};

class SystemClock final : public IClock {
public:
    QDateTime nowUtc() const override;
};

} // namespace sentinel::core
