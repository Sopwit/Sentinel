#include "sentinel/core/app/IClock.h"

namespace sentinel::core {

QDateTime SystemClock::nowUtc() const {
    return QDateTime::currentDateTimeUtc();
}

} // namespace sentinel::core
