#include "sentinel/core/RuntimeSafety.h"

namespace sentinel::core {

QString runtimeSafetyRuleSummary(const RuntimeSafetyRule& rule) {
    const auto state = rule.enforced ? QStringLiteral("Enforced") : QStringLiteral("Not Enforced");
    return rule.summary.isEmpty()
               ? QStringLiteral("%1 (%2)").arg(rule.name, state)
               : QStringLiteral("%1 (%2): %3").arg(rule.name, state, rule.summary);
}

QStringList runtimeSafetyRuleSummaries(const QList<RuntimeSafetyRule>& rules) {
    QStringList summaries;
    for (const auto& rule : rules) {
        summaries.append(runtimeSafetyRuleSummary(rule));
    }
    return summaries;
}

QString safeRuntimeSafetySummary(const RuntimeSafetyReport& report) {
    if (!report.summary.isEmpty()) {
        return report.summary;
    }

    return QStringLiteral("%1 safety report (%2 rules).")
        .arg(runtimeSafetyDecisionName(report.decision))
        .arg(report.rules.size());
}

RuntimeSafetyReport StaticRuntimeSafetyPolicy::evaluate() const {
    RuntimeSafetyReport report;
    report.policy = RuntimeSafetyPolicy{
        QStringLiteral("runtime-safety-policy-1"),
        QStringLiteral("Static Runtime Safety Policy"),
        true,
        true,
        QStringLiteral("Local-only and no-execution runtime safety posture is active."),
    };
    report.rules = {
        RuntimeSafetyRule{
            QStringLiteral("runtime-safety.local-only"),
            QStringLiteral("Local-Only Posture"),
            true,
            QStringLiteral("Cloud/provider execution paths remain out of scope."),
        },
        RuntimeSafetyRule{
            QStringLiteral("runtime-safety.no-execution"),
            QStringLiteral("No Execution"),
            true,
            QStringLiteral("Runtime request metadata cannot execute models, tools, or providers."),
        },
        RuntimeSafetyRule{
            QStringLiteral("runtime-safety.no-side-effects"),
            QStringLiteral("No Side Effects"),
            true,
            QStringLiteral(
                "No process launch, networking, filesystem, or system action is allowed."),
        },
    };
    report.decision = RuntimeSafetyDecision::Compliant;
    report.summary =
        QStringLiteral("Runtime safety policy report: local-only and no-execution posture is "
                       "enforced with deterministic metadata rules.");
    return report;
}

} // namespace sentinel::core
