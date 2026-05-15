#include "sentinel/core/IClock.h"

namespace sentinel::core {

QDateTime SystemClock::nowUtc() const {
    return QDateTime::currentDateTimeUtc();
}

} // namespace sentinel::core
