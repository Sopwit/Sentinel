#include "sentinel/core/observability/HealthService.h"

namespace sentinel::core {

void HealthService::setCheck(HealthCheck check) {
    for (HealthCheck& existing : checks_) {
        if (existing.id == check.id) {
            existing = std::move(check);
            return;
        }
    }
    checks_.append(std::move(check));
}

void HealthService::removeCheck(const QString& id) {
    for (auto it = checks_.begin(); it != checks_.end();) {
        if (it->id == id) {
            it = checks_.erase(it);
        } else {
            ++it;
        }
    }
}

HealthReport HealthService::report() const {
    HealthReport result;
    result.healthy = true;
    result.ready = true;
    for (const HealthCheck& check : checks_) {
        result.healthy = result.healthy && check.passed;
        if (check.critical) result.ready = result.ready && check.passed;
    }
    result.summary = result.ready ? QStringLiteral("Sentinel is ready.")
                                  : QStringLiteral("Sentinel is not ready.");
    return result;
}

QList<HealthCheck> HealthService::checks() const {
    return checks_;
}

} // namespace sentinel::core
