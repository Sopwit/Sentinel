#pragma once

#include <QList>
#include <QString>

namespace sentinel::core {

struct HealthCheck {
    QString id;
    bool passed = false;
    bool critical = true;
    QString summary;
};

struct HealthReport {
    bool healthy = false;
    bool ready = false;
    QList<HealthCheck> checks;
    QString summary;
};

class HealthService final {
public:
    void setCheck(HealthCheck check);
    void removeCheck(const QString& id);
    HealthReport report() const;
    QList<HealthCheck> checks() const;

private:
    QList<HealthCheck> checks_;
};

} // namespace sentinel::core
