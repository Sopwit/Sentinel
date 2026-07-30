#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

struct RuntimeSafetyPolicy {
    QString id;
    QString name;
    bool localOnly = true;
    bool noExecution = true;
    QString summary;
};

struct RuntimeSafetyRule {
    QString id;
    QString name;
    bool enforced = false;
    QString summary;
};

enum class RuntimeSafetyDecision : std::uint8_t {
    Compliant,
    Blocked,
};

inline QString runtimeSafetyDecisionName(RuntimeSafetyDecision decision) {
    switch (decision) {
    case RuntimeSafetyDecision::Compliant:
        return QStringLiteral("Compliant");
    case RuntimeSafetyDecision::Blocked:
        return QStringLiteral("Blocked");
    }

    return QStringLiteral("Blocked");
}

struct RuntimeSafetyReport {
    RuntimeSafetyPolicy policy;
    QList<RuntimeSafetyRule> rules;
    RuntimeSafetyDecision decision = RuntimeSafetyDecision::Blocked;
    QString summary;
};

QString runtimeSafetyRuleSummary(const RuntimeSafetyRule& rule);
QStringList runtimeSafetyRuleSummaries(const QList<RuntimeSafetyRule>& rules);
QString safeRuntimeSafetySummary(const RuntimeSafetyReport& report);

class IRuntimeSafetyPolicy {
public:
    virtual ~IRuntimeSafetyPolicy() = default;

    virtual RuntimeSafetyReport evaluate() const = 0;
};

class StaticRuntimeSafetyPolicy final : public IRuntimeSafetyPolicy {
public:
    RuntimeSafetyReport evaluate() const override;
};

} // namespace sentinel::core
